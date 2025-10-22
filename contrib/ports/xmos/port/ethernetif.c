// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "netif/ethernetif.h"

#include "netif/configure.h"
#include "debug_print.h"
#include "xassert.h"
#include "xtcp.h"

#include "lwip/def.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/igmp.h"
#include "lwip/inet.h"
#include "lwip/init.h"
#include "lwip/ip4_frag.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/snmp.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "netif/xcore_netif_output.h"


// Our network interface structure
// TODO - will need to be per-interface if we support multiple interfaces
static struct netif xcore_netif;

/* Network interface short description. */
#define IFNAME0 'x'
#define IFNAME1 'm'

static err_t ethernetif_init(struct netif *netif);
static err_t xcore_ethernetif_linkoutput(struct netif *netif, struct pbuf *p);

void print_netif_ipaddr(void) {
  debug_printf("IP: %s\n", ipaddr_ntoa(&xcore_netif.ip_addr));
}

int xcore_ethernetif_init(const uint8_t* mac_address_phy, const xtcp_ipconfig_t* ipconfig) {
  ip_addr_t ip_addr;
  ip_addr_t netmask;
  ip_addr_t gw_addr;

  memcpy(&ip_addr, &ipconfig->ipaddr, sizeof(ip_addr_t));
  memcpy(&netmask, &ipconfig->netmask, sizeof(ip_addr_t));
  memcpy(&gw_addr, &ipconfig->gateway, sizeof(ip_addr_t));

  xassert(NUM_TIMEOUTS == lwip_num_cyclic_timers && msg("NUM_TIMEOUTS must be the same as lwip_num_cyclic_timers"));

  lwip_init();

  /* note, we set netif->state = mac_addr, here, maybe reassigned in init() */
  if (netif_add(&xcore_netif, &ip_addr, &netmask, &gw_addr, (void*)mac_address_phy, ethernetif_init, ethernet_input) == NULL) {
    debug_printf("xcore_net_init: netif_add (ethernetif_init) failed\n");
    return -1;
  }

  netif_set_default(&xcore_netif);
  netif_set_up(&xcore_netif);
  
#if LWIP_DHCP
  if (ip_addr_isany_val(ip_addr)) {
    debug_printf("Starting DHCP...\n");
    if (dhcp_start(&xcore_netif) != ERR_OK) {
      debug_printf("DHCP start error\n");
    }
  }
#endif

  /* Low level driver initialised by lib_ethernet */

  xnetif_configure();

  return 0;
}

__attribute__((fptrgroup("net_add_init"))) static err_t ethernetif_init(struct netif *netif) {
#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = XTCP_HOSTNAME;
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;

  netif->linkoutput = xcore_ethernetif_linkoutput;
  netif->output = etharp_output;
  // netif->output_ip6 = ethip6_output;
  netif->mtu = 1500;

  // Should NETIF_FLAG_BROADCAST be selectively set?
  /* TODO - check if config IP_SOF_BROADCAST and IP_SOF_BROADCAST_RECV needed */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP;
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, 100000000);

  unsigned char* mac_address_phy = (unsigned char*)netif->state;
  SMEMCPY(netif->hwaddr, mac_address_phy, ETH_HWADDR_LEN);
  netif->hwaddr_len = ETH_HWADDR_LEN;

#if LWIP_IPV6 && LWIP_IPV6_MLD
  /*
   * For hardware/netifs that implement MAC filtering.
   * All-nodes link-local is handled by default, so we must let the hardware know
   * to allow multicast packets in.
   * Should set mld_mac_filter previously. */
  if (netif->mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

  return ERR_OK;
}

__attribute__((fptrgroup("netif_linkoutput"))) 
static err_t xcore_ethernetif_linkoutput(struct netif *netif, struct pbuf *p) {

  (void)netif;
  LINK_STATS_INC(link.xmit);

#if ETH_PAD_SIZE
  pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif

  /* Update SNMP stats (only if you use SNMP) */
  MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
  int unicast = ((((u8_t *)p->payload)[0] & 0x01) == 0);
  if (unicast) {
    MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
  } else {
    MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
  }

  int32_t chain = pbuf_clen(p);
  if (chain > 1) {
    debug_printf("linkoutput: pbuf chain length %d\n", chain);
  }
  
  size_t n_bytes = p->tot_len;
  if (n_bytes < 60) {
    // Short buffer, extend to 60 bytes with zeros

    // this is an 'int' buffer to keep mii happy, but we otherwise use bytes
    int txbuf[(60 + 3) / 4];
    char * tx_bytes = (char *)txbuf;

    memcpy(tx_bytes, p->payload, n_bytes);
    memset(&tx_bytes[n_bytes], 0, 60 - n_bytes);

    n_bytes = 60;
    /* Start MAC transmit here */
    if (p->flags & PBUF_FLAG_TX_TIMESTAMP)
      p->timestamp = xcore_netif_low_level_output_timed(txbuf, n_bytes);
    else
      xcore_netif_low_level_output(txbuf, n_bytes);
  } else {
    // NOTE: this is not expecting or setup to deal with pbuf chains

    /* Start MAC transmit here */
    if (p->flags & PBUF_FLAG_TX_TIMESTAMP)
      p->timestamp = xcore_netif_low_level_output_timed(p->payload, n_bytes);
    else
      xcore_netif_low_level_output(p->payload, n_bytes);
  }

#if ETH_PAD_SIZE
  pbuf_add_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

static struct pbuf *xcore_net_low_level_input(const uint8_t buffer[], int32_t n_bytes) {
  struct pbuf *p;

  if (ETH_PAD_SIZE) {
    n_bytes += ETH_PAD_SIZE; /* allow room for Ethernet padding */
  }
  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, (uint16_t)n_bytes, PBUF_POOL);

  if (p != NULL) {
    if (ETH_PAD_SIZE) {
      pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
    }
    /* We iterate over the pbuf chain until we have read the entire
     * packet into the pbuf. */
    err_t take = pbuf_take(p, buffer, (uint16_t)n_bytes);
    if (take == ERR_OK) {
      MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);
      if (((u8_t *)p->payload)[0] & 1) {
        /* broadcast or multicast packet*/
        MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
      } else {
        /* unicast packet*/
        MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
      }
    } else {
      debug_printf("Pbuf_take: %d, %d, %u, %u\n", take, n_bytes, p->len, p->tot_len);
      pbuf_free(p);
      p = NULL;
    }
    if (ETH_PAD_SIZE) {
      pbuf_add_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
    }
  } else {
    debug_printf("No buffers free\n");
    // drop packet();
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.drop);
    MIB2_STATS_NETIF_INC(netif, ifindiscards);
  }
  return p;
}

