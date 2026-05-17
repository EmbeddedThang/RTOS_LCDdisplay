/*
 * Touch_XPT2046.h
 *
 *  Created on: 2026
 *  Description: Driver đọc vị trí chạm cho màn hình resistive touch
 *               sử dụng IC XPT2046 (tương thích ADS7843) qua SPI1.
 *
 *  Kết nối phần cứng (dựa theo project Open405R-C):
 *    TP_CS   → PB (TP_CS_Pin)   — Chip select của XPT2046
 *    TP_IRQ  → PB (TP_IRQ_Pin)  — Tín hiệu ngắt khi có chạm (active LOW)
 *    SCK     → PB3  (SPI1_SCK)
 *    MOSI    → PA7  (SPI1_MOSI)
 *    MISO    → PA6  (SPI1_MISO)
 */

#ifndef TOUCH_XPT2046_H
#define TOUCH_XPT2046_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * CẤU HÌNH — chỉnh sửa nếu pin khác
 * ========================================================================= */

/* Pin Chip Select của XPT2046 */
#define TP_CS_GPIO_PORT     GPIOB
#define TP_CS_GPIO_PIN      TP_CS_Pin       /* Định nghĩa trong main.h / CubeMX */

/* Pin IRQ của XPT2046 (LOW khi có chạm) */
#define TP_IRQ_GPIO_PORT    TP_IRQ_GPIO_Port  /* Từ CubeMX */
#define TP_IRQ_GPIO_PIN     TP_IRQ_Pin

/* Kích thước màn hình (pixel) */
#define TP_LCD_WIDTH        240
#define TP_LCD_HEIGHT       320

/* Số lần lấy mẫu để lấy trung bình (tăng = mượt hơn nhưng chậm hơn) */
#define TP_SAMPLE_COUNT     8

/* Ngưỡng lọc nhiễu: hai giá trị raw cách nhau > threshold thì loại bỏ */
#define TP_NOISE_THRESHOLD  50

/* Giá trị hiệu chỉnh (calibration) — chỉnh sau khi đo thực tế
 * Cách đo: chạm góc (0,0) → ghi raw_x_min, raw_y_min
 *           chạm góc (239,319) → ghi raw_x_max, raw_y_max        */
#define TP_X_MIN    200
#define TP_X_MAX    3800
#define TP_Y_MIN    300
#define TP_Y_MAX    3700

/* =========================================================================
 * Cấu trúc dữ liệu
 * ========================================================================= */

typedef struct {
    uint16_t x;     /* Tọa độ X sau map (0 → LCD_WIDTH-1)  */
    uint16_t y;     /* Tọa độ Y sau map (0 → LCD_HEIGHT-1) */
    bool     valid; /* true nếu kết quả hợp lệ              */
} TouchPoint_t;

/* =========================================================================
 * API công khai
 * ========================================================================= */

/**
 * @brief Kiểm tra nhanh xem có đang chạm không (đọc pin IRQ).
 * @return true nếu màn hình đang bị chạm.
 */
bool Touch_IsPressed(void);

/**
 * @brief Đọc tọa độ ADC thô từ XPT2046 (chưa map sang pixel).
 * @param raw_x  Con trỏ nhận giá trị ADC trục X (0–4095).
 * @param raw_y  Con trỏ nhận giá trị ADC trục Y (0–4095).
 * @return true nếu đọc thành công và giá trị hợp lệ.
 */
bool Touch_ReadRaw(uint16_t *raw_x, uint16_t *raw_y);

/**
 * @brief Đọc tọa độ chạm đã lọc nhiễu + hiệu chỉnh, map sang pixel màn hình.
 * @param pt  Con trỏ nhận kết quả (x, y, valid).
 *            pt->valid = false nếu không có chạm hoặc đọc thất bại.
 */
void Touch_GetPoint(TouchPoint_t *pt);

/**
 * @brief Đọc tọa độ trung bình từ nhiều mẫu (lọc nhiễu tốt hơn).
 *        Nên dùng hàm này thay vì Touch_GetPoint khi cần độ chính xác cao.
 * @param pt  Con trỏ nhận kết quả.
 */
void Touch_GetAveragedPoint(TouchPoint_t *pt);

#endif /* TOUCH_XPT2046_H */
