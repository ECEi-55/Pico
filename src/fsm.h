#ifndef FSM_H
#define FSM_H
#include "limit.h"
#include "motor.h"


/**
 * FSM States
*/
typedef enum state_t {
    IDLE,
    ACTIVE,
    RETRACT,
    STOW,
    EMPTY
} state_t;

/**
 * FSM control signals
*/
typedef enum signal_t {
    START,
    RAISE,
    STOP,
    RESET,
    LOWER_LIMIT,
    UPPER_LIMIT
} signal_t;

/**
 * Callback invoked when state changes
 * First parameter is new state, second parameter is old state
*/
typedef void (*stateChangeCallback_t)(state_t, state_t);

/**
 * @brief Initialize the FSM
 * 
 * @param callback Callback to be invoked when the state is changed
*/
void fsm_init(stateChangeCallback_t callback);

/**
 * @brief Periodic update for the FSM
*/
void fsm_update(motor_t *motor, limit_t *upperLimit, limit_t *lowerLimit);

/**
 * @brief Get the current FSM state
*/
state_t fsm_current_state();
/**
 * @brief Send a signal to the fsm
*/
void fsm_signal(signal_t signal);

/**
 * @brief Get the name of a state
 * @param state State to get name of
*/
const char* fsm_state_name(state_t state);

/**
 * @brief Get the name of a signal
 * @param signal Signal to get name of
*/
const char* fsm_signal_name(signal_t signal);
#endif