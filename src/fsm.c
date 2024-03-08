#include <stdbool.h>
#include "fsm.h"
#include "limit.h"
#include "motor.h"

#define ACTIVE_SPEED 0.4
#define RAISE_SPEED 1
#define RETRACT_SPEED RAISE_SPEED / 2.0

// TODO: Set a value that works, find a way that's less linked to calls to fsm_update?
#define RAISE_DURATION 1000

state_t _currentState;
uint _raiseCount;

stateChangeCallback_t _stateChangeCallback;

// Make sure these match the order in the header
const char* STATE_NAMES[] = {
    "IDLE", "ACTIVE", "RETRACT", "STOW", "EMPTY"
};
const char* SIGNAL_NAMES[] = {
    "START", "RAISE", "STOP", "RESET", "LOWER LIMIT", "UPPER LIMIT"
};


void _change_state(state_t newState){
    state_t old = _currentState;
    _currentState = newState;
    _stateChangeCallback(old, _currentState);
}

void fsm_init(stateChangeCallback_t callback){
    _stateChangeCallback = callback;
    // Initialize into idle state
    _change_state(IDLE);
}

state_t fsm_current_state() {
    return _currentState;
}

void fsm_update(motor_t *motor, limit_t *upperLimit, limit_t *lowerLimit) {
    switch(_currentState) {
        case ACTIVE:
            // When empty or stowing, lower applicator until the limit is hit
            motor_set(motor, lowerLimit->isClosed ? 0 : ACTIVE_SPEED);
            break;
        case RETRACT:
            // When retracting, raise a bit
            motor_set(motor, upperLimit->isClosed ? 0 : -0.5);
            if(++_raiseCount > RAISE_DURATION){
                fsm_signal(RAISE_TIMEOUT);
            }
            break;
        case STOW:
        case EMPTY:
            // When empty or stowing, raise applicator until at limit
            motor_set(motor, upperLimit->isClosed ? 0 : -RAISE_SPEED);
            break;
        default:
            // Don't move motor in idle state
            motor_set(motor, 0);
            break;
    }
}

void fsm_signal(signal_t signal) {
    switch(signal) {
        case START:
            if(_currentState == IDLE)
                _change_state(ACTIVE);
            break;
        case RAISE:
            if(_currentState == ACTIVE || _currentState == IDLE){
                _raiseCount = 0;
                _change_state(RETRACT);
            }
            break;
        case STOP:
            // If stop signal received, begin stowing the manipulator
            if(_currentState != EMPTY)
                _change_state(STOW);
            break;
        case RESET:
            if(_currentState == EMPTY)
                _change_state(IDLE);
            break;
        case LOWER_LIMIT:
            // When loewr limit hit, start retracting and set empty flag
            _change_state(EMPTY);
            break;
        case UPPER_LIMIT:
            // When upper limit hit, switch to idle if not in empty state
            if(_currentState != EMPTY)
                _change_state(IDLE);
            break;
        case RAISE_TIMEOUT:
            if(_currentState == RETRACT)
                _change_state(IDLE);
            break;
    }
}

const char* fsm_state_name(state_t state) {
    return STATE_NAMES[state];
}

const char* fsm_signal_name(signal_t signal) {
    return SIGNAL_NAMES[signal];
}