/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : BẢN CODE CHỐNG BỊ CUBEMX XÓA - FULL CHỨC NĂNG
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "LCD_Driver.h"
#include "Touch_Driver.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define FRAM_I2C_ADDR 0xA0
#define V25             0.76f       /* V — voltage at 25°C            */
#define AVG_SLOPE       0.0025f     /* V/°C — 2.5 mV/°C              */
#define VDDA            3.3f        /* V — điện áp tham chiếu ADC     */
#define ADC_RESOLUTION  4095.0f
typedef struct {
    float temp;
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} DataLog_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c2;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;

osThreadId defaultTaskHandle;
osThreadId TouchTaskHandle;
osMutexId LcdMutexHandle;
/* USER CODE BEGIN PV */
float temperature = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C2_Init(void);
static void MX_RTC_Init(void);
void StartDefaultTask(void const * argument);
void StartTouchTask(void const * argument);

/* USER CODE BEGIN PFP */
float Read_Temperature(void);
void Draw_UI_Nhom07(void);
HAL_StatusTypeDef FRAM_Write(uint16_t addr, uint8_t *data, uint16_t len);
HAL_StatusTypeDef FRAM_Read(uint16_t addr, uint8_t *data, uint16_t len);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_SPI1_Init();
  MX_I2C2_Init();
  MX_RTC_Init();

  /* USER CODE BEGIN 2 */
  /* KHỞI ĐỘNG MÀN HÌNH SAU KHI INIT */
  HAL_GPIO_WritePin(GPIOB, LCD_BL_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, LCD_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(50);
  HAL_GPIO_WritePin(GPIOB, LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(50);
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
  TP_Init();
  /* USER CODE END 2 */

  /* Create the mutex(es) */
  /* definition and creation of LcdMutex */
  osMutexDef(LcdMutex);
  LcdMutexHandle = osMutexCreate(osMutex(LcdMutex));

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 1024);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of TouchTask */
  osThreadDef(TouchTask, StartTouchTask, osPriorityAboveNormal, 0, 1024);
  TouchTaskHandle = osThreadCreate(osThread(TouchTask), NULL);

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/* ========================================================================= */
/* LƯU Ý: Phần cấu hình CubeMX sinh ra được giữ nguyên.                      */
/* Mình chỉ sửa lại logic RTC và GPIO ngay bên trong các thẻ USER CODE       */
/* ========================================================================= */

void SystemClock_Config(void)
{
  // Code giữ nguyên như của CubeMX
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) { Error_Handler(); }
}

static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK) { Error_Handler(); }

  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) { Error_Handler(); }

  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) { Error_Handler(); }
}

static void MX_I2C2_Init(void)
{
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK) { Error_Handler(); }
}

static void MX_RTC_Init(void)
{
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK) { Error_Handler(); }

  /* USER CODE BEGIN Check_RTC_BKUP */
  // Kiểm tra xem đã từng cài đặt thời gian chưa, nếu rồi thì thoát để không bị reset về 0
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) == 0x9999) {
      return;
  }
  /* USER CODE END Check_RTC_BKUP */

  sTime.Hours = 19;
  sTime.Minutes = 15;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) { Error_Handler(); }

  sDate.WeekDay = RTC_WEEKDAY_WEDNESDAY;
  sDate.Month = RTC_MONTH_APRIL;
  sDate.Date = 15;
  sDate.Year = 26;
  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) { Error_Handler(); }

  /* USER CODE BEGIN RTC_Init 2 */
  // Lưu cờ đánh dấu đã khởi tạo
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x9999);
  /* USER CODE END RTC_Init 2 */
}