void xcore_net_link_up(void) { netif_set_link_up(&xcore_netif); }

void xcore_net_link_down(void) { netif_set_link_down(&xcore_netif); }

xtcp_ipconfig_t xcore_netif_get_ipconfig(void) {
  xtcp_ipconfig_t ipconfig;
  memcpy(&ipconfig.ipaddr, &xcore_netif.ip_addr, sizeof(xtcp_ipaddr_t));
  memcpy(&ipconfig.netmask, &xcore_netif.netmask, sizeof(xtcp_ipaddr_t));
  memcpy(&ipconfig.gateway, &xcore_netif.gw, sizeof(xtcp_ipaddr_t));
  return ipconfig;
}

void xcore_lwip_init_timers(uint32_t period[NUM_TIMEOUTS], uint32_t timeout[NUM_TIMEOUTS], uint32_t time_now) {
  // Period pre-loaded with XS1_TIMER_KHZ, multiple-accumulate here...
  period[ARP_TIMEOUT] *= ARP_TMR_INTERVAL;
#if IP_REASSEMBLY
  period[IP_REASS_TIMEOUT] *= IP_TMR_INTERVAL;
#endif
  period[TCP_TIMEOUT] *= TCP_TMR_INTERVAL;
#if LWIP_IGMP
  period[IGMP_TIMEOUT] *= IGMP_TMR_INTERVAL;
#endif  // LWIP_IGMP
#if LWIP_DHCP
  // Due to rate of xcore timers (100MHz) we cannot time beyond ~40 seconds
  // so we use a divisor to reduce the timer period, multiply up at the other end.
  period[DHCP_COARSE_TIMEOUT] *= (DHCP_COARSE_TIMER_MSECS / XNETIF_DHCP_DIVISOR);
  // period[DHCP_COARSE_TIMEOUT] = (UINT_MAX / 4) * 3;
  period[DHCP_FINE_TIMEOUT] *= DHCP_FINE_TIMER_MSECS;
#endif  // LWIP_DHCP
  // period[ACD_TMR_TIMEOUT] *= ACD_TMR_INTERVAL;
  period[DNS_TIMEOUT] *= DNS_TMR_INTERVAL;

  for (int i = 0; i < NUM_TIMEOUTS; i++) {
    timeout[i] = time_now + period[i];
  }
}

void xcore_timeout(xtcp_lwip_timeout_type timeout) {
  switch (timeout) {
    case ARP_TIMEOUT:
      etharp_tmr();
      break;

#if IP_REASSEMBLY
    case IP_REASS_TIMEOUT:
      ip_reass_tmr();
      break;
#endif

    case TCP_TIMEOUT:
      tcp_tmr();
      break;

#if LWIP_IGMP
    case IGMP_TIMEOUT:
      igmp_tmr();
      break;
#endif

#if LWIP_DHCP
    case DHCP_COARSE_TIMEOUT:
    {
      // stats_display();
      static unsigned dhcp_coarse_tmr_count = 0;
      dhcp_coarse_tmr_count++;
      if (dhcp_coarse_tmr_count >= XNETIF_DHCP_DIVISOR) {
        dhcp_coarse_tmr_count = 0;
        dhcp_coarse_tmr();
      }
      break;
    }
    case DHCP_FINE_TIMEOUT:
      dhcp_fine_tmr();
      break;
#endif

#if LWIP_ACD
    case ACD_TMR_TIMEOUT:
      acd_tmr();
      break;
#endif

#if LWIP_DNS
    case DNS_TIMEOUT:
      dns_tmr();
      break;
#endif

    default:
      debug_printf("Unknown xcore_timeout timeout: %d\n", timeout);
      while(1) {}
      break;
  }
}

/**
 * This function reads a received packet from the network interface.
 * It uses the function xcore_net_low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
void ethernetif_input(const uint8_t buffer[], int32_t n_bytes, uint32_t timestamp) {
  /* move received packet into a new pbuf */
  struct pbuf *p = xcore_net_low_level_input(buffer, n_bytes);

  /* if no packet could be read, silently ignore this */
  if (p != NULL) {
    p->timestamp = timestamp;

    /* pass all packets to ethernet_input, which decides what packets it supports */
    if (xcore_netif.input(p, &xcore_netif) != ERR_OK) {

      LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
      pbuf_free(p);
      p = NULL;
    }
  }
}

unsigned get_if_state(void) {
  return netif_is_link_up(&xcore_netif);
}

xtcp_error_code_t ethernetif_has_ip_address(void)
{
  xtcp_error_code_t result = XTCP_EINVAL;
  if (dhcp_supplied_address(&xcore_netif)) {
      result = XTCP_SUCCESS;

  } else if (!ip_addr_isany(&xcore_netif.ip_addr)) {
    // Static IP, or could be DHCP assigned
    result = XTCP_SUCCESS;
  }
  return result;
}