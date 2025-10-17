/*
 * userApp.c
 *
 *  Created on: Dec 8, 2023
 *      Author: Niall.OKeeffe@atu.ie
 */

#include "userApp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>

//--------------------------------------------------------------
//used for real time stats, do not delete code from this section
extern TIM_HandleTypeDef htim7;
extern volatile unsigned long ulHighFrequencyTimerTicks;
void configureTimerForRunTimeStats(void)
{
    ulHighFrequencyTimerTicks = 0;
    HAL_TIM_Base_Start_IT(&htim7);
}
unsigned long getRunTimeCounterValue(void)
{
	return ulHighFrequencyTimerTicks;
}
//end of real time stats code
//----------------------------------------------------------------

extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim6;

SemaphoreHandle_t resetSemaphore = NULL, timerSemaphore = NULL;
static void timerTask(void * pvParameters);

// _write function used for printf
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
	return len;
}

//Button interrupt callback
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if(GPIO_Pin == BUTTON_EXTI13_Pin){
		printf("Button pressed, sending reset semaphore\r\n\n");
		xSemaphoreGiveFromISR(resetSemaphore, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}
}

//TIM6 interrupt callback
void TIM6_Handler(){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(timerSemaphore, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void userApp() {
	printf("Starting application\r\n\n");

	//HAL_UART_Receive_IT(&huart1, &ch, 1);
	__HAL_TIM_CLEAR_IT(&htim6, TIM_IT_UPDATE);
	HAL_NVIC_GetPendingIRQ(TIM6_DAC_IRQn);
	HAL_TIM_Base_Start_IT(&htim6);

	xTaskCreate(timerTask, "timerTask", 200, NULL, 2, NULL);

	resetSemaphore = xSemaphoreCreateBinary();
	timerSemaphore = xSemaphoreCreateBinary();

	vTaskStartScheduler();

	while(1) {
	}
}

void timerTask(void * pvParameters) {
	uint32_t sec = 0;
	printf("Starting timer task\r\n\n");
	printf("Timer: %02lu\r\n", sec);
	while(1) {
		if(xSemaphoreTake(resetSemaphore, 1) == pdTRUE){
			sec = 0;
			printf("Resetting time to 0\r\n\n");
			printf("Timer: %02lu\r\n", sec);
		}
		else if(xSemaphoreTake(timerSemaphore, 1) == pdTRUE){
			sec++;
			printf("Timer: %02lu\r\n", sec);
		}

	}
}


