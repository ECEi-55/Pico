#include <stdlib.h>
#include <stdbool.h>
#ifndef LIMIT_H
#define LIMIT_H

typedef struct {
    unsigned char _ncPin;
    bool isClosed;
    void (*_pressedCallback)(void);
} limit_t;

/**
 * @brief Initialize a limit switch
 * 
 * @param limit Limit switch struct to initialize
 * @param ncPin Pin used for normally-closed connection
 * @param pressedCallback Callback fired when limit is pressed
 */
void limit_init(limit_t *limit, unsigned char ncPin, void (*pressedCallback)(void));

/**
 * @brief 
 * 
 * @param limit 
 */
void limit_update(limit_t *limit);

#endif