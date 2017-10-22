#ifndef GPIO_H_
#define GPIO_H_

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <stdint.h>
#include <stdio.h>

// Initialize the port and pin as outputs.
void GPIOinitOut(uint8_t portNum, uint32_t pinNum);

// Initialize the port and pin as input.
void GPIOinitIn(uint8_t portNum, uint32_t pinNum);

// Activate the pin
void setGPIO(uint8_t portNum, uint32_t pinNum);

// Deactivate the pin
void clearGPIO(uint8_t portNum, uint32_t pinNum);

void delay(uint32_t delay_ms);


#endif /* GPIO_H_ */
