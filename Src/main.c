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
#define ACC_ADDR (0x19 << 1)
#define MAG_ADDR (0x1E << 1)

static void i2c1_init(void)
{
	// give clock signal for GPIO block port B (pins PB6 && PB9) and I2C1 block
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_6 | GPIO_PIN_9;
    g.Mode = GPIO_MODE_AF_OD;
    g.Pull = GPIO_PULLUP;
    g.Alternate = GPIO_AF4_I2C1; // connect pins to I2C (AF4 for PB6, PB9)
    HAL_GPIO_Init(GPIOB, &g);

    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000; // 100 kHz bus clock
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT; // 7-bit mode
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

// write one register
static void wr(uint16_t dev, uint8_t reg, uint8_t val)
{
    HAL_I2C_Mem_Write(&hi2c1, dev, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
}
/* Connect pins to I2C1 --- end */

/* Configure gyro --- start */
SPI_HandleTypeDef hspi_gyro;

static void gyro_spi_init(void)
{
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin       = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_PULLDOWN;
    g.Speed     = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &g);

    g.Pin  = GYRO_CS_PIN;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GYRO_CS_GPIO_PORT, &g);
    GYRO_CS_HIGH();

    hspi_gyro.Instance               = SPI1;
    hspi_gyro.Init.Mode              = SPI_MODE_MASTER;
    hspi_gyro.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi_gyro.Init.CLKPolarity       = SPI_POLARITY_LOW;
    hspi_gyro.Init.CLKPhase          = SPI_PHASE_1EDGE;
    hspi_gyro.Init.NSS               = SPI_NSS_SOFT;
    hspi_gyro.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // 100 MHz / 16 = 6,25 MHz (chip max 10)
    HAL_SPI_Init(&hspi_gyro);
}

// write one register
static void gyro_wr(uint8_t reg, uint8_t val)
{
	uint8_t tx[2] = { reg, val };
	uint8_t rx[2];
	GYRO_CS_LOW();
	HAL_SPI_TransmitReceive(&hspi_gyro, tx, rx, 2, 100);
	GYRO_CS_HIGH();
}

