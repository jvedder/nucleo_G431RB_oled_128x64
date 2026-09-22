/**
  ******************************************************************************
  * @file    oled.h
  * @brief   This file contains all the function prototypes for
  *          the oled.c file
  ******************************************************************************
  * @attention
  * Copyright (c) 2026 John Vedder
  * MIT License
  ******************************************************************************
  */
#ifndef __OLED_H__
#define __OLED_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include "main.h"

/* Private defines */


/* Prototypes */

void OLED_Init(void);
void OLED_SendData(  uint8_t page_addr, uint8_t col_addr, const uint8_t *data, uint8_t length);
void OLED_PutChar1(uint8_t x, uint8_t y, uint8_t glyph);
void OLED_PutChar1Double(uint8_t x, uint8_t y, uint8_t glyph);
void OLED_Clear(void);
void OLED_Fill(void);
void OLED_Fill2x(void);

#ifdef __cplusplus
}
#endif
#endif /*__ OLED_H__ */
