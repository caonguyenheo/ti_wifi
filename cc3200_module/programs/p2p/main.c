//*****************************************************************************
//
// Application Name     - CV2212 Wifi Bridge
// Application Overview - 
//
//*****************************************************************************

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cc3200_aes.h"
#include "mqtt_handler.h"
#include "ntp.h"
#include "p2p_main.h"
#include "user_common.h"
#define SPI_MASTER_MODE         0
#if (SPI_MASTER_MODE)
#include "spi_master_handler.h"
#else
#include "spi_slave_handler.h"
#endif // SPI_MASTER_MODE
#include "notification.h"
#include "date_time.h"
#include "sleep_profile.h"
// REGISTER
#include "httpserverapp.h"
#include "button.h"
#include "system.h"
#include "userconfig.h"
#include "shellTask.h"
#include "state_machine.h"
#include "cc3200_system.h"

#define OSI_STACK_SIZE          1024

#define TASK_PRIORITY_NETWORK           20
#define TASK_PRIORITY_OTA               9
#define TASK_PRIORITY_MQTT              23
#define TASK_PRIORITY_P2P_MAIN          22
#define TASK_PRIORITY_SPI_SLAVE         11
#define TASK_PRIORITY_NTP               12
#define TASK_PRIORITY_GPIO              13
#define TASK_PRIORITY_NOTIFICATION      19
#define TASK_PRIORITY_IDLE_TASK         1
#define TASK_PRIORITY_REGISTER          21
#define TASK_PRIORITY_BUTTON            10
#define TASK_PRIORITY_CLI               15
#define TASK_PRIORITY_STATE_MACHINE     22
#define TASK_PRIORITY_LED_CONTROL       2

#if 0
#define ENABLE_NETWORK      1
#define ENABLE_OTA          0
#define ENABLE_MQTT         1
#define ENABLE_P2P          1
#define ENABLE_SPI          1
#define ENABLE_NTP          1
#define ENABLE_NOTIFICATION 1
#define ENABLE_IDLE_TASK    0
#define ENABLE_REGISTER     1
#define ENABLE_CLI_TASK     1
#define ENABLE_STATE_MACHINE     1
#define ENABLE_BUTTON_TASK  1
#else
#define ENABLE_NETWORK      1
#define ENABLE_OTA          1
#define ENABLE_MQTT         1
#define ENABLE_P2P          1
#define ENABLE_SPI          1
#define ENABLE_NTP          1
#define ENABLE_NOTIFICATION 1
#define ENABLE_IDLE_TASK    0
#define ENABLE_REGISTER     1
#define ENABLE_CLI_TASK     1
#define ENABLE_STATE_MACHINE     1
#define ENABLE_BUTTON_TASK  0
#endif

#define RESET_CAUSE         1

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
extern void init_power_manager(void);
extern void idle_task(void *arg);

extern int lp3p0_enter_hibernate();

OsiMsgQ_t g_tConnectionFlag;
OsiMsgQ_t g_tConnectionMaintainFlag;
OsiMsgQ_t g_MQTT_connection_flag;
OsiMsgQ_t g_MQTT_check_msg_flag;
OsiMsgQ_t g_send_notification_flag;
// OsiMsgQ_t g_state_machine_event;
OsiSyncObj_t g_NTP_init_done_flag;
OsiSyncObj_t g_registered;
OsiSyncObj_t g_userconfig_init;
OsiSyncObj_t g_start_streaming;
OsiSyncObj_t g_getNTP;

//uint8_t pair_pin_val = 0;
uint8_t ring_pin_val = 0;
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

/****************************************************************************/
/*                      LOCAL FUNCTION PROTOTYPES                           */
/****************************************************************************/
void NetworkTask(void *pvParameters);
long ResetNwp()
{
    long lRetVal = -1;
    /* Restart Network processor */
    lRetVal = sl_Stop(SL_STOP_TIMEOUT);
    
    lRetVal = sl_Start(NULL,NULL,NULL);
    ASSERT_ON_ERROR(lRetVal);
    return lRetVal;
}