static void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK) { Error_Handler(); }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, BTN_SAVE_Pin|BTN_READ_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, LCD_RST_Pin|FRAM_CS_Pin|LCD_BL_Pin|LCD_CS_Pin|LCD_DC_Pin|TP_CS_Pin, GPIO_PIN_RESET);

  /* USER CODE BEGIN MX_GPIO_Init_1 */
  // GHI ĐÈ LẠI CẤU HÌNH DO CUBEMX LÀM SAI (CubeMX set PA1 thành Pulldown là sai với board của bạn)
  GPIO_InitStruct.Pin = BTN_SAVE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN; // PA0 nối 3.3V
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = BTN_READ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;   // PA1 nối GND (Cái này CubeMX làm sai, mình fix lại ở đây)
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  /* USER CODE END MX_GPIO_Init_1 */

  GPIO_InitStruct.Pin = LCD_RST_Pin|LCD_BL_Pin|LCD_CS_Pin|LCD_DC_Pin|TP_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = FRAM_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(FRAM_CS_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TP_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(TP_IRQ_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
HAL_StatusTypeDef FRAM_Write(uint16_t addr, uint8_t *data, uint16_t len) {
    HAL_StatusTypeDef st = HAL_I2C_Mem_Write(&hi2c2, FRAM_I2C_ADDR, addr, I2C_MEMADD_SIZE_8BIT, data, len, 500);
    HAL_Delay(5);
    return st;
}

HAL_StatusTypeDef FRAM_Read(uint16_t addr, uint8_t *data, uint16_t len) {
    return HAL_I2C_Mem_Read(&hi2c2, FRAM_I2C_ADDR, addr, I2C_MEMADD_SIZE_8BIT, data, len, 500);
}

void Draw_UI_Nhom07(void)
{
    lcd_clear_screen(0x0292);
    lcd_fill_rect(10, 10, 120, 35, WHITE);
    lcd_display_string(15, 20, (uint8_t*)"[Nhom 07]", FONT_1608, 0x0292);
    lcd_fill_rect(10, 55, 220, 35, WHITE);
    lcd_display_string(15, 65, (uint8_t*)"Internal Temp: ", FONT_1206, 0x0292);
    lcd_fill_rect(10, 100, 220, 85, WHITE);
    lcd_display_string(50, 165, (uint8_t*)"(WAKE-SAVE)", FONT_1206, 0x0292);
    lcd_display_string(150, 165, (uint8_t*)"(USER-READ)", FONT_1206, 0x0292);
    lcd_fill_rect(10, 195, 220, 110, WHITE);
    lcd_display_string(15, 200, (uint8_t*)"Data Log View:", FONT_1206, 0x0292);
}

float Read_Temperature(void)
{
	    ADC->CCR |= ADC_CCR_TSVREFE;

	    HAL_ADC_Start(&hadc1);

	    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK)
	    {
	        HAL_ADC_Stop(&hadc1);
	        return -999.0f;
	    }
	    uint32_t adc_raw = HAL_ADC_GetValue(&hadc1);
	    HAL_ADC_Stop(&hadc1);

	    float v_sense = ((float)adc_raw / ADC_RESOLUTION) * VDDA;

	    /* 6. Áp dụng công thức datasheet Section 5.3.21:
	          Temp = (V_SENSE - V25) / Avg_Slope + 25             */
	    float temperature = ((v_sense - V25) / AVG_SLOPE) + 25.0f;

	    return temperature;

}

void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  char buf[32];
  if (osMutexWait(LcdMutexHandle, osWaitForever) == osOK) {
      HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);
      lcd_init();
      Draw_UI_Nhom07();
      HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
      osMutexRelease(LcdMutexHandle);
  }

  for(;;)
  {
    temperature = Read_Temperature();
    if (osMutexWait(LcdMutexHandle, osWaitForever) == osOK)
    {
        HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);
        lcd_fill_rect(160, 60, 70, 25, WHITE);

        int t_int = (int)temperature;
        int t_frac = (int)((temperature - t_int) * 10.0f);
        if(t_frac < 0) t_frac = -t_frac;
        sprintf(buf, "%d.%d C", t_int, t_frac);

        lcd_display_string(160, 65, (uint8_t*)buf, FONT_1206, 0x0292);
        HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
        osMutexRelease(LcdMutexHandle);
    }
    osDelay(500);
  }
  /* USER CODE END 5 */
}

