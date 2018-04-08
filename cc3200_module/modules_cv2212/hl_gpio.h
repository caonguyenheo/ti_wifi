#ifndef __HL_GPIO_H_
#define __HL_GPIO_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    GPIO_DIR_IN = 0,
    GPIO_DIR_OUT,
} io_t;


typedef uint32_t gpio_option_t;

//!==============================================================================
//! brief: Function to set gpio direction is input or output, and resistor option 
//! param[in]  id - gpio id, valid in range (0, 7)
//! param[in]  option - confiuration for gpio pin 
//! return     0 if success, others for failure
//!==============================================================================
int32_t hl_gpio_setup(uint32_t id, gpio_option_t option); 

//!==============================================================================
//! brief: Function to get current value of pin 
//! param[in]   id - gpio id, valid in range (0,7)
//! param[out]  value - return value of pin
//! return      0 if success, others for failure
//!==============================================================================
int32_t hl_gpio_read(uint32_t id, bool *value);

//!==============================================================================
//! brief: Function to set value to gpio pin 
//! param[in]  id - gpio id
//! param[in]  value - value to set 
//! return     0 if success, others for failure
//!==============================================================================
int32_t hl_gpio_write(uint32_t id, bool value);

#endif // end of __HL_GPIO_H_
