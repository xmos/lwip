// Copyright 2015-2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef XCORE_NETIF_H
#define XCORE_NETIF_H


#if defined(__XC__) || defined(DOXYGEN)
/** Function called to assign transmit interface for the network stack.
 *
 * @param i_eth_tx  The Ethernet transmit interface to use
 * 
 * \note This function should be called before the network stack is initialized.
 * 
 * \todo Check how to handle dual-PHY configuration
 */
void xcore_netif_output_init(client ethernet_tx_if ?i_eth_tx, client mii_if ?i_mii);
#endif /* __XC__ || DOXYGEN__ */

/** Function called when a when a packet is to be sent.
 *
 * \param buffer    The data to send
 * \param n_bytes   Length of data to send
 * 
 * \note This function will likely be called from C, netif->linkoutput().
 */
void xcore_netif_low_level_output(int buffer[], size_t n_bytes);

uint32_t xcore_netif_low_level_output_timed(int buffer[], size_t n_bytes);

#endif /* __XCORE_NETIF_H__ */
