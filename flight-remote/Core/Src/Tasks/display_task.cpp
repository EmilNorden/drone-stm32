/*
 * display_task.cpp
 *
 *  Created on: Nov 21, 2023
 *      Author: Emil_
 */

#include "display_task.hpp"
#include "ssd1351.hpp"
#include "cmsis_os.h"

void run_display_task(SPI_HandleTypeDef *hspi, void *argument) {
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
	ssd1351_t screen = ssd1351_init(hspi, GPIOA, GPIO_PIN_4, GPIOB,
			GPIO_PIN_2, GPIOB, GPIO_PIN_1, 128, 128);
	for (;;) {
		ssd1351_fillrect(&screen, 0, 0, 64, 64, BLACK);
		osDelay(250);
		ssd1351_fillrect(&screen, 0, 0, 64, 64, RED);
		osDelay(250);
	}
}
