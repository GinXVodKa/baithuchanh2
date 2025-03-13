#include <stdio.h>
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "string.h"

// Function declarations
void Timer_Config(void);
void Delay_ms(uint32_t ms);
void Delay_us(uint32_t us);
void USART_Config(void);
void USART_SendString(char *str);
void DHT11_Config(void);
void USART_SendNumber(uint8_t num);

// Global variables
uint16_t u16Tim;
uint8_t u8Buff[5];
uint8_t u8CheckSum;
unsigned int i, j;

int main(void) {
    DHT11_Config();
    USART_Config();
    Timer_Config();
    
    while (1) {
        // Start signal
        GPIO_ResetBits(GPIOB, GPIO_Pin_12);
        Delay_ms(20);
        GPIO_SetBits(GPIOB, GPIO_Pin_12);
        
        // Wait for response from DHT11
        TIM_SetCounter(TIM2, 0);
        while (TIM_GetCounter(TIM2) < 10) {
            if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12)) {
                break;
            }
        }
        u16Tim = TIM_GetCounter(TIM2);
        if (u16Tim >= 10) {
            continue;
        }

        // Wait for 45us low signal
        TIM_SetCounter(TIM2, 0);
        while (TIM_GetCounter(TIM2) < 45) {
            if (!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12)) {
                break;
            }
        }
        u16Tim = TIM_GetCounter(TIM2);
        if ((u16Tim >= 45) || (u16Tim <= 5)) {
            continue;
        }

        // Wait for 90us high signal
        TIM_SetCounter(TIM2, 0);
        while (TIM_GetCounter(TIM2) < 90) {
            if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12)) {
                break;
            }
        }
        u16Tim = TIM_GetCounter(TIM2);
        if ((u16Tim >= 90) || (u16Tim <= 70)) {
            continue;
        }

        // Wait for 95us low signal
        TIM_SetCounter(TIM2, 0);
        while (TIM_GetCounter(TIM2) < 95) {
            if (!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12)) {
                break;
            }
        }
        u16Tim = TIM_GetCounter(TIM2);
        if ((u16Tim >= 95) || (u16Tim <= 75)) {
            continue;
        }

        // Read 5 bytes of data
        for (i = 0; i < 5; i++) {
            for (j = 0; j < 8; j++) {
                // Wait for 65us low signal
                TIM_SetCounter(TIM2, 0);
                while (TIM_GetCounter(TIM2) < 65) {
                    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12)) {
                        break;
                    }
                }
                u16Tim = TIM_GetCounter(TIM2);
                if ((u16Tim >= 65) || (u16Tim <= 45)) {
                    continue;
                }

                // Wait for 80us high signal
                TIM_SetCounter(TIM2, 0);
                while (TIM_GetCounter(TIM2) < 80) {
                    if (!GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12)) {
                        break;
                    }
                }
                u16Tim = TIM_GetCounter(TIM2);
                if ((u16Tim >= 80) || (u16Tim <= 10)) {
                    continue;
                }

                // Store the data bit
                u8Buff[i] <<= 1;
                if (u16Tim > 45) {
                    u8Buff[i] |= 1;
                } else {
                    u8Buff[i] &= ~1;
                }
            }
        }

        // Verify checksum
        u8CheckSum = u8Buff[0] + u8Buff[1] + u8Buff[2] + u8Buff[3];
        if (u8CheckSum != u8Buff[4]) {
            continue;
        }

        // Send temperature and humidity data via USART
        USART_SendString("Temperature: ");
        USART_SendNumber(u8Buff[2]);
        USART_SendString("*C\n");
        USART_SendString("Humidity: ");
        USART_SendNumber(u8Buff[0]);
        USART_SendString("%\n");
    }
}

// Timer configuration for delays
void Timer_Config(void) {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    TIM_TimeBaseInitTypeDef Timer;
    Timer.TIM_Period = 0xFFFF;
    Timer.TIM_Prescaler = 72 - 1;
    Timer.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &Timer);
    TIM_Cmd(TIM2, ENABLE);
}

// Delay in microseconds
void Delay_us(uint32_t us) {
    TIM_SetCounter(TIM2, 0);
    while (TIM_GetCounter(TIM2) < us);
}

// Delay in milliseconds
void Delay_ms(uint32_t ms) {
    while (ms--) {
        Delay_us(1000);
    }
}

// USART configuration for communication
void USART_Config(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef gpio;
    
    // PA9 - Tx
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    
    // PA10 - Rx
    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    USART_InitTypeDef uart;
    uart.USART_BaudRate = 9600;
    uart.USART_WordLength = USART_WordLength_8b;
    uart.USART_StopBits = USART_StopBits_1;
    uart.USART_Parity = USART_Parity_No;
    uart.USART_Mode = USART_Mode_Tx;
    uart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &uart);

    USART_Cmd(USART1, ENABLE);
}

// Send a string via USART
void USART_SendString(char *str) {
    while (*str) {
        USART_SendData(USART1, *str);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        str++;
    }
}

// Send a number via USART
void USART_SendNumber(uint8_t num) {
    char buffer[5];
    sprintf(buffer, "%d", num);
    USART_SendString(buffer);
}

// DHT11 sensor configuration
void DHT11_Config(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef gpio;
    
    gpio.GPIO_Pin = GPIO_Pin_12;
    gpio.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);
}
