#ifndef _CC3200_AES_H__
#define _CC3200_AES_H__

// Standard includes
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


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
void aes_init(void);

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
                size_t size_in, size_t* size_out);

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
                size_t size_in, size_t* size_out);

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
aes_encrypt_ecb(uint8_t* key, uint8_t* data_buf);

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
aes_decrypt_ecb(uint8_t* key, uint8_t* data_buf);
#endif /* end of file */
