#include "shellTask.h"
#include "gpio_if.h"
// Driverlib Includes
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_ints.h"
#include "spi.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"
#include "prcm.h"
#include "uart.h"
#include "interrupt.h"

// SimpleLink include
#include "simplelink.h"

// Free-RTOS/TI-RTOS include
#include "osi.h"
#include "common.h"
#include "uart_if.h"

#include "system.h"
#include "cc3200_system.h"
#include "userconfig.h"

// disable hibernate
#include "sleep_profile.h" 
#include "state_machine.h"
#include "spi_slave_handler.h"

#include "network_handler.h"

//*****************************************************************************
//                           MACRO DEFINATION
//*****************************************************************************
#define CONSOLE                   UARTA0_BASE
#define FOREVER                   1
#define RETERR_IF_TRUE(condition) {if(condition) return FAILURE;}
#define RET_IF_ERR(Func)          {int iRetVal = (Func); \
                                   if (SUCCESS != iRetVal) \
                                     return  iRetVal;}

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//****************************************************************************
int32_t cmd_mac_address(char *param);
int32_t cmd_set_wifi(char *param);
int32_t cmd_set_parameter(char *param);
int g_exit_loop_http = 0;
//*****************************************************************************
//
//! Display a prompt for the user to enter command
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
DisplayPrompt()
{
    UART_PRINT("\n\r>");
}

//*****************************************************************************
//
//! Display the usage of the I2C commands supported
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
/*
void
DisplayUsage()
{
//    UART_PRINT("ate-help\n\r");
    UART_PRINT("ate-version\n\r");
    UART_PRINT("ate-udid\n\r");
    UART_PRINT("ate-resetfactory\n\r");
    UART_PRINT("ate-mac\n\r");
//    UART_PRINT("ate-mac [MAC_ADDRESS]\n\r");
//    UART_PRINT("\t - get device's MAC address\n\r");
//    UART_PRINT("\t - set device's MAC address if provide [MAC_ADDRESS] (disabled)\n\r");
    UART_PRINT("ate-reboot\n\r");
    UART_PRINT("ate-ak_up\n\r");
    UART_PRINT("ate-ak_down\n\r");  
    UART_PRINT("ate-wificonnect [SSID] [PASSWD]\n\r");
}*/

//****************************************************************************
//
//! Parses the read command parameters and invokes the I2C APIs
//!
//! \param pcInpString pointer to the user command parameters
//!
//! This function
//!    1. Parses the read command parameters.
//!    2. Invokes the corresponding I2C APIs
//!
//! \return 0: Success, < 0: Failure.
//
//****************************************************************************
int
ProcessLedState(char *pcInpString)
{
    unsigned char ucDevAddr, ucLen;
    unsigned char aucDataBuf[256];
    char *pcErrPtr;
    int iRetVal;
    char ledNum;
    char ledVal;
    //
    // Get LED identify
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    if (!strcmp(pcInpString, "a"))
    {
        ledNum = MCU_ALL_LED_IND;
    }

    else if (!strcmp(pcInpString, "r"))
    {
        ledNum = MCU_RED_LED_GPIO;
    }

    else if (!strcmp(pcInpString, "g"))
    {
        ledNum = MCU_GREEN_LED_GPIO;
    }

    else if (!strcmp(pcInpString, "o"))
    {
        ledNum = MCU_ORANGE_LED_GPIO;
    }

    else
    {
        UART_PRINT("LED not supported\n\r");
        RETERR_IF_TRUE(true);
    }

    //
    // Get LED state
    //
    pcInpString = strtok(NULL, " ");
    RETERR_IF_TRUE(pcInpString == NULL);
    if (!strcmp(pcInpString, "on"))
    {
        GPIO_IF_LedOn(ledNum);
    }

    else if (!strcmp(pcInpString, "off"))
    {
        GPIO_IF_LedOff(ledNum);
    }

    else if (!strcmp(pcInpString, "toggle"))
    {
        GPIO_IF_LedToggle(ledNum);
    }

    else
    {
        UART_PRINT("LED state invalid\n\r");
        RETERR_IF_TRUE(true);
    }
    return SUCCESS;
}

