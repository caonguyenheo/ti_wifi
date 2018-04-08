#ifndef __AP_SCAN_H__
#define __AP_SCAN_H__

//*****************************************************************************
//
//! \brief This function Get the Scan router Result
//!
//! \param[out]      router_lst - router list result 
//! \param[int]      maxScanList - maximum of router in List can scan
//!
//! \return         Success - Size of Scan Result Array
//!                    Failure   - -1 
//!
//! \note
//!
//
//*****************************************************************************
int 
ap_scan_get_list(Sl_WlanNetworkEntry_t* router_lst, 
                 int                    max_scan_list);

//*****************************************************************************
//
//! \brief This function to convert router list array to xml format
//!
//! \param[in]      router_lst - router list result 
//! \param[in]      num_of_router - number of routers in list 
//! \param[out]     xml_ret_buf - string buffer return in xml format for router 
//!                 list 
//! \param[in]      xml_ret_buf_size - max size of return buffer 
//!
//! \return         Success - 0
//!                    Failure   - -1 
//!
//! \note
//!
//
//*****************************************************************************
int 
ap_scan_list_to_xml(Sl_WlanNetworkEntry_t* router_lst, 
                    int                    num_of_router, 
                    char*                  xml_ret_buf, 
                    int                    xml_ret_buf_size);

//*****************************************************************************
//
//! \brief This function to get router list info array in  xml format
//!
//! \param[out]     router_lst - router list result 
//! \param[in]      max_scan_lst - number of routers in list 
//! \param[out]     xml_ret_buf - string buffer return in xml format for router 
//!                 list 
//! \param[in]      xml_ret_buf_size - max size of return buffer 
//!
//! \return         Success - 0
//!                    Failure   - -1 
//!
//! \note
//!
//
//*****************************************************************************
int 
ap_scan_mycin_format(int       max_scan_lst,
                      char*     xml_ret_buf, 
                      int       xml_ret_buf_size);

#endif /***/
