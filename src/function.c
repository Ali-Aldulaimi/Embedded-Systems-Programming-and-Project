/*
 * function.c
 *
 *  Created on: 26 Nov 2023
 *      Author: Muhammad Zeeshan
 */

#include "function.h"

/* Define an array for select pins */
const int selectPins[2] = { Select_Pin_A, Select_Pin_B };

void Adc_set_up() {

	// set Analog pin
	RCC->AHBENR |= 1;					// enable GPIOA clock
	GPIOA->MODER |= 0x3;				// PA0 for LM35
	GPIOA->MODER |= (1 << 3) | (1 << 2); // PA1 for the multiplexer
	GPIOA->MODER |= (1 << 9) | (1 << 8); // PA4 for the multiplexer
	// setup ADC1 and temp sensor. p287
	RCC->APB2ENR |= 0x00000200; // enable ADC1 clock
	// set the resolution
	ADC1->CR1 &= ~0x03000000; // resolution 12-bit
	// set the sampling rate
	ADC1->SMPR3 = 7;		// sampling time longest 384 cycles for channel 0
	ADC1->SMPR3 |= (1 << 3) | (1 << 4) | (1 << 5);// sampling time longest 384 cycles for channel 1
	ADC1->SMPR3 |= (1 << 12) | (1 << 13) | (1 << 14);// sampling time longest 384 cycles for channel 4
	ADC1->SMPR2 |= (1 << 18) | (1 << 19) | (1 << 20); // sampling time longest 384 cycles for channel 16
	// set single-mode , bit 11 = 0 align right
	ADC1->CR2 = 0;
	// enable temp sensor
	ADC->CCR |= (1 << 23);
}

void Mux_set_up() {
	// set select pins

	GPIOA->MODER |= (1 << 10); // pin PA5 as Output   D13 	(B)
	GPIOA->MODER |= (1 << 12); // pin PA6 as Output   D12	(A)
}

void MFET_set_up() {
	GPIOA->MODER |= (2 << 16); // pin PA8 for C_FET  D7
	GPIOA->MODER |= (2 << 18); // pin PA9 for D_FET  D8
}

void Select_mux_pin(int channel) {
	for (int i = 0; i < 2; i++) {
		if (channel & (1 << i)) {
			GPIOA->ODR |= (1 << selectPins[i]);
		} else
			GPIOA->ODR &= ~(1 << selectPins[i]);
	}
}

float Read_Cell_Voltage(void) {
	int result = 0;
	float mili_volt = 0;
	ADC1->SQR5 = 1;	// Set the conversion sequence to start at the specified channel
	ADC1->CR2 |= 1;			 // Bit 0, ADC on/off (1=on, 0=off)
	ADC1->CR2 |= 0x40000000; // Start conversion
	while (!(ADC1->SR & 2)) {
	}				   // Wait for conversion complete
	result = ADC1->DR; // Read conversion result

	mili_volt = ((result * 3.33) / 4095.0) * 1000;

	ADC1->CR2 &= ~1; // bit 0, ADC on/off (1=on, 0=off)

	return mili_volt;
}

int Internal_Temp_Read() {
	int result = 0;	 // store adc value
	ADC1->SQR5 = 16; // SQ1 for channel 16 (internal sensor)
	ADC1->CR2 |= 1;	 // bit 0, ADC on/off (1=on, 0=off)

	ADC1->CR2 |= 0x40000000; // start conversion
	while (!(ADC1->SR & 2))
		; // wait for conversion to complete

	result = ADC1->DR;

	float result2 = (110.0 - 30.0) / ((float) TS_CAL2 - (float) TS_CAL1)
			* ((float) result - (float) TS_CAL1) + 50.0;
	result2 = result2 * 100;
	int result3 = (int) result2;

	ADC1->CR2 &= ~1; // bit 0, ADC on/off (1=on, 0=off)

	return result3;
}

int LM35_Temp_read() {
	int result = 0;
	ADC1->SQR5 = 0;			 // SQ1 for channel (A0)
	ADC1->CR2 |= 1;			 // bit 0, ADC on/off (1=on, 0=off)
	ADC1->CR2 |= 0x40000000; // start conversion
	while (!(ADC1->SR & 2))
		; // wait for conversion to complete

	result = ADC1->DR;

	float deg_dec = (result / 4095.0) * 3.3;
	deg_dec *= 100;					 // to get rid of decimal
	int final_deg = (deg_dec) / 100; // to grab integer part
	final_deg *= 13.33;
	int final_deg_dec = ((int) deg_dec % 100); // to grab decimal part
	final_deg_dec = (final_deg_dec * 13.33) / 100;
	int final_value = final_deg + final_deg_dec;

	ADC1->CR2 &= ~1; // bit 0, ADC on/off (1=on, 0=off)

	return final_value;
}