//*****************************************************************************
//
//! Check the device mode and switch to STATION(STA) mode
//! restart the NWP to activate STATION mode
//!
//! \param  iMode (device mode)
//!
//! \return None
//
//*****************************************************************************
// void SwitchToStaMode(int iMode)
// {
//     if(iMode != ROLE_STA)
//     {
//         sl_WlanSetMode(ROLE_STA);
//         MAP_UtilsDelay(80000);
//         sl_Stop(SL_STOP_TIMEOUT);
//         MAP_UtilsDelay(80000);
//         sl_Start(0,0,0);
//     }

// }
// void spi_main_task(void *pvParameters);

// void random_char(char *output,  int len);
int g_input_state=LED_STATE_INVALID;
int g_current_state=LED_STATE_NONE;

void led_control(int LED_THREAD_SLEEP_MS){
	int LED_POWER_UP_CYCLE =(2000/LED_THREAD_SLEEP_MS);
	int LED_STATE_CALL_LOWBAT_CYCLE =(3000/LED_THREAD_SLEEP_MS);
	int LED_STATE_CALL_NORMALBAT_CYCLE =(2000/LED_THREAD_SLEEP_MS);
	int LED_STATE_USER_CONNECTED_CYCLE =(5000/LED_THREAD_SLEEP_MS);
	int LED_STATE_START_PAIR_MODE_CYCLE =(60*1000/LED_THREAD_SLEEP_MS);
	int LED_STATE_AP_CONNECTING_MAXCYCLE = (2000/LED_THREAD_SLEEP_MS);
	int LED_STATE_AP_CONNECTING_ONCYCLE = (200/LED_THREAD_SLEEP_MS);
	int LED_STATE_USER_CONNECTED_MAXCYCLE = (1000/LED_THREAD_SLEEP_MS);
	int LED_STATE_USER_CONNECTED_ONCYCLE = (200/LED_THREAD_SLEEP_MS);
	int LED_STATE_CALL_LOWBAT_MAXCYCLE = (300/LED_THREAD_SLEEP_MS);
	int LED_STATE_CALL_LOWBAT_ONCYCLE = (200/LED_THREAD_SLEEP_MS);
    //ak_led1_set(0);
    static char count=0;

    if (g_input_state==LED_STATE_FTEST)
        return;
        
    //Comment because no more space
    if (g_input_state>LED_STATE_INVALID){
        if (g_current_state!=g_input_state){
            //UART_PRINT("LED state %d %d\r\n",g_current_state,g_input_state);
            g_current_state = g_input_state;
            g_input_state = LED_STATE_INVALID;
            count = 0;
        }
    }

    switch (g_current_state) {
        case LED_STATE_NONE:
            ak_led1_set(0);
            break;
        case LED_STATE_POWER_UP:
            if (count<LED_POWER_UP_CYCLE){
                ak_led1_set(1);
                count++;
            } else {
                g_current_state = LED_STATE_NONE;
                //ak_led1_set(0); //Accept 250ms error
            }
            break;
        case LED_STATE_AP_CONNECTING:
            if ((count%LED_STATE_AP_CONNECTING_MAXCYCLE)<LED_STATE_AP_CONNECTING_ONCYCLE){
                ak_led1_set(1);
            } else
                ak_led1_set(0);
            count++;
            break;
        case LED_STATE_FACTORY_CONNECTED:
            ak_led1_set(1);
            break;
        case LED_STATE_RE_CONNECTED_ROUTER:
        case LED_STATE_USER_CONNECTED:
            if (count<=LED_STATE_USER_CONNECTED_CYCLE){
                ak_led1_set(1);
                count++;
            } else {
                g_current_state = LED_STATE_NONE;
                //ak_led1_set(0); //Accept 250ms error
            }
            break;
        case LED_STATE_START_MODE_WIFI_SETUP:
        case LED_STATE_START_PAIR_MODE:
            if ((count%LED_STATE_USER_CONNECTED_MAXCYCLE)<LED_STATE_USER_CONNECTED_ONCYCLE){
                ak_led1_set(1);
            } else
                ak_led1_set(0);
            count++;
            if (count>=LED_STATE_START_PAIR_MODE_CYCLE)
                g_current_state = LED_STATE_NONE;
            break;
        case LED_STATE_CALL_LOWBAT:
        case LED_STATE_START_PAIR_MODE_LOWBAT:
            if ((count>=LED_STATE_CALL_LOWBAT_CYCLE) && (g_current_state==LED_STATE_CALL_LOWBAT))
                g_current_state = LED_STATE_NONE;
            if ((count%LED_STATE_CALL_LOWBAT_MAXCYCLE)<LED_STATE_CALL_LOWBAT_ONCYCLE)
                ak_led1_set(1);
            else
                ak_led1_set(0);
            count++;
            break;
        case LED_STATE_CALL_NORMALBAT:
            if (count>=LED_STATE_CALL_NORMALBAT_CYCLE){
                g_current_state = LED_STATE_NONE;
                //ak_led1_set(0); //Accept 250ms error
            } else {
                ak_led1_set(1);
                count++;
            }
            break;
        default:
            break;
    }
}


