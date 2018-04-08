#include <stdio.h>
#include <stdint.h>

// simplelink includes
#include "simplelink.h"

// mycin mqtt client
#include "server_mqtt.h"

// Free-RTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "portmacro.h"
#include "osi.h"

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_apps_rcm.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "rom.h"
#include "rom_map.h"
#include "timer.h"
#include "utils.h"
#include "prcm.h"

#include "pinmux.h"

#include "common.h"

// common interface includes
#include "network_if.h"
#ifndef NOTERM
#include "uart_if.h"
#endif

#include "gpio_if.h"
#include "button.h"

#include "system.h"

void OptionPressedCb(buttonStatus_t status)
{
    UART_PRINT("Pressed callback: %d\n", status);
    if (PRESSEDnHOLD_3SECS == status) {
        // will delete config file
        UART_PRINT("Delete system configuration\n");
        system_Deregister();
        UART_PRINT("Will restart\n");
        system_reboot();
    }
}

void ButtonHandingTask(void *pvParameters)
{
    button_t optionButt = {
        .status = SNONE,
        .edge = ENONE,
        .count = 0,
        .event_cb = OptionPressedCb
    };

    unsigned int uiGPIOPort;
    unsigned char pucGPIOPin;
    //Read GPIO
    GPIO_IF_GetPortNPin(OPTION_GPIO_NO, &uiGPIOPort, &pucGPIOPin);

    static unsigned char ucPinValue = 1, ucPinLastValue = 1;
    
        // uint32_t uiGPIOPort_pair;
        // uint8_t pucGPIOPin_pair;
        // uint32_t port_pair = 4;
        // uint8_t pin_value_pair = 0;
        // GPIO_IF_GetPortNPin(port_pair, &uiGPIOPort_pair, &pucGPIOPin_pair);
   
    while (1) {

        // read pin value
        ucPinValue = GPIO_IF_Get(OPTION_GPIO_NO, uiGPIOPort, pucGPIOPin);
        // {
        //     pin_value_pair = GPIO_IF_Get(port_pair, uiGPIOPort_pair, pucGPIOPin_pair);
        //     UART_PRINT("VALUE=%d\r\n", pin_value_pair);
        // }

        // check edge
        if (ucPinValue == 1 && ucPinLastValue == 0) {
            optionButt.edge = FALLING;
            // UART_PRINT("Option button pressed\n");
        }
        else if (ucPinValue == 0 && ucPinLastValue == 1) {
            optionButt.edge = RAISING;
            // UART_PRINT("Option button released\n");
        }

        ucPinLastValue = ucPinValue;

        // we count when having falling edge
        if (optionButt.edge == FALLING) {
            optionButt.count++;
        }

        // check status
        if (optionButt.edge == RAISING && optionButt.count > 0) {

            if (optionButt.count <= (BUTTON_PRESSED_TIME/BUTTON_SAMPLE)) {
                optionButt.status = PRESSED;
            }
            else {
                optionButt.status = PRESSEDnHOLD_3SECS;
            }

            // make callback
            optionButt.event_cb(optionButt.status);

            // reset counter
            optionButt.count = 0;
            optionButt.edge = ENONE;
        }
        else {
            optionButt.status = SNONE;
        }

        osi_Sleep(BUTTON_SAMPLE);
    }
}
