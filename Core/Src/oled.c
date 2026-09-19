/**
  ******************************************************************************
  * @file    oled.c
  * @brief   This file provides code for the configuration
  *          of the Adafruit 4650 OLED display with SH1107 chip.
  ******************************************************************************
  * @attention
  * Copyright (c) 2026 John Vedder
  * MIT License
  ******************************************************************************
  */

/* Includes */
#include "font.h"
#include "i2c.h"
#include "main.h"
#include "stm32g4xx_hal.h"
#include "oled.h"

#include <stdint.h>
#include <stdio.h>


/* Private Defines */

// clang-format off
#define SH1107_SET_LO_COL_ADDR     0x00    // 4 LSB are low column address bits
#define SH1107_SET_HI_COL_ADDR     0x10    // 3 LSB are high column address bits
#define SH1107_ADDR_MODE_PAGE      0x20
#define SH1107_ADDR_MODE_VERT      0x21
#define SH1107_SET_CONTRAST        0x81
#define SH1107_SEG_REMAP_DOWN      0xA0    // ADC = “L”
#define SH1107_SEG_REMAP_UP        0xA1    // ADC = "H"
#define SH1107_SET_ALL_NORMAL      0xA4
#define SH1107_SET_ALL_ON          0xA5
#define SH1107_REVERSE_OFF         0xA6
#define SH1107_REVERSE_ON          0xA7
#define SH1107_SET_MULTIPLEX       0xA8
#define SH1107_DCDC                0xAD
#define SH1107_DISPLAY_OFF         0xAE
#define SH1107_DISPLAY_ON          0xAF
#define SH1107_SET_PAGE_ADDR       0xB0    // 4 LSB are the page address bits
#define SH1107_SCAN_DIR_LO_HI      0xC0     
#define SH1107_SCAN_DIR_HI_LO      0xC8
#define SH1107_SET_DISPLAY_OFFSET  0xD3
#define SH1107_SET_CLOCK_FREQ      0xD5
#define SH1107_SET_CHARGE_PERIOD   0xD9
#define SH1107_SET_VCOM_DESELECT   0xDB
#define SH1107_SET_DISP_START_LINE 0xDC    // Specify Column address of first line (COM0)

#define SH1107_I2C_ADDR            0x78    // I2C device address
#define SH1107_I2C_COMMAND         0x00    // I2C Control Byte = Command
#define SH1107_I2C_DATA            0x40    // I2C Control Byte = RAM Data

// clang-format on

/* maps 4 bits to 8 bits by doubling each bit */
static const uint8_t double_bit[] = 
{
  0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F, 
  0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF
};

/* Init sequence */
// clang-format off
static const uint8_t init_cmds[] = 
{
  SH1107_I2C_COMMAND,           // 0x00 (remainder are commands, not RAM Data)
  SH1107_DISPLAY_OFF,           // 0xAE
  SH1107_SET_CLOCK_FREQ,        // 0xD5, 0x51 <-- Should this be 0x50 (div by 1, POR) instead of 0x51 (div by 2)?
  0x51, 
  SH1107_ADDR_MODE_PAGE,        // 0x20 (D=0, POR)
  //  SH1107_ADDR_MODE_VERT,        // 0x21 (D=1, POR)
  SH1107_SET_CONTRAST,          // 0x81, 0x4F <-- Should this be 0x80 (POR)?
  0x4F,
  SH1107_DCDC,                  // 0xAD, 0x8A  (Switch freq is 1.1 x 500 KHz)
  0x8A, 
  SH1107_SEG_REMAP_DOWN,        // 0xA0 (ADC=0, POR)
  //  SH1107_SEG_REMAP_UP,          // 0xA1 (ADC=1)
  SH1107_SCAN_DIR_LO_HI,       // 0xC0
  SH1107_SET_DISP_START_LINE,  // 0xDC, 0x00  (Start at COM0)
  0x00, 
  SH1107_SET_DISPLAY_OFFSET,   // 0xD3, 0x60 (??? COM start at 0x60 = 96 dec)
  0x60,
  SH1107_SET_CHARGE_PERIOD,    // 0xD9, 0x22 (POR value)
  0x22,
  SH1107_SET_VCOM_DESELECT,    // 0xDB, 0x35 (POR value)
  0x35,
  SH1107_SET_MULTIPLEX,        // 0xA8, 0x3F (POR Value)
  0x3F,
  SH1107_SET_ALL_NORMAL,       // 0xA4 
  SH1107_REVERSE_OFF,          // 0xA6 (Not Reversed)
  SH1107_DISPLAY_ON            // 0xAF
};
// clang-format on