#define RETRIE_AFTER_DISCONNECT_AP  5000
void system_reboot(void);
extern char ap_ssid[];
extern char ap_pass[];
void NetworkTask(void *pvParameters)
{
    long lRetVal;
    int count=0;
    unsigned char ucSyncMsg = 0;
    uint32_t mcuState;


    if (MAP_PRCMSysResetCauseGet() != PRCM_HIB_EXIT) {
        //UART_PRINT("FRESH\r\n");
    }

    while (1) {
        osi_SyncObjWait(&g_registered, OSI_WAIT_FOREVER);
        osi_SyncObjSignal(&g_registered);
        mcuState = Network_IF_CurrentMCUState();
        if (!IS_CONNECTED(mcuState) || !IS_IP_ACQUIRED(mcuState)) {
            // osi_Sleep(200);
        }
        else {
            osi_MsgQWrite(&g_tConnectionFlag, &ucSyncMsg,
                    OSI_WAIT_FOREVER);
            osi_MsgQRead(&g_tConnectionMaintainFlag, &ucSyncMsg, OSI_WAIT_FOREVER);
            UART_PRINT("NetworkTask: check network\r\n");
        }
    }

#if (RESET_CAUSE)
    if (MAP_PRCMSysResetCauseGet() != PRCM_HIB_EXIT) {
        UART_PRINT("woken from New\r\n");


        osi_SyncObjWait(&g_registered, OSI_WAIT_FOREVER);

        // enable_deep_sleep();
    }
    // else if (MAP_PRCMSysResetCauseGet() == PRCM_HIB_EXIT) {
    else {
        // WAKE UP FROM HIB
        //UART_PRINT("woken from HIB\n\r");
        // sl_Start(NULL, NULL, NULL);
        while (1) {
            // check network connection
            mcuState = Network_IF_CurrentMCUState();
            if (!IS_CONNECTED(mcuState) || !IS_IP_ACQUIRED(mcuState)) {
                // disconnected
                // reset NWP
                // ResetNwp();
                continue;
            }

            // connected --> notify network status 
            lRetVal = osi_MsgQWrite(&g_tConnectionFlag, &ucSyncMsg,
                OSI_WAIT_FOREVER);
            if (lRetVal < 0)
            {
                UART_PRINT("unable to create the msg queue\n\r");
                // FIXME: handler this case
            }
        // wait for hibernate
        osi_MsgQRead(&g_tConnectionMaintainFlag, &ucSyncMsg, OSI_WAIT_FOREVER);
        }
    }
    // else if (MAP_PRCMSysResetCauseGet() == PRCM_WDT_RESET) {
    //     UART_PRINT("woken from WDT Reset\n\r");
    // }
    // else {
    //     UART_PRINT("woken cause unknown\n\r");
    // }
#endif
    #if 0
    // Initialize AP security params
    SlSecParams_t SecurityParams = {0}; // AP Security Parameters
    SecurityParams.Key = (signed char*)ap_pass;
    SecurityParams.KeyLen = strlen(ap_pass);
    // FIXME: DkS
    SecurityParams.Type = SECURITY_TYPE;
    #endif
#if 0
    while (1) {
        mcuState = Network_IF_CurrentMCUState();
        UART_PRINT("NetworkTask: check network\r\n");
        if (!IS_CONNECTED(mcuState) || !IS_IP_ACQUIRED(mcuState)) {
            // Connect to the Access Point
            #if 0
            UART_PRINT("MCU connecting to AP %s password %s\r\n",ap_ssid,ap_pass);
            lRetVal = Network_IF_ConnectAP(ap_ssid, SecurityParams);
            if(lRetVal < 0) {
                UART_PRINT("Connection to AP failed\n");
                count++;
            }
            else {
                {

                    lRetVal = sl_WlanProfileDel(0xFF);    
                    lRetVal = sl_WlanProfileAdd((signed char*)ap_ssid,
                                      strlen(ap_ssid), 0, &SecurityParams, 0, 6, 0);
                }
                count = 0;
                lRetVal = osi_MsgQWrite(&g_tConnectionFlag, &ucSyncMsg,
                    OSI_WAIT_FOREVER);
                if (lRetVal < 0)
                {
                    UART_PRINT("unable to create the msg queue\n\r");
                    // FIXME: handler this case
                }
            }
            if (count==5)
                system_reboot();
            #endif
            osi_Sleep(200);
        }
        else {
            osi_MsgQWrite(&g_tConnectionFlag, &ucSyncMsg,
                    OSI_WAIT_FOREVER);
            osi_MsgQRead(&g_tConnectionMaintainFlag, &ucSyncMsg, OSI_WAIT_FOREVER);
        }
        // osi_Sleep(RETRIE_AFTER_DISCONNECT_AP);
    }
#endif
    //
    // Loop here
    //
    //LOOP_FOREVER();
}



