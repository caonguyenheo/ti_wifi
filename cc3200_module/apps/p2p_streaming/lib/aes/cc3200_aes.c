#include "cc3200_aes.h"

// Standard includes
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// Driverlib includes
#include "hw_aes.h"
#include "hw_memmap.h"
#include "hw_ints.h"
#include "hw_common_reg.h"
#include "hw_types.h"
#include "rom.h"
#include "rom_map.h"
#include "aes.h"
#include "interrupt.h"
#include "prcm.h"
#include "uart.h"
#include "utils.h"

// Common interface includes
#include "uart_if.h"
#include "pinmux.h"

/****************************************************************************/
/*                      LOCAL FUNCTION PROTOTYPES                           */
/****************************************************************************/
static volatile bool g_bContextInIntFlag;
static volatile bool g_bDataInIntFlag;
static volatile bool g_bContextOutIntFlag;
static volatile bool g_bDataOutIntFlag;
/****************************************************************************/

/****************************************************************************/
/*                      LOCAL FUNCTION PROTOTYPES                           */
/****************************************************************************/
static void 
aes_crypt(uint32_t ui_config, uint8_t  *puiKey1,
          uint8_t  *puiData, uint8_t  *puiResult,
          uint32_t uiDataLen, uint8_t  *uiIV );

static void 
aes_int_handler(void);

//*****************************************************************************
//
//! AES Interrupt Handler
//!
//! This function
//!        1. Handles Interrupts based on Interrupt Sources. Set Flags
//! \param none
//!
//! \return none
//
//*****************************************************************************
static void 
aes_crypt(uint32_t ui_config, uint8_t  *puiKey1,
          uint8_t  *puiData, uint8_t  *puiResult,
          uint32_t uiDataLen, uint8_t  *uiIV )
{
  //
  // Step1:  Enable Interrupts
  // Step2:  Wait for Context Ready Inteerupt 
  // Step3:  Set the Configuration Parameters (Direction,AES Mode and Key Size) 
  // Step4:  Set the Initialization Vector 
  // Step5:  Write Key 
  // Step6:  Start the Crypt Process 
  //
  
  //
  // Clear the flags.
  //
  g_bContextInIntFlag = false;
  g_bDataInIntFlag = false;
  g_bContextOutIntFlag = false;
  g_bDataOutIntFlag = false;
  
  //
  // Enable all interrupts.
  //
  MAP_AESIntEnable(AES_BASE, AES_INT_CONTEXT_IN |
                   AES_INT_CONTEXT_OUT | AES_INT_DATA_IN |
                     AES_INT_DATA_OUT);
  
  //
  // Wait for the context in flag, the flag will be set in the Interrupt handler.
  //
  while(!g_bContextInIntFlag) {
  }
  
  //
  // Configure the AES module with direction (encryption or decryption) and 
  // the key size.
  //
  MAP_AESConfigSet(AES_BASE, ui_config | AES_CFG_KEY_SIZE_128BIT);
  
  //
  // Write the initial value registers if needed, depends on the mode.
  //
  if(((ui_config & AES_CFG_MODE_M) == AES_CFG_MODE_CBC) ||
     ((ui_config & AES_CFG_MODE_M) == AES_CFG_MODE_CFB) ||
       ((ui_config & AES_CFG_MODE_M) == AES_CFG_MODE_CTR) ||
         ((ui_config & AES_CFG_MODE_M) == AES_CFG_MODE_ICM) 
           )
  {
    MAP_AESIVSet(AES_BASE, uiIV);
  }
  
  //
  // Write key1.
  //
  MAP_AESKey1Set(AES_BASE, puiKey1, AES_CFG_KEY_SIZE_128BIT);
  
  //
  // Start Crypt Process
  //
  MAP_AESDataProcess(AES_BASE, puiData, puiResult, uiDataLen);
}


