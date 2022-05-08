/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f769i_discovery.h"
#include "stm32f769i_discovery_lcd.h"
#include "stm32f769i_discovery_ts.h"

#include <stdio.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ASTEROIDS_NUM 8
#define ASTEROIDS_RADIUS_SMALL 30
#define ASTEROIDS_RADIUS_MEDIUM 55
#define ASTEROIDS_RADIUS_BIG 70
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE BEGIN PV */
uint8_t  lcd_status = LCD_OK;
uint32_t ts_status = TS_OK;
TS_StateTypeDef  TS_State = {0};

osThreadId_t drawTaskHandle;
osMessageQueueId_t playerX_queue;

osThreadId_t playerBulletTaskHandle;
osMessageQueueId_t playerBullet_queue;
osMessageQueueId_t bulletX_queue;

osThreadId_t asteroidTaskHandle;
osMessageQueueId_t asteroids_queue;

osMessageQueueId_t asteroidsToBulletCollision_queue;
osMessageQueueId_t resetAsteroid_queue;

osMessageQueueId_t addToScore_queue;

struct player {
	uint16_t x;
};

struct player startingPos;

struct bullet{
	uint16_t x;
	uint16_t y;
	uint8_t isActive; /*yes - 1, no - 0, only one player bullet can be active*/
};

struct bullet playerBullet;

struct asteroid{
	uint16_t x;
	uint16_t y;
	uint16_t speed;
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */
void DrawTask(void *argument);
void PlayerBulletTask(void *argument);
void AsteroidTask(void *argument);
int collidesCircleAndRect(int circX, int circY, int circRad, int rectX, int rectY, int rectWidth, int rectHeight);
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
  /* USER CODE BEGIN 2 */
  BSP_LCD_Init();

  BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
  BSP_LCD_Clear(LCD_COLOR_WHITE);

  ts_status = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
  while(ts_status != TS_OK);

  ts_status = BSP_TS_ITConfig();
  while(ts_status != TS_OK);

  startingPos.x = 400;
  playerBullet.y = 395;
  playerBullet.isActive = 0;
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  playerX_queue = osMessageQueueNew(1, sizeof(uint8_t), NULL);
  playerBullet_queue = osMessageQueueNew(1, sizeof(struct bullet), NULL);
  bulletX_queue = osMessageQueueNew(1, sizeof(uint16_t), NULL);
  asteroids_queue = osMessageQueueNew(1, sizeof(struct asteroid [ASTEROIDS_NUM]), NULL);

  asteroidsToBulletCollision_queue = osMessageQueueNew(1, sizeof(struct asteroid [ASTEROIDS_NUM]), NULL);

  resetAsteroid_queue = osMessageQueueNew(1, sizeof(uint8_t), NULL);
  addToScore_queue = osMessageQueueNew(1, sizeof(uint8_t), NULL);
  /* 1 - +10, 0 - -10, 2 - no movement*/
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  drawTaskHandle = osThreadNew(DrawTask, NULL, &defaultTask_attributes);
  playerBulletTaskHandle = osThreadNew(PlayerBulletTask, NULL, &defaultTask_attributes);
  asteroidTaskHandle = osThreadNew(AsteroidTask, NULL, &defaultTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

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

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef  ret = HAL_OK;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is
	 clocked below the maximum system frequency, to update the voltage scaling value
	 regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 432;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  RCC_OscInitStruct.PLL.PLLR = 7;

  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
	while(1) { ; }
  }

