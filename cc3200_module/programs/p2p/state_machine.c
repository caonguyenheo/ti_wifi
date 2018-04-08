#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "osi.h"
#include "common.h"
#include "uart_if.h"
#include "state_machine.h"
#include "sleep_profile.h"
#include "spi_slave_handler.h"
#include "system.h"
#if (LPDS_MODE == 0)
#include "hw_types.h"
#include "prcm.h"
#include "rom_map.h"
#endif
#include "wlan.h"
#include "cc3200_system.h"

extern void RemoveTimerAsWkUp();

extern OsiSyncObj_t g_start_streaming;

#define MAX_FSM_QUEUE_LEN		5
OsiMsgQ_t g_fsm_event_queue;
extern int ota_done;
extern int mqtt_noKL_toreset;
void state_machine(void *pvParameters) {
	int32_t ret_val = 0;
	int32_t event_id = 0;
	while (ota_done==0)
		osi_Sleep(1000);
	state_machine_t cc3200_machine;
	ret_val = fsm_init(&cc3200_machine);
	if (ret_val != 0) {
		UART_PRINT("Init state machine failed\r\n");
		vTaskDelete(NULL);
	}
	sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    UART_PRINT("Start state machine");
    while(1) {
    	ret_val = osi_MsgQRead(&g_fsm_event_queue, &event_id, OSI_WAIT_FOREVER);
    	if (ret_val != 0) {
    		//UART_PRINT("ERROR RECV FSM event\r\n");
    		// OR break???
    		continue;
    	}
    	//UART_PRINT("RECV event=%d\r\n", event_id);
    	ret_val = fsm_state_transition(&cc3200_machine, event_id);
    	if (ret_val != 0) {
    		UART_PRINT("ERROR: invalid event\r\n");
    	}
    }
    vTaskDelete(NULL);
}

#define INIT_STATE			0

// prototype
void state_init_handler();
void state_stop_wait_handler();
void state_start_stream_handler();
void state_streaming_handler();
void state_stop_streaming_handler();

state_t STATE_TABLE[eSTATE_MAX] = {
	{eSTATE_INIT, state_init_handler},
	{eSTATE_STOP_WAIT, state_stop_wait_handler},
	{eSTATE_START, state_start_stream_handler},
	{eSTATE_STREAMING, state_streaming_handler},
	{eSTATE_STOP_STREAMING, state_stop_streaming_handler},
};

transition_t tran_table[] = {
	{eEVENT_INIT_DONE, 			eSTATE_INIT, 			eSTATE_STOP_WAIT},
	{eEVENT_START_STREAMING, 	eSTATE_STOP_WAIT, 		eSTATE_START},
	{eEVENT_STREAMING, 			eSTATE_START, 			eSTATE_STREAMING},
	{eEVENT_STOP_STREAMING, 	eSTATE_STREAMING, 		eSTATE_STOP_STREAMING},
	{eEVENT_STOP_WAIT, 			eSTATE_STOP_STREAMING, 	eSTATE_STOP_WAIT}
};
int32_t tran_table_s = sizeof(tran_table) / sizeof(transition_t); 

// Create message queue, must be call first
int32_t fsm_init_env() {
	int32_t ret_val = 0;
    //
    // message queue for state machine
    //
    ret_val = osi_MsgQCreate(&g_fsm_event_queue, NULL, sizeof( int ),
       MAX_FSM_QUEUE_LEN);
    if(ret_val < 0)
    {
        UART_PRINT("could not create msg queue\n\r");
        return ret_val;
    }

	return 0;
}
// Init fsm
int32_t fsm_init(state_machine_t *fsm) {
	if (fsm == NULL) {
		return -1;
	}

	memset(fsm, 0x00, sizeof(state_machine_t));
	// FIXME: how to invoke init state handler
	fsm->cur_state 		= &(STATE_TABLE[INIT_STATE]);
	fsm->prev_state 	= NULL;
	fsm->tran_table 	= tran_table; 	// hardcode
	fsm->state_table 	= STATE_TABLE;  // hardcode
	return 0;
}
int32_t fsm_add_transition(state_machine_t *fsm, transition_t *tran) {
	return 0;
}
int32_t fsm_state_transition(state_machine_t *fsm, uint32_t event) {
	if (fsm == NULL) {
		UART_PRINT("ERROR: fsm is NULL\r\n");
		return -1;
	}

	if (event >= eEVENT_MAX) {
		UART_PRINT("ERROR: invalid event value %d\r\n", event);
		return -2;
	}

	// FIND STATE
	state_t *next_state = NULL;
	next_state = fsm_next_state(fsm, event);
	if (next_state == NULL) {
		UART_PRINT("ERROR: Invalid event for current state, state = %d, event = %d\r\n"
			, fsm->cur_state->state_id, event);
		return -3;
	}
	
	// EXECUTE STATE
	next_state->handler();
	fsm->prev_state = fsm->cur_state;
	fsm->cur_state = next_state;
	return 0;
}
state_t *fsm_next_state(state_machine_t *fsm, uint32_t event) {
	if (fsm == NULL) {
		return NULL;
	}

	if (event >= eEVENT_MAX) {
		return NULL;
	}
	#if 0
	int size_tran_table = sizeof(fsm->tran_table) / sizeof(transition_t);
	#else
	int size_tran_table = tran_table_s;
	#endif
//	UART_PRINT("DEBUG: size_tran_table = %d\r\n", size_tran_table);
	int i = 0;
	for (i = 0; i < size_tran_table; i++) {
		if (fsm->tran_table[i].event_id == event 
			&& fsm->tran_table[i].cur_state_id == fsm->cur_state->state_id) {
			// FIND NEXT STATE
			int j = 0;
			for (j = 0; j < eSTATE_MAX; j++) {
				if (fsm->state_table[j].state_id == fsm->tran_table[i].next_state_id) {
					return &(fsm->state_table[j]);
				}
			}
			return NULL;
		}
	}
	return NULL;
}