//****************************************************************************
//
//! Main function
//!
//! \param none
//!
//! \return None.
//
//****************************************************************************
extern void start_mycin_ota(void *pvParameters);
extern int ota_done;


#include "adc.h"
#include "pin.h"
#include "adc_userinput.h"
#define NO_OF_SAMPLES 		2
unsigned int adc_read(unsigned long  uiAdcInputPin)
{
    //unsigned long  uiAdcInputPin = PIN_60;  
    unsigned int  uiChannel;
    unsigned int  uiIndex=0;
    unsigned long ulSample;      
    unsigned long pulAdcSamples[NO_OF_SAMPLES+8]={0};
    unsigned int l_value=0, l_temp;
    //
    // Configuring UART for Receiving input and displaying output
    // 1. PinMux setting
    // 2. Initialize UART
    // 3. Displaying Banner
    //

    {
        //
        // Initialize Array index for multiple execution
        //
        uiIndex=0;
      
#ifdef CC3200_ES_1_2_1
        //
        // Enable ADC clocks.###IMPORTANT###Need to be removed for PG 1.32
        //
        HWREG(GPRCM_BASE + GPRCM_O_ADC_CLK_CONFIG) = 0x00000043;
        HWREG(ADC_BASE + ADC_O_ADC_CTRL) = 0x00000004;
        HWREG(ADC_BASE + ADC_O_ADC_SPARE0) = 0x00000100;
        HWREG(ADC_BASE + ADC_O_ADC_SPARE1) = 0x0355AA00;
#endif
        //
        // Pinmux for the selected ADC input pin
        //
        MAP_PinTypeADC(uiAdcInputPin,PIN_MODE_255);

        //
        // Convert pin number to channel number
        //
        switch(uiAdcInputPin)
        {
            case PIN_58:
                uiChannel = ADC_CH_1;
                break;
            case PIN_59:
                uiChannel = ADC_CH_2;
                break;
            case PIN_60:
                uiChannel = ADC_CH_3;
                break;
            default:
                break;
        }

        //
        // Configure ADC timer which is used to timestamp the ADC data samples
        //
        MAP_ADCTimerConfig(ADC_BASE,2^17);

        //
        // Enable ADC timer which is used to timestamp the ADC data samples
        //
        MAP_ADCTimerEnable(ADC_BASE);

        //
        // Enable ADC module
        //
        MAP_ADCEnable(ADC_BASE);

        //
        // Enable ADC channel
        //

        MAP_ADCChannelEnable(ADC_BASE, uiChannel);

        while(uiIndex < NO_OF_SAMPLES + 4)
        {
            if(MAP_ADCFIFOLvlGet(ADC_BASE, uiChannel))
            {
                ulSample = MAP_ADCFIFORead(ADC_BASE, uiChannel);
                pulAdcSamples[uiIndex++] = ulSample;
            }


        }

        MAP_ADCChannelDisable(ADC_BASE, uiChannel);

        uiIndex = 0;

        //UART_PRINT("\n\rTotal no of 32 bit ADC data printed :4096 \n\r");
        //UART_PRINT("\n\rADC data format:\n\r");
        //UART_PRINT("\n\rbits[13:2] : ADC sample\n\r");
        //UART_PRINT("\n\rbits[31:14]: Time stamp of ADC sample \n\r");

        //
        // Print out ADC samples
        //

        while(uiIndex < NO_OF_SAMPLES)
        {
	        l_temp = ((pulAdcSamples[4+uiIndex] >> 2 ) & 0x0FFF)*1467/4096;
	        l_value += l_temp;
            uiIndex++;
        }
        l_value=l_value/NO_OF_SAMPLES;
        //UART_PRINT("Voltage is %d mV\n\r",l_value);

        return l_value;
    }
}
unsigned int adc_cds_read(){
	unsigned int l_bat_val=0;
	l_bat_val = adc_read(PIN_60);
	l_bat_val = adc_read(PIN_60);
	//UART_PRINT("CDS GPIO05 ADC3:%dmV\r\n",l_bat_val);
	return l_bat_val;
}

