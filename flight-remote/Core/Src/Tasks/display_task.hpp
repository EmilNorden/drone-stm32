/*
 * display_task.hpp
 *
 *  Created on: Nov 21, 2023
 *      Author: Emil_
 */

#ifndef SRC_TASKS_DISPLAY_TASK_HPP_
#define SRC_TASKS_DISPLAY_TASK_HPP_

#include "stm32f7xx_hal.h"

#ifdef __cplusplus

extern "C" {

#endif

void run_display_task(SPI_HandleTypeDef *hspi, void *argument);

#ifdef __cplusplus
}
#endif

#endif /* SRC_TASKS_DISPLAY_TASK_HPP_ */
