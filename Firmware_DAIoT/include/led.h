/**
 * @file Led.h
 * @brief Header file for the LED class that controls RGB LED strips using RMT.
 */

#ifndef LED_H
#define LED_H

#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"

// Define LED strip configuration constants
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_RESOL_TO_TICKS 1000000
#define T0H 0.3
#define T0L 0.9
#define T1H 0.9
#define T1L 0.3
#define TRST 100
#define LEDTAG "LED"
#define RGB_LED_PIN 48

/**
 * @brief Set the brightness level for the LED.
 *
 * @param brillo_ Brightness level for the LED (0-100)
 */
void set_led_brightness(uint8_t brillo_);

/**
 * @brief Initialize the LED strip.
 */
void begin_led(void);

/**
 * @brief Set the color for the LED.
 *
 * @param R Red value (0-255)
 * @param G Green value (0-255)
 * @param B Blue value (0-255)
 */
void set_led_color(uint8_t R, uint8_t G, uint8_t B);

/**
 * @brief Show the LED color.
 */
void show_led(void);

#endif // LED_H