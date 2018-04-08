#ifndef __HL_APCONFIG_H_
#define __HL_APCONFIG_H_

#include <stdint.h>

//*****************************************************************************
//  MACRO DEFINATION
//*****************************************************************************
#define MAX_SSID_LEN            64
#define DEF_AP_IP_ADDR          0xC0A8C101      // 192.168.193.1
#define DEF_SUBNET_MASK         0xFFFFFF00
#define DHCP_IP_START           0xC0A8C102      // 192.168.193.2
#define DHCP_IP_END             0xC0A8C1FE      // 192.168.193.254
#define DHCP_LEASE_TIME         (1*3600)        // 1 hour
#define PRODUCT_NAME            "CameraHD"

//*****************************************************************************
//  FUNCTION PROTOTYPE
//*****************************************************************************

//!****************************************************************************
//! set ap name
//!
//! \param[in] dev_name         device name - product name
//! \param[in] mac_addr         mac address of device
//!
//! \return    0 if success, others for failure.
//!****************************************************************************
int32_t
hl_ap_setname(char *dev_name, char *mac_addr);

//!****************************************************************************
//! set ap ip address
//!
//! \param[in] ip               ip to set
//!
//! \return    0 if success, others for failure.
//!****************************************************************************
int32_t
hl_ap_setip(int ip);

//!****************************************************************************
//! set ap dhcp client ip range
//!
//! \param[in] ip_start       start of ip range
//! \param[in] ip_end         end of ip range
//!
//! \return    0 if success, others for failure.
//!****************************************************************************
int32_t
hl_ap_dhcp_range(int ip_start, int ip_end);

//!****************************************************************************
//! Configure for device in AP role, include SSID name, IP, DHCP IP range
//!
//! \param     None
//!
//! \return    0 if success, others for failure.
//!****************************************************************************
int32_t
hl_ap_config(void);

#endif // end of file __HL_APCONFIG_H_