//****************************************************************************
//
//! Parses the user input command and invokes the I2C APIs
//!
//! \param pcCmdBuffer pointer to the user command
//!
//! This function
//!    1. Parses the user command.
//!    2. Invokes the corresponding I2C APIs
//!
//! \return 0: Success, < 0: Failure.
//
//****************************************************************************
extern int httpServerOn;
extern void wdt_de_init(void);
extern OsiMsgQ_t g_send_notification_flag;
int
ParseNProcessCmd(char *pcCmdBuffer)
{
    char *pcInpString;
    int iRetVal = FAILURE;
    char l_delim[4] = " \n\r";
    pcInpString = strtok(pcCmdBuffer, l_delim);
    if (!strcmp(pcInpString, "atecmd"))

    {
        pcInpString = strtok(NULL, l_delim);
        /*if (!strcmp(pcInpString, "ate-help"))
        {
            //
            // Display command usage to user
            //
            DisplayUsage();
            iRetVal = SUCCESS;
        }
        else */if (!strcmp(pcInpString, "ver"))
        {
            UART_PRINT("CC_VER=%s, AK_VER=%s\r\nOK\r\n", g_version_8char, ak_version_format);
            disable_deep_sleep();
            MAP_WatchdogIntClear(WDT_BASE); // Don't know why if uart is active, WDT de-init fail, quick fix
            httpServerOn=0;
            iRetVal = SUCCESS;
        }

        else if (!strcmp(pcInpString, "deviceid"))
        {
            char udid[27] = {0};
            get_uid(udid);
            UART_PRINT("%s\r\nOK\r\n", udid);
            iRetVal = SUCCESS;
        }

        else if (!strcmp(pcInpString, "WIFI")) // nguyen cao command
        {
            iRetVal = cmd_mac_address(pcInpString);
            iRetVal = SUCCESS;
        }

        else if (!strcmp(pcInpString, "resetfactory"))
        {
            osi_Sleep(1000); // Quick way to make sure system has been initialized (not more than 1s)
            userconfig_set(NULL, DEV_NOT_REGISTERED, MODE_ID);
            userconfig_flash_write();
            UART_PRINT("OK\r\n");
            iRetVal = SUCCESS;
	        /*
        	wdt_de_init();
        	UART_PRINT("\n\rDelete system configuration\n\r");
        	system_Deregister();
        	UART_PRINT("\n\rWill restart\n\r");
//        	system_reboot_1();*/

        }

        // else if (!strcmp(pcInpString, "token"))
        // {
        //     char authenToken[32] = {0};
        //     get_authen_token(authenToken);
        //     UART_PRINT("\t- Token Key: %s\n\r", authenToken);
        //     iRetVal = SUCCESS;
        // }

        // else if (!strcmp(pcInpString, "topic"))
        // {
        //     char topic[64] = {0};
        //     // get_topicID(topic);
        //     UART_PRINT("\t- topic: %s\n\r", topic);
        //     iRetVal = SUCCESS;
        // }

        // else if (!strcmp(pcInpString, "led"))
        // {
        //     iRetVal = ProcessLedState(pcInpString);
        // }

        else if (!strcmp(pcInpString, "reboot"))
        {
            UART_PRINT("OK\r\n");
            system_reboot();
            iRetVal = SUCCESS;
        }
        /* don't need this
        else if (!strcmp(pcInpString, "ate-sm"))
        {
            UART_PRINT("\t- state machine ...\n\r");

            extern OsiMsgQ_t g_fsm_event_queue;
            pcInpString = strtok(NULL, " ");
            // UART_PRINT("STRING = %s\r\n", pcInpString);
            if (pcInpString == NULL) {
                UART_PRINT("error\r\n");
                iRetVal = FAILURE;
            }
            else {
                #include <string.h>
                int event_id = atoi(pcInpString);
                UART_PRINT("Got event %d\r\n", event_id);
                signal_event(event_id);
                // iRetVal = osi_MsgQWrite(&g_fsm_event_queue, &event_id, OSI_NO_WAIT);
                iRetVal = SUCCESS;
            }
        }*/
        /*
        else if (!strcmp(pcInpString, "ate-on"))
        {
            UART_PRINT("\t- ONNNNNNNNN ...\n\r");
            enable_deep_sleep();
            iRetVal = SUCCESS;

        }
        else if (!strcmp(pcInpString, "ate-off"))
        {
            UART_PRINT("\t- OFFFFFFFFF ...\n\r");
            disable_deep_sleep();
            iRetVal = SUCCESS;

        }*/
        else if (!strcmp(pcInpString, "akup"))
        {
            ak_power_up();
            UART_PRINT("OK\r\n");
            iRetVal = SUCCESS;
        }
       else if (!strcmp(pcInpString, "akdown"))
        {
            ak_power_down();
            UART_PRINT("OK\r\n");
            iRetVal = SUCCESS;
        }
        /*
       else if (!strcmp(pcInpString, "ate-ak_int"))
        {
            UART_PRINT("\t- AK  INTTTTTTTT ...\n\r");
            ak_read_interrupt();
            iRetVal = SUCCESS;
        }
    #if (AK_SPI_TEST)
        else if (!strcmp(pcInpString, "ate-ak_spi"))
        {
            UART_PRINT("\t- AK  SEND SPI ...\n\r");
            spi_send_data(NULL, 0);
            iRetVal = SUCCESS;
        }
    #endif // AK_SPI_TEST*/
        else if (!strcmp(pcInpString, "wificonnect"))
        {
            iRetVal = cmd_set_wifi(pcInpString);
            if (iRetVal<0)
                UART_PRINT("FAIL\r\n");
            else
                UART_PRINT("OK\r\n");
            iRetVal = SUCCESS;
        }
        else if (!strcmp(pcInpString, "cdsvalue"))
        {
            UART_PRINT("%d\r\nOK\r\n", adc_cds_read());
            iRetVal = SUCCESS;
        }
        else if (!strcmp(pcInpString, "bat"))
        {
#ifndef NTP_CHINA
            UART_PRINT("Bat1: %dmV, Bat2: %dmV \r\nOK\r\n", adc_bat1_read(), adc_bat2_read());
#else
            UART_PRINT("Bat: %dmV \r\nOK\r\n", adc_bat_read());
#endif
            iRetVal = SUCCESS;
        }
        else if (!strcmp(pcInpString, "calling"))
        {
	        int queue_msg = 11; // Not really important, but set to 11
            osi_MsgQWrite(&g_send_notification_flag, &queue_msg, OSI_NO_WAIT);
            UART_PRINT("OK\r\n");
            iRetVal = SUCCESS;
        }
        else if (!strcmp(pcInpString, "setpara"))
        {
            httpServerOn=0;
            iRetVal = cmd_set_parameter(pcInpString);
            if (iRetVal<0){
                UART_PRINT("FAIL\r\n");
            }else{
                UART_PRINT("OK\r\n");
                g_exit_loop_http=1;
            }
            httpServerOn=1;
            iRetVal = SUCCESS;
        }
        else
        {
            UART_PRINT("Unsupported command\n\r");
            return FAILURE;
        }
    }

    return iRetVal;
}

