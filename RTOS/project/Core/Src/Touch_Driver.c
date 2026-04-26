/*
 * Touch_Driver.c
 *
 *  Created on: Apr 26, 2026
 *      Author: nguyenhoa
 *
 *  XPT2046 resistive touch controller driver.
 *  The controller shares SPI1 with the LCD but uses a separate chip-select
 *  pin (TP_CS_Pin / GPIOB pin 9) and an active-low interrupt line
 *  (TP_IRQ_Pin / GPIOB pin 4).
 */

#include "Touch_Driver.h"

extern SPI_HandleTypeDef hspi1;

/* ---------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/**
 * @brief  Send one byte over SPI1 and return the received byte.
 */
static uint8_t TP_SPI_Transfer(uint8_t tx)
{
    uint8_t rx = 0;
    HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, 10);
    return rx;
}

/**
 * @brief  Read a single 12-bit ADC value from the XPT2046 for the given
 *         channel command byte.  CS must be asserted before calling.
 *
 * The XPT2046 returns the 12-bit result MSB-first, packed in the upper 12
 * bits of a 16-bit transfer (bits 14:3 after the leading null bit).
 * Concretely: byte0[6:0] are bits 11:5, byte1[7:3] are bits 4:0.
 *
 *  TX:  [cmd]  [0x00] [0x00]
 *  RX:  [busy] [HH]   [LL]   -> result = (HH<<5) | (LL>>3)
 */
static uint16_t TP_ReadRaw(uint8_t cmd)
{
    uint8_t hi, lo;

    TP_SPI_Transfer(cmd);   /* send command, discard busy byte */
    hi = TP_SPI_Transfer(0x00);
    lo = TP_SPI_Transfer(0x00);

    return (uint16_t)(((uint16_t)hi << 5) | ((uint16_t)lo >> 3));
}

/**
 * @brief  Read the requested channel XPT2046_AVG_SAMPLES times and return
 *         the average, discarding the min and max to reduce noise.
 */
static uint16_t TP_ReadAveraged(uint8_t cmd)
{
    uint16_t samples[XPT2046_AVG_SAMPLES];
    uint16_t sum   = 0;
    uint16_t vmin  = 0xFFFF;
    uint16_t vmax  = 0;
    uint8_t  i;

    TP_CS_L();
    for (i = 0; i < XPT2046_AVG_SAMPLES; i++) {
        samples[i] = TP_ReadRaw(cmd);
        if (samples[i] < vmin) vmin = samples[i];
        if (samples[i] > vmax) vmax = samples[i];
        sum += samples[i];
    }
    TP_CS_H();

    /* Drop the min and max samples, average the rest */
    sum -= vmin;
    sum -= vmax;
    return sum / (XPT2046_AVG_SAMPLES - 2);
}

/**
 * @brief  Map a raw 12-bit ADC value to a pixel coordinate.
 *
 * @param  raw        Raw ADC value (0–4095).
 * @param  raw_min    ADC value that corresponds to pixel 0.
 * @param  raw_max    ADC value that corresponds to pixel (dim-1).
 * @param  dim        Number of pixels along this axis.
 * @retval Clamped pixel coordinate.
 */
static uint16_t TP_MapToPixel(uint16_t raw, uint16_t raw_min,
                               uint16_t raw_max, uint16_t dim)
{
    int32_t pixel;

    if (raw <= raw_min) return 0;
    if (raw >= raw_max) return dim - 1;

    pixel = ((int32_t)(raw - raw_min) * (int32_t)(dim - 1)) /
            (int32_t)(raw_max - raw_min);

    return (uint16_t)pixel;
}

/* ---------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/**
 * @brief  Initialise the touch controller.
 *         Call once after the SPI peripheral has been configured.
 */
void TP_Init(void)
{
    /* De-assert CS so the touch IC stays idle during LCD transfers */
    TP_CS_H();
}

/**
 * @brief  Return non-zero when a touch is detected (IRQ pin is LOW).
 */
uint8_t TP_IsTouched(void)
{
    return (HAL_GPIO_ReadPin(TP_IRQ_GPIO_Port, TP_IRQ_Pin) == GPIO_PIN_RESET)
           ? 1u : 0u;
}

/**
 * @brief  Read and convert touch coordinates to LCD pixel coordinates.
 *
 * @param[out] x  Pointer to store the X pixel coordinate (0–239).
 * @param[out] y  Pointer to store the Y pixel coordinate (0–319).
 * @retval 1 if a valid touch was read, 0 otherwise.
 */
uint8_t TP_ReadCoordinates(uint16_t *x, uint16_t *y)
{
    uint16_t raw_x, raw_y;

    if (!TP_IsTouched()) {
        return 0;
    }

    raw_x = TP_ReadAveraged(XPT2046_CMD_X);
    raw_y = TP_ReadAveraged(XPT2046_CMD_Y);

    /* Verify the touch is still present after the ADC reads */
    if (!TP_IsTouched()) {
        return 0;
    }

    *x = TP_MapToPixel(raw_x, TP_X_MIN, TP_X_MAX, TP_LCD_WIDTH);
    *y = TP_MapToPixel(raw_y, TP_Y_MIN, TP_Y_MAX, TP_LCD_HEIGHT);

    return 1;
}
