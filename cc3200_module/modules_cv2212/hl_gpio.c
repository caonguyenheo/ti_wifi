#include "hl_gpio.h"
#include "gpio_if.h"
#include "hw_types.h"
#include "gpio.h"

static int32_t hl_gpio_get_pin(uint32_t id, uint32_t* pin_num);
//!==============================================================================
//! brief: Function to set gpio direction is input or output, and resistor option 
//! param[in]  id - gpio id, valid in range (0, 7)
//! param[in]  option - 
//! return     0 if success, others for failure
//!==============================================================================
int32_t hl_gpio_setup(uint32_t id, gpio_option_t option)
{
    int32_t ret_val = 0;
    unsigned int port;
    unsigned char pin_pos;
    uint32_t pin_num;

    ret_val = hl_gpio_get_pin(id, &pin_num);
    if (ret_val < 0) {
        return ret_val;
    }

    GPIO_IF_GetPortNPin(pin_num, &port, &pin_pos);
    GPIODirModeSet(port, pin_pos, option);
    return ret_val;
} 

//!==============================================================================
//! brief: Function to get current value of pin 
//! param[in]   id - gpio id, valid in range (0,7)
//! param[out]  value - return value of pin
//! return      0 if success, others for failure
//!==============================================================================
int32_t hl_gpio_read(uint32_t id, bool *value)
{
    int32_t ret_val = 0;
    unsigned int port;
    unsigned char pin_pos;
    uint32_t pin_num;

    ret_val = hl_gpio_get_pin(id, &pin_num);
    if (ret_val < 0) {
        return ret_val;
    }
    GPIO_IF_GetPortNPin(pin_num, &port, &pin_pos);
    *value = GPIO_IF_Get(pin_num, port, pin_pos);
    return ret_val;
}

//!==============================================================================
//! brief: Function to set value to gpio pin 
//! param[in]  id - gpio id
//! param[in]  value - value to set 
//! return     0 if success, others for failure
//!==============================================================================
int32_t hl_gpio_write(uint32_t id, bool value)
{
    int32_t ret_val = 0;
    unsigned int port;
    unsigned char pin_pos;
    uint32_t pin_num;

    ret_val = hl_gpio_get_pin(id, &pin_num);
    if (ret_val < 0) {
        return ret_val;
    }

    GPIO_IF_GetPortNPin(pin_num, &port, &pin_pos);
    GPIO_IF_Set(pin_num, port, pin_pos, value);
    return ret_val;

}

//!==============================================================================
//! brief: Function to get pin number from pin id 
//! param[in]  id - gpio id, valid in range (0, 7)
//! param[out]  pin_num - pin number 
//! return     0 if success, others for failure
//!==============================================================================
static int32_t hl_gpio_get_pin(uint32_t id, uint32_t* pin_num)
{
    int32_t ret_val = 0;
    switch (id) {
    case 0:
        *pin_num = 4;
        break;
    case 1:
        *pin_num = 6;
        break;
    case 2:
        *pin_num = 7;
        break;
    case 3:
        *pin_num = 8;
        break;
    case 4:
        *pin_num = 14;
        break;
    case 5:
        *pin_num = 15;
        break;
    case 6:
        *pin_num = 22;
        break;
    case 7:
        *pin_num = 28;
        break;
    default:
        *pin_num = 0;
        ret_val = -1;
        break;
    }
    
    return ret_val; 
}
