#ifndef __STATE_MACHINE_H__
#define __STATE_MACHINE_H__
#include "stdint.h"
#include "osi.h"
extern OsiMsgQ_t g_fsm_event_queue;

enum STATE {
	eSTATE_INIT = 0,
	eSTATE_STOP_WAIT,
	eSTATE_START,
	eSTATE_STREAMING,
	eSTATE_STOP_STREAMING,
	eSTATE_MAX
};

enum EVENT {
	eEVENT_INIT_DONE = 0,
	eEVENT_START_STREAMING,
	eEVENT_STREAMING,
	eEVENT_STOP_STREAMING,
	eEVENT_STOP_WAIT,
	eEVENT_MAX
};
typedef void (* state_handler_f)(void);
typedef struct state {
	uint32_t state_id;
	state_handler_f handler;
} state_t;

typedef struct transition {
	uint32_t event_id;
	uint32_t cur_state_id;
	uint32_t next_state_id;
} transition_t;

typedef struct state_machine {
	state_t *cur_state;
	state_t *prev_state;
	transition_t *tran_table;
	state_t *state_table;
} state_machine_t;

void state_machine(void *pvParameters);
int32_t fsm_init_env();
int32_t fsm_init(state_machine_t *fsm);
int32_t fsm_add_transition(state_machine_t *fsm, transition_t *tran);
int32_t fsm_state_transition(state_machine_t *fsm, uint32_t event);
state_t *fsm_next_state(state_machine_t *fsm, uint32_t event);
int32_t signal_event(int32_t event);
#endif // __STATE_MACHINE_H__