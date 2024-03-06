#include <stdlib.h>
#include "pico/stdlib.h"
#include "limit.h"

void limit_init(limit_t *limit, unsigned char ncPin, void (*pressedCallback)(void)){
    // Configure the pico pin
    gpio_init(ncPin);
    gpio_set_dir(ncPin, GPIO_IN);
    gpio_pull_up(ncPin);

    limit->_ncPin = ncPin;
    limit->_pressedCallback = pressedCallback;
    limit_update(limit);
}

void limit_update(limit_t *limit){
    // Get current state
    bool current = gpio_get(limit->_ncPin);
    // If there's a rising edge, set callback flag
    bool callback = current && !limit->isClosed;
    
    // Update limit state
    limit->isClosed = current;
    // Fire callback if applicable
    if(limit->_pressedCallback && callback) (limit->_pressedCallback)();
}