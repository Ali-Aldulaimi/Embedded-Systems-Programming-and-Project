/* Includes */
#include <stddef.h>
#include <stdio.h>
#include "function.h"
#include "nucleo152start.h"

int main(void) {

	/* Configure the system clock to 32 MHz and update SystemCoreClock */
	SetSysClock();
	SystemCoreClockUpdate();
	USART2_Init();
	Adc_set_up();
	Mux_set_up();
	MFET_set_up();

	char Received_frame[13] = { 0 };
	uint16_t wlength = 13;
	//int int_temp = 0;
	int LM35_temp = 0;
	//int cell_voltage = 0;
	int check = 0;
	int Int_temp = 0;
	char internal_sensor_high_byte = 0;
	char internal_sensor_low_byte = 0;
	char external_sensor_high_byte = 0;
	char external_sensor_low_byte = 0;
	char cell_high_byte = 0;
	char cell_low_byte = 0;

	//char buf[200];	// contain transfering data

	char cell_vol[4] = { 0, 0, 0, 0 };
	float battery_vol = 0;
	//float shunt_resistor = 0.01;
	//float current = 0;

	GPIOA->ODR &= ~(1 << Select_Pin_B);
	GPIOA->ODR &= ~(1 << Select_Pin_A);
	/* Infinite loop */
	while (1) {
		switch (Mflag) {
		case 0:
			for (int i = 0; i < wlength; i++) {
				Received_frame[i] = '0';
			}
			break;

		case 1:

			Received_frame[0] = SLAVE_ADDR;
			for (int i = 1; i < wlength; i++) {
				Received_frame[i] = USART2_read();// take the remaining 7 bytes of the request frame
			}

			for (int i = 0; i < wlength; i++) {
				USART2_write(Received_frame[i]);		// display for debugging

			}

			unsigned short int CRC2 = CRC16(Received_frame, wlength);
			USART2_write(CRC2);				// debugging
			delay_Ms(10);
			if (CRC2 == 0) {

				check = check_input_reg(Received_frame[3]);
				if (check) {

					Int_temp = Internal_Temp_Read();
					delay_Ms(200);

					internal_sensor_high_byte = (Int_temp >> 8)
							| internal_sensor_high_byte;
					internal_sensor_low_byte = Int_temp
							| internal_sensor_low_byte;

					LM35_temp = LM35_Temp_read();
					delay_Ms(200);

					external_sensor_high_byte = (LM35_temp >> 8)
							| external_sensor_high_byte;
					external_sensor_low_byte = LM35_temp
							| external_sensor_low_byte;

					for (int i = 0; i < 4; i++) {
						Select_mux_pin(i);
						cell_vol[i] = Read_Cell_Voltage();
						delay_Ms(200);
						battery_vol += cell_vol[i];

						cell_high_byte = (cell_vol[0] >> 8) | cell_high_byte;
						cell_low_byte = cell_vol[0] | cell_low_byte;

						Response_frame(Int_temp, LM35_temp, cell_vol[0]);
					}
				}
				Mflag = 0;
				USART2->CR1 |= 0x0020;			//enable RX interrupt

				break;

				case 2:
				wrong_slave();
				break;
			}

		}
		return 0;
	}
}

/*******************************************************************************************/