void StartTouchTask(void const * argument)
{
  /* USER CODE BEGIN StartTouchTask */
  DataLog_t myData = {0};
  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;
  char msg[128];
  HAL_StatusTypeDef status;
  uint16_t touch_x = 0, touch_y = 0;

  for(;;)
  {
    /* ===== XPT2046 TOUCH PANEL – PLAY / PAUSE BUTTONS ===== */
    if (TP_IsTouched())
    {
        if (TP_ReadCoordinates(&touch_x, &touch_y))
        {
            /* PLAY button: left half of the white rectangle (x: 10-120, y: 100-185) */
            if (touch_x >= BTN_PLAY_X_MIN  && touch_x <= BTN_PLAY_X_MAX &&
                touch_y >= BTN_AREA_Y_MIN  && touch_y <= BTN_AREA_Y_MAX)
            {
                if (osMutexWait(LcdMutexHandle, osWaitForever) == osOK)
                {
                    HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);
                    lcd_fill_rect(10, 210, 220, 80, WHITE);
                    lcd_display_string(15, 220, (uint8_t*)"PLAY pressed!", FONT_1206, 0x07E0);
                    HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
                    osMutexRelease(LcdMutexHandle);
                }
                /* Debounce: wait until the finger is lifted (max 2 s) */
                {
                    uint32_t debounce_count = 0;
                    while (TP_IsTouched() && debounce_count < TOUCH_DEBOUNCE_MAX_ITER)
                    {
                        osDelay(10);
                        debounce_count++;
                    }
                }
            }
            /* PAUSE button: right half of the white rectangle (x: 120-230, y: 100-185) */
            else if (touch_x >= BTN_PAUSE_X_MIN && touch_x <= BTN_PAUSE_X_MAX &&
                     touch_y >= BTN_AREA_Y_MIN   && touch_y <= BTN_AREA_Y_MAX)
            {
                if (osMutexWait(LcdMutexHandle, osWaitForever) == osOK)
                {
                    HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);
                    lcd_fill_rect(10, 210, 220, 80, WHITE);
                    lcd_display_string(15, 220, (uint8_t*)"PAUSE pressed!", FONT_1206, 0xF800);
                    HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
                    osMutexRelease(LcdMutexHandle);
                }
                /* Debounce: wait until the finger is lifted (max 2 s) */
                {
                    uint32_t debounce_count = 0;
                    while (TP_IsTouched() && debounce_count < TOUCH_DEBOUNCE_MAX_ITER)
                    {
                        osDelay(10);
                        debounce_count++;
                    }
                }
            }
        }
    }

    /* NÚT WAKEUP (PA0) -> LƯU DỮ LIỆU */
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
    {
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

        myData.temp  = temperature;
        myData.day   = sDate.Date;
        myData.month = sDate.Month;
        myData.year  = sDate.Year;
        myData.hour  = sTime.Hours;
        myData.min   = sTime.Minutes;
        myData.sec   = sTime.Seconds;

        status = FRAM_Write(0x0000, (uint8_t*)&myData, sizeof(myData));

        if (osMutexWait(LcdMutexHandle, osWaitForever) == osOK)
        {
            HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);
            lcd_fill_rect(10, 210, 220, 80, WHITE);
            if(status == HAL_OK)
                lcd_display_string(15, 220, (uint8_t*)"SAVE: Fram Stored OK!", FONT_1206, 0x07E0);
            else
                lcd_display_string(15, 220, (uint8_t*)"SAVE FAIL: I2C ERROR", FONT_1206, 0xF800);

            HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
            osMutexRelease(LcdMutexHandle);
        }
        while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) osDelay(10);
    }

    /* NÚT USER (PA1) -> ĐỌC DỮ LIỆU */
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET)
    {
        status = FRAM_Read(0x0000, (uint8_t*)&myData, sizeof(myData));

        if (osMutexWait(LcdMutexHandle, osWaitForever) == osOK)
        {
            HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);
            lcd_fill_rect(10, 210, 220, 80, WHITE);
            if(status == HAL_OK) {
                int t_int = (int)myData.temp;
                int t_frac = (int)((myData.temp - t_int) * 10.0f);
                if(t_frac < 0) t_frac = -t_frac;

                /* Hiển thị định dạng: Ngày/Tháng/Năm Giờ:Phút:Giây */
                sprintf(msg, "%02d/%02d/%02d", myData.day, myData.month, myData.year);
                lcd_display_string(15, 220, (uint8_t*)msg, FONT_1206, 0x0292);

                sprintf(msg, "%02d:%02d:%02d - %d.%d C", myData.hour, myData.min, myData.sec, t_int, t_frac);
                lcd_display_string(15, 245, (uint8_t*)msg, FONT_1206, 0x0292);
            } else {
                lcd_display_string(15, 220, (uint8_t*)"READ FAIL: I2C ERROR", FONT_1206, 0xF800);
            }
            HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
            osMutexRelease(LcdMutexHandle);
        }
        while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) osDelay(10);
    }
    osDelay(50);
  }
  /* USER CODE END StartTouchTask */
}

void Error_Handler(void) { __disable_irq(); while (1) {} }
