#ifndef __HL_UART_H_
#define __HL_UART_H_

#include <stdint.h>
#include <stdbool.h>
#include "hw_memmap.h"

///////////////////////////////////////////////////////////////////////////////
///////////////// MACRO DECLARATION //////////////////////
//////////////////////////////////////////
#define HL_UART_INF         UARTA1_BASE
#define HL_UART_PERIPH      PRCM_UARTA1
#define HL_UART_BAUD        115200

#define SLIP_END            '\n'
#define SLIP_ESC            '\r'
#define SLIP_ESC_END        0xDC
#define SLIP_ESC_ESC        0xDD
///////////////////////////////////////////////////////////////////////////////
///////////////// DATA STRUCTURE //////////////////////
//////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
///////////////// FUNCTIONS PROTOTYPE //////////////////////
//////////////////////////////////////////

//!***************************************************************************
//! Configure UART1 to communicate with myCin link
//!
//! \param  None
//!
//! \return 0 for success, others for failure
//!***************************************************************************
void hl_uart_setup(void);

//!***************************************************************************
//! Receive data from UART1
//!
//! \param[out] rec_buf       - receive buffer
//! \param[in]  rec_buf_size  - receive buffer size
//!
//! \return     none
//!***************************************************************************
void hl_uart_read_handler(char *rec_buf, uint32_t rec_buf_size);

//!***************************************************************************
//!  S++++end data to uart1
//!
//! \param[in]  send_buf      - send buffer
//! \param[in]  send_buf_size - send buffer size
//!
//! \return     length of bytes sent to uart1. -1 if send_buf is null
//!***************************************************************************
int32_t hl_uart_write(char *send_buf, uint32_t send_buf_size);

#endif // end of __HL_UART_H_