/* Make the buffer long enough for one full column plus a control byte. */
/* Round up to 4 bytes to keep it 32-bit aligned FWIW. */
#define OLED_COLUMN_LENGTH 64
#define OLED_I2C_BUFFER_LENGTH  (OLED_COLUMN_LENGTH+4)
static uint8_t i2c_buffer[OLED_I2C_BUFFER_LENGTH]; 


/** 
 * Send Configuration string to OLED displaty 
 */
void OLED_Init(void)
{
  printf("OLED_Init()\r\n");

  HAL_StatusTypeDef ret;

  /* send initialization string */
  ret = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)  SH1107_I2C_ADDR, (uint8_t *) init_cmds, sizeof(init_cmds), 1000);
  if (ret != HAL_OK)
  {
    printf("Error sending OLED init.\r\n");
    Blink_Code(2);
  }
  return;
}

void OLED_SendData(uint8_t page_addr, uint8_t col_addr, const uint8_t *data, uint8_t length)
{
  HAL_StatusTypeDef ret;

  /* limit data to length of one column */
  if (length > OLED_COLUMN_LENGTH) length = OLED_COLUMN_LENGTH;

  //printf("pg: %u, col: %u len:%u\r\n", page_addr, col_addr, length);

  i2c_buffer[0] = SH1107_I2C_COMMAND;
  i2c_buffer[1] = SH1107_SET_PAGE_ADDR + (page_addr & 0x0F);
  i2c_buffer[2] = SH1107_SET_HI_COL_ADDR + ((col_addr >> 4) & 0x07);
  i2c_buffer[3] = SH1107_SET_LO_COL_ADDR + (col_addr & 0x0F);
  
  /* send set page and column address */
  ret =  HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)  SH1107_I2C_ADDR, i2c_buffer, 4, 1000);
  if (ret != HAL_OK)
  {
    printf("Error sending page/col address.\r\n");
    Blink_Code(3);
  }

  /* put DATA control byte in front of user data */
  i2c_buffer[0] = SH1107_I2C_DATA;
  for (uint8_t i= 0; i<length; i++)
  {
    i2c_buffer[i+1] = data[i];  
  }

  /* send RAM image data  */
  ret = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)  SH1107_I2C_ADDR, i2c_buffer, length+1, 1000);
  if (ret != HAL_OK)
  {
    printf("Error sending RAM data.\r\n");
    Blink_Code(4);
  }
  return;
}
void OLED_PutChar(uint8_t x, uint8_t y, uint8_t glyph)
{
  // printf("(%u, %u) = %02X\r\n", x, y, glyph);

  /* limit to screen size */
  uint8_t page_addr = 15 - (x & 0x0F);
  uint8_t col_addr = (8 * (y & 0x07));

  /* index into font data */
  const uint8_t *data = &font[glyph * 8];

  OLED_SendData(page_addr, col_addr, data, 8);

  return;
}

void OLED_Clear(void)
{
  for(uint8_t y = 0; y < 8; y++)
  {
    for (uint8_t x = 0; x < 16; x++)  
    {
      OLED_PutChar(x, y , ' ');
    }
  }
  return;
}

void OLED_Fill(void)
{
  uint8_t glyph = '0';

  for(uint8_t y = 0; y < 8; y++)
  {
    for (uint8_t x = 0; x < 16; x++)  
    {
      OLED_PutChar(x, y, glyph);
      glyph++;
      if (glyph > 0x7E) glyph = 0x21;
    }
  }
  return;
}