  /* Activate the OverDrive to reach the 216 MHz Frequency */
  ret = HAL_PWREx_EnableOverDrive();
  if(ret != HAL_OK)
  {
	while(1) { ; }
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
  if(ret != HAL_OK)
  {
	while(1) { ; }
  }
}

/* USER CODE BEGIN 4 */
void DrawTask(void *argument)
{
  /* USER CODE BEGIN 5 */
	struct player thisPlayer;
	thisPlayer.x = startingPos.x;
	uint16_t bulletX;
	uint8_t playerXInc;

	struct bullet thisBullet;

	struct asteroid theseAsteroids[ASTEROIDS_NUM];

	int score = 0;
	uint8_t addToScore = 0;
	char scoreStr[10] = "0";
  /* Infinite loop */
  for(;;)
  {
	BSP_LCD_Clear(LCD_COLOR_WHITE);
	BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);

	//draw player
	osMessageQueueGet(playerX_queue, &playerXInc, NULL, 10);
	if(playerXInc == 1){
		thisPlayer.x += 15;
	}
	else if (playerXInc == 0){
		thisPlayer.x -= 15;
	}
	BSP_LCD_DrawRect(thisPlayer.x+10, 400, 30,50);
	BSP_LCD_DrawRect(thisPlayer.x, 450, 50,30);
	BSP_LCD_FillRect(thisPlayer.x+10, 400, 30,50);
	BSP_LCD_FillRect(thisPlayer.x, 450, 50,30);

	//get calc bullet X position
	bulletX = thisPlayer.x;
	osMessageQueuePut(bulletX_queue, &bulletX, 0, 10);
	//draw bullet
	osMessageQueueGet(playerBullet_queue, &thisBullet, NULL, 10);
	if(thisBullet.isActive == 1){
		BSP_LCD_DrawRect(thisBullet.x+20, thisBullet.y, 10,20);
	}

	//draw asteroids && check for collision between player and asteroid
	osMessageQueueGet(asteroids_queue, &theseAsteroids, NULL, 10);
	for(int i = 0; i < ASTEROIDS_NUM; i++){

		BSP_LCD_DrawCircle(theseAsteroids[i].x, theseAsteroids[i].y, ASTEROIDS_RADIUS_SMALL);

		int collides = collidesCircleAndRect((int) theseAsteroids[i].x,(int) theseAsteroids[i].y, ASTEROIDS_RADIUS_SMALL,(int) (thisPlayer.x + 10), 400 + 50, 30, 50); //we add plus 50 to player y to make it easier for dodging
		if(collides == 1){
			osKernelLock();
			BSP_LCD_Clear(LCD_COLOR_WHITE);
			BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);
			uint8_t strptr[] = "                   GAME OVER";
			BSP_LCD_DisplayStringAtLine(10, strptr);
			uint8_t scoreAppend[] = "                 Score: ";
			strncat(scoreAppend, scoreStr, 10);
			BSP_LCD_DisplayStringAtLine(12, scoreAppend);
			for(;;){
				int i = 1;
			}
		}

	}

	//write score
	osMessageQueueGet(addToScore_queue, &addToScore, NULL, 10);
	if(addToScore == 1){
		score += 10;
		snprintf( scoreStr, 10, "%d", score );
		addToScore = 0;
	}
	BSP_LCD_SetFont(&Font24);
	BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);
	BSP_LCD_DisplayStringAtLine(1, scoreStr);

    osDelay(20);
  }
  /* USER CODE END 5 */
}

void PlayerBulletTask(void *argument)
{
  /* USER CODE BEGIN 5 */
	struct bullet thisBullet;
	thisBullet.y = playerBullet.y;
	thisBullet.isActive = playerBullet.isActive;
	uint16_t bulletX;

	struct asteroid theseAsteroids[ASTEROIDS_NUM];
	uint8_t addToScore = 1;
  /* Infinite loop */
  for(;;)
  {
	//check collision between bullet and any asteroid
	osMessageQueueGet(asteroidsToBulletCollision_queue, &theseAsteroids, NULL, 10);
	for(int i = 0; i < ASTEROIDS_NUM; i++){
		if(thisBullet.isActive == 0){
			break;
		}
		int collides = collidesCircleAndRect((int) theseAsteroids[i].x,(int) theseAsteroids[i].y, ASTEROIDS_RADIUS_SMALL,(int) (thisBullet.x + 20),(int) thisBullet.y, 10, 20);
		if(collides == 1){
			//reset bullet
			thisBullet.isActive = 0;
			thisBullet.y = playerBullet.y;

			//send message to asteroid queue to delete asteroid
			osMessageQueuePut(resetAsteroid_queue, &i, 0, 10);

			//send message to increase score
			osMessageQueuePut(addToScore_queue, &addToScore, 0, 10);
		}
	}

	//if inactive set bulletX to playerX
	osMessageQueueGet(bulletX_queue, &bulletX, NULL, 10);
	if(thisBullet.isActive == 0){
		thisBullet.x = bulletX;
	}
	BSP_TS_GetState(&TS_State);
	//if we touch near top of screen and no bullet on screen spawn one
	if(TS_State.touchDetected > 0){
		if(TS_State.touchY[0] < 200 && TS_State.touchY[0] > 30){
			if(thisBullet.isActive == 0){
				thisBullet.isActive = 1;
			}
		}
	}

	//if bullet on screen move it upward
	if(thisBullet.isActive == 1){
		thisBullet.y -= 15;
	}

	//if bullet leaves screen set it to inactive and reset bullet y position
	if(thisBullet.y < 30){
		thisBullet.isActive = 0;
		thisBullet.y = playerBullet.y;
	}

	osMessageQueuePut(playerBullet_queue, &thisBullet, 0, 10);
    osDelay(20);
  }
  /* USER CODE END 5 */
}

