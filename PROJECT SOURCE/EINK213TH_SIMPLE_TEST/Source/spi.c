#include "spi.h"
#include "Debug.h"
#include "hal_key.h"
#include <stdlib.h>

/**************************************************************************************************
 *                                          CONSTANTS
 **************************************************************************************************/
/*
  LCD pins

  //control
  P0.0 - LCD_MODE (DC EPD)
  P1.1 - LCD_FLASH_RESET (RST EPD)
  P0.7 - LCD_CS (CS EPD)
  P0.4 - LCD_BUSY (BUSY EPD)

  //spi
  P1.5 - CLK
  P1.6 - MOSI
  P1.7 - MISO
*/

/* LCD Control lines */
#ifndef HAL_LCD_MODE_POR
#define HAL_LCD_MODE_PORT 0
#endif
#ifndef HAL_LCD_MODE_PIN
#define HAL_LCD_MODE_PIN  0
#endif

#ifndef HAL_LCD_RESET_PORT
#define HAL_LCD_RESET_PORT 1
#endif
#ifndef HAL_LCD_RESET_PIN
#define HAL_LCD_RESET_PIN  1
#endif

#ifndef HAL_LCD_CS_PORT
#define HAL_LCD_CS_PORT 0
#endif
#ifndef HAL_LCD_CS_PIN
#define HAL_LCD_CS_PIN  7
#endif

#ifndef HAL_LCD_BUSY_PORT
#define HAL_LCD_BUSY_PORT 0
#endif
#ifndef HAL_LCD_BUSY_PIN
#define HAL_LCD_BUSY_PIN  4
#endif

/* LCD SPI lines */
#define HAL_LCD_CLK_PORT 1
#define HAL_LCD_CLK_PIN  5

#define HAL_LCD_MOSI_PORT 1
#define HAL_LCD_MOSI_PIN  6

#define HAL_LCD_MISO_PORT 1
#define HAL_LCD_MISO_PIN  7

/* SPI settings */

#define HAL_SPI_CLOCK_POL_LO       0x00 // CPOL 0 0x00, CPOL 1 0x80
#define HAL_SPI_CLOCK_PHA_0        0x00 // CPHA 0 0x00, CPHA 1 0x40
#define HAL_SPI_TRANSFER_MSB_FIRST 0x20 // ORDER 0 0x00 LSB first, ORDER 1 0x20 MSB first

#define HAL_IO_SET(port, pin, val)        HAL_IO_SET_PREP(port, pin, val)
#define HAL_IO_SET_PREP(port, pin, val)   st( P##port##_##pin = val; )

#define HAL_CONFIG_IO_OUTPUT(port, pin, val)      HAL_CONFIG_IO_OUTPUT_PREP(port, pin, val)
#define HAL_CONFIG_IO_OUTPUT_PREP(port, pin, val) st( P##port##SEL &= ~BV(pin); \
                                                      P##port##_##pin = val; \
                                                      P##port##DIR |= BV(pin); )

#define HAL_CONFIG_IO_INPUT(port, pin, val)      HAL_CONFIG_IO_INPUT_PREP(port, pin, val)
#define HAL_CONFIG_IO_INPUT_PREP(port, pin, val) st( P##port##SEL &= ~BV(pin); \
                                                      P##port##_##pin = val; \
                                                      P##port##DIR &= ~BV(pin); )

#define HAL_CONFIG_IO_PERIPHERAL(port, pin)      HAL_CONFIG_IO_PERIPHERAL_PREP(port, pin)
#define HAL_CONFIG_IO_PERIPHERAL_PREP(port, pin) st( P##port##SEL |= BV(pin); )

#define HAL_CONFIG_IO_GP(port, pin)      HAL_CONFIG_IO_GP_PREP(port, pin)
#define HAL_CONFIG_IO_GP_PREP(port, pin) st( P##port##SEL &= ~BV(pin); )


/* SPI interface control */
#define LCD_SPI_BEGIN()     HAL_IO_SET(HAL_LCD_CS_PORT,   HAL_LCD_CS_PIN,   0); /* chip select */
#define LCD_SPI_END()                                                         \
{                                                                             \
  asm("NOP");                                                                 \
  asm("NOP");                                                                 \
  asm("NOP");                                                                 \
  asm("NOP");                                                                 \
  HAL_IO_SET(HAL_LCD_CS_PORT,  HAL_LCD_CS_PIN,  1); /* chip select */         \
}

/* clear the received and transmit byte status, write tx data to buffer, wait till transmit done */
#define LCD_SPI_TX(x)                   { U1CSR &= ~(BV(2) | BV(1)); U1DBUF = x; while( !(U1CSR & BV(1)) ); }
#define LCD_SPI_WAIT_RXRDY()            { while(!(U1CSR & BV(1))); }


/* Control macros */
#define LCD_DO_WRITE()        HAL_IO_SET(HAL_LCD_MODE_PORT,  HAL_LCD_MODE_PIN,  1);
#define LCD_DO_CONTROL()      HAL_IO_SET(HAL_LCD_MODE_PORT,  HAL_LCD_MODE_PIN,  0);

