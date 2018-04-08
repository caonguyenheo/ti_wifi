//*****************************************************************************
// rom_pin_mux_config.c
//
// configure the device pins for different signals
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

// This file was automatically generated on 1/15/2016 at 3:42:49 PM
// by TI PinMux version 
//
//*****************************************************************************

#include "pin_mux_config.h" 
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_gpio.h"
#include "pin.h"
#include "gpio.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"

//*****************************************************************************
void PinMuxConfig(void)
{
    //
    // Enable Peripheral Clocks 
    //
    MAP_PRCMPeripheralClkEnable(PRCM_TIMERA3, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_TIMERA2, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_TIMERA1, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_ADC, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_GPIOA0, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_GPIOA1, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_GPIOA2, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_GPIOA3, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_UARTA0, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_UARTA1, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralClkEnable(PRCM_I2CA0, PRCM_RUN_MODE_CLK);

    //
    // Configure PIN_02 for TimerPWM7 GT_PWM07
    //
    MAP_PinTypeTimer(PIN_02, PIN_MODE_3);

    //
    // Configure PIN_01 for TimerPWM6 GT_PWM06
    //
    MAP_PinTypeTimer(PIN_01, PIN_MODE_3);

    //
    // Configure PIN_64 for TimerPWM5 GT_PWM05
    //
    MAP_PinTypeTimer(PIN_64, PIN_MODE_3);

    //
    // Configure PIN_21 for TimerPWM2 GT_PWM02
    //
    MAP_PinTypeTimer(PIN_21, PIN_MODE_9);

    //
    // Configure PIN_58 for ADC0 ADC_CH1
    //
    MAP_PinTypeADC(PIN_58, PIN_MODE_255);

    //
    // Configure PIN_60 for ADC0 ADC_CH3
    //
    MAP_PinTypeADC(PIN_60, PIN_MODE_255);

    //
    // Configure PIN_59 for GPIO Input
    //
    MAP_PinTypeGPIO(PIN_59, PIN_MODE_0, false);
    MAP_GPIODirModeSet(GPIOA0_BASE, 0x10, GPIO_DIR_MODE_IN);

    //
    // Configure PIN_61 for GPIO Input
    //
    MAP_PinTypeGPIO(PIN_61, PIN_MODE_0, false);
    MAP_GPIODirModeSet(GPIOA0_BASE, 0x40, GPIO_DIR_MODE_IN);

    //
    // Configure PIN_62 for GPIO Input
    //
    MAP_PinTypeGPIO(PIN_62, PIN_MODE_0, false);
    MAP_GPIODirModeSet(GPIOA0_BASE, 0x80, GPIO_DIR_MODE_IN);

    //
    // Configure PIN_63 for GPIO Input
    //
    MAP_PinTypeGPIO(PIN_63, PIN_MODE_0, false);
    MAP_GPIODirModeSet(GPIOA1_BASE, 0x1, GPIO_DIR_MODE_IN);

    //
    // Configure PIN_05 for GPIO Input
    //
    MAP_PinTypeGPIO(PIN_05, PIN_MODE_0, false);
    MAP_GPIODirModeSet(GPIOA1_BASE, 0x40, GPIO_DIR_MODE_IN);

    //
    // Configure PIN_06 for GPIO Input
    //
    MAP_PinTypeGPIO(PIN_06, PIN_MODE_0, false);
    MAP_GPIODirModeSet(GPIOA1_BASE, 0x80, GPIO_DIR_MODE_IN);

    //
    // Configure PIN_15 for GPIO Input
    //
    MAP_PinTypeGPIO(PIN_15, PIN_MODE_0, false);
    MAP_GPIODirModeSet(GPIOA2_BASE, 0x40, GPIO_DIR_MODE_IN);

    //
    // Configure PIN_18 for GPIO Input
    //
    MAP_PinTypeGPIO(PIN_18, PIN_MODE_0, false);
    MAP_GPIODirModeSet(GPIOA3_BASE, 0x10, GPIO_DIR_MODE_IN);

    //
    // Configure PIN_50 for SPI0 GSPI_CS
    //
    MAP_PinTypeSPI(PIN_50, PIN_MODE_9);

    //
    // Configure PIN_45 for SPI0 GSPI_CLK
    //
    MAP_PinTypeSPI(PIN_45, PIN_MODE_7);

    //
    // Configure PIN_53 for SPI0 GSPI_MISO
    //
    MAP_PinTypeSPI(PIN_53, PIN_MODE_7);

    //
    // Configure PIN_52 for SPI0 GSPI_MOSI
    //
    MAP_PinTypeSPI(PIN_52, PIN_MODE_8);

    //
    // Configure PIN_55 for UART0 UART0_TX
    //
    MAP_PinTypeUART(PIN_55, PIN_MODE_3);

    //
    // Configure PIN_57 for UART0 UART0_RX
    //
    MAP_PinTypeUART(PIN_57, PIN_MODE_3);

    //
    // Configure PIN_07 for UART1 UART1_TX
    //
    MAP_PinTypeUART(PIN_07, PIN_MODE_5);

    //
    // Configure PIN_08 for UART1 UART1_RX
    //
    MAP_PinTypeUART(PIN_08, PIN_MODE_5);

    //
    // Configure PIN_03 for I2C0 I2C_SCL
    //
    MAP_PinTypeI2C(PIN_03, PIN_MODE_5);

    //
    // Configure PIN_04 for I2C0 I2C_SDA
    //
    MAP_PinTypeI2C(PIN_04, PIN_MODE_5);
}