void AsteroidTask(void *argument)
{
  /* USER CODE BEGIN 5 */
	struct asteroid asteroids[ASTEROIDS_NUM];

	for(int i = 0; i < ASTEROIDS_NUM; i++){
		asteroids[i].y = 30;
		asteroids[i].x = rand() % 700;
		asteroids[i].speed = (rand() % (12 - 10+1) + 10);
	}

	int collidedAsteroid = ASTEROIDS_NUM+1;
  /* Infinite loop */
  for(;;)
  {
	//if we collide, reset the asteroid
	osMessageQueueGet(resetAsteroid_queue, &collidedAsteroid, NULL, 10);
	if(collidedAsteroid <= ASTEROIDS_NUM){
		asteroids[collidedAsteroid].y = 30;
		asteroids[collidedAsteroid].x = rand() % 700;
		asteroids[collidedAsteroid].speed = (rand() % (12 - 10+1) + 10);
		collidedAsteroid = ASTEROIDS_NUM + 1; //this is needed to stop the task from continuosly deleting the same asteroid
	}

	for(int i = 0; i < ASTEROIDS_NUM; i++){

		//move asteroid downward
		asteroids[i].y += asteroids[i].speed;

		//if asteroid below screen reset
		if(asteroids[i].y > 450){
			asteroids[i].y = 30;
			asteroids[i].x = rand() % 700;
			asteroids[i].speed = (rand() % (12 - 10+1) + 10);
		}

	}

	osMessageQueuePut(asteroids_queue, &asteroids, 0, 10);
	osMessageQueuePut(asteroidsToBulletCollision_queue, &asteroids, 0, 10);
    osDelay(20);
  }
  /* USER CODE END 5 */
}

//function to check for collision between a circle and a rectangle
int collidesCircleAndRect(int circX, int circY, int circRad, int rectX, int rectY, int rectWidth, int rectHeight)
{
    int circleDistanceX = abs(circX - rectX);
    int circleDistanceY = abs(circY - rectY);

    if (circleDistanceX > (rectWidth/2 + circRad)) { return 0; }
    if (circleDistanceY > (rectHeight/2 + circRad)) { return 0; }

    if (circleDistanceX <= (rectWidth/2)) { return 1; }
    if (circleDistanceY <= (rectHeight/2)) { return 1; }

    int cornerDistance_sq = ((circleDistanceX - rectWidth/2)^2) +
                         	 ((circleDistanceY - rectHeight/2)^2);

    return (cornerDistance_sq <= (circRad^2));
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
	uint8_t playerXInc;
  /* Infinite loop */
  for(;;)
  {
    BSP_TS_GetState(&TS_State);
    //if touching left part of screen go left, if right go right
	if(TS_State.touchDetected > 0) {
		if(TS_State.touchY[0] > 200){
			if (TS_State.touchX[0] > 400){
				playerXInc = 1;
			}
			else{
				playerXInc = 0;
			}
		}
	}
	else{
		playerXInc = 2;
	}
	osMessageQueuePut(playerX_queue, &playerXInc, 0, 10);
    osDelay(20);
  }
  /* USER CODE END 5 */
}

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

#ifdef  USE_FULL_ASSERT
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

