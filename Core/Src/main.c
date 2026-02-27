/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum{/* Enum amb tots els estats possibles de Ready To Drive */
	R2D_inicial = 0,// Estat inicial: res preparat
	R2D_step1, // STEP1: condició inicial complerta (brake+botó+SDC+aire)
	R2D_step2, // STEP2: botó alliberat i esperant 2s
	R2D_step3, // STEP3: botó premut de nou amb condicions OK
	R2D_step4,// STEP4: sistema en Ready To Drive
}r2d_state_t;

typedef  int bool;// definim que significa per nosaltres bool, 0 si val 1 o 0 i el mateix per true
#define true 1
#define false 0
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define Brake_LLINDAR 300
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
bool SDC = 0; // Estat de l'entrada SDC (Safety Disconnect / Shutdown circuit)
bool EstatAirpositiu  = 0;// Estat del sensor d'aire positiu (pressió OK)
bool Button_R2DX = 0; // Estat del botó Ready To Drive
bool Enable = 0;// Senyal d'habilitació (per APPS / inverter)
uint16_t Brake_SensorX = 0;// Valor analògic del sensor de fre (lectura ADC)
uint32_t temp_R2D = 0;// Temps (en ms) en què entrem a STEP2
r2d_state_t state_r2d = R2D_inicial;// Estat actual de la funció


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* Lògica completa de Ready To Drive: llegir entrades i generar sortides */





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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	// Llegir l'entrada digital SDC (GPIOB pin 6): HAL_GPIO_ReadPin retorna GPIO_PIN_SET o GPIO_PIN_RESET
	SDC = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6);
	// Llegir l'entrada de l'air positiu (GPIOB pin 7)
	EstatAirpositiu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7);
	// Llegir l'estat del botó Ready To Drive (GPIOB pin 9)
	Button_R2DX = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9);


	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	Brake_SensorX = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);

	// Màquina d'estats finits per la seqüència Ready To Drive
	switch (state_r2d)
	{

		// Condicions per passar d'inicial a STEP1: fre premut (Brake_SensorX >= 300), SDC actiu (SDC != 0), botó R2D premut (Button_R2DX != 0),
		//Aire positiu OK (EstatAirpositiu != 0) i sense error d'APPS (!error_apps)
		case R2D_inicial:
			if (Brake_SensorX >= 300 && SDC == true && Button_R2DX == true && EstatAirpositiu == true)
				state_r2d = R2D_step1;
			break;



		// En STEP1 exigim: Fre continua premut, SDC actiu, botó alliberat (!Button_R2DX), air positiu OK i sense error APPS
		case R2D_step1:
			if (Brake_SensorX >= 300 && SDC== true && Button_R2DX == 0 && EstatAirpositiu == true)
			{
				temp_R2D = HAL_GetTick();// HAL_GetTick() dona el nombre de mil·lisegons des de HAL_Init() (és un contador global de temps del sistema incrementat per l'interrupt de SysTick).
				state_r2d = R2D_step2;// Passem a STEP2 (finestra d'espera de 2 segons)
			}
			else if (SDC == 0 && EstatAirpositiu == 0) // Si es perd SDC, air positiu o hi ha error d'APPS, tornem a l'estat inicial
				state_r2d = R2D_inicial;
			break;




		// En STEP2: SDC actiu, botó segueix alliberat, air positiu OK, sense error APPS i han passat com a mínim 2000 ms (2 s) des de temp_R2D
		case R2D_step2:
			if (SDC == true && Button_R2DX == 0 && EstatAirpositiu== true && (HAL_GetTick() - temp_R2D) >= 2000)// Aquí, determinem el temps que s'ha avançat respecte el punt inicial
				state_r2d = R2D_step3;// Si es compleix tot això, passem a STEP3
			else if (SDC == 0 && EstatAirpositiu== 0) // Qualsevol pèrdua de SDC, air positiu o error APPS ens fa tornar a inicial
				state_r2d = R2D_inicial;
			break;



		// En STEP3 tornem a demanar: fre premut, SDC actiu, botó R2D premut, air positiu OK i sense error APPS
		case R2D_step3:
			if (Brake_SensorX >= 300 && SDC == true && Button_R2DX == true && EstatAirpositiu == true)
				state_r2d = R2D_step4;// Tot correcte: entrem a READY TO DRIVE (STEP4)
			else if (SDC == 0 && EstatAirpositiu == 0)
				state_r2d = R2D_inicial; // Qualsevol fallada de seguretat → tornar a inicial
			break;



		// En Ready To Drive, vigilem contínuament que: SDC segueixi actiu, air positiu OK i sense error d'APPS
		case R2D_step4:
			if (Brake_SensorX >= 300 && SDC == 0 && Button_R2DX == 0 && EstatAirpositiu == true)
				state_r2d = R2D_inicial;// Si alguna condició de seguretat cau, deshabilitem R2D
			else if(SDC == 0 && EstatAirpositiu == 0)
				state_r2d = R2D_inicial;
			break;
	}

	// Sortides segons l'estat

	// Estat segur/no preparat: tot apagat
	if (state_r2d == R2D_inicial || state_r2d == R2D_step1)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET); //Estat buzzer en repòs
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);//Estat enable en repòs
	}
	else if (state_r2d == R2D_step2)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);//Estat buzzer actiu
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);//Estat enable en repòs
	}
	else
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);//Estat buzzer en repòs
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);//Estat enable en actiu
	}
}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
