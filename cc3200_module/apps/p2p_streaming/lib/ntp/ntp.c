//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ // 
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


//*****************************************************************************
//
// Application Name     -   Get Time
// Application Overview -   Get Time application connects to a SNTP server,
//                            request for time, process the data and displays
//                            the time on Hyperterminal. The communication between
//                            the device and server is based on the Network Time
//                            Protocol (NTP)
//
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_Info_Center_Get_Time_Application
// or
// docs\examples\CC32xx_Info_Center_Get_Time_Application.pdf
//
//*****************************************************************************


//****************************************************************************
//
//! \addtogroup get_time
//! @{
//
//****************************************************************************

// Standard includes
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// simplelink includes
#include "simplelink.h"

// driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "prcm.h"
#include "pin.h"
#include "rom.h"
#include "rom_map.h"
#include "timer.h"
#include "utils.h"
#include "system.h"

//Free_rtos/ti-rtos includes
#include "osi.h"

// common interface includes
#include "network_if.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "gpio_if.h"
#include "timer_if.h"
#include "udma_if.h"
#include "common.h"
#include "ntp.h"
#include "date_time.h"

#include "FreeRTOS.h"
#include "task.h"

// #define APP_NAME                "Get Time"
// #define APPLICATION_VERSION     "1.1.1"

#define TIME2013                3565987200u      /* 113 years + 28 days(leap) */
#define YEAR2013                2013
// UTC
#define TIME1970                2208988800u      /* (70*365 + 17)*86400 = 2208988800 */
#define YEAR1970                1970
#define SEC_IN_MIN              60
#define SEC_IN_HOUR             3600
#define SEC_IN_DAY              86400

#define SERVER_RESPONSE_TIMEOUT 5
#define GMT_DIFF_TIME_HRS       5
#define GMT_DIFF_TIME_MINS      30

#define SLEEP_TIME              (1000*60*30)
// #define OSI_STACK_SIZE          2048
#define NTP_STAND_ALONE_APP     1 // 0 - not waiting for network

