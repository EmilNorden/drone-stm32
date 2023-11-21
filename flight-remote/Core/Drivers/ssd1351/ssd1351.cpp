/*
 * ssd1351.cpp
 *
 *  Created on: Nov 21, 2023
 *      Author: Emil_
 */

#include "ssd1351.hpp"
#include "cmsis_os.h"

#define SSD1351_CMD_SETCOLUMN 0x15      ///< See datasheet
#define SSD1351_CMD_SETROW 0x75         ///< See datasheet
#define SSD1351_CMD_WRITERAM 0x5C       ///< See datasheet
#define SSD1351_CMD_READRAM 0x5D        ///< Not currently used
#define SSD1351_CMD_SETREMAP 0xA0       ///< See datasheet
#define SSD1351_CMD_STARTLINE 0xA1      ///< See datasheet
#define SSD1351_CMD_DISPLAYOFFSET 0xA2  ///< See datasheet
#define SSD1351_CMD_DISPLAYALLOFF 0xA4  ///< Not currently used
#define SSD1351_CMD_DISPLAYALLON 0xA5   ///< Not currently used
#define SSD1351_CMD_NORMALDISPLAY 0xA6  ///< See datasheet
#define SSD1351_CMD_INVERTDISPLAY 0xA7  ///< See datasheet
#define SSD1351_CMD_FUNCTIONSELECT 0xAB ///< See datasheet
#define SSD1351_CMD_DISPLAYOFF 0xAE     ///< See datasheet
#define SSD1351_CMD_DISPLAYON 0xAF      ///< See datasheet
#define SSD1351_CMD_PRECHARGE 0xB1      ///< See datasheet
#define SSD1351_CMD_DISPLAYENHANCE 0xB2 ///< Not currently used
#define SSD1351_CMD_CLOCKDIV 0xB3       ///< See datasheet
#define SSD1351_CMD_SETVSL 0xB4         ///< See datasheet
#define SSD1351_CMD_SETGPIO 0xB5        ///< See datasheet
#define SSD1351_CMD_PRECHARGE2 0xB6     ///< See datasheet
#define SSD1351_CMD_SETGRAY 0xB8        ///< Not currently used
#define SSD1351_CMD_USELUT 0xB9         ///< Not currently used
#define SSD1351_CMD_PRECHARGELEVEL 0xBB ///< Not currently used
#define SSD1351_CMD_VCOMH 0xBE          ///< See datasheet
#define SSD1351_CMD_CONTRASTABC 0xC1    ///< See datasheet
#define SSD1351_CMD_CONTRASTMASTER 0xC7 ///< See datasheet
#define SSD1351_CMD_MUXRATIO 0xCA       ///< See datasheet
#define SSD1351_CMD_COMMANDLOCK 0xFD    ///< See datasheet
#define SSD1351_CMD_HORIZSCROLL 0x96    ///< Not currently used
#define SSD1351_CMD_STOPSCROLL 0x9E     ///< Not currently used
#define SSD1351_CMD_STARTSCROLL 0x9F    ///< Not currently used

static uint8_t init_commands[] = {
SSD1351_CMD_COMMANDLOCK, 1, // Set command lock, 1 arg
		0x12,
		SSD1351_CMD_COMMANDLOCK, 1, // Set command lock, 1 arg
		0xB1,
		SSD1351_CMD_DISPLAYOFF, 0, // Display off, no args
		SSD1351_CMD_CLOCKDIV, 1, 0xF1, // 7:4 = Oscillator Freq, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
		SSD1351_CMD_MUXRATIO, 1, 127,
		SSD1351_CMD_DISPLAYOFFSET, 1, 0x0,
		SSD1351_CMD_SETGPIO, 1, 0x00,
		SSD1351_CMD_FUNCTIONSELECT, 1, 0x01, // internal (diode drop)
		SSD1351_CMD_PRECHARGE, 1, 0x32,
		SSD1351_CMD_VCOMH, 1, 0x05,
		SSD1351_CMD_NORMALDISPLAY, 0,
		SSD1351_CMD_CONTRASTABC, 3, 0xC8, 0x80, 0xC8,
		SSD1351_CMD_CONTRASTMASTER, 1, 0x0F,
		SSD1351_CMD_SETVSL, 3, 0xA0, 0xB5, 0x55,
		SSD1351_CMD_PRECHARGE2, 1, 0x01,
		SSD1351_CMD_DISPLAYON, 0,  // Main screen turn on
		0 }; // END OF COMMAND LIST

static void ssd1351_select(ssd1351_t *screen) {
	HAL_GPIO_WritePin(screen->CS_port, screen->CS_pin, GPIO_PIN_RESET);
}

static void ssd1351_unselect(ssd1351_t *screen) {
	HAL_GPIO_WritePin(screen->CS_port, screen->CS_pin, GPIO_PIN_SET);
}

static void ssd1351_set_command_mode(ssd1351_t *screen) {
	// Pull DC Low => Command mode
	HAL_GPIO_WritePin(screen->DC_port, screen->DC_pin, GPIO_PIN_RESET);
}

static void ssd1351_set_data_mode(ssd1351_t *screen) {
	// Pull DC High => Data mode
	HAL_GPIO_WritePin(screen->DC_port, screen->DC_pin, GPIO_PIN_SET);
}

static void ssd1351_reset(ssd1351_t *screen) {
	HAL_GPIO_WritePin(screen->RES_port, screen->RES_pin, GPIO_PIN_SET);
	osDelay(100);
	HAL_GPIO_WritePin(screen->RES_port, screen->RES_pin, GPIO_PIN_RESET);
	osDelay(100);
	HAL_GPIO_WritePin(screen->RES_port, screen->RES_pin, GPIO_PIN_SET);
	osDelay(200);
}

