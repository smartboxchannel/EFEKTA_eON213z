/**************************************************************************************************
  SHTC3 sensor lightweight driver
  Written by Andrew Lamchenko, November, 2021.
**************************************************************************************************/
#include "SHTC3.h"
#include "Debug.h"
#include <stdlib.h>
#include "hal_i2c.h"

static shtc3Error SHTC3_CheckCrc(uint8_t data[], uint8_t nbrOfBytes,
                              uint8_t checksum);

void wakeup_sensor(void)
{
	uint16_t command = SHTC3_CMD_WAKEUP;
	HalI2CSend(SHTC3_ADDR_WRITE, (uint8_t*)&command, 2);
        SHTC3_WaitMs(1);
}



void soft_reset_sensor(void)
{
	uint16_t command = SHTC3_CMD_SOFT_RESET;
	HalI2CSend(SHTC3_ADDR_WRITE, (uint8_t*)&command, 2);
}



void sleep_sensor(void)
{
	uint16_t command = SHTC3_CMD_SLEEP;
	HalI2CSend(SHTC3_ADDR_WRITE, (uint8_t*)&command, 2);
}



void getTempHumi(float* temp, float* hum) {

        shtc3Error error; 
        uint8_t  maxPolling = 22;
        uint16_t command = SHTC3_CMD_NORM_READ_TEMP_FIRST;
	HalI2CSend(SHTC3_ADDR_WRITE, (uint8_t*)&command, 2);
        
	uint8 getbuf[6];
        uint8 checksum;

        while(maxPolling--) {
	HalI2CReceive(SHTC3_ADDR_READ, getbuf, 6);
        
        checksum = getbuf[2];
         error = SHTC3_CheckCrc(&getbuf[3], 2, checksum);
        
        checksum = getbuf[5]; 
        error = SHTC3_CheckCrc(&getbuf[3], 2, checksum);
        
        if(error == NO_ERROR) break;

        SHTC3_WaitMs(1);
    }
    if(error == NO_ERROR){
      uint16_t raw_temp = getbuf[0] << 8 | getbuf[1];
        *temp = 175 * (float)raw_temp / 65536.0f - 45.0f;
      
      uint16_t raw_hum = getbuf[3] << 8 | getbuf[4];
        *hum = 100 * (float)raw_hum / 65536.0f;
    }else{
        *temp = -100.0;
        *hum = 0.0;
    }
}


void SHTC3_WaitUs(uint16 microSecs) {
  while(microSecs--) {
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


void SHTC3_WaitMs(uint32_t period) { SHTC3_WaitUs(period * 1000); }


static shtc3Error SHTC3_CheckCrc(uint8_t data[], uint8_t numOfBytes,
                              uint8_t checksum){
  uint8_t crc = 0xFF;

  for(uint8_t byteCtr = 0; byteCtr < numOfBytes; byteCtr++) {
    crc ^= (data[byteCtr]);
    for(uint8_t bit = 8; bit > 0; --bit) {
      if(crc & 0x80) {
        crc = (crc << 1) ^ CRC_POLYNOMIAL;
      } else {
        crc = (crc << 1);
      }
    }
  }
  if(crc != checksum) {
    return CHECKSUM_ERROR;
  } else {
    return NO_ERROR;
  }
}