#include <stdbool.h>
#include "fsm.h"
#include "limit.h"
#include "motor.h"

// TODO: Set a value that works, find a way that's less linked to calls to fsm_update?
#define RAISE_DURATION 1000

state_t _currentState;
uint _raiseCount;

stateChangeCallback_t _stateChangeCallback;

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
            set_motor(motor, lowerLimit->isClosed ? 0 : 1);
            break;
        case RETRACT:
            // When retracting, raise a bit
            set_motor(motor, upperLimit->isClosed ? 0 : -0.5);
            if(++_raiseCount > RAISE_DURATION){
                _change_state(IDLE);
            }
            break;
        case STOW:
        case EMPTY:
            // When empty or stowing, raise applicator until at limit
            set_motor(motor, upperLimit->isClosed ? 0 : -1);
            break;
        default:
            // Don't move motor in idle state
            set_motor(motor, 0);
            break;
    }
}

void fsm_signal(signal_t signal) {
    switch(signal) {
        case START:
            break;
        case RAISE:
            _raiseCount = 0;
            _change_state(RETRACT);
            break;
        case STOP:
            // If stop signal received, begin stowing the manipulator
            _change_state(STOW);
            break;
        case RESET:
            if(_currentState == EMPTY)
                _change_state(IDLE);
            break;
        case LOWER_LIMIT:
            // When loewr limit hit, start retracting and set empty flag
            _change_state(EMPTY);
        case UPPER_LIMIT:
            // When upper limit hit, switch to idle if not in empty state
            if(_currentState != EMPTY)
                _change_state(IDLE);
    }
}