/*
 * Assumes CS is already LOW
 */
static void ssd1351_send_data(ssd1351_t *screen, uint8_t *data, size_t size) {
	ssd1351_set_data_mode(screen);
	HAL_SPI_Transmit(screen->hspi, data, size, HAL_MAX_DELAY);
}

/*
 * Assumes CS is already LOW
 */
static void ssd1351_send_command(ssd1351_t *screen, uint8_t cmd, uint8_t *args,
		uint8_t num_args) {

	ssd1351_set_command_mode(screen);
	HAL_SPI_Transmit(screen->hspi, &cmd, 1, HAL_MAX_DELAY);

	//for (int i = 0; i < num_args; ++i) {
	if(num_args > 0) {
		ssd1351_send_data(screen, args, num_args);
	}

	//}

	ssd1351_set_data_mode(screen);
}

static void ssd1351_set_rotation(ssd1351_t *screen, uint8_t rotation) {
	// madctl bits:
	// 6,7 Color depth (01 = 64K)
	// 5   Odd/even split COM (0: disable, 1: enable)
	// 4   Scan direction (0: top-down, 1: bottom-up)
	// 3   Reserved
	// 2   Color remap (0: A->B->C, 1: C->B->A)
	// 1   Column remap (0: 0-127, 1: 127-0)
	// 0   Address increment (0: horizontal, 1: vertical)
	uint8_t madctl = 0b01100100; // 64K, enable split, CBA

	rotation = rotation & 3; // Clip input to valid range

	switch (rotation) {
	case 0:
		madctl |= 0b00010000; // Scan bottom-up
		//_width = screen->width;
		//_height = screen->height;
		break;
	case 1:
		madctl |= 0b00010011; // Scan bottom-up, column remap 127-0, vertical
		//_width = screen->height;
		//_height = screen->width;
		break;
	case 2:
		madctl |= 0b00000010; // Column remap 127-0
		//_width = screen->width;
		//_height = screen->height;
		break;
	case 3:
		madctl |= 0b00000001; // Vertical
		//_width = screen->height;
		//_height = screen->width;
		break;
	}

	ssd1351_send_command(screen, SSD1351_CMD_SETREMAP, &madctl, 1);
	uint8_t startline = (rotation < 2) ? screen->height : 0;
	ssd1351_send_command(screen, SSD1351_CMD_STARTLINE, &startline, 1);
}

static void ssd1351_set_addr_window(ssd1351_t *screen, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
	uint8_t column_data[] = {x, x + width - 1};
	uint8_t row_data[] = {y, y + height - 1};

	// TODO Handle rotation
	ssd1351_send_command(screen, SSD1351_CMD_SETCOLUMN, column_data, 2);
	ssd1351_send_command(screen, SSD1351_CMD_SETROW, row_data, 2);
	ssd1351_send_command(screen, SSD1351_CMD_WRITERAM, NULL, 0);
}

static void ssd135_write_color(ssd1351_t *screen, uint16_t color, size_t len) {
	if(!len) {
		return;
	}

	uint8_t hi = color >> 8;
	uint8_t lo = color;
}


ssd1351_t ssd1351_init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *CS_port,
		uint16_t CS_pin, GPIO_TypeDef *DC_port, uint16_t DC_pin,
		GPIO_TypeDef *RES_port, uint16_t RES_pin,		uint16_t width,
		uint16_t height) {
	ssd1351_t ssd;
	ssd.hspi = hspi;
	ssd.CS_port = CS_port;
	ssd.CS_pin = CS_pin;
	ssd.DC_port = DC_port;
	ssd.DC_pin = DC_pin;
	ssd.RES_port = RES_port;
	ssd.RES_pin = RES_pin;
	ssd.width = width;
	ssd.height = height;

	ssd1351_unselect(&ssd);
	ssd1351_set_data_mode(&ssd);
	ssd1351_reset(&ssd);


	uint8_t *commands = init_commands;
	uint8_t cmd;

	ssd1351_select(&ssd);
	while ((cmd = *commands++) > 0) {
		uint8_t num_args = (*commands++) & 0x7F;
		ssd1351_send_command(&ssd, cmd, commands, num_args);
		commands += num_args;
	}

	ssd1351_set_rotation(&ssd, 0);

	ssd1351_unselect(&ssd);
	return ssd;
}

static uint16_t transfer_buffer[128*128];
void ssd1351_fillrect(
		ssd1351_t *screen,
		int16_t x, int16_t y,
		int16_t width, int16_t height,
		uint16_t color) {
	if(width == 0 || height == 0) {
		return;
	}

	// TODO: Handle input sanitation here. Clip rectangle. Handle negative width/height etc.
	ssd1351_select(screen);
	ssd1351_set_addr_window(screen, x, y, width, height);
	for(int i = 0; i < width * height; ++i) {
		transfer_buffer[i] = (color >> 8) | ((color << 8) & 0xFF00);
	}
	//memset((void*)pixel_buffer, color, width*height);
	//for(int i = 0; i < width * height; ++i) {
		/*uint8_t hi = color >> 8;
		uint8_t lo = color;
		HAL_SPI_Transmit(screen->hspi, &hi, 1, HAL_MAX_DELAY);
		HAL_SPI_Transmit(screen->hspi, &lo, 1, HAL_MAX_DELAY);*/
		HAL_SPI_Transmit(screen->hspi, (uint8_t*)&transfer_buffer, (width*height)*2, HAL_MAX_DELAY);
	//}
	ssd1351_unselect(screen);
}