unsigned int adc_bat_read(void){
#if (CALLGPIO==11)
	unsigned int l_bat_val=0;
	l_bat_val = adc_read(PIN_59);
	l_bat_val = adc_read(PIN_59);
#ifdef NTP_CHINA
	l_bat_val =(l_bat_val*43/10);
	//UART_PRINT("BATTERY GPIO04 ADC2*4.3=%dmV\r\n",l_bat_val);
#else
	l_bat_val =(l_bat_val*63/20);
	UART_PRINT("BATTERY GPIO04 ADC2*630/200=%dmV\r\n",l_bat_val);
#endif

#ifdef NTP_CHINA
	if (l_bat_val<BAT_LOW_THRESHOLD_MV){
		low_bat_push=1;
	} else
		low_bat_push=0;
#endif
	return l_bat_val;
#endif
}

unsigned int adc_bat1_read(void){
	int val=0;
	ak_batlvlsel2_set(0);
	ak_batlvlsel1_set(1);
	osi_Sleep(50); // Target 10ms with new HW
	val = adc_bat_read();
	ak_batlvlsel1_set(0);
	if (val<BAT_LOW_THRESHOLD_MV){
		low_bat_push=1;
	} else
		low_bat_push=0;
	return val;
}

unsigned int adc_bat2_read(void){
	int val=0;
	ak_batlvlsel1_set(0);
	ak_batlvlsel2_set(1);
	osi_Sleep(50); // Target 10ms with new HW
	val = adc_bat_read();
	ak_batlvlsel2_set(0);
	if (val<BAT_LOW_THRESHOLD_MV){
		low_bat_push2=1;
	} else
		low_bat_push2=0;
	return val;
}

void wdt_init(int l_time_ms);
int get_ring_pin_val()
{
    // RING PIN
    uint32_t uiGPIOPort_ring;
    uint8_t pucGPIOPin_ring;
    uint32_t port_ring = CALLGPIO;
    uint8_t l_ring_pin_val;
    
    GPIO_IF_GetPortNPin(port_ring, (unsigned int *)&uiGPIOPort_ring, &pucGPIOPin_ring);
    l_ring_pin_val = GPIO_IF_Get(port_ring, uiGPIOPort_ring, pucGPIOPin_ring);
    UART_PRINT("RING %D VALUE=%d\r\n", port_ring, l_ring_pin_val);    
    return l_ring_pin_val;
}
    