void state_init_handler() {
	UART_PRINT("state_init_handler\r\n");
}
#define LSI_SLEEP_DURATION_IN_MSEC 1000
_u16 PolicyBuff[4] = {0,0,LSI_SLEEP_DURATION_IN_MSEC,0}; /* PolicyBuff[2] is max sleep time in mSec */
void set_low_power(){
	sl_WlanPolicySet(SL_POLICY_PM , SL_LONG_SLEEP_INTERVAL_POLICY, (_u8*)PolicyBuff,sizeof(PolicyBuff));
}
void wdt_de_init(void);
int call_button_activate=0;
extern void motion_upload_close(void);
void state_stop_wait_handler() {
	motion_upload_close();
	wdt_de_init();
	UART_PRINT("state_stop_wait_handler\r\n");
	push_timestamp = 0;
	set_low_power();
	#if (LPDS_MODE)
    enable_deep_sleep();
    #else
    if (MAP_PRCMSysResetCauseGet() != PRCM_HIB_EXIT) {
    	RemoveTimerAsWkUp();
    	enable_deep_sleep();
    }
    #endif
    call_button_activate=1;
}
int send_cds_request=-1;
extern void wdt_init(int l_time_ms);
void state_start_stream_handler() {
	// UART_PRINT("state_start_stream_handler\r\n");
	int l_bat1, l_bat2;
	long lRetVal = -1;
	lRetVal = sl_WlanPolicySet(SL_POLICY_PM , SL_LOW_LATENCY_POLICY, NULL, 0);
	spi_thread_pause=0;
	ak_power_up();
    
	mqtt_noKL_toreset=DEFAULT_MQTT_NOOK_TORESET_WAKEUP;
	send_cds_request=0;
	UART_PRINT("state_start_stream_handler, return_code=%d\r\n", lRetVal);

	#if (LPDS_MODE)
	disable_deep_sleep();
	#endif
	wdt_init(15*1000);
	
#ifndef NTP_CHINA
	l_bat1=adc_bat1_read();
	l_bat2=adc_bat2_read();
	ak_bat1_set(1);
	ak_bat2_set(1);
	osi_Sleep(10);
	
	if (l_bat2>l_bat1){
		UART_PRINT("USE BK BAT------\r\n");
		ak_bat1_set(0);
	} else {
		UART_PRINT("USE Normal BAT------\r\n");
		ak_bat2_set(0);
	}
#else
	adc_bat_read();
#endif
}
void state_streaming_handler() {
	osi_SyncObjSignal(&g_start_streaming);
	UART_PRINT("state_streaming_handler\r\n");
}
void state_stop_streaming_handler() {
	send_cds_request=-1;
	UART_PRINT("state_stop_streaming_handler\r\n");

	ak_power_down();
	spi_thread_pause=1;
	mqtt_noKL_toreset=DEFAULT_MQTT_NOKL_TORESET_SLEEP;
	signal_event(eEVENT_STOP_WAIT);
	#if (LPDS_MODE == 0)
	// for hibernate case, This is the end of flow
    RemoveTimerAsWkUp();
	enable_deep_sleep();
	#endif
}
int32_t signal_event(int32_t event) {
	UART_PRINT("EVENT = %d\r\n", event);
    return osi_MsgQWrite(&g_fsm_event_queue, &event, OSI_NO_WAIT);
}
