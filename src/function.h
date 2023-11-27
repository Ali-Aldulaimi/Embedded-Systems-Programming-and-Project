/*
 * function.h
 *
 *  Created on: 26 Nov 2023
 *      Author: Muhammad Zeeshan
 */

#ifndef FUNCTION_H_
#define FUNCTION_H_

#include "stm32l1xx.h"

#define TS_CAL1 *((uint16_t *)0x1FF800FA) // TS_CAL1 for the internal temp sensor
#define TS_CAL2 *((uint16_t *)0x1FF800FE) // TS_CAL2 for the internal temp sensor

#define Select_Pin_B 5
#define Select_Pin_A 6

#define SLAVE_ADDR 0x01
#define START_ADDR 0x01

unsigned short int calculatedCRC;

void Adc_set_up();

void Mux_set_up();

void MFET_set_up();

void Select_mux_pin(int channel);

float Read_Cell_Voltage(void);

int Internal_Temp_Read();

int LM35_Temp_read();

void USART2_Init(void);

void USART2_write(char data);

char USART2_read(void);

void delay_Ms(int delay);

void display(char* buf);

unsigned short int CRC16(char *nData, unsigned short int wLength);
int check_input_reg(char rec);
void Response_frame(int Value1, int value2, int value3);
void wrong_slave(void);
int Mflag = 0;

#endif /* FUNCTION_H_ */