void main()
{
    long lRetVal = -1;
    g_input_state = LED_STATE_POWER_UP;
    //
    // Initialize board confifurations
    //
    BoardInit();
   
    wdt_init(10*1000);
    
#ifndef NTP_CHINA
    ak_bat1_set(1);
    ak_bat2_set(0);
    //adc_bat1_read();
    //adc_bat2_read();
    ak_speakerEnable_set(1);
#endif
    ring_pin_val = get_ring_pin_val();

    //
    // Start the SimpleLink Host
    //
    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);

    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }
    // sl_Start(0, 0, 0);

    //
    // sync object for inter thread communication
    //
    lRetVal = osi_MsgQCreate(&g_tConnectionFlag, NULL, sizeof( unsigned char ),
       1);
    /*if(lRetVal < 0)
    {
        //UART_PRINT("could not create msg queue\n\r");
        LOOP_FOREVER();
    }*/ 

    //
    // sync object for inter thread communication
    //
    lRetVal = osi_MsgQCreate(&g_tConnectionMaintainFlag, NULL, sizeof( unsigned char ),
       1);
    /*if(lRetVal < 0)
    {
        //UART_PRINT("could not create msg queue\n\r");
        //LOOP_FOREVER();
    }*/ 

    //
    // sync object for mqtt init
    //
    lRetVal = osi_MsgQCreate(&g_MQTT_connection_flag, NULL, sizeof( unsigned char ),
       1);
    if(lRetVal < 0)
    {
        //UART_PRINT("could not create msg queue\n\r");
        //LOOP_FOREVER();
    } 

    //
    // sync object for mqtt checking msg
    //
    lRetVal = osi_MsgQCreate(&g_MQTT_check_msg_flag, NULL, sizeof( unsigned char ),
       2);
    if(lRetVal < 0)
    {
        //UART_PRINT("could not create msg queue\n\r");
        //LOOP_FOREVER();
    }


    //
    // sync object for ntp init
    //
    lRetVal = osi_MsgQCreate(&g_send_notification_flag, NULL, sizeof( int ),
       1);
    if(lRetVal < 0)
    {
        //UART_PRINT("could not create msg queue\n\r");
        //LOOP_FOREVER();
    }

    // //
    // // sync object for ntp init
    // //
    // lRetVal = osi_MsgQCreate(&g_state_machine_event, NULL, sizeof( int ),
    //    1);
    // if(lRetVal < 0)
    // {
    //     UART_PRINT("could not create msg queue\n\r");
    //     LOOP_FOREVER();
    // }
    
    // init fsm
    lRetVal = fsm_init_env();
    /*if (lRetVal < 0) {
        UART_PRINT("Could not init FSM\r\n");
        LOOP_FOREVER();
    }*/
    

    //
    // sync object for ntp init
    //
    lRetVal = osi_SyncObjCreate(&g_NTP_init_done_flag);
    if(lRetVal < 0)
    {
        //UART_PRINT("could not create msg queue\n\r");
        //LOOP_FOREVER();
    }

    //
    // sync object for register status
    //
    lRetVal = osi_SyncObjCreate(&g_registered);
    if(lRetVal < 0)
    {
        //UART_PRINT("could not create msg queue\n\r");
        //LOOP_FOREVER();
    }

    //
    // sync object for userconfig init done
    //
    lRetVal = osi_SyncObjCreate(&g_userconfig_init);
    if(lRetVal < 0)
    {
        //UART_PRINT("could not create msg queue\n\r");
        //LOOP_FOREVER();
    }

    //
    // sync object for start streaming
    //
    lRetVal = osi_SyncObjCreate(&g_start_streaming);
    if(lRetVal < 0)
    {
        //UART_PRINT("could not create sync object\n\r");
        //LOOP_FOREVER();
    }

    //
    // sync object for register status
    //
    lRetVal = osi_SyncObjCreate(&g_getNTP);
    if(lRetVal < 0)
    {
        //UART_PRINT("could not create msg queue\n\r");
        //LOOP_FOREVER();
    }
    // init date time
    dt_start();

    //
    // Init aes encrypt
    //
    aes_init();


