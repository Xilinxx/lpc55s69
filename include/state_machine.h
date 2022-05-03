/**
 * @file state_machine.h
 * @brief  Some very silly macro's to ease the development of statemachines.
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @version v0.1
 * @date 2020-07-15
 */

#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_

#include <stdio.h>
#include <stdint.h>

#define STATEM_CREATE_NEW_STATE_LIST(nme, ...)   \
	typedef enum {          \
		__VA_ARGS__,                    \
		nme ## _NR_OF_STATES             \
	}nme ## _state_t;

#define STATEM_CREATE_NEW_EVENT_LIST(nme, ...)   \
	typedef enum {          \
		__VA_ARGS__,                    \
		nme ## _NR_OF_EVENTS             \
	}nme ## _event_t;

#define STATEM_CREATE_NEW_STATEMACHINE(nme)     \
	struct nme ## _state_machine_t; \
	typedef int (*nme ## _event_handler_t)(void *st_machine, \
					       int event, \
					       void *data); \
	struct nme ## _state_machine_t { \
		nme ## _state_t current_state;   \
		nme ## _event_handler_t states[nme ## _NR_OF_STATES][nme ## \
								     _NR_OF_EVENTS \
		];        \
	};

#define STATEM_GENERATE_DEFAULT_STATE_HANDLER(nme) \
	int nme ## _default_event_handler( \
		void *st_machine, \
		int event, void *data) \
	{ \
		(void)st_machine; \
		(void)event; \
		(void)data; \
		return 0;                       \
	}

#define STATEM_SETUP_DEFAULT_STATE_HANDLERS(st, nr_states, nr_events, handler) \
	for (int ix = 0; ix < nr_states; ix++) { \
		for (int jx = 0; jx < nr_events; jx++) { \
			st.states[ix][jx] = handler; \
		} \
	}
#define STATEM_SET_STATE_EVENT_HANDLER(st, state, event, handler) \
	st.states[state][event] = handler;

#define STATEM_HANDLE_EVENT(st_machine, event, data) \
	st_machine.states[st_machine.current_state][event]((void *)&st_machine, \
							   event, \
							   data)

#endif /* _STATE_MACHINE_H_ */
