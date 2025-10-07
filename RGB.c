/**
 * @file LPC1769_registers.c
 * @brief Controls the RGB LED sequence on the LPC1769 board using GPIO registers.
 *
 * This program configures the GPIO pins connected to the onboard RGB LED of the LPC1769
 * and cycles through a sequence of colors: red, green, blue, cyan, magenta, yellow, and white.
 * The LEDs are active-low.
 */
#include "LPC17xx.h"

/**
 * @def REDLED
 * @brief Bit mask for the red LED (P0.22).
 */
#define RED_LED      (1<<22)

/**
 * @def GREENLED
 * @brief Bit mask for the green LED (P3.25).
 */
#define GREEN_LED    (1<<25)

/**
 * @def BLUELED
 * @brief Bit mask for the blue LED (P3.26).
 */
#define BLUE_LED     (1<<26)

/**
 * @def DELAY
 * @brief Delay constant for LED timing.
 */
#define DELAY       2500

/**
 * @brief Configures the GPIO pins for the RGB LEDs as outputs.
 *
 * Sets the pin function to GPIO and configures the direction as output
 * for each color channel.
 */
void configGPIO(void);

/**
 * @brief Sets the RGB LED to the specified color.
 * @param red Enable red channel (0 or 1).
 * @param green Enable green channel (0 or 1).
 * @param blue Enable blue channel (0 or 1).
 */
void setLEDColor(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Generates a blocking delay using nested loops.
 */
void delay();

/**
 * @brief Main function. Initializes GPIO and cycles through RGB LED color sequence.
 *
 * The sequence includes primary colors (red, green, blue), secondary colors (cyan, magenta, yellow),
 * and white (all LEDs on), repeating indefinitely.
 * @return int Always returns 0 (never reached).
 */
int main(void) {
    configGPIO();

    while(1) {
        // Blue
        setLEDColor(0, 0, 1);
        delay();

        // Red
        setLEDColor(1, 0, 0);
        delay();

        // Green
        setLEDColor(0, 1, 0);
        delay();

        // Cyan (Green + Blue)
        setLEDColor(0, 1, 1);
        delay();

        // Magenta (Red + Blue)
        setLEDColor(1, 0, 1);
        delay();

        // Yellow (Red + Green)
        setLEDColor(1, 1, 0);
        delay();

        // White (Red + Green + Blue)
        setLEDColor(1, 1, 1);
        delay();
    }
    return 0 ;
}

void configGPIO(void) {
    LPC_PINCON->PINSEL1 &= ~(0x3<<12);          // P0.22 as GPIO
    LPC_PINCON->PINSEL7 &= ~(0xf<<18);          // P3.25 and P3.26 as GPIO

    LPC_GPIO0->FIODIR |= RED_LED;               // Set P0.22 as output
    LPC_GPIO3->FIODIR |= BLUE_LED | GREEN_LED;  // Set P3.25 and P3.26 as output
}

void setLEDColor(uint8_t red, uint8_t green, uint8_t blue) {
    if (red)
        LPC_GPIO0->FIOCLR = RED_LED;        // Clear P0.22 to turn on red LED
    else
        LPC_GPIO0->FIOSET = RED_LED;        // Set P0.22 to turn off red LED

    if (green)
        LPC_GPIO3->FIOCLR = GREEN_LED;      // Clear P3.25 to turn on green LED
    else
        LPC_GPIO3->FIOSET = GREEN_LED;      // Set P3.25 to turn off green LED

    if (blue)
        LPC_GPIO3->FIOCLR = BLUE_LED;       // Clear P3.26 to turn on blue LED
    else
        LPC_GPIO3->FIOSET = BLUE_LED;       // Set P3.26 to turn off blue LED
}

void delay() {
    for(uint32_t i=0; i<DELAY; i++)
        for(uint32_t j=0; j<DELAY; j++);
}
