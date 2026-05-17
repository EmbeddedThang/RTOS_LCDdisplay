/*
 * Touch_XPT2046.c
 *
 *  Created on: 2026
 *  Description: Driver đọc vị trí chạm cho màn hình resistive touch
 *               sử dụng IC XPT2046 (tương thích ADS7843) qua SPI1.
 *
 * ============================================================================
 * NGUYÊN LÝ HOẠT ĐỘNG XPT2046:
 *   - Giao tiếp SPI: CPOL=0, CPHA=0, MSB first, tốc độ ≤ 2 MHz.
 *   - Gửi 1 byte lệnh → đọc lại 2 byte kết quả (12-bit ADC, căn trái).
 *   - Lệnh đọc X:  0xD0  (DFR mode, differential, channel X+)
 *   - Lệnh đọc Y:  0x90  (DFR mode, differential, channel Y+)
 *   - Pin IRQ kéo xuống LOW khi có chạm (cấu hình pull-up trong CubeMX).
 *
 * ============================================================================
 * CHÚ Ý QUAN TRỌNG — CHIA SẺ SPI1 VỚI LCD:
 *   SPI1 được dùng chung cho cả LCD (ST7789) và Touch (XPT2046).
 *   Hai thiết bị dùng chân CS riêng biệt để không xung đột:
 *     - LCD_CS  → chỉ kéo LOW khi giao tiếp LCD
 *     - TP_CS   → chỉ kéo LOW khi giao tiếp XPT2046
 *   Đảm bảo KHÔNG kéo cả hai CS xuống cùng lúc.
 *   Khi dùng FreeRTOS: bảo vệ bằng LcdMutex trước khi gọi Touch_GetPoint.
 * ============================================================================
 */

#include "Touch_XPT2046.h"

/* =========================================================================
 * Biến ngoài và khai báo nội bộ
 * ========================================================================= */

extern SPI_HandleTypeDef hspi1;

/* Lệnh gửi cho XPT2046 */
#define XPT2046_CMD_READ_X   0xD0   /* Đọc kênh X (differential mode) */
#define XPT2046_CMD_READ_Y   0x90   /* Đọc kênh Y (differential mode) */
#define XPT2046_DUMMY        0x00   /* Byte dummy để tạo clock          */

/* Macro điều khiển CS của XPT2046 */
#define TP_CS_LOW()   HAL_GPIO_WritePin(TP_CS_GPIO_PORT, TP_CS_GPIO_PIN, GPIO_PIN_RESET)
#define TP_CS_HIGH()  HAL_GPIO_WritePin(TP_CS_GPIO_PORT, TP_CS_GPIO_PIN, GPIO_PIN_SET)

/* =========================================================================
 * Hàm nội bộ (static)
 * ========================================================================= */

/**
 * @brief Truyền 1 byte qua SPI1 và nhận 1 byte trả về.
 */
static uint8_t tp_spi_transfer(uint8_t tx_byte)
{
    uint8_t rx_byte = 0;
    HAL_SPI_TransmitReceive(&hspi1, &tx_byte, &rx_byte, 1, 10);
    return rx_byte;
}

/**
 * @brief Gửi lệnh và đọc 12-bit ADC từ XPT2046.
 *        Giao thức: gửi 1 byte lệnh → đọc 2 byte → ghép thành 12-bit.
 *        CS phải được kéo LOW trước khi gọi hàm này.
 * @param cmd  Byte lệnh (0xD0 cho X, 0x90 cho Y).
 * @return Giá trị ADC 12-bit (0–4095).
 */
static uint16_t tp_read_adc(uint8_t cmd)
{
    uint8_t hi, lo;

    tp_spi_transfer(cmd);       /* Gửi lệnh → XPT2046 bắt đầu chuyển đổi */
    hi = tp_spi_transfer(XPT2046_DUMMY);  /* Byte cao (bit 11–4)           */
    lo = tp_spi_transfer(XPT2046_DUMMY);  /* Byte thấp (bit 3–0, căn trái) */

    /* Kết quả 12-bit: hi[7:0] là bit[11:4], lo[7:4] là bit[3:0] */
    return (uint16_t)(((uint16_t)hi << 4) | (lo >> 4));
}

/**
 * @brief Map giá trị từ khoảng [in_min, in_max] sang [out_min, out_max].
 *        Kết quả được giới hạn trong [out_min, out_max].
 */
static uint16_t tp_map(uint16_t value,
                        uint16_t in_min,  uint16_t in_max,
                        uint16_t out_min, uint16_t out_max)
{
    if (value <= in_min) return out_min;
    if (value >= in_max) return out_max;
    return (uint16_t)((uint32_t)(value - in_min) *
                      (out_max - out_min) /
                      (in_max - in_min) + out_min);
}

/* =========================================================================
 * API công khai
 * ========================================================================= */

/**
 * @brief Kiểm tra xem màn hình có đang bị chạm không.
 *        XPT2046 kéo IRQ xuống LOW khi có chạm.
 */