#if (ENABLE_REGISTER)
    //
    // Start the network interface
    //
    lRetVal = osi_TaskCreate(HttpServerAppTask,
                            (const signed char *)"Network interface control",
                            OSI_STACK_SIZE * 6, NULL, TASK_PRIORITY_REGISTER, NULL );
    /*if(lRetVal < 0) {
        UART_PRINT("Main: Fail to start network interface task\n");
        LOOP_FOREVER();
    }*/
#endif
    // goto COMMON_TASK;

#if (ENABLE_NETWORK)
    //
    // Start the Network Task
    //
    lRetVal = osi_TaskCreate(NetworkTask, (const signed char *)"Network task",
        OSI_STACK_SIZE * 2, NULL, TASK_PRIORITY_NETWORK, NULL);
    if(lRetVal < 0) {
        //ERR_PRINT(lRetVal);
        //LOOP_FOREVER();
    }
#endif
#if (ENABLE_OTA)
    //
    // Start OTA
    //
    ota_done = 0;
    
    lRetVal = osi_TaskCreate(start_mycin_ota, (const signed char *)"start_mycin_ota",
        OSI_STACK_SIZE*6, NULL, TASK_PRIORITY_OTA, NULL);

    if(lRetVal < 0) {
        //ERR_PRINT(lRetVal);
        //LOOP_FOREVER();
    }
#endif

#if (ENABLE_NTP)    
    //
    // Start the GetNTPTime task
    //
    lRetVal = osi_TaskCreate(GetNTPTimeTask,
        (const signed char *)"Get NTP Time",
        OSI_STACK_SIZE,
        NULL,
        TASK_PRIORITY_NTP,
        NULL );

    if(lRetVal < 0)
    {
        //ERR_PRINT(lRetVal);
        //LOOP_FOREVER();
    }
#endif
#if (ENABLE_MQTT)
    //
    // Start the MQTT client task
    //
    lRetVal = osi_TaskCreate(MQTTClientTask, (const signed char *)"MQTT client task",
        OSI_STACK_SIZE*7, NULL, TASK_PRIORITY_MQTT, NULL);

    if(lRetVal < 0) {
        //ERR_PRINT(lRetVal);
        //LOOP_FOREVER();
    }
#endif
#if (ENABLE_P2P)
    //
    // Start the P2P task
    //
    lRetVal = osi_TaskCreate(p2p_main,
        (const signed char *)"P2P MAIN",
        OSI_STACK_SIZE*7,
        NULL,
        TASK_PRIORITY_P2P_MAIN,
        NULL );

    if(lRetVal < 0)
    {
        //ERR_PRINT(lRetVal);
        //LOOP_FOREVER();
    }
#endif
#if (SPI_MASTER_MODE)
    //
    // Start the master spi task
    //
    lRetVal = osi_TaskCreate(spiMasterHandleTask,
        (const signed char *)"SPI MASTER",
        OSI_STACK_SIZE,
        NULL,
        18,
        NULL );

    if(lRetVal < 0)
    {
        //ERR_PRINT(lRetVal);
        //LOOP_FOREVER();
    }
#else

    //
    // Start the slave spi task
    //
#if (ENABLE_SPI)
    lRetVal = osi_TaskCreate(spiSlaveHandleTask,
        (const signed char *)"SPI SLAVE",
        OSI_STACK_SIZE * 4,
        NULL,
        TASK_PRIORITY_SPI_SLAVE,
        NULL );

    if(lRetVal < 0)
    {
        //ERR_PRINT(lRetVal);
        //LOOP_FOREVER();
    }
#endif
#endif // SPI_MASTER_MODE    

#if (ENABLE_NOTIFICATION)
    lRetVal = osi_TaskCreate(notification_task,
        (const signed char *)"notification task",
                    OSI_STACK_SIZE*3,
                    NULL,
                    TASK_PRIORITY_NOTIFICATION,
                    NULL );

    if(lRetVal < 0)
    {
        //ERR_PRINT(lRetVal);
        //LOOP_FOREVER();
    }
#endif

#if (ENABLE_IDLE_TASK)
    lRetVal = osi_TaskCreate(idle_task,
        (const signed char *)"idle task",
        OSI_STACK_SIZE,
        NULL,
        TASK_PRIORITY_IDLE_TASK,
        NULL );

    if(lRetVal < 0)
    {
        //ERR_PRINT(lRetVal);
        //LOOP_FOREVER();
    }