//*****************************************************************************
//                           TASK IMPLEMENTATION
//*****************************************************************************
void ShellTask(void * param)
{
    int lRetVal;
    char acCmdStore[512];

    while (FOREVER)
    {
        //
        // Provide a prompt for the user to enter a command
        //
        //DisplayPrompt();
        //
        // Get the user command line
        //
        lRetVal = GetCmd(acCmdStore, sizeof(acCmdStore));

        if (lRetVal < 0)
        {
            //
            // Error in parsing the command as length is exceeded.
            //
            UART_PRINT("Command length exceeded 512 bytes \n\r");
            //DisplayUsage();
        }/*
        else if (lRetVal == 0)
        {
            //
            // No input. Just an enter pressed probably. Display a prompt.
            //
            // Press enter to go into CLI mode
            // 
            disable_deep_sleep();
            
        }*/
        else
        {
            //
            // Parse the user command and try to process it.
            //
            lRetVal = ParseNProcessCmd(acCmdStore);
            if (lRetVal < 0)
            {
                UART_PRINT("Error in processing command\n\r");
                //DisplayUsage();
            }
        }

    }
}

////////////////////////////////////////////////////////////////////////////////
/// COMMAND DEFINE HERE
////////////////////////////////////////////////////////////////////////////////

int32_t cmd_mac_address(char *param) {
    char mac[13] = {0};
    get_mac_address(mac);
    UART_PRINT("%s\r\nOK\r\n", mac);
    return 0;
}
int32_t cmd_set_parameter(char *param)
{
    char *token = NULL;
    char *timezone = NULL;
    char *api_url=NULL;
    char *mqtt_url=NULL;
    char *ntp_url=NULL;
    char *rms_url=NULL;
    char *stun_url=NULL;
    char *ssid = NULL;
    char *passwd = NULL;
    char *wap2 = NULL;

    //int lRetVal = 0;

    token = strtok(NULL, " ");
//     if (token == NULL) {
//     	return -1;
//     }
    timezone = strtok(NULL, " ");
//     if (timezone == NULL) {
//     	return -1;
//     }
    api_url = strtok(NULL, " ");
//     if (api_url == NULL) {
//     	return -1;
//     }
    mqtt_url = strtok(NULL, " ");
//     if (mqtt_url == NULL) {
//     	return -1;
//     }
    ntp_url = strtok(NULL, " ");
//     if (ntp_url == NULL) {
//     	return -1;
//     }
    rms_url = strtok(NULL, " ");
//     if (rms_url == NULL) {
//     	return -1;
//     }
    stun_url = strtok(NULL, " ");
//     if (stun_url == NULL) {
//     	return -1;
//     }
    ssid = strtok(NULL, " ");
//     if (ssid == NULL) {
//     	return -1;
//     }
    passwd = strtok(NULL, " ");
//     if (passwd == NULL) {
//     	return -1;
//     }
    wap2 = strtok(NULL, " ");
//     if (wap2 == NULL) {
//     	return -1;
//     }

    /*UART_PRINT("\t- TO SET parameter: %s %s %s %s %s %s %s %s %s %s\n\r", token, timezone, api_url, mqtt_url, ntp_url, rms_url, stun_url, ssid, passwd, wap2);*/
    set_server_authen(token, timezone);
    set_url(api_url,mqtt_url,ntp_url,rms_url,stun_url);
    set_wireless_config(ssid, passwd, wap2);
    return 0;
}
int32_t cmd_set_wifi(char *param) {
    char *ssid = NULL;
    char *passwd = NULL;
    int lRetVal;

    ssid = strtok(NULL, " ");
    // only show mac address
    if (ssid == NULL) {
        return -1;
    }
    passwd = strtok(NULL, " ");
    if (passwd == NULL) {
        return -1;
    }
    httpServerOn=0;
    UART_PRINT("\t- TRY TO CONNECT: %s %s\n\r", ssid, passwd);
    Network_IF_DeInitDriver();
    lRetVal = Network_IF_InitDriver(ROLE_STA);
    if (lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }
    g_input_state = LED_STATE_AP_CONNECTING;
    lRetVal = Network_IF_Connect2AP_static_ip(ssid, passwd, 5);
    set_down_anyka(0x80|LANGUAGE_ENGLISH|PROMPT_FACTORY);
    g_input_state=LED_STATE_FACTORY_CONNECTED;
    return lRetVal;
}