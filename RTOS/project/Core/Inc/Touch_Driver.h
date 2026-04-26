/*
 * Touch_Driver.h
 *
 *  Created on: Apr 26, 2026
 *      Author: nguyenhoa
 */

#ifndef __TOUCH_DRIVER_H__
#define __TOUCH_DRIVER_H__

#include "main.h"
#include "stm32f4xx_hal.h"

/* -------------------------------------------------------------------------
 * XPT2046 SPI command bytes
 *   Format: S A2 A1 A0 MODE SER/DFR PD1 PD0
 *   S    = 1 (start bit)
 *   MODE = 0 (12-bit ADC)
 *   SER/DFR = 0 (differential reference)
 *   PD   = 00 (power-down between conversions)
 * ----------------------------------------------------------------------- */
#define XPT2046_CMD_X    0xD0   /* channel 101 -> X position */
#define XPT2046_CMD_Y    0x90   /* channel 001 -> Y position */

/* Number of samples averaged per read to reduce noise */
#define XPT2046_AVG_SAMPLES  4

/* ADC calibration constants – map raw 12-bit ADC values to LCD pixels.
 * Adjust these values after measuring raw reads at the four screen corners. */
#define TP_X_MIN    150
#define TP_X_MAX   3900
#define TP_Y_MIN    150
#define TP_Y_MAX   3900

/* LCD display dimensions (must match LCD_Driver.h) */
#define TP_LCD_WIDTH    240
#define TP_LCD_HEIGHT   320

/* Touch button regions on the 240x320 display
 *   Both buttons share the white rectangle drawn at (10,100), 220x85 px.
 *   Dividing that rectangle at x=120 gives left=Play, right=Pause.
 */
#define BTN_AREA_Y_MIN   100
#define BTN_AREA_Y_MAX   185
#define BTN_PLAY_X_MIN    10
#define BTN_PLAY_X_MAX   120
#define BTN_PAUSE_X_MIN  120
#define BTN_PAUSE_X_MAX  230

/* Chip-select helpers for the touch controller */
#define TP_CS_L()  HAL_GPIO_WritePin(TP_CS_GPIO_Port,  TP_CS_Pin,  GPIO_PIN_RESET)
#define TP_CS_H()  HAL_GPIO_WritePin(TP_CS_GPIO_Port,  TP_CS_Pin,  GPIO_PIN_SET)

/* Debounce timeout: number of 10 ms iterations to wait for finger release
 * (200 × 10 ms = 2 000 ms maximum hold time). */
#define TOUCH_DEBOUNCE_MAX_ITER  200u

/* Public API */
void     TP_Init(void);
uint8_t  TP_IsTouched(void);
uint8_t  TP_ReadCoordinates(uint16_t *x, uint16_t *y);

#endif /* __TOUCH_DRIVER_H__ */
