/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include "stdio.h"

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);


/* Function processing DMA Rx data. Counts how many capital and small letters are in sentence.
 * Result is supposed to be stored in global variable of type "letter_count_" that is defined in "main.h"
 *
 * @param1 - received sign
 */
void proccesDmaData(uint8_t sign);
void characterCaseCounter(char *data, uint8_t length);
void sendBufOccupMessage(uint16_t bufferCapacity,uint16_t occupMem);

/* Space for your global variables. */
letter_count_ letter_count;

int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* Configure the system clock */
  SystemClock_Config();
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();

  /* Space for your local variables, callback registration ...*/

  USART2_RegisterCallback(proccesDmaData);

  while (1)
  {
	  /* Periodic transmission of information about DMA Rx buffer state.
	   * Transmission frequency - 5Hz.
	   * Message format - "Buffer capacity: %d bytes, occupied memory: %d bytes, load [in %]: %f%"
	   * Example message (what I wish to see in terminal) - Buffer capacity: 1000 bytes, occupied memory: 231 bytes, load [in %]: 23.1%
	   */
	  sendBufOccupMessage(DMA_USART2_BUFFER_SIZE, USART2_dma_occupied_memory);
	  LL_mDelay(200);
  }
  /* USER CODE END 3 */
}


void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

  if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0)
  {
  Error_Handler();  
  }
  LL_RCC_HSI_Enable();

   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {
    
  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
  {
  
  }
  LL_Init1msTick(8000000);
  LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
  LL_SetSystemCoreClock(8000000);
}

/*
 * Implementation of function processing data received via USART.
 */
void proccesDmaData(uint8_t sign){
	const uint8_t buffer_length = 36;
	static char buffer[36];
	static uint8_t recieved_starting_char;
	static uint8_t pos;

	//check for starting character
	if(sign == '#'){
		recieved_starting_char = 1;
	}
	else if(recieved_starting_char == 0){
		return;
	}

	//starting character received
	if(pos < buffer_length ){
		buffer[pos] = sign;
		pos++;
	}
	else{
		// ending '$' char not received within 35 function calls since starting char
		recieved_starting_char = 0;
		pos = 0;
	}

	//check for ending character
	if((sign == '$')&&(recieved_starting_char)){
		characterCaseCounter(buffer,pos);
		recieved_starting_char = 0;
		pos = 0;
	}
}

void characterCaseCounter(char *data, uint8_t length){
	letter_count.small_letter = 0;
	letter_count.capital_letter = 0;
	for(int i = 0; i<length; i++){
		//count upper case characters
		if(('A' <= data[i]) && (data[i] <= 'Z')){
			letter_count.capital_letter++;
		}
		//count lower case characters
		if(('a' <= data[i]) && (data[i] <= 'z')){
			letter_count.small_letter++;
		}
	}
}

void sendBufOccupMessage(uint16_t bufferCapacity,uint16_t occupMem){
	//Buffer capacity: %d bytes, occupied memory: %d bytes, load [in %]: %f%"
	uint8_t numString[10];
	static uint8_t part1[] = "Buffer capacity: ";
	USART2_PutBuffer(part1, sizeof(part1));
	while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7)){};

	USART2_PutBuffer(numString, sprintf((char*)numString,"%d",bufferCapacity));
	while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7)){};

	static uint8_t part2[] = " bytes, occupied memory: ";
	USART2_PutBuffer(part2, sizeof(part2));
	while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7)){};

	USART2_PutBuffer(numString, sprintf((char*)numString,"%d",occupMem));
	while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7)){};

	static uint8_t part3[] = " bytes, load [in %]: ";
	USART2_PutBuffer(part3, sizeof(part3));
	while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7)){};


	USART2_PutBuffer(numString, sprintf((char*)numString,"%f",(float)occupMem/bufferCapacity*100));
	while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7)){};

	static uint8_t part4[] = "\n\r";
	USART2_PutBuffer(part4, sizeof(part4));
	while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7)){};
}


void Error_Handler(void)
{

}

#ifdef  USE_FULL_ASSERT

void assert_failed(char *file, uint32_t line)
{ 

}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
