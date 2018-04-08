#include "hl_uart.h"
#include <string.h>

#if defined(USE_FREERTOS) || defined(USE_TI_RTOS)
#include "osi.h"
#endif

// Driverlib includes
#include "hw_types.h"
#include "hw_memmap.h"
#include "prcm.h"
#include "pin.h"
#include "uart.h"
#include "rom.h"
#include "rom_map.h"

#include "uart_if.h"


//!***************************************************************************
//! MACRO DEFINATIONS
//!
//!***************************************************************************
#define UART_PRINT          Report
#define MAX_BUFF_SIZE       1500

//!***************************************************************************
//! GLOBAL VARIABLES AND BUFFERS
//!
//!***************************************************************************
char     uart_rx_buf[MAX_BUFF_SIZE] = {0};
uint32_t uart_rx_len = 0;

//
// callback function assignment when receive uart frame
//
void (*uart_recv_cb)(char *, uint32_t) = hl_uart_read_handler;

//*****************************************************************************
//
//! Interrupt handler for UART interupt
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
static void UARTIntHandler(void)
{
    uint32_t int_status = MAP_UARTIntStatus(HL_UART_INF, true);
    if ((int_status & (UART_INT_RT | UART_INT_RX)) && MAP_UARTCharsAvail(HL_UART_INF)) {
        uart_rx_len++;
        uart_rx_buf[uart_rx_len] = (unsigned char)MAP_UARTCharGetNonBlocking(HL_UART_INF);
        if ((uart_rx_buf[uart_rx_len] == SLIP_END) ||
                (uart_rx_buf[uart_rx_len] == SLIP_ESC)) {
            // callback function to handle uart command
            uart_recv_cb(uart_rx_buf + 1, uart_rx_len);
            // reset buffer
            uart_rx_buf[0] = uart_rx_len;
            uart_rx_len = 0;
        }
    }

    MAP_UARTIntClear(HL_UART_INF, int_status);
}

//!***************************************************************************
//! Configure UART1 to communicate with myCin link
//!
//! \param  None
//!
//! \return 0 for success, others for failure
//!***************************************************************************
void
hl_uart_setup(void)
{
    //
    // Register interrupt handler for UART
    //
    MAP_UARTIntRegister(HL_UART_INF, UARTIntHandler);

    //
    // Enable receive interrupts for uart
    //
    MAP_UARTIntEnable(HL_UART_INF, UART_INT_RX | UART_INT_RT);

    //
    // Clear interrupt flags
    //
    MAP_UARTIntClear(HL_UART_INF, UART_INT_RX | UART_INT_RT);

    //
    // Config uart
    //
    MAP_UARTConfigSetExpClk(HL_UART_INF, MAP_PRCMPeripheralClockGet(HL_UART_PERIPH),
                            HL_UART_BAUD, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                                           UART_CONFIG_PAR_NONE));
}

//!***************************************************************************
//!  send data to uart1
//!
//! \param[in]  send_buf      - send buffer
//! \param[in]  send_buf_size - send buffer size
//!
//! \return     length of bytes sent to uart1. -1 if send_buf is null
//!***************************************************************************
int32_t
hl_uart_write(char *send_buf, uint32_t send_buf_size)
{
    uint32_t char_cnt = 0;

    if (send_buf != NULL) {
        while ( (*send_buf != '\0') || (char_cnt < send_buf_size) ) {
            // while (*send_buf != '\0') {
            MAP_UARTCharPut(HL_UART_INF, *send_buf++);
            char_cnt++;
        }
    } else {
        return -1;
    }
    return char_cnt;
}

