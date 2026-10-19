/**
  ******************************************************************************
  * @file    GPIO/GPIO_EXTI/Src/main.c 
  * @author  MCD Application Team
  * @brief   This example describes how to configure and use GPIOs through 
  *          the STM32F4xx HAL API.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>

I2C_HandleTypeDef hi2c1; // stm32 HAL

/* Connect pins to I2C1 --- start */
#define ACC_ADDR (0x19 << 1)             // Sensor: 7-bit address; <<1 because HAL wants it shifted
#define MAG_ADDR (0x1E << 1)             // Sensor: magnetometer has its own address

static void i2c1_init(void)
{
	// give clock signal for GPIO block port B (pins PB6 && PB9) and I2C1 block
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    // configure && apply settings for GPIO block port B
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_6 | GPIO_PIN_9;
    g.Mode = GPIO_MODE_AF_OD; // required by I2C
    g.Pull = GPIO_PULLUP; // pull up resistor: on
    g.Alternate = GPIO_AF4_I2C1; // connect pins to I2C (AF4 for PB6, PB9)
    HAL_GPIO_Init(GPIOB, &g);

    // configure && apply settings for I2C block
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000; // 100 kHz bus clock
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT; // 7-bit mode
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static void wr(uint16_t dev, uint8_t reg, uint8_t val)       // helper: write one register
{
    HAL_I2C_Mem_Write(&hi2c1, dev, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
}
static uint8_t rd(uint16_t dev, uint8_t reg)                 // helper: read one register
{
    uint8_t v = 0;
    HAL_I2C_Mem_Read(&hi2c1, dev, reg, I2C_MEMADD_SIZE_8BIT, &v, 1, 100);
    return v;
}
/* Connect pins to I2C1 --- end */

/* Configure gyro --- start */
SPI_HandleTypeDef hspi_gyro;                    // STM32 HAL: SPI1 settings/state

static void gyro_spi_init(void)
{
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};                   // C: zeroed struct
    g.Pin       = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;   // PA5 SCK, PA6 MISO, PA7 MOSI
    g.Mode      = GPIO_MODE_AF_PP;              // alternate function, push-pull (SPI actively drives high and low)
    g.Pull      = GPIO_PULLDOWN;
    g.Speed     = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF5_SPI1;                // STM32: connect these pins to SPI1
    HAL_GPIO_Init(GPIOA, &g);

    g.Pin  = GYRO_CS_PIN;                        // PE3 = chip select, driven by hand as a plain output
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GYRO_CS_GPIO_PORT, &g);
    GYRO_CS_HIGH();                             // idle = chip deselected

    hspi_gyro.Instance               = SPI1;
    hspi_gyro.Init.Mode              = SPI_MODE_MASTER;
    hspi_gyro.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi_gyro.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi_gyro.Init.CLKPolarity       = SPI_POLARITY_LOW;    // Sensor: L3GD20 uses SPI mode 0
    hspi_gyro.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi_gyro.Init.NSS               = SPI_NSS_SOFT;         // we toggle CS ourselves
    hspi_gyro.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;   // 100 MHz / 16 = 6.25 MHz (chip max 10)
    hspi_gyro.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi_gyro.Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi_gyro.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi_gyro);
}

static void gyro_wr(uint8_t reg, uint8_t val)             // write one register
{
	uint8_t tx[2] = { reg, val };      // bit 7 = 0 means write
	uint8_t rx[2];
	GYRO_CS_LOW();
	HAL_SPI_TransmitReceive(&hspi_gyro, tx, rx, 2, 100);
	GYRO_CS_HIGH();
}

static HAL_StatusTypeDef gyro_rd(uint8_t reg, uint8_t *buf, uint8_t n)   // n must be <= 6
{
    uint8_t tx[7] = {0};                                  // C: zeroed; the dummy bytes after the command stay 0
    uint8_t rx[7] = {0};
    tx[0] = reg | READWRITE_CMD | (n > 1 ? MULTIPLEBYTE_CMD : 0);   // STM32 BSP header constants

    GYRO_CS_LOW();
    HAL_StatusTypeDef s = HAL_SPI_TransmitReceive(&hspi_gyro, tx, rx, n + 1, 100);   // STM32 HAL
    GYRO_CS_HIGH();

    for (uint8_t i = 0; i < n; i++) buf[i] = rx[i + 1];   // rx[0] was clocked in during the command byte, so skip it
    return s;
}
/* Configure gyro --- end */

