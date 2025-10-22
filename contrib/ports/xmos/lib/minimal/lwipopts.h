
#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#define LWIP_IPV4                   1
#define LWIP_IPV6                   0


#define NO_SYS                      1
#define SYS_LIGHTWEIGHT_PROT        0
#define NO_SYS_NO_TIMERS            1

/* ---------- API options ---------- */
// lib_xtcp uses the raw API, so NO_SYS is 1, and these are all 0
#define LWIP_NETCONN                0
#define LWIP_SOCKET                 0
#define LWIP_NETIF_API              0

/* ---------- TCP/IP options ------- */
#define LWIP_TCP                    1
#define LWIP_UDP                    1
// TCP event handling selection
#define LWIP_EVENT_API              1
#define LWIP_CALLBACK_API           0

#define LWIP_IGMP                   LWIP_IPV4
#define LWIP_ICMP                   LWIP_IPV4

#define LWIP_DHCP                   1
#define LWIP_SNMP                   0

#define LWIP_DNS                    1
#define DNS_MAX_NAME_LENGTH         256
#define LWIP_NETIF_HOSTNAME         0

/* ---------- TCP options ---------- */
#define TCP_MSS                     536  // Default is 536
// #define TCP_WND                     2144 // Default is TCP_MSS*4 (2144)
// #define TCP_SND_BUF                 1072 // Default is TCP_MSS*2 (1072)

#define LWIP_ALTCP                  0   // Optionally (LWIP_TCP)
#ifdef LWIP_HAVE_MBEDTLS
#define LWIP_ALTCP_TLS              0
#define LWIP_ALTCP_TLS_MBEDTLS      0
#endif

#define LWIP_NETIF_REMOVE_CALLBACK  0
#define LWIP_NETIF_STATUS_CALLBACK  0
#define LWIP_NETIF_LINK_CALLBACK    0

/* ---------- Memory options ---------- */
#define MEM_LIBC_MALLOC             0   // Disable libc malloc() use
#define MEMP_MEM_MALLOC             0   // Disable using malloc() for mem-pools
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    1600
#define MEMP_NUM_PBUF               16
#define MEMP_NUM_UDP_PCB            4
#define MEMP_NUM_TCP_PCB            5
#define MEMP_NUM_TCP_PCB_LISTEN     8
#define MEMP_NUM_TCP_SEG            16
#define MEMP_NUM_REASSDATA          5   // Ensure that MEMP_NUM_REASSDATA < IP_REASS_MAX_PBUFS
#define MEMP_NUM_RAW_PCB            6
#define MEMP_NUM_ARP_QUEUE          10

/* ---------- Pbuf options ---------- */
#define PBUF_POOL_SIZE              16  // Ensure that PBUF_POOL_SIZE > IP_REASS_MAX_PBUFS
// #define PBUF_POOL_BUFSIZE           592

#define LWIP_WND_SCALE              0
#define TCP_RCV_SCALE               0

/* ---------- ARP options ---------- */
#define LWIP_ARP                    1
#define ARP_QUEUEING                1
#define ARP_QUEUE_LEN               4

/* ---------- IP options ---------- */
#define IP_REASS_MAX_PBUFS          10
#define IP_DEFAULT_TTL              255

#define IP_FRAG                     0
#define IP_REASSEMBLY               0

/* ---------- Broadcast options ---------- */
#define IP_SOF_BROADCAST            0
#define IP_SOF_BROADCAST_RECV       0
#define LWIP_BROADCAST_PING         0
#define LWIP_MULTICAST_PING         0

/* ---------- RAW options ---------- */
#define LWIP_RAW                    0

// LWIP_NETIF_TX_SINGLE_PBUF tries to prepare a single pbuf for transmit
// LWIP_NETIF_TX_SINGLE_PBUF (default 0) is tied to LWIP_SUPPORT_CUSTOM_PBUF (see pbuf.h)
// When LWIP_NETIF_TX_SINGLE_PBUF is 0, it enables LWIP_SUPPORT_CUSTOM_PBUF
// Then, pbuf_free() calls custom_free_function() -> ipfrag_free_pbuf_custom() which is recursive back to pbuf_free()
#define LWIP_NETIF_TX_SINGLE_PBUF   1

// TODO - ACD with DHCP would be nice to include, but fptrgroup resolution not working for 
// netif_set_add() when it calls acd_put_in_passive_mode() which uses acd->acd_conflict_callback()
// With DHCP and LWIP_DHCP_DOES_ACD_CHECK, acd_conflict_callback() become recursive through netif_set_addr().
#define LWIP_ACD                    0   // Default is LWIP_AUTOIP|LWIP_DHCP_DOES_ACD_CHECK
#define LWIP_DHCP_DOES_ACD_CHECK    0   // Default is LWIP_DHCP

/* ---------- AUTOIP options ------- */
#define LWIP_AUTOIP                 0
#define LWIP_DHCP_AUTOIP_COOP       0

#define TCP_LISTEN_BACKLOG          0

#define LWIP_NETIF_HWADDRHINT       0

#define LWIP_STATS                  0

#if LWIP_STATS
#define LINK_STATS                  1
#define IP_STATS                    1
#define ICMP_STATS                  1
#define IGMP_STATS                  1
#define IPFRAG_STATS                1
#define UDP_STATS                   1
#define TCP_STATS                   1
#define MEM_STATS                   1
#define MEMP_STATS                  1
#define PBUF_STATS                  1
#define SYS_STATS                   1
#endif // LWIP_STATS == 0

/* ---------- Debug options -------- */
#define LWIP_DEBUG                  1

#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define PPP_DEBUG                   LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF

// Custom data in pbuf to hold remote address for UDP recvfrom()
#include "xtcp.h"
#define LWIP_PBUF_CUSTOM_DATA         \
  xtcp_host_t remote;                 \
  unsigned timestamp;

#define LWIP_PBUF_CUSTOM_DATA_INIT(p) \
  p->remote.ipaddr[0] = 0;            \
  p->remote.ipaddr[1] = 0;            \
  p->remote.ipaddr[2] = 0;            \
  p->remote.ipaddr[3] = 0;            \
  p->remote.port_number = 0;          \
  p->timestamp = 0;

// this reuses the flag field to avoid adding an additional field;
// care should be taken if lwIP adds additional flags
#define PBUF_FLAG_TX_TIMESTAMP      0x80

#endif /* __LWIPOPTS_H__ */