void USART2_Init(void) {
	RCC->APB1ENR |= 0x00020000;	 // set bit 17 (USART2 EN)
	RCC->AHBENR |= 0x00000001;	 // enable GPIOA port clock bit 0 (GPIOA EN)
	GPIOA->AFR[0] = 0x00000700;	 // GPIOx_AFRL p.188,AF7 p.177
	GPIOA->AFR[0] |= 0x00007000; // GPIOx_AFRL p.188,AF7 p.177
	GPIOA->MODER |= 0x00000020;	// MODER2=PA2(TX) to mode 10=alternate function mode. p184
	GPIOA->MODER |= 0x00000080;	// MODER2=PA3(RX) to mode 10=alternate function mode. p184

	USART2->BRR = 0x00000116;  // 11500 BAUD and crystal 32MHz. p710, 116
	USART2->CR1 = 0x00000008;  // TE bit. p739-740. Enable transmit
	USART2->CR1 |= 0x00000004; // RE bit. p739-740. Enable receiver
	USART2->CR1 |= 0x00002000; // UE bit. p739-740. Uart enable
}

void USART2_write(char data) {
	// wait while TX buffer is empty
	while (!(USART2->SR & 0x0080)) {
	}					 // TXE: Transmit data register empty. p736-737
	USART2->DR = (data); // p739
}

char USART2_read() {
	char data = 0;
	// wait while RX buffer data is ready to be read
	while (!(USART2->SR & 0x0020)) {
	}				   // Bit 5 RXNE: Read data register not empty
	data = USART2->DR; // p739
	return data;
}

void delay_Ms(int delay) {
	int i = 0;
	for (; delay > 0; delay--)
		for (i = 0; i < 2460; i++)
			; // measured with oscilloscope
}

void display(char* buf) {
	int len = 0;
	while (buf[len] != '\0')
		len++;

	for (int i = 0; i < len; i++) {
		USART2_write(buf[i]);
	}

	USART2_write('\n');
	USART2_write('\r');

}

/*******************************************************************************************/

unsigned short int CRC16(char *nData, unsigned short int wLength) {
	uint8_t i, j;
	unsigned short int crc = 0xFFFF;
	for (i = 0; i < wLength; i++) {
		crc ^= nData[i];
		for (j = 0; j < 8; j++) {
			if (crc & 1) {
				crc = (crc >> 1) ^ 0xA001;
			} else {
				crc = crc >> 1;
			}
		}
	}
	return crc;
}
/*******************************************************************************************/

void wrong_slave(void) {

	USART2->CR1 &= ~0x00000004;		//RE bit. p739-740. Disable receiver
	delay_Ms(10); 					//time=1/9600 x 10 bits x 7 byte = 7,29 ms
	USART2->CR1 |= 0x00000004;		//RE bit. p739-740. Enable receiver
	USART2->CR1 |= 0x0020;			//enable RX interrupt
	Mflag = 0;
}

/*******************************************************************************************/

void USART2_IRQHandler(void) {

	char received_addr = 0;
	//char buf[20];

	//This bit is set by hardware when the content of the
	//RDR shift register has been transferred to the USART_DR register.
	if (USART2->SR & 0x0020) 		//if data available in DR register. p737
			{
		received_addr = USART2->DR;
		//USART_write(received_addr);
	}
	if (received_addr == SLAVE_ADDR) {
		Mflag = 1;
		//USART2_write(received_addr);

	}

	else {
		Mflag = 2;
		//USART2_write(received_addr);
	}

	USART2->CR1 &= ~0x0020;	//Disable USARTx interrupt (RXNEIE interrupt disable)
}

/*******************************************************************************************/

int check_input_reg(char rec) {
	int check;
	if (rec == START_ADDR) {
		check = 1;

	} else
		check = 0;
	return check;
}

/*******************************************************************************************/

void Response_frame(int internal_Temp, int external_Temp, int cell_VOl) {
// Turn on a control LED while sending data
	GPIOA->ODR |= 0x20;

// Create a frame for communication
	char response_frame[13] = { SLAVE_ADDR, 0x04, 0x06, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0 };

	char internal_sensor_high_byte = 0;
	char internal_sensor_low_byte = 0;
	char external_sensor_high_byte = 0;
	char external_sensor_low_byte = 0;
	char cell_high_byte = 0;
	char cell_low_byte = 0;

// Fill in the sensor data into the frame
	/******* for Internal Temperature**********/
	internal_sensor_high_byte = (internal_Temp >> 8)
			| internal_sensor_high_byte;
	internal_sensor_low_byte = internal_Temp | internal_sensor_low_byte;

	response_frame[3] = internal_sensor_high_byte;
	response_frame[4] = internal_sensor_low_byte;

	/******* for External Temperature**********/

	external_sensor_high_byte = (external_Temp << 8)
			| external_sensor_high_byte;
	external_sensor_low_byte = external_Temp | external_sensor_low_byte;

	response_frame[5] = external_sensor_high_byte;
	response_frame[6] = external_sensor_low_byte;

	/******* for Cell Voltages**********/

	cell_high_byte = (cell_VOl << 8) | cell_high_byte;
	cell_low_byte = cell_VOl | cell_low_byte;

	response_frame[7] = cell_high_byte;
	response_frame[8] = cell_low_byte;

// Calculate and add CRC (Cyclic Redundancy Check) for error detection
	calculatedCRC = CRC16(response_frame, 11); // Adjust the size based on your frame structure
	response_frame[11] = (calculatedCRC >> 8); // High byte of CRC
	response_frame[12] = calculatedCRC; // Low byte of CRC

// Send the frame over USART2
	for (int i = 0; i < 13; i++) {
// Assuming USART2_write is a function that sends data over USART2
		USART2_write(response_frame[i]);
	}

// Turn off the control LED, indicating the end of transmission
	GPIOA->ODR &= ~0x20;
}