#define DEBUG_NTP               0

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
extern OsiMsgQ_t g_tConnectionFlag;
extern OsiSyncObj_t g_NTP_init_done_flag;
extern OsiMsgQ_t g_MQTT_check_msg_flag;
extern OsiSyncObj_t g_getNTP;
// Application specific status/error codes
typedef enum{
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    SERVER_GET_TIME_FAILED = -0x7D0,
    DNS_LOOPUP_FAILED = SERVER_GET_TIME_FAILED  -1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;
extern char g_acSNTPserver[NUM_NTP_SERVER][NTP_RUL_MAXLEN];
unsigned short g_usTimerInts;
SlSecParams_t SecurityParams = {0};

//!    ######################### list of SNTP servers ##################################
//!    ##
//!    ##          hostname         |        IP       |       location
//!    ## -----------------------------------------------------------------------------
//!    ##   nist1-nj2.ustiming.org  | 165.193.126.229 |  Weehawken, NJ
//!    ##   nist1-pa.ustiming.org   | 206.246.122.250 |  Hatfield, PA
//!    ##   time-a.nist.gov         | 129.6.15.28     |  NIST, Gaithersburg, Maryland
//!    ##   time-b.nist.gov         | 129.6.15.29     |  NIST, Gaithersburg, Maryland
//!    ##   time-c.nist.gov         | 129.6.15.30     |  NIST, Gaithersburg, Maryland
//!    ##   ntp-nist.ldsbc.edu      | 198.60.73.8     |  LDSBC, Salt Lake City, Utah
//!    ##   nist1-macon.macon.ga.us | 98.175.203.200  |  Macon, Georgia
//!
//!    ##   For more SNTP server link visit 'http://tf.nist.gov/tf-cgi/servers.cgi'
//!    ###################################################################################

static uint16_t ntp_server_idx = 0;

#if (DEBUG_NTP)
// Tuesday is the 1st day in 2013 - the relative year
const char g_acDaysOfWeek2013[7][3] = {{"Tue"},
                                    {"Wed"},
                                    {"Thu"},
                                    {"Fri"},
                                    {"Sat"},
                                    {"Sun"},
                                    {"Mon"}};

const char g_acMonthOfYear[12][3] = {{"Jan"},
                                  {"Feb"},
                                  {"Mar"},
                                  {"Apr"},
                                  {"May"},
                                  {"Jun"},
                                  {"Jul"},
                                  {"Aug"},
                                  {"Sep"},
                                  {"Oct"},
                                  {"Nov"},
                                  {"Dec"}};

const char g_acNumOfDaysPerMonth[12] = {31, 28, 31, 30, 31, 30,
                                        31, 31, 30, 31, 30, 31};
#endif

// const char g_acDigits[] = "0123456789";

static struct
{
    unsigned long ulDestinationIP;
    int iSockID;
    unsigned long ulElapsedSec;
    short isGeneralVar;
    unsigned long ulGeneralVar;
    unsigned long ulGeneralVar1;
    char acTimeStore[30];
    char *pcCCPtr;
    unsigned short uisCCLen;
}g_sAppData;

static SlSockAddr_t sAddr;
static SlSockAddrIn_t sLocalAddr;
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************



//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//*****************************************************************************
static long GetSNTPTime(unsigned char ucGmtDiffHr, unsigned char ucGmtDiffMins);
// static void DisplayBanner(char * AppName);


//*****************************************************************************
//
//! Gets the current time from the selected SNTP server
//!
//! \brief  This function obtains the NTP time from the server.
//!
//! \param  GmtDiffHr is the GMT Time Zone difference in hours
//! \param  GmtDiffMins is the GMT Time Zone difference in minutes
//!
//! \return 0 : success, -ve : failure
//!
//
//*****************************************************************************
long GetSNTPTime(unsigned char ucGmtDiffHr, unsigned char ucGmtDiffMins)
{
  
/*
                            NTP Packet Header:


       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9  0  1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |LI | VN  |Mode |    Stratum    |     Poll      |   Precision    |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                          Root  Delay                           |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                       Root  Dispersion                         |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                     Reference Identifier                       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                    Reference Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                    Originate Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                     Receive Timestamp (64)                     |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                     Transmit Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                 Key Identifier (optional) (32)                 |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                                                                |
      |                 Message Digest (optional) (128)                |
      |                                                                |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
    char cDataBuf[48];
    long lRetVal = 0;
    int iAddrSize;
    //
    // Send a query ? to the NTP server to get the NTP time
    //
    memset(cDataBuf, 0, sizeof(cDataBuf));
    cDataBuf[0] = '\x1b';

    sAddr.sa_family = AF_INET;
    // the source port
    sAddr.sa_data[0] = 0x00;
    sAddr.sa_data[1] = 0x7B;    // UDP port number for NTP is 123
    sAddr.sa_data[2] = (char)((g_sAppData.ulDestinationIP>>24)&0xff);
    sAddr.sa_data[3] = (char)((g_sAppData.ulDestinationIP>>16)&0xff);
    sAddr.sa_data[4] = (char)((g_sAppData.ulDestinationIP>>8)&0xff);
    sAddr.sa_data[5] = (char)(g_sAppData.ulDestinationIP&0xff);

    lRetVal = sl_SendTo(g_sAppData.iSockID,
                     cDataBuf,
                     sizeof(cDataBuf), 0,
                     &sAddr, sizeof(sAddr));
    if (lRetVal != sizeof(cDataBuf))
    {
        // could not send SNTP request
        ASSERT_ON_ERROR(SERVER_GET_TIME_FAILED);
    }

    //
    // Wait to receive the NTP time from the server
    //
    iAddrSize = sizeof(SlSockAddrIn_t);

    lRetVal = sl_RecvFrom(g_sAppData.iSockID,
                       cDataBuf, sizeof(cDataBuf), 0,
                       (SlSockAddr_t *)&sLocalAddr,
                       (SlSocklen_t*)&iAddrSize);
    ASSERT_ON_ERROR(lRetVal);

    //
    // Confirm that the MODE is 4 --> server
    //
    if ((cDataBuf[0] & 0x7) != 4)    // expect only server response
    {
         ASSERT_ON_ERROR(SERVER_GET_TIME_FAILED);  // MODE is not server, abort
    }
    else
    {
        unsigned char iIndex;

        //
        // Getting the data from the Transmit Timestamp (seconds) field
        // This is the time at which the reply departed the
        // server for the client
        //
        g_sAppData.ulElapsedSec = cDataBuf[40];
        g_sAppData.ulElapsedSec <<= 8;
        g_sAppData.ulElapsedSec += cDataBuf[41];
        g_sAppData.ulElapsedSec <<= 8;
        g_sAppData.ulElapsedSec += cDataBuf[42];
        g_sAppData.ulElapsedSec <<= 8;
        g_sAppData.ulElapsedSec += cDataBuf[43];

        UART_PRINT("Transmit timestamp %u ref from 0h 1Jan1900, UTC %u\r\n",g_sAppData.ulElapsedSec,g_sAppData.ulElapsedSec - TIME1970); 
        if (g_sAppData.ulElapsedSec<TIME2013)
            return -1;
            
        /////////////////////////////////////////////////
        /// SYNC DATE_TIME
        /////////////////////////////////////////////////
        if (dt_set_time(g_sAppData.ulElapsedSec - TIME1970, 0) != 0) {
          UART_PRINT("ERROR set DATE_TIME\n");
        }

        // sync
        osi_SyncObjSignal(&g_NTP_init_done_flag);
        #if (DEBUG_NTP)
        //
        // seconds are relative to 0h on 1 January 1900
        //
        g_sAppData.ulElapsedSec -= TIME2013;

        //
        // in order to correct the timezone
        //
        g_sAppData.ulElapsedSec += (ucGmtDiffHr * SEC_IN_HOUR);
        g_sAppData.ulElapsedSec += (ucGmtDiffMins * SEC_IN_MIN);

        g_sAppData.pcCCPtr = &g_sAppData.acTimeStore[0];

        //
        // day, number of days since beginning of 2013
        //
        g_sAppData.isGeneralVar = g_sAppData.ulElapsedSec/SEC_IN_DAY;
        memcpy(g_sAppData.pcCCPtr,
               g_acDaysOfWeek2013[g_sAppData.isGeneralVar%7], 3);
        g_sAppData.pcCCPtr += 3;
        *g_sAppData.pcCCPtr++ = '\x20';

        //
        // month
        //
        g_sAppData.isGeneralVar %= 365;
        for (iIndex = 0; iIndex < 12; iIndex++)
        {
            g_sAppData.isGeneralVar -= g_acNumOfDaysPerMonth[iIndex];
            if (g_sAppData.isGeneralVar < 0)
                    break;
        }
        if(iIndex == 12)
        {
            iIndex = 0;
        }
        memcpy(g_sAppData.pcCCPtr, g_acMonthOfYear[iIndex], 3);
        g_sAppData.pcCCPtr += 3;
        *g_sAppData.pcCCPtr++ = '\x20';

        //
        // date
        // restore the day in current month
        //
        g_sAppData.isGeneralVar += g_acNumOfDaysPerMonth[iIndex];
        g_sAppData.uisCCLen = itoa(g_sAppData.isGeneralVar + 1,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;
        *g_sAppData.pcCCPtr++ = '\x20';

        //
        // time
        //
        g_sAppData.ulGeneralVar = g_sAppData.ulElapsedSec%SEC_IN_DAY;

        // number of seconds per hour
        g_sAppData.ulGeneralVar1 = g_sAppData.ulGeneralVar%SEC_IN_HOUR;

        // number of hours
        g_sAppData.ulGeneralVar /= SEC_IN_HOUR;
        g_sAppData.uisCCLen = itoa(g_sAppData.ulGeneralVar,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;
        *g_sAppData.pcCCPtr++ = ':';

        // number of minutes per hour
        g_sAppData.ulGeneralVar = g_sAppData.ulGeneralVar1/SEC_IN_MIN;

        // number of seconds per minute
        g_sAppData.ulGeneralVar1 %= SEC_IN_MIN;
        g_sAppData.uisCCLen = itoa(g_sAppData.ulGeneralVar,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;
        *g_sAppData.pcCCPtr++ = ':';
        g_sAppData.uisCCLen = itoa(g_sAppData.ulGeneralVar1,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;
        *g_sAppData.pcCCPtr++ = '\x20';

        //
        // year
        // number of days since beginning of 2013
        //
        g_sAppData.ulGeneralVar = g_sAppData.ulElapsedSec/SEC_IN_DAY;
        g_sAppData.ulGeneralVar /= 365;
        g_sAppData.uisCCLen = itoa(YEAR2013 + g_sAppData.ulGeneralVar,
                                   g_sAppData.pcCCPtr);
        g_sAppData.pcCCPtr += g_sAppData.uisCCLen;

        *g_sAppData.pcCCPtr++ = '\0';

        UART_PRINT("response from server: ");
        UART_PRINT(&g_acSNTPserver[ntp_server_idx][0]);
        UART_PRINT("\n\r");
        UART_PRINT(g_sAppData.acTimeStore);
        UART_PRINT("\n\r\n\r");
        #endif
    }
    return SUCCESS;
}
//*****************************************************************************
//
//! Periodic Timer Interrupt Handler
//!
//! \param None
//!
//! \return None
//
//*****************************************************************************
// void
// TimerPeriodicIntHandler(void)
// {
//     unsigned long ulInts;

//     //
//     // Clear all pending interrupts from the timer we are
//     // currently using.
//     //
//     ulInts = MAP_TimerIntStatus(TIMERA0_BASE, true);
//     MAP_TimerIntClear(TIMERA0_BASE, ulInts);

//     //
//     // Increment our interrupt counter.
//     //
//     g_usTimerInts++;
//     if(!(g_usTimerInts & 0x1))
//     {
//         //
//         // Off Led
//         //
//         GPIO_IF_LedOff(MCU_RED_LED_GPIO);
//     }
//     else
//     {
//         //
//         // On Led
//         //
//         GPIO_IF_LedOn(MCU_RED_LED_GPIO);
//     }
// }

//****************************************************************************
//
//! Function to configure and start timer to blink the LED while device is
//! trying to connect to an AP
//!
//! \param none
//!
//! return none
//
//****************************************************************************
// void LedTimerConfigNStart()
// {
//     //
//     // Configure Timer for blinking the LED for IP acquisition
//     //
//     Timer_IF_Init(PRCM_TIMERA0,TIMERA0_BASE,TIMER_CFG_PERIODIC,TIMER_A,0);
//     Timer_IF_IntSetup(TIMERA0_BASE,TIMER_A,TimerPeriodicIntHandler);
//     Timer_IF_Start(TIMERA0_BASE,TIMER_A,100);  // time is in mSec
// }

//****************************************************************************
//
//! Disable the LED blinking Timer as Device is connected to AP
//!
//! \param none
//!
//! return none
//
//****************************************************************************
// void LedTimerDeinitStop()
// {
//     //
//     // Disable the LED blinking Timer as Device is connected to AP
//     //
//     Timer_IF_Stop(TIMERA0_BASE,TIMER_A);
//     Timer_IF_DeInit(TIMERA0_BASE,TIMER_A);

// }

//****************************************************************************
//
//! Task function implementing the gettime functionality using an NTP server
//!
//! \param none
//!
//! This function
//!    1. Initializes the required peripherals
//!    2. Initializes network driver and connects to the default AP
//!    3. Creates a UDP socket, gets the NTP server IP address using DNS
//!    4. Periodically gets the NTP time and displays the time
//!
//! \return None.
//
//****************************************************************************
int get_ntp_status=1;
#define NTP_DURATION_SEC (60*2)
void GetNTPTimeTask(void *pvParameters)
{
    int iSocketDesc;
    long lRetVal = -1;
    int sCntGetNtp = 0;
    uint32_t u32TimeGet = 0, u32PastNtp = 0;
    static int i32NtpClose = 0;
    
    // uint32_t mcuState;
    //UART_PRINT("GET_TIME: Begin\n\r");
#if (NTP_STAND_ALONE_APP)
    unsigned char ucSyncMsg;
    osi_MsgQRead(&g_tConnectionFlag, &ucSyncMsg, OSI_WAIT_FOREVER);
    osi_MsgQWrite(&g_tConnectionFlag, &ucSyncMsg, OSI_NO_WAIT);
#endif

    //
    // Create UDP socket
    //
    // Trung debug

    /*
    iSocketDesc = sl_Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(iSocketDesc < 0)
    {
        ERR_PRINT(iSocketDesc);
        goto end;
    }
    g_sAppData.iSockID = iSocketDesc;

    UART_PRINT("Socket created\n\r");
    */
    i32NtpClose = 1;
    
    /////////////////////////////////////////////////
    /// INIT DATE_TIME
    /////////////////////////////////////////////////
    dt_start();
    

        struct SlTimeval_t timeVal;
        
            /*
        // Trung debug
        timeVal.tv_sec =  SERVER_RESPONSE_TIMEOUT;    // Seconds
        timeVal.tv_usec = 0;     // Microseconds. 10000 microseconds resolution
        lRetVal = sl_SetSockOpt(g_sAppData.iSockID,SL_SOL_SOCKET,SL_SO_RCVTIMEO,\
                        (unsigned char*)&timeVal, sizeof(timeVal)); 
        if(lRetVal < 0)
        {
           ERR_PRINT(lRetVal);
           LOOP_FOREVER();
        }*/
        
        while(1)
        {
            u32TimeGet = dt_get_time_s();

            UART_PRINT("NTP: %d, old %d, url %s \n", u32TimeGet, u32PastNtp, (char*)&g_acSNTPserver[ntp_server_idx][0]);
            if ((u32TimeGet<u32PastNtp)||(u32TimeGet>=(u32PastNtp+NTP_DURATION_SEC))||(u32PastNtp==0)){
                if ((sCntGetNtp++)>=8){
                    sCntGetNtp=0;
                    break;
                }
                ///////////////////////////////////////////////////////////////////
                // Trung debug NTP
                //
                // Create UDP socket
                //
                if(i32NtpClose)
                {
                    iSocketDesc = sl_Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                    if(iSocketDesc < 0)
                    {
                        continue;
                    }
                    g_sAppData.iSockID = iSocketDesc;
                    g_sAppData.ulElapsedSec = 0;
                    UART_PRINT("NTP: Socket created\r\n");
                    timeVal.tv_sec =  SERVER_RESPONSE_TIMEOUT;    // Seconds
                    timeVal.tv_usec = 0;     // Microseconds. 10000 microseconds resolution
                    lRetVal = sl_SetSockOpt(g_sAppData.iSockID,SL_SOL_SOCKET,SL_SO_RCVTIMEO,\
                                    (unsigned char*)&timeVal, sizeof(timeVal)); 
                    if(lRetVal < 0)
                    {
                        if(sl_Close(iSocketDesc) != 0)
                        {
                            UART_PRINT("NTP: Close socket failed\r\n");
                        }
                        continue;
                    }
                    i32NtpClose = 0;
                }
                ///////////////////////////////////////////////////////////////////
    
                // UART_PRINT("GET TIME NTP\n\r");
                //
                // Get the NTP server host IP address using the DNS lookup
                //
                lRetVal = Network_IF_GetHostIP((char*)&g_acSNTPserver[ntp_server_idx][0], \
                                                &g_sAppData.ulDestinationIP);
                if( lRetVal < 0)
                {
                    UART_PRINT("DNS lookup failed. \n\r");
                    ntp_server_idx = (ntp_server_idx+1) % NUM_NTP_SERVER;
                    continue;
                }
    
                //
                // Binding received address
                //
                sLocalAddr.sin_family = SL_AF_INET;
                sLocalAddr.sin_port = 0;
                sLocalAddr.sin_addr.s_addr = 0;
                if(g_sAppData.ulElapsedSec == 0)
                {
                    lRetVal = sl_Bind(g_sAppData.iSockID,
                            (SlSockAddr_t *)&sLocalAddr,
                            sizeof(SlSockAddrIn_t));
                }
    
                //lRetVal = GetSNTPTime(GMT_DIFF_TIME_HRS, GMT_DIFF_TIME_MINS);
                lRetVal = GetSNTPTime(0, 0);
                if(lRetVal < 0)
                {
                    //UART_PRINT("Server Get Time failed\n\r");
                    ntp_server_idx = (ntp_server_idx+1) % NUM_NTP_SERVER;
                    // Trung fix NTp failed
                    UART_PRINT("NTP: Close socket: %d\n", iSocketDesc);
                    if(sl_Close(iSocketDesc) != 0)
                    {
                        UART_PRINT("NTP: Close socket failed!!\n");
                    }
                    i32NtpClose = 1;
                    continue;
                    // Should retry , create a new connection
                    // break;                
                }
                else
                {
                    sCntGetNtp=0;
                    UART_PRINT("NTP:OK!\n");
                    u32PastNtp = dt_get_time_s();
                }
            } else
                osi_SyncObjSignal(&g_NTP_init_done_flag);
            get_ntp_status=0;
            osi_SyncObjWait(&g_getNTP, OSI_WAIT_FOREVER);
            get_ntp_status=1;
            continue;

#if (NTP_STAND_ALONE_APP)
            // unsigned char ucSyncMsg;
            // osi_MsgQRead(&g_MQTT_check_msg_flag, &ucSyncMsg, OSI_WAIT_FOREVER);
#endif            
            //UART_PRINT("NTPsleep:%d\n", SLEEP_TIME);
            //osi_Sleep(SLEEP_TIME);
        }

    for(lRetVal=0;lRetVal<10;lRetVal++)
    	UART_PRINT("NTP-reboot\n\r");
    
    system_reboot();

    //
    // Loop here
    //
    //vTaskDelete(NULL);
    // LOOP_FOREVER();
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
