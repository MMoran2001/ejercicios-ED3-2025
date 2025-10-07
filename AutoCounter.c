/**
 * @file LPC1769_registers.c
 * @brief Controls a 7-segment display using GPIO pins on the LPC1769 board.
 *
 * This program configures GPIO pins P2.0-P2.6 to drive a 7-segment display.
 * It cycles through hexadecimal digits (0-F) by updating the display in a loop.
 * The display segments are controlled via direct register access.
 */

#include "LPC17xx.h"

/**
 * @def DELAY
 * @brief Delay constant for LED timing.
 */
#define DELAY 2500

/**
 * @brief Configures GPIO pins P2.0-P2.6 as outputs to control a 7-segment display.
 *
 * Sets the pin function to GPIO and sets the direction to output.
 * Turns off all segments initially.
 */
void configGPIO(void);

/**
 * @brief Generates a blocking delay using nested loops.
 */
void delay();

int main(void) {
    configGPIO();

    volatile uint32_t i = 0;
    const uint32_t digits[16] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
                                 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};

    while (1) {
        LPC_GPIO2->FIOCLR = 0x7F;               // Turns off all segments.
        LPC_GPIO2->FIOSET |= digits[i % 16];    // Sets the segments for the current digit.

        i++;
        delay();
    }
    return 0;
}

void configGPIO(void) {
    LPC_PINCON->PINSEL4 &= ~(0x3FFF);           // Sets P2.0-P2.6 as GPIO.

    LPC_GPIO2->FIODIR |= 0x7F;                  // Sets P2.0-P2.6 as output.

    LPC_GPIO2->FIOCLR = 0x7F;                   // Turns off all segments.
}

void delay() {
    for (uint32_t i = 0; i < DELAY; i++)
        for (uint32_t j = 0; j < DELAY; j++);
}