//*****************************************************************************
//
//! AES Interrupt Handler
//!
//! This function
//!        1. Handles Interrupts based on Interrupt Sources. Set Flags
//! \param none
//!
//! \return none
//
//*****************************************************************************
static void
aes_int_handler(void)
{
  uint32_t uiIntStatus;
  
  //
  // Read the AES masked interrupt status.
  //
  uiIntStatus = MAP_AESIntStatus(AES_BASE, true);
  
  //
  // Set Different flags depending on the interrupt source.
  //
  if(uiIntStatus & AES_INT_CONTEXT_IN)
  {
    MAP_AESIntDisable(AES_BASE, AES_INT_CONTEXT_IN);
    g_bContextInIntFlag = true;
  }
  if(uiIntStatus & AES_INT_DATA_IN)
  {
    MAP_AESIntDisable(AES_BASE, AES_INT_DATA_IN);
    g_bDataInIntFlag = true;
  }
  if(uiIntStatus & AES_INT_CONTEXT_OUT)
  {
    MAP_AESIntDisable(AES_BASE, AES_INT_CONTEXT_OUT);
    g_bContextOutIntFlag = true;
  }
  if(uiIntStatus & AES_INT_DATA_OUT)
  {
    MAP_AESIntDisable(AES_BASE, AES_INT_DATA_OUT);
    g_bDataOutIntFlag = true;
  }
}

/****************************************************************************/

/****************************************************************************/
/*                      PUBLIC FUNCTION PROTOTYPES                           */
/****************************************************************************/


//*****************************************************************************
//
//! AES Init
//!
//! This function
//!        1. Init CC3200 AES hardware 
//! \param none
//!
//! \return none
//
//*****************************************************************************
void aes_init()
{
    //
    // Enable AES Module
    //
    MAP_PRCMPeripheralClkEnable(PRCM_DTHE, PRCM_RUN_MODE_CLK);
    
    //
    // Enable AES interrupts.
    //
    MAP_AESIntRegister(AES_BASE, aes_int_handler);
}

//*****************************************************************************
//
//! AES CBC Encryption
//!
//! This function
//!        1. AES encrypt by CBC 
//! \param:
//! \in key - CBC key
//! \in iv  - initialize vector
//! \in data_in   - input data buffer to encrypt
//! \in data_out  - output data buffer to store encrypted data
//! \in size_in   - size of data input 
//! \out size_out - return size of data output 
//!
//! \return none
//
//*****************************************************************************
void
aes_encrypt_cbc(uint8_t* key, uint8_t* iv, uint8_t* data_in, uint8_t* data_out, 
                size_t size_in, size_t* size_out)
{
  uint32_t ui_config = AES_CFG_DIR_ENCRYPT | AES_CFG_MODE_CBC;
  *size_out = size_in;
  aes_crypt(ui_config, key, data_in, data_out, size_in, iv);
}


//*****************************************************************************
//
//! AES CBC Decryption
//!
//! This function
//!        1. AES decrypt by CBC 
//! \param:
//! \in key - CBC key
//! \in iv  - initialize vector
//! \in data_in   - input data buffer to decrypt
//! \in data_out  - output data buffer to store decrypted data
//! \in size_in   - size of data input 
//! \out size_out - return size of data output 
//!
//! \return none
//
//*****************************************************************************
void
aes_decrypt_cbc(uint8_t* key, uint8_t* iv, uint8_t* data_in, uint8_t* data_out, 
                size_t size_in, size_t* size_out)
{
  uint32_t ui_config = AES_CFG_DIR_DECRYPT | AES_CFG_MODE_CBC;
  *size_out = size_in;
  aes_crypt(ui_config, key, data_in, data_out, size_in, iv);
}

//*****************************************************************************
//
//! AES ECB Encryption
//!
//! This function
//!        1. AES encrypt by ECB method.
//! \param:
//! \in key       -  ECB key
//! \in data_buf  -  data buffer for both input and store 
//!
//! \return none
//
//*****************************************************************************
void 
aes_encrypt_ecb(uint8_t* key, uint8_t* data_buf)

{
  uint8_t *iv = NULL;          //  ECB not use Initial Vector
  uint32_t ui_config = AES_CFG_DIR_ENCRYPT | AES_CFG_MODE_ECB;
  aes_crypt(ui_config, key, data_buf, data_buf, 16, iv);
}


//*****************************************************************************
//
//! AES ECB Decryption
//!
//! This function
//!        1. AES decrypt by ECB method.
//! \param:
//! \in key       -  ECB key
//! \in data_buf  -  data buffer for both input and store 
//!
//! \return none
//
//*****************************************************************************
void 
aes_decrypt_ecb(uint8_t* key, uint8_t* data_buf)
{
  uint8_t *iv = NULL;          //  ECB not use Initial Vector
  uint32_t ui_config = AES_CFG_DIR_DECRYPT | AES_CFG_MODE_ECB;
  aes_crypt(ui_config, key, data_buf, data_buf, 16, iv);
}