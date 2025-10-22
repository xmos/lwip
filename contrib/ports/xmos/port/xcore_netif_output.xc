// Copyright 2015-2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

/* XMOS library includes */
#include "ethernet.h"
#include "debug_print.h"
#include "mii.h"
#include "xassert.h"

/* Network interface port includes */
#include "netif/xcore_netif_output.h"

enum xcore_netif_eth_e {
  XCORE_NETIF_ETH_NONE,
  XCORE_NETIF_ETH_MII,
  XCORE_NETIF_ETH_TX,
};

/*
 * Ethernet TX interface passed in from xtcp_lwip()
 * This interface is used to send packets over the Ethernet interface using xcore_netif_low_level_output().
 */
static client interface ethernet_tx_if unsafe xtcp_i_eth_tx;
static client interface mii_if unsafe xtcp_i_mii;
static mii_info_t xtcp_mii_info;
static enum xcore_netif_eth_e xcore_netif_eth = XCORE_NETIF_ETH_NONE;

void xcore_netif_output_init(client ethernet_tx_if ?i_eth_tx, client mii_if ?i_mii) {
  if (!isnull(i_eth_tx)) {
    unsafe {
      xtcp_i_eth_tx = i_eth_tx;
    }
    xcore_netif_eth = XCORE_NETIF_ETH_TX;

  } else if (!isnull(i_mii)) {
    unsafe {
      xtcp_mii_info = i_mii.init();
      xtcp_i_mii = (client mii_if unsafe) i_mii;
    }
    xcore_netif_eth = XCORE_NETIF_ETH_MII;

  } else {
    xcore_netif_eth = XCORE_NETIF_ETH_NONE;
    fail("xcore_netif_output_init: netif output interface not set.\n");
  }
}


void xcore_netif_low_level_output(int buffer[], size_t n_bytes) {
  if (xcore_netif_eth == XCORE_NETIF_ETH_TX) {
    unsafe {
      // TODO - Check whether ETHERNET_ALL_INTERFACES is correct when we support dual-PHY
      ((client interface ethernet_tx_if)xtcp_i_eth_tx).send_packet((char *)buffer, n_bytes, ETHERNET_ALL_INTERFACES);
    }
  } else if (xcore_netif_eth == XCORE_NETIF_ETH_MII) {
    fail("xcore_netif_low_level_output: TODO test mii execution/data flow.\n");
    unsafe {
      /* TODO: test "mii" execution flow */
      xtcp_i_mii.send_packet((int *)buffer, n_bytes);
    }
  } else {
    fail("xcore_netif_low_level_output: netif output interface not set.\n");
  }
}

uint32_t xcore_netif_low_level_output_timed(int buffer[], size_t n_bytes) {
  if (xcore_netif_eth == XCORE_NETIF_ETH_TX) {
    unsafe {
      // TODO - Check whether ETHERNET_ALL_INTERFACES is correct when we support dual-PHY
      return ((client interface ethernet_tx_if)xtcp_i_eth_tx).send_timed_packet((char *)buffer,
                                                                                n_bytes,
                                                                                ETHERNET_ALL_INTERFACES);
    }
  } else {
    fail("xcore_netif_low_level_output: netif output interface not set.\n");
    return 0;
  }
}