#define LCD_ACTIVATE_RESET()  HAL_IO_SET(HAL_LCD_RESET_PORT, HAL_LCD_RESET_PIN, 0);
#define LCD_RELEASE_RESET()   HAL_IO_SET(HAL_LCD_RESET_PORT, HAL_LCD_RESET_PIN, 1);

void spi_HW_WaitUs(uint16 i);

void SPIInit(void);

void HalLcd_HW_Init(void);
void HalLcd_HW_Control(uint8 cmd);
void HalLcd_HW_Write(uint8 data);


void SPIInit(void) {
  
  /* Initialize LCD IO lines */
  spi_ConfigIO();

  /* Initialize SPI */
  spi_ConfigSPI();
}


void HalLcd_HW_Init(void){
  /* Perform reset */
  LCD_ACTIVATE_RESET();
  spi_HW_WaitUs(15000); // 15 ms
  LCD_RELEASE_RESET();
  spi_HW_WaitUs(15000); // 15 us
}


void spi_ConfigIO(void)
{
  /* GPIO configuration */
  HAL_CONFIG_IO_OUTPUT(HAL_LCD_MODE_PORT,  HAL_LCD_MODE_PIN,  1);
  HAL_CONFIG_IO_OUTPUT(HAL_LCD_RESET_PORT, HAL_LCD_RESET_PIN, 1);
  HAL_CONFIG_IO_OUTPUT(HAL_LCD_CS_PORT,    HAL_LCD_CS_PIN,    1);
  HAL_CONFIG_IO_INPUT (HAL_LCD_BUSY_PORT,  HAL_LCD_BUSY_PIN,  0);
//  P2INP |= HAL_KEY_BIT5; // pull down
//  P2INP &= ~HAL_KEY_BIT5; // pull up
}


void spi_ConfigSPI(void)
{
  /* UART/SPI Peripheral configuration */

   uint8 baud_exponent;
   uint8 baud_mantissa;

  /* Set SPI on UART 1 alternative 2 */
  PERCFG |= 0x02;

  /* Configure clk, master out and master in lines */
  HAL_CONFIG_IO_PERIPHERAL(HAL_LCD_CLK_PORT,  HAL_LCD_CLK_PIN);
  HAL_CONFIG_IO_PERIPHERAL(HAL_LCD_MOSI_PORT, HAL_LCD_MOSI_PIN);
  HAL_CONFIG_IO_PERIPHERAL(HAL_LCD_MISO_PORT, HAL_LCD_MISO_PIN);


  /* Set SPI speed to 1 MHz (the values assume system clk of 32MHz)
   * Confirm on board that this results in 1MHz spi clk.
   */
  baud_exponent = 15;
  baud_mantissa =  0;

  /* Configure SPI */
  U1UCR  = 0x00;      /* Flush and goto IDLE state. 8-N-1. */
  U1CSR  = 0x00;      /* SPI mode, master. */
  U1GCR  = HAL_SPI_TRANSFER_MSB_FIRST | HAL_SPI_CLOCK_PHA_0 | HAL_SPI_CLOCK_POL_LO | baud_exponent;
  U1BAUD = baud_mantissa;
}


void spi_ConfigGP(void)
{
  /* Set SPI on UART 1 alternative 2 */
  PERCFG &= ~0x02;

  /* Configure clk, master out and master in lines */
  HAL_CONFIG_IO_GP(HAL_LCD_CLK_PORT,  HAL_LCD_CLK_PIN);
  HAL_CONFIG_IO_GP(HAL_LCD_MOSI_PORT, HAL_LCD_MOSI_PIN);
  HAL_CONFIG_IO_GP(HAL_LCD_MISO_PORT, HAL_LCD_MISO_PIN);
}


void spi_HW_WaitUs(uint16 microSecs)
{
  while(microSecs--)
  {
    /* 32 NOPs == 1 usecs */
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    asm("nop"); asm("nop");
  }
}


/**************************************************************************************************
 * @fn      HalLcd_HW_Control
 *
 * @brief   Write 1 command to the LCD
 *
 * @param   uint8 cmd - command to be written to the LCD
 *
 * @return  None
 **************************************************************************************************/
void HalLcd_HW_Control(uint8 cmd)
{
  LCD_SPI_BEGIN();
  LCD_DO_CONTROL();
  LCD_SPI_TX(cmd);
  LCD_SPI_WAIT_RXRDY();
  LCD_SPI_END();
}

/**************************************************************************************************
 * @fn      HalLcd_HW_Write
 *
 * @brief   Write 1 byte to the LCD
 *
 * @param   uint8 data - data to be written to the LCD
 *
 * @return  None
 **************************************************************************************************/
void HalLcd_HW_Write(uint8 data)
{
  LCD_SPI_BEGIN();
  LCD_DO_WRITE();
  LCD_SPI_TX(data);
  LCD_SPI_WAIT_RXRDY();
  LCD_SPI_END();
}