/* Send characters through ITM */
int _write(int file, char *ptr, int len) {
	for (int i = 0; i < len; i++) {
		ITM_SendChar(*ptr++);
	}
	return len;
}

/* Record samples --- start */
#define N_SAMPLES 500 // n_samples * sample_period_ms = recording period [s]
#define SAMPLE_PERIOD_MS 10 // 100 Hz

typedef struct {                  // C: a struct bundles related values
    uint32_t t_ms;                // time of the sample
    int16_t  ax, ay, az;          // accelerometer, mg
    int16_t  gx, gy, gz;          // gyro, filled in later
    int8_t   gtemp;
} sample_t;

static sample_t samples[N_SAMPLES];        // 16 bytes x 500 = 8 KB; global, not on the stack
static volatile uint8_t start_flag = 0;    // C: volatile = "an interrupt may change this"

static void record_samples(void)
{
    uint8_t b[6];
    uint32_t next = HAL_GetTick();                      // STM32 HAL: ms since boot

    for (int k = 0; k < N_SAMPLES; k++) {
        next += SAMPLE_PERIOD_MS;                       // absolute schedule, so no drift
        while ((int32_t)(HAL_GetTick() - next) < 0) {}  // wait until that moment

        samples[k].t_ms = HAL_GetTick();                // timestamp first, before the slow I2C read

        // accel values
        HAL_I2C_Mem_Read(&hi2c1, ACC_ADDR, 0x28 | 0x80, I2C_MEMADD_SIZE_8BIT, b, 6, 100);
        samples[k].ax = (int16_t)((b[1] << 8) | b[0]) >> 4;
        samples[k].ay = (int16_t)((b[3] << 8) | b[2]) >> 4;
        samples[k].az = (int16_t)((b[5] << 8) | b[4]) >> 4;

        // gyro values
        uint8_t g[6];
        gyro_rd(0x28, g, 6);                              // OUT_X_L ... OUT_Z_H, low byte first
        samples[k].gx = (int16_t)((g[1] << 8) | g[0]);    // C: combine two bytes into a signed 16-bit value
        samples[k].gy = (int16_t)((g[3] << 8) | g[2]);
        samples[k].gz = (int16_t)((g[5] << 8) | g[4]);

        uint8_t tr;
        gyro_rd(0x26, &tr, 1);                            // OUT_TEMP
        samples[k].gtemp = (int8_t)tr;                    // C: reinterpret the byte as signed
    }
}
/* Record samples --- end */

/* Print samples --- start */
static void print_samples(void)
{
    // t_ms, ax_mg, ay_mg, az_mg, gx_mdps, gy_mdps, gz_mdps, gtemp_raw
    for (int k = 0; k < N_SAMPLES; k++) {
        printf("[ms] %lu | [mG] %d, %d, %d | [mdps] %ld, %ld, %ld, %d\n", samples[k].t_ms,
               samples[k].ax, samples[k].ay, samples[k].az,
               (long)samples[k].gx * 175 / 10,            // +-500 dps: 17.5 mdps per LSB
               (long)samples[k].gy * 175 / 10,
               (long)samples[k].gz * 175 / 10,
               samples[k].gtemp);
    }
}
/* Print samples --- end */

/* Phase detection: free fall --- start */
#define FF_THRESH_MG   300   // below this magnitude = "weightless"
#define FF_MIN_SAMPLES 3     // must hold for 3 samples (30 ms) in a row, so one noisy sample can't trigger it

static void detect_freefall(void)
{
    int in_ff = 0;                    // C: flag, 1 while in free fall
    int run = 0;                      // C: how many samples in a row were below the threshold
    uint32_t t_start = 0;
    const int32_t thr2 = (int32_t)FF_THRESH_MG * FF_THRESH_MG;   // compare squares, so no sqrt is needed

    for (int k = 0; k < N_SAMPLES; k++) {
        int32_t ax = samples[k].ax, ay = samples[k].ay, az = samples[k].az;
        int32_t mag2 = ax*ax + ay*ay + az*az;          // magnitude squared; fits in int32

        if (mag2 < thr2) {
            run++;
            if (!in_ff && run >= FF_MIN_SAMPLES) {
                in_ff = 1;
                t_start = samples[k - FF_MIN_SAMPLES + 1].t_ms;   // the first sample of the run
                printf("FREEFALL start at %lu ms\n", t_start);
            }
        } else {
            if (in_ff) {
                in_ff = 0;
                uint32_t dur = samples[k].t_ms - t_start;
                printf("FREEFALL end at %lu ms, duration %lu ms\n", samples[k].t_ms, dur);
            }
            run = 0;
        }
    }
}
/* Phase detection: free fall --- end */

