#ifndef SPI_H
#define SPI_H

void spi_ConfigIO(void);
void spi_ConfigSPI(void);
void spi_ConfigGP(void);
void spi_HW_WaitUs(uint16 i);

extern void SPIInit(void);
extern void HalLcd_HW_Init(void);
extern void HalLcd_HW_Control(uint8 cmd);
extern void HalLcd_HW_Write(uint8 data);

#endif