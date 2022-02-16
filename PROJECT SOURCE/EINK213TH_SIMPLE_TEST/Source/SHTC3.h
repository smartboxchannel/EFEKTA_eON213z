/**************************************************************************************************
  SHTC3 sensor lightweight driver
  Written by Andrew Lamchenko, November, 2021.
**************************************************************************************************/

#ifndef SHTC3_H
#define SHTC3_H

#define SENSOR_ADDR 0x70

#define SHTC3_ADDR_READ          (0x70 << 1) | 0x01
#define SHTC3_ADDR_WRITE         (0x70 << 1) | 0x00

#define SHTC3_CMD_WAKEUP                                    0x1735
#define SHTC3_CMD_SLEEP                                     0x98B0
#define SHTC3_CMD_SOFT_RESET                                0x5D80


#define SHTC3_CMD_NORM_READ_TEMP_FIRST                      0x6678
#define SHTC3_CMD_NORM_READ_TEMP_FIRST_LOW_POWER            0x1A40

#define CRC_POLYNOMIAL  0x31

typedef  uint32_t shtc3Error;

enum sht3cError_e {
  NO_ERROR = 0,
  CHECKSUM_ERROR = 1,
};

extern void wakeup_sensor(void);
extern void soft_reset_sensor(void);
extern void sleep_sensor(void);
extern void getTempHumi(float* temp, float* hum);

void SHTC3_WaitUs(uint16 microSecs);
void SHTC3_WaitMs(uint32_t period);

#endif