/** @addtogroup STM32F4xx_HAL_Examples
  * @{
  */

/** @addtogroup GPIO_EXTI
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);
static void EXTILine0_Config(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
 /* This sample code shows how to use STM32F4xx GPIO HAL API to toggle PD12, PD13,
    PD14, and PD14 IOs (connected to LED4, LED3, LED5 and LED6 on STM32F411E-DISCO board (MB1115B)) 
    in an infinite loop.
    To proceed, 3 steps are required: */

  /* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */
  HAL_Init();
 
  /* Configure LED3, LED4, LED5 and LED6 */
  BSP_LED_Init(LED3);
  BSP_LED_Init(LED4);
  BSP_LED_Init(LED5);
  BSP_LED_Init(LED6);
  
  /* Configure the system clock to 100 MHz */
  SystemClock_Config();
    
  /* Configure EXTI Line0 (connected to PA0 pin) in interrupt mode */
  EXTILine0_Config();

  i2c1_init();

  /* check devices */
  printf("accel LSM303AGR 0x19: %s\n",
         HAL_I2C_IsDeviceReady(&hi2c1, ACC_ADDR, 3, 100) == HAL_OK ? "yes" : "no");
  printf("mag LSM303AGR 0x1E: %s\n",
         HAL_I2C_IsDeviceReady(&hi2c1, MAG_ADDR, 3, 100) == HAL_OK ? "yes" : "no");

  // write accel data
  wr(ACC_ADDR, 0x20, 0x57);   // 100 Hz, normal power, XYZ on
  wr(ACC_ADDR, 0x23, 0x08);   // +-2 g, high resolution
  /* wr(MAG_ADDR, 0x00, 0x90); wr(MAG_ADDR, 0x01, 0x20); wr(MAG_ADDR, 0x02, 0x00); */

  gyro_spi_init();

  uint8_t id = 0;
  HAL_StatusTypeDef st = gyro_rd(0x0F, &id, 1);
  printf("gyro I3G4250D WHO_AM_I 0x%02X, HAL status %d\n", id, st); // D3

  // write gyro data
  gyro_wr(0x20, 0x4F);   // CTRL_REG1: 190 Hz output, powered on, X/Y/Z enabled
  gyro_wr(0x23, 0x90);   // CTRL_REG4: block-data-update on, +-500 dps

  printf("> %lu Press B1 to start the drop test\n", HAL_GetTick());

  while (1)
  {

	  // drop test: drop > read values > print after drop
      start_flag = 0;
      while (!start_flag) {} // wait for B1 press

      // settling for recording: delay after the B1 press (so that pushing doesn't affect measurements)
      BSP_LED_On(LED3);
      HAL_Delay(1000);
      BSP_LED_Off(LED3);

      // recording
      BSP_LED_On(LED6);
      record_samples();
      BSP_LED_Off(LED6);

      print_samples();
      detect_freefall();
      printf("> %lu Drop test ended (press B1 to repeat)\n", HAL_GetTick());
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSI)
  *            SYSCLK(Hz)                     = 100000000
  *            HCLK(Hz)                       = 100000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 2
  *            APB2 Prescaler                 = 1
  *            HSI Frequency(Hz)              = 16000000
  *            PLL_M                          = 16
  *            PLL_N                          = 400
  *            PLL_P                          = 4
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 3
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
  /* Enable HSI Oscillator and activate PLL with HSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 0x10;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}



/**
  * @brief  Configures EXTI Line0 (connected to PA0 pin) in interrupt mode
  * @param  None
  * @retval None
  */
static void EXTILine0_Config(void)
{
  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOA clock */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  
  /* Configure PA0 pin as input floating */
  GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Pin = GPIO_PIN_0;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Enable and set EXTI Line0 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

/**
  * @brief EXTI line detection callbacks
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == KEY_BUTTON_PIN)
  {
	  start_flag = 1;
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Turn LED5 on */
  BSP_LED_On(LED5);
  while(1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */
