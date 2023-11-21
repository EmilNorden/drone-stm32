/*
 * ssd1351.hpp
 *
 *  Created on: Nov 21, 2023
 *      Author: Emil_
 */

#ifndef DRIVERS_SSD1351_SSD1351_HPP_
#define DRIVERS_SSD1351_SSD1351_HPP_


#include "stm32f7xx_hal.h"

#ifdef __cplusplus

extern "C" {

#endif

typedef struct ssd1351 {
	SPI_HandleTypeDef *hspi;
	GPIO_TypeDef *CS_port;
	GPIO_TypeDef *DC_port;
	GPIO_TypeDef *RES_port;
	uint16_t CS_pin;
	uint16_t DC_pin;
	uint16_t RES_pin;
	uint16_t width;
	uint16_t height;
} ssd1351_t;

ssd1351_t ssd1351_init(
		SPI_HandleTypeDef *hspi,
		GPIO_TypeDef *CS_port,
		uint16_t CS_pin,
		GPIO_TypeDef *DC_port,
		uint16_t DC_pin,
		GPIO_TypeDef *RES_port,
		uint16_t RES_pin,
		uint16_t width,
		uint16_t height);


void ssd1351_fillrect(
		ssd1351_t *screen,
		int16_t x, int16_t y,
		int16_t width, int16_t height,
		uint16_t color);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_SSD1351_SSD1351_HPP_ */
