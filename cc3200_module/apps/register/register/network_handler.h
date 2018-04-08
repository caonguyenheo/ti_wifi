#ifndef __NETWORK_HANDLER_H__
#define __NETWORK_HANDLER_H__
//****************************************************************************
//
//!    \brief This function initializes the application variables
//!
//!    \param[in]  None
//!
//!    \return     0 on success, negative error-code on error
//
//****************************************************************************
int Network_IF_WifiSetMode(int iMode);
//****************************************************************************
//
//!    \brief This function initializes the application variables
//!
//!    \param[in]  None
//!
//!    \return     0 on success, negative error-code on error
//
//****************************************************************************
int Network_IF_Connect2AP(char* ssid, char* pass, int retry);


//*****************************************************************************
//
//! \brief This function Get the Scan router Result
//!
//! \param[out]      netEntries - Scan router list Result 
//!
//! \return         Success - Size of Scan Result Array
//!                    Failure   - -1 
//!
//! \note
//!
//
//*****************************************************************************
int Network_IF_provisionAP();
//****************************************************************************
//
//!    \brief This function initializes the application variables
//!
//!    \param[in]  None
//!
//!    \return     0 on success, negative error-code on error
//
//****************************************************************************
// int Network_IF_Init();
int Network_IF_Connect2AP_static_ip(char* ssid, char* pass, int retry);


//****************************************************************************
//
//!    \brief This function update wifi strength
//!
//!    \param[in]  None
//!
//!    \return     Wifi strenght in percent scale
//
//****************************************************************************
int get_wifi_strength();
extern int wifi_strength;


#endif /***/
