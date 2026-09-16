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
#include "i2c.h"
#include "main.h"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_i2c.h"
#include "oled.h"

#include <stdio.h>



/* Private Defines */

// clang-format off
#define SH1107_MEMORYMODE          0x20
#define SH1107_COLUMNADDR          0x21
#define SH1107_PAGEADDR            0x22
#define SH1107_SETCONTRAST         0x81
#define SH1107_CHARGEPUMP          0x8D
#define SH1107_SEGREMAP            0xA0
#define SH1107_DISPLAYALLON_RESUME 0xA4
#define SH1107_DISPLAYALLON        0xA5
#define SH1107_NORMALDISPLAY       0xA6
#define SH1107_INVERTDISPLAY       0xA7
#define SH1107_SETMULTIPLEX        0xA8
#define SH1107_DCDC                0xAD
#define SH1107_DISPLAYOFF          0xAE
#define SH1107_DISPLAYON           0xAF
#define SH1107_SETPAGEADDR         0xB0     // Specify page address to load display RAM data to page address
#define SH1107_COMSCANINC          0xC0     // Not currently used
#define SH1107_COMSCANDEC          0xC8
#define SH1107_SETDISPLAYOFFSET    0xD3
#define SH1107_SETDISPLAYCLOCKDIV  0xD5
#define SH1107_SETPRECHARGE        0xD9
#define SH1107_SETCOMPINS          0xDA
#define SH1107_SETVCOMDETECT       0xDB
#define SH1107_SETDISPSTARTLINE    0xDC     // Specify Column address to determine the initial display line or COM0
#define SH1107_SETLOWCOLUMN        0x00     // Not currently used
#define SH1107_SETHIGHCOLUMN       0x10     // Not currently used
#define SH1107_SETSTARTLINE        0x40

#define SH1107_I2C_ADDR            0x78     // I2C device address
#define SH1107_I2C_COMMAND         0x00     // Command Control Byte
#define SH1107_I2C_DATA            0x40     // RAM Data Control Byte

#define SH1107_SET_LO_COL_ADDR     0x00     // 4 LSB are low column address bits
#define SH1107_SET_HI_COL_ADDR     0x10     // 3 LSB are high column address bits
#define SH1107_SET_PAGE_ADDR       0xB0     // 4 LSB are the page address Specify page address to load display RAM data to page address

// clang-format on


/* Init sequence, make sure its under 32 bytes, or split into multiples packets */
// clang-format off
/* static const */ 
uint8_t init_cmds[] = 
{
  SH1107_I2C_COMMAND,           // 0x00
  SH1107_DISPLAYOFF,            // 0xAE
  SH1107_SETDISPLAYCLOCKDIV,    // 0xD5, 0x51 <-- Should this be 0x50 (div by 1, POR) instead of 0x51 (div by 2)
  0x51, 
  SH1107_MEMORYMODE,            // 0x20
  SH1107_SETCONTRAST,           // 0x81, 0x4F <-- Should this be 0x80 (POR)
  0x4F,
  SH1107_DCDC,                  // 0xAD, 0x8A  (Switch freq is 1.1 x 500 KHz)
  0x8A, 
  SH1107_SEGREMAP,              // 0xA0 (normal)
  SH1107_COMSCANINC,           // 0xC0 (low to High)
  SH1107_SETDISPSTARTLINE,     // 0xDC, 0x00  (Start at COM0)
  0x00, 
  SH1107_SETDISPLAYOFFSET,     // 0xD3, 0x60 (??? COM start at 0x60 = 96 dec)
  0x60,
  SH1107_SETPRECHARGE,         // 0xD9, 0x22 (POR value)
  0x22,
  SH1107_SETVCOMDETECT,        // 0xDB, 0x35 (POR value)
  0x35,
  SH1107_SETMULTIPLEX,         // 0xA8, 0x3F (POR Value)
  0x3F,
  SH1107_DISPLAYALLON_RESUME,  // 0xA4 
  SH1107_NORMALDISPLAY,        // 0xA6 (Not Reversed)
  SH1107_DISPLAYON             // 0xAF
};
// clang-format on

/** 
 * Send Configuration string to OLED displaty 
 */
void OLED_Init(void)
{

  HAL_StatusTypeDef ret;

  /* send initialization string */
  ret = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)  SH1107_I2C_ADDR, init_cmds, sizeof(init_cmds), 1000);
  while (ret != HAL_OK)
  {
    printf("Error sending OLED init.\r\n");
    Blink_Code(2);
  }

  uint8_t page_addr = 0x00;
  uint8_t col_addr = 0x00;

  for(page_addr = 0; page_addr < 8; page_addr++)
  {
    //printf("********** page: %u *********\r\n", (unsigned int) page_addr);
    for (col_addr = 0; col_addr < 128; col_addr+=8)
    {
      //printf("col: %u\r\n", (unsigned int) col_addr);

      uint8_t addr_cmds[] = 
      {
        SH1107_I2C_COMMAND,
        SH1107_SET_PAGE_ADDR + (page_addr & 0x0F),
        SH1107_SET_HI_COL_ADDR + ((col_addr > 4) & 0x07),
        SH1107_SET_LO_COL_ADDR + (col_addr & 0x0F)
      };
      /* send set page and column address */
      ret =  HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)  SH1107_I2C_ADDR, addr_cmds, sizeof(addr_cmds), 1000);
      while (ret != HAL_OK)
      {
        printf("Error sending page/col address.\r\n");
        Blink_Code(3);
      }

      uint8_t ram_data[] = 
      {
        SH1107_I2C_DATA,
        0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
      };

      /* send RAM image data  */
      ret = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)  SH1107_I2C_ADDR, ram_data, sizeof(ram_data), 1000);
      while (ret != HAL_OK)
      {
        printf("Error sending RAM data.\r\n");
        Blink_Code(4);
      }
    }
  }
}
