/******************************************************************************
** @file    system.c
** @brief   Support generic functions for setup and get info of device
** @date    Feb, 2016
**
******************************************************************************/
#include "system.h"
// standard lib include
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "simplelink.h"

//!*****************************************************************************
//! Get device mac address
//!
//! \param[out] mac_addr    mac address return from this function
//! \return     None
//!
//!*****************************************************************************

#define MAC_SIZE    6

// void
// get_mac_address(char *mac_addr, size_t size)
// {
//     uint8_t mac_addr_len = MAC_SIZE;
//     uint8_t mac[MAC_SIZE];

//     sl_NetCfgGet(SL_MAC_ADDRESS_GET, NULL, &mac_addr_len, (_u8 *)mac);

//     snprintf(mac_addr, size, "%02X%02X%02X%02X%02X%02X",
//              mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
// }