bool Touch_IsPressed(void)
{
    return (HAL_GPIO_ReadPin(TP_IRQ_GPIO_PORT, TP_IRQ_GPIO_PIN) == GPIO_PIN_RESET);
}

/**
 * @brief Đọc giá trị ADC thô (raw) của X và Y từ XPT2046.
 *        Hàm lấy 1 mẫu duy nhất, không lọc nhiễu.
 */
bool Touch_ReadRaw(uint16_t *raw_x, uint16_t *raw_y)
{
    if (!Touch_IsPressed()) {
        return false;
    }

    /* Bảo đảm LCD không đang dùng SPI: LCD_CS phải HIGH */
    TP_CS_LOW();

    *raw_x = tp_read_adc(XPT2046_CMD_READ_X);
    *raw_y = tp_read_adc(XPT2046_CMD_READ_Y);

    TP_CS_HIGH();

    /* Kiểm tra giá trị hợp lệ (loại bỏ nhiễu biên) */
    if (*raw_x < 100 || *raw_x > 4000 ||
        *raw_y < 100 || *raw_y > 4000) {
        return false;
    }

    return true;
}

/**
 * @brief Đọc tọa độ chạm: 1 lần lấy mẫu + map sang pixel.
 */
void Touch_GetPoint(TouchPoint_t *pt)
{
    uint16_t raw_x, raw_y;

    pt->valid = false;

    if (!Touch_ReadRaw(&raw_x, &raw_y)) {
        return;
    }

    /* Map ADC raw → tọa độ pixel màn hình */
    pt->x     = tp_map(raw_x, TP_X_MIN, TP_X_MAX, 0, TP_LCD_WIDTH  - 1);
    pt->y     = tp_map(raw_y, TP_Y_MIN, TP_Y_MAX, 0, TP_LCD_HEIGHT - 1);
    pt->valid = true;
}

/**
 * @brief Đọc tọa độ chạm với lọc nhiễu: lấy TP_SAMPLE_COUNT mẫu,
 *        loại bỏ các mẫu lệch quá ngưỡng, tính trung bình.
 *
 *        Thuật toán:
 *          1. Lấy TP_SAMPLE_COUNT mẫu liên tiếp.
 *          2. Bỏ các mẫu có |giá trị - median| > TP_NOISE_THRESHOLD.
 *          3. Tính trung bình các mẫu còn lại.
 *          4. Map kết quả sang tọa độ pixel.
 */
void Touch_GetAveragedPoint(TouchPoint_t *pt)
{
    uint16_t buf_x[TP_SAMPLE_COUNT];
    uint16_t buf_y[TP_SAMPLE_COUNT];
    uint8_t  valid_count = 0;
    uint32_t sum_x = 0, sum_y = 0;

    pt->valid = false;

    /* Bước 1: Thu thập mẫu */
    TP_CS_LOW();
    for (uint8_t i = 0; i < TP_SAMPLE_COUNT; i++) {
        if (!Touch_IsPressed()) {
            /* Ngón tay nhấc lên giữa chừng — dừng lại */
            break;
        }
        buf_x[i] = tp_read_adc(XPT2046_CMD_READ_X);
        buf_y[i] = tp_read_adc(XPT2046_CMD_READ_Y);

        /* Lọc giá trị biên rõ ràng */
        if (buf_x[i] >= 100 && buf_x[i] <= 4000 &&
            buf_y[i] >= 100 && buf_y[i] <= 4000) {
            valid_count++;
        }
    }
    TP_CS_HIGH();

    if (valid_count < (TP_SAMPLE_COUNT / 2)) {
        /* Không đủ mẫu hợp lệ */
        return;
    }

    /* Bước 2: Tính median đơn giản bằng giá trị giữa (tránh sort phức tạp) */
    uint16_t mid_x = buf_x[TP_SAMPLE_COUNT / 2];
    uint16_t mid_y = buf_y[TP_SAMPLE_COUNT / 2];

    /* Bước 3: Tính trung bình, loại bỏ mẫu lệch > ngưỡng */
    uint8_t accepted = 0;
    for (uint8_t i = 0; i < valid_count; i++) {
        int16_t dx = (int16_t)buf_x[i] - (int16_t)mid_x;
        int16_t dy = (int16_t)buf_y[i] - (int16_t)mid_y;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;

        if (dx <= TP_NOISE_THRESHOLD && dy <= TP_NOISE_THRESHOLD) {
            sum_x += buf_x[i];
            sum_y += buf_y[i];
            accepted++;
        }
    }

    if (accepted == 0) {
        return;
    }

    uint16_t avg_x = (uint16_t)(sum_x / accepted);
    uint16_t avg_y = (uint16_t)(sum_y / accepted);

    /* Bước 4: Map sang tọa độ pixel */
    pt->x     = tp_map(avg_x, TP_X_MIN, TP_X_MAX, 0, TP_LCD_WIDTH  - 1);
    pt->y     = tp_map(avg_y, TP_Y_MIN, TP_Y_MAX, 0, TP_LCD_HEIGHT - 1);
    pt->valid = true;
}