#endif
#if (ENABLE_BUTTON_TASK)
    //
    // Start the button handling
    //
    lRetVal = osi_TaskCreate(ButtonHandingTask,
                            (const signed char *)"Button handling task",
                            OSI_STACK_SIZE, NULL, TASK_PRIORITY_BUTTON, NULL );
    if(lRetVal < 0) {
        //UART_PRINT("Main: Fail to start button handling task\n");
        //LOOP_FOREVER();
    }
#endif
#if (ENABLE_CLI_TASK)
    lRetVal = osi_TaskCreate(ShellTask,
                  "ShellTask",
                  OSI_STACK_SIZE*3,
                  NULL,
                  TASK_PRIORITY_CLI,
                  NULL );  

    if(lRetVal < 0) {
        //UART_PRINT("Main: Fail to start uart if task\n");
        //LOOP_FOREVER();
    }
#endif

#if (ENABLE_STATE_MACHINE)
     lRetVal = osi_TaskCreate(state_machine,
                  "state machine",
                  OSI_STACK_SIZE,
                  NULL,
                  TASK_PRIORITY_STATE_MACHINE,
                  NULL );  

    if(lRetVal < 0) {
        //UART_PRINT("Main: Fail to start state machine task\n");
        //LOOP_FOREVER();
    }   
#endif

/*
#ifndef NTP_CHINA
     lRetVal = osi_TaskCreate(led_control,
                  "led_control",
                  OSI_STACK_SIZE,
                  NULL,
                  TASK_PRIORITY_LED_CONTROL,
                  NULL );  

    if(lRetVal < 0) {
        //UART_PRINT("Main: Fail to start state machine task\n");
        //LOOP_FOREVER();
    }
#endif
*/
    init_power_manager();


    // unittest_cmd();
    // sl_Start(0,0,0);
    //
    // Start the task scheduler
    //
    osi_start();
}


//****Watch dog timer******//
#include "wdt_if.h"
#include "rom_map.h"
#include "prcm.h"
#define WD_PERIOD_MS                 10000
#define MAP_SysCtlClockGet           80000000
#define MILLISECONDS_TO_TICKS(ms)    ((MAP_SysCtlClockGet / 1000) * (ms))
extern void WatchdogIntHandler1(void);
static inline void HIBEntrePreamble()
{
    HWREG(0x400F70B8) = 1;
    UtilsDelay(800000/5);
    HWREG(0x400F70B0) = 1;
    UtilsDelay(800000/5);

    HWREG(0x4402E16C) |= 0x2;
    UtilsDelay(800);
    HWREG(0x4402F024) &= 0xF7FFFFFF;
}
int wdt_run_status=0;
void wdt_de_init(void){
	WDT_IF_DeInit();
	UART_PRINT("WDT De-init\r\n");
	wdt_run_status=0;
}
void wdt_init(int l_time_ms){
	long bRetcode;
	unsigned long ulResetCause;
	    
	MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);
    PRCMCC3200MCUInit();
    
    WDT_IF_Init(NULL, MILLISECONDS_TO_TICKS(l_time_ms));
    
    //
    // Get the reset cause
    //
    ulResetCause = PRCMSysResetCauseGet();

    //
    // If watchdog triggered reset request hibernate
    // to clean boot the system
    //
    if( ulResetCause == PRCM_WDT_RESET )
    {
        HIBEntrePreamble();
        MAP_PRCMOCRRegisterWrite(0,1);
        MAP_PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);
        MAP_PRCMHibernateIntervalSet(330);
        MAP_PRCMHibernateEnter();
    }
    
    bRetcode = MAP_WatchdogRunning(WDT_BASE);
    if(!bRetcode){
    	wdt_de_init();
    	return;
	}
	wdt_run_status=1;
    UART_PRINT("WDT init\r\n");  
}
void wdt_clear(void)
{
  //
  // Acknowledge watchdog by clearing interrupt
  //
  if (wdt_run_status){
    //UART_PRINT("WDT ACK\r\n");
    MAP_WatchdogIntClear(WDT_BASE);
  }
}
