#ifndef __BUTTON_H__
#define __BUTTON_H__

#define OPTION_GPIO_NO               25

typedef enum {
    ENONE,
    FALLING,
    RAISING
} buttonEdge_t;

typedef enum {
    SNONE,
    PRESSED,
    PRESSEDnHOLD_3SECS
} buttonStatus_t;

typedef void (*event_callback_t)(buttonStatus_t status);

typedef struct {
    buttonStatus_t status;
    buttonEdge_t edge;
    uint32_t count;
    event_callback_t event_cb;
} button_t;

#define BUTTON_SAMPLE           500             // 500 ms
#define BUTTON_PRESSED_TIME     1000            // on hold for 1000 ms
#define BUTTON_PRESSnHOLD_3SECS 3000            // on hold for 3000 ms

void ButtonHandingTask(void *pvParameters);

#endif