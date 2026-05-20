# RTOS LCD Display — STM32F405RGT6

Dự án điều khiển màn hình LCD sử dụng **FreeRTOS** trên kit phát triển **Waveshare Open405R-C Standard** (MCU: STM32F405RGT6, Cortex-M4 @ 168 MHz).

---

## 📋 Mô tả

Dự án minh hoạ việc tích hợp FreeRTOS với driver LCD trên STM32F4. Chương trình khởi tạo các task RTOS để xử lý hiển thị nội dung lên màn hình LCD thông qua giao tiếp SPI/FSMC, đồng thời tận dụng cơ chế đa nhiệm của FreeRTOS để quản lý luồng dữ liệu hiển thị.

---

## 🛠️ Phần cứng yêu cầu

| Thành phần | Chi tiết |
|---|---|
| Kit phát triển | [Waveshare Open405R-C Standard](https://www.waveshare.com/Open405R-C-Standard.htm) |
| MCU | STM32F405RGT6 (ARM Cortex-M4, 168 MHz, 1 MB Flash, 192 KB SRAM) |
| Màn hình LCD | LCD module tương thích (SPI hoặc FSMC) |
| Debugger/Programmer | ST-Link V2 hoặc J-Link (kết nối qua JTAG/SWD — board không tích hợp debugger) |
| Nguồn | 5V DC jack hoặc qua cổng USB |

> **Lưu ý:** Open405R-C **không tích hợp mạch debug**. Cần debugger ngoài kết nối qua cổng JTAG/SWD để nạp và debug firmware.

---

## 💻 Phần mềm & Công cụ

- **IDE:** STM32CubeIDE (Eclipse-based)
- **HAL Library:** STM32CubeF4 HAL
- **RTOS:** FreeRTOS (tích hợp qua STM32CubeIDE/CubeMX)
- **Ngôn ngữ:** C (96.5%), CSS (1.7%), Makefile (1.5%)

---

## 📁 Cấu trúc thư mục

```
RTOS_LCDdisplay/
├── .metadata/          # Metadata của STM32CubeIDE workspace
└── RTOS/               # Project chính
    ├── Core/
    │   ├── Inc/        # Header files (.h)
    │   └── Src/        # Source files (.c) — main.c, task definitions
    ├── Drivers/
    │   ├── CMSIS/      # CMSIS core & device headers
    │   └── STM32F4xx_HAL_Driver/   # STM32 HAL drivers
    ├── Middlewares/
    │   └── Third_Party/FreeRTOS/   # FreeRTOS kernel
    └── Makefile        # Build script
```

---

## 🚀 Hướng dẫn build & nạp firmware

### 1. Clone repo

```bash
git clone https://github.com/EmbeddedThang/RTOS_LCDdisplay.git
cd RTOS_LCDdisplay
```

### 2. Mở project bằng STM32CubeIDE

- Chọn **File → Import → Existing Projects into Workspace**
- Trỏ đến thư mục `RTOS/`

### 3. Build project

- Nhấn **Ctrl+B** hoặc chọn **Project → Build All**

### 4. Nạp firmware

Kết nối debugger (ST-Link/J-Link) với board qua cổng SWD, sau đó:

- Chọn **Run → Debug** trong CubeIDE, hoặc
- Dùng lệnh với STM32CubeProgrammer:

```bash
STM32_Programmer_CLI -c port=SWD -w RTOS/Debug/RTOS.elf -v -rst
```

Ngoài ra, board hỗ trợ nạp qua **UART Bootloader** (cần USB-UART module).

---

## ⚙️ Nguyên lý hoạt động

Chương trình khởi tạo FreeRTOS scheduler với các task chính:

- **LCD Task** — Khởi tạo giao tiếp với màn hình và cập nhật nội dung hiển thị theo chu kỳ.
- **Default/Main Task** — Điều phối logic ứng dụng, gửi dữ liệu đến LCD task qua queue/semaphore của FreeRTOS.

Cơ chế RTOS đảm bảo việc hiển thị LCD không block các tác vụ khác trong hệ thống.

---

## 📌 Giao diện kết nối (Open405R-C)

| Giao tiếp | Mô tả |
|---|---|
| SPI / FSMC | Kết nối màn hình LCD |
| JTAG / SWD | Nạp và debug firmware |
| UART3 | Kết nối RS232, USB-UART |
| SDIO | Kết nối thẻ Micro SD |
| I2C1/I2C2 | Kết nối ngoại vi I2C |
| I2S2/I2S3 | Kết nối module âm thanh |
| CAN | Giao tiếp CAN bus |

---

## 📚 Tài liệu tham khảo

- [Waveshare Open405R-C Wiki](https://www.waveshare.com/wiki/Open405R-C)
- [STM32F405RGT6 Datasheet — ST Microelectronics](https://www.st.com/en/microcontrollers-microprocessors/stm32f405rg.html)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
- [STM32CubeIDE User Guide](https://www.st.com/en/development-tools/stm32cubeide.html)

---

## 📝 License

Dự án này được phát hành cho mục đích học tập và nghiên cứu. Vui lòng ghi nguồn khi sử dụng lại.
