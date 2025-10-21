// Copyright 2011-2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef ETHERNETIF_H
#define ETHERNETIF_H

#include <stdint.h>

#include "xtcp.h"

#if defined(__XC__) || defined(__cplusplus)
extern "C" {
#endif

/* 
 * Due to rate of xcore timers (100MHz) we cannot time beyond ~40 seconds
 * so we use a divisor to reduce the timer period to below this.
 * Practically, the timer expires immediately if timeout value is greater then
 * UINT_MAX / 2, hence /4 here
 */
#define XNETIF_DHCP_DIVISOR 4

/* list of timers that must match the enabled LwIP timers */
typedef enum {
  ARP_TIMEOUT,
  // IP_REASS_TIMEOUT,  // IP Fragmentation not supported for now
  TCP_TIMEOUT,
  IGMP_TIMEOUT,
  DHCP_COARSE_TIMEOUT,
  DHCP_FINE_TIMEOUT,
  // ACD_TMR_TIMEOUT,
  DNS_TIMEOUT,
  NUM_TIMEOUTS
} xtcp_lwip_timeout_type;

/* Initialize the Ethernet network interface and lwip */
int xcore_ethernetif_init(const uint8_t* mac_address_phy, const xtcp_ipconfig_t* ipconfig);
/* Wrapper to init LwIP timers */
void xcore_lwip_init_timers(uint32_t period[NUM_TIMEOUTS], uint32_t timeout[NUM_TIMEOUTS], uint32_t time_now);

/* LwIP Ethernet packet input function */
void ethernetif_input(const uint8_t buffer[], int32_t n_bytes, uint32_t timestamp);

/* Wrapper for LwIP link notifications */
void xcore_net_link_up(void);
/* Wrapper for LwIP link notifications */
void xcore_net_link_down(void);

/* Get the current IP configuration from the network interface */
// TODO - might need parameter if we support multiple interfaces
xtcp_ipconfig_t xcore_netif_get_ipconfig(void);

/* Wrapper to run LwIP timers */
void xcore_timeout(xtcp_lwip_timeout_type timeout);

/* Print the current IP address of the network interface */
void print_netif_ipaddr(void);

/** Get the current state of the network interface
 *
 *  \return 1 if the interface is up, 0 if it is down
 */
// TODO - might need parameter if we support multiple interfaces
unsigned get_if_state(void);

/* Check whether netif has IP address, either static or DHCP assigned */
xtcp_error_code_t ethernetif_has_ip_address(void);

#if defined(__XC__) || defined(__cplusplus)
}
#endif

#endif /* ETHERNETIF_H */