static HAL_StatusTypeDef gyro_rd(uint8_t reg, uint8_t *buf, uint8_t n)
{
    uint8_t tx[7] = {0};
    uint8_t rx[7] = {0};
    tx[0] = reg | READWRITE_CMD | (n > 1 ? MULTIPLEBYTE_CMD : 0);

    GYRO_CS_LOW();
    HAL_StatusTypeDef s = HAL_SPI_TransmitReceive(&hspi_gyro, tx, rx, n + 1, 100);
    GYRO_CS_HIGH();

    for (uint8_t i = 0; i < n; i++) buf[i] = rx[i + 1];
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

#define SAMPLE_PERIOD_MS 10 // 100 Hz

typedef struct {
    uint32_t t_ms;
    int16_t  ax, ay, az;
    int16_t  gx, gy, gz;
    int8_t   gtemp;
} sample_t;

static volatile uint8_t start = 0; // volatile > read value from memory each run

/* Read data */
static void read_sample(sample_t *s)
{
    uint8_t b[6], g[6], tr;

    s->t_ms = HAL_GetTick();

    HAL_I2C_Mem_Read(&hi2c1, ACC_ADDR, 0x28 | 0x80, I2C_MEMADD_SIZE_8BIT, b, 6, 100);
    s->ax = (int16_t)((b[1] << 8) | b[0]) >> 4;
    s->ay = (int16_t)((b[3] << 8) | b[2]) >> 4;
    s->az = (int16_t)((b[5] << 8) | b[4]) >> 4;

    gyro_rd(0x28, g, 6);
    s->gx = (int16_t)((g[1] << 8) | g[0]);
    s->gy = (int16_t)((g[3] << 8) | g[2]);
    s->gz = (int16_t)((g[5] << 8) | g[4]);

    gyro_rd(0x26, &tr, 1);
    s->gtemp = (int8_t)tr;
}

/*// UNUSED FUNCTION: Recording data samples after B1 press --- start
#define N_SAMPLES 500 // Recording time (500 x 10 ms = 5 s)

static sample_t samples[N_SAMPLES];

static void record_samples(void)
{
    uint32_t next = HAL_GetTick();

    for (int k = 0; k < N_SAMPLES; k++) {
        next += SAMPLE_PERIOD_MS;
        while ((int32_t)(HAL_GetTick() - next) < 0) {}

        read_sample(&samples[k]);
    }
}

static void print_samples(void)
{
    // t_ms | ax, ay, az (mg) | gx, gy, gz (mdps) | gyro temp (raw)
    for (int k = 0; k < N_SAMPLES; k++) {
        printf("%lu ms | %d, %d, %d mG | %ld, %ld, %ld mdps | %d\n temp", samples[k].t_ms,
               samples[k].ax, samples[k].ay, samples[k].az,
               (long)samples[k].gx * 175 / 10,           // +-500 dps: 17.5 mdps per LSB
               (long)samples[k].gy * 175 / 10,
               (long)samples[k].gz * 175 / 10,
               samples[k].gtemp);
    }
}
// UNUSED FUNCTION: Recording data samples after B1 press --- end */

/* Get live data samples --- start */
#define SETTLE_MS     1000
#define FF_THRESH_MG   300 // below this magnitude = "weightless"
#define FF_MIN_SAMPLES 3

static void get_live_data(void)
{
    sample_t s;
    uint32_t next = HAL_GetTick();
    int run = 0, in_ff = 0; // free fall detect
    uint32_t run_start = 0;
    const int32_t thr2 = (int32_t)FF_THRESH_MG * FF_THRESH_MG;

    while (!start) {
        next += SAMPLE_PERIOD_MS;
        while ((int32_t)(HAL_GetTick() - next) < 0) {}

        read_sample(&s);

        printf("%lu ms | %d, %d, %d mG | %ld, %ld, %ld mdps | %d\n *C", s.t_ms,
               s.ax, s.ay, s.az,
               (long)s.gx * 175 / 10, // mdps range: +-500 dps
               (long)s.gy * 175 / 10,
               (long)s.gz * 175 / 10,
               s.gtemp);

        // free fall detection
        int32_t mag2 = (int32_t)s.ax*s.ax + (int32_t)s.ay*s.ay + (int32_t)s.az*s.az;
        if (mag2 < thr2) {
            if (run == 0) run_start = s.t_ms;
            run++;
            if (!in_ff && run >= FF_MIN_SAMPLES) {
                in_ff = 1;
                printf(">>> Freefall start at %lu ms\n", run_start);
            }
        } else {
            if (in_ff) {
                in_ff = 0;
                printf(">>> Freefall end at %lu ms, duration %lu ms\n",
                       s.t_ms, s.t_ms - run_start);
            }
            run = 0;
        }
    }
}
/* Get live data samples --- end */

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

  /* Check devices */
  printf("accel LSM303AGR 0x19: %s\n",
         HAL_I2C_IsDeviceReady(&hi2c1, ACC_ADDR, 3, 100) == HAL_OK ? "yes" : "no");
  printf("mag LSM303AGR 0x1E: %s\n",
         HAL_I2C_IsDeviceReady(&hi2c1, MAG_ADDR, 3, 100) == HAL_OK ? "yes" : "no");

  // write accel data
  wr(ACC_ADDR, 0x20, 0x57);
  wr(ACC_ADDR, 0x23, 0x08);

  gyro_spi_init();

  uint8_t id = 0;
  HAL_StatusTypeDef st = gyro_rd(0x0F, &id, 1);
  printf("gyro I3G4250D WHO_AM_I 0x%02X, HAL status %d\n", id, st); // expected D3

  // write gyro data
  gyro_wr(0x20, 0x4F);
  gyro_wr(0x23, 0x90);

  printf("> %lu Press B1 to record values\n", HAL_GetTick());

  while (1)
  {
	  // previous implementation: drop test (press B1 > drop > record data > output data samples after the test)

      start = 0;
      while (!start) {} // wait for B1 press

      // delay after the B1 press, so that pushing B1 doesn't affect measurements
      BSP_LED_On(LED3);
      HAL_Delay(1000);
      BSP_LED_Off(LED3);
      start = 0;

      // live data
      BSP_LED_On(LED6);
      get_live_data();
      BSP_LED_Off(LED6);

      printf("> %lu Recording ended (press B1 to resume)\n", HAL_GetTick());
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
	  start = 1;
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
