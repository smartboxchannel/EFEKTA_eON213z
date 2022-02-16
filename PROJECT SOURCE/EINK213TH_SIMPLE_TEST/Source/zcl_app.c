
#include "AF.h"
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "OSAL_PwrMgr.h"
#include "ZComDef.h"
#include "ZDApp.h"
#include "ZDNwkMgr.h"
#include "ZDObject.h"
#include "math.h"

#include "nwk_util.h"
#include "zcl.h"
#include "zcl_app.h"
#include "zcl_diagnostic.h"
#include "zcl_general.h"
#include "zcl_ms.h"

#include "bdb.h"
#include "bdb_interface.h"
#include "gp_interface.h"

#include "Debug.h"

#include "OnBoard.h"

#include <stdio.h>
#include <stdlib.h>

/* HAL */
#include "inttempsens.h"
#include "hal_adc.h"
#include "hal_drivers.h"
#include "hal_key.h"
#include "hal_led.h"

#include "spi.h"
#include "hal_i2c.h"

#include "battery.h"
#include "commissioning.h"
#include "factory_reset.h"
#include "utils.h"
#include "version.h"

#include "ssd1675.h"
#include "SHTC3.h"


#include "imagedata.h"
#include "epdpaint.h"

/*********************************************************************
 * MACROS
 */
#define HAL_KEY_CODE_RELEASE_KEY HAL_KEY_CODE_NOKEY

#define myround(x) ((int)((x)+0.5))

/*********************************************************************
 * GLOBAL VARIABLES
 */

extern bool requestNewTrustCenterLinkKey;
byte zclApp_TaskID;

/*********************************************************************
 * LOCAL VARIABLES
 */
bool firstLoop = true;
bool changeData = false;
static uint8 currentSensorsReadingPhase = 0;
int16 colorPrint = 0x00;  // 00 - black on white, ff -  white on black, ..still not implemented, no time :(
int16 temp_old = 0;
int16 tempTr = 33;
uint16 hum_old = 0;
uint16 humTr = 250;
int16 startWork = 0;
int16 sendBattCount = 0;
bool pushBut = false;
bool epdStart = false;
bool first = false;
bool upTemp;
bool downTemp;
uint16 last_tmp = 0;;
bool upHum;
bool downHum;
uint16 last_hum = 0;;
uint8 loopCount = 5;

uint16 einkTemp;
uint16 einkHum;

void graphData(uint16 temp, uint16 hum);
void epdGraphData(void);
int massDataT[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint16 numDataT;
int numMassDataT = 0;
uint32 amountDataT;

int massDataH[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint16 numDataH;
int numMassDataH = 0;
uint32 amountDataH;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclApp_HandleKeys(byte shift, byte keys);
static void zclApp_Report(void);

static void zclApp_ReadSensors(void);
static void zclApp_TempHumiSens(void);

void conveyor(int *Arr, int n, int l);

void EpdRefresh(void);
void EpdStart(void);
void epdTemperatureData(uint16 tt);
void epdHumidityData(uint16 hh);
void epdBatteryData(uint8 b);
void epdZigbeeStatusData(void);


/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclApp_CmdCallbacks = {
    NULL, // Basic Cluster Reset command
    NULL, // Identify Trigger Effect command
    NULL, // On/Off cluster commands
    NULL, // On/Off cluster enhanced command Off with Effect
    NULL, // On/Off cluster enhanced command On with Recall Global Scene
    NULL, // On/Off cluster enhanced command On with Timed Off
    NULL, // RSSI Location command
    NULL  // RSSI Location Response command
};

void zclApp_Init(byte task_id) {
    HalI2CInit();
    SPIInit();

    EpdStart(); 

    // this is important to allow connects throught routers
    // to make this work, coordinator should be compiled with this flag #define TP2_LEGACY_ZC
    requestNewTrustCenterLinkKey = FALSE;

    zclApp_TaskID = task_id;

    zclGeneral_RegisterCmdCallbacks(1, &zclApp_CmdCallbacks);
    zcl_registerAttrList(zclApp_FirstEP.EndPoint, zclApp_AttrsFirstEPCount, zclApp_AttrsFirstEP);
    bdb_RegisterSimpleDescriptor(&zclApp_FirstEP);

    zcl_registerForMsg(zclApp_TaskID);

    // Register for all key events - This app will handle all key events
    RegisterForKeys(zclApp_TaskID);
    
    LREP("Started build %s \r\n", zclApp_DateCodeNT);
    HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
    zclApp_ReadSensors();
    
    osal_start_reload_timer(zclApp_TaskID, APP_REPORT_EVT, APP_REPORT_DELAY);
}

uint16 zclApp_event_loop(uint8 task_id, uint16 events) {
    afIncomingMSGPacket_t *MSGpkt;

    (void)task_id; // Intentionally unreferenced parameter
    if (events & SYS_EVENT_MSG) {
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclApp_TaskID))) {
            switch (MSGpkt->hdr.event) {
            case KEY_CHANGE:
                zclApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
                break;
            case ZCL_INCOMING_MSG:
                if (((zclIncomingMsg_t *)MSGpkt)->attrCmd) {
                    osal_mem_free(((zclIncomingMsg_t *)MSGpkt)->attrCmd);
                }
                break;

            default:
                break;
            }
            // Release the memory
            osal_msg_deallocate((uint8 *)MSGpkt);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & APP_REPORT_EVT) {
        LREPMaster("APP_REPORT_EVT\r\n");
        zclApp_Report();
        return (events ^ APP_REPORT_EVT);
    }

    if (events & APP_READ_SENSORS_EVT) {
        LREPMaster("APP_READ_SENSORS_EVT\r\n");
        zclApp_ReadSensors();
        if(epdStart){
        EpdRefresh();
        epdStart = false;
        }
        return (events ^ APP_READ_SENSORS_EVT);
    }

    // Discard unknown events
    return 0;
}

static void zclApp_HandleKeys(byte portAndAction, byte keyCode) {
    LREP("zclApp_HandleKeys portAndAction=0x%X keyCode=0x%X\r\n", portAndAction, keyCode);
    
    zclFactoryResetter_HandleKeys(portAndAction, keyCode);
    zclCommissioning_HandleKeys(portAndAction, keyCode);
    
    if (portAndAction & HAL_KEY_RELEASE) {
        LREPMaster("Key press\r\n");
        
        if (bdbAttributes.bdbNodeIsOnANetwork){
        osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 100);
        }else{
          osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 1000);
        }
        pushBut = true;
    }
    
}


static void zclApp_ReadSensors(void) {
    LREP("currentSensorsReadingPhase %d\r\n", currentSensorsReadingPhase);
    /**
     * FYI: split reading sensors into phases, so single call wouldn't block processor
     * for extensive ammount of time
     * */
    switch (currentSensorsReadingPhase++) {  
    case 0:
      if(startWork <= 2){
        startWork++;
        zclBattery_Report();
        pushBut = true;
      }
      
      if(startWork == 3){
      sendBattCount++;
      if(sendBattCount == 720){
        zclBattery_Report();
        sendBattCount = 0;
        pushBut = true;
      }else{
        if(pushBut){
          zclBattery_Report();
          sendBattCount = 0;
        }
      }
      }
      break;    
       
    case 1:
        zclApp_TempHumiSens();
        if(pushBut == true){
        pushBut = false;
        }
        break;

    default:
        currentSensorsReadingPhase = 0;
        epdStart = true;
        break;
    }
    if (currentSensorsReadingPhase != 0) {
        osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 20);
    }
}





static void zclApp_TempHumiSens(void) {
  
  float t = 0.0;
  float h = 0.0;
  
  wakeup_sensor();
  getTempHumi(&t, &h);
  sleep_sensor();
  zclApp_Temperature_Sensor_MeasuredValue = (int16)(t*100);
  zclApp_HumiditySensor_MeasuredValue = (uint16)(h*100);
  
  einkTemp = myround(t*10.0);
  einkHum = (uint16)myround(h);
  
  graphData(einkTemp, einkHum);
  
    if(!pushBut){
    if (abs(zclApp_Temperature_Sensor_MeasuredValue - temp_old) >= tempTr) {
        temp_old = zclApp_Temperature_Sensor_MeasuredValue;
        
        LREP("ReadIntTempSens t=%d\r\n", zclApp_Temperature_Sensor_MeasuredValue);
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, TEMP, ATTRID_MS_TEMPERATURE_MEASURED_VALUE);
        changeData = true;
    }
    if (abs(zclApp_HumiditySensor_MeasuredValue - hum_old) >= humTr) {
        hum_old = zclApp_HumiditySensor_MeasuredValue;
        
        LREP("ReadIntTempSens t=%d\r\n", zclApp_HumiditySensor_MeasuredValue);
        bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE);
        changeData = true;
    }
    }else{
      temp_old = zclApp_Temperature_Sensor_MeasuredValue;
      
      LREP("ReadIntTempSens t=%d\r\n", zclApp_Temperature_Sensor_MeasuredValue);
      bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, TEMP, ATTRID_MS_TEMPERATURE_MEASURED_VALUE);
      
      hum_old = zclApp_HumiditySensor_MeasuredValue;
      
      LREP("ReadIntTempSens t=%d\r\n", zclApp_HumiditySensor_MeasuredValue);
      bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE);
      changeData = true;
    }
}



static void zclApp_Report(void) { osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 20); }


/****************************************************************************
// E-ink display
****************************************************************************/
void EpdStart(void)
{
    EpdInitFull();
    EpdSetFrameMemory(LOGO);
    EpdDisplayFrame();
    user_delay_ms(1000);
    EpdSetFrameMemory(ESPEC);
    EpdDisplayFrame();
    user_delay_ms(3000);
    first = true;
    EpdSleep(); 
}

void EpdRefresh(void)
{
  unsigned char image[672];
  PaintPaint(image, 0, 0);
  
  EpdReset();
  if(first == true){
    EpdClearFrameMemoryF(0xFF);
    EpdDisplayFrame();
    first=false;
  }
  if(changeData == true){
    if(firstLoop == true){
     firstLoop = false; 
    }
 
    EpdInitPartial();
    
    loopCount--;
    if(loopCount == 0){
      EpdInitFull();
      EpdClearFrameMemoryF(0xFF);
      EpdDisplayFrame();
      EpdInitPartial();
      loopCount = 5;
    }else{
      EpdInitPartial();
    }
 
  epdTemperatureData(einkTemp);
  epdHumidityData(einkHum);
  epdZigbeeStatusData();
  epdBatteryData(zclBattery_PercentageRemainig/2);
  epdGraphData();

  if(loopCount == 0){
  loopCount = 5;
  }else{
  }
  
  EpdDisplayFramePartial(); 
  
  EpdSleep(); 

  changeData = false;
  }
}


void epdTemperatureData(uint16 tt) {
  
  byte one_t = tt / 100;
  byte two_t = tt % 100 / 10;
  byte three_t = tt % 10;
  
  PaintSetWidth(72);
  PaintSetHeight(44);
  PaintSetRotate(ROTATE_180);

    if (one_t == 0) {
      PaintClear(UNCOLORED);
      switch (two_t) {
        case 0:
          PaintDrawImage(ZERO, 4, 0, 67, 44, COLORED);
          break;
        case 1:
          PaintDrawImage(ONE, 4, 0, 67, 44, COLORED);
          break;
        case 2:
          PaintDrawImage(TWO, 4, 0, 67, 44, COLORED);
          break;
        case 3:
          PaintDrawImage(THREE, 4, 0, 67, 44, COLORED);
          break;
        case 4:
          PaintDrawImage(FOUR, 4, 0, 67, 44, COLORED);
          break;
        case 5:
          PaintDrawImage(FIVE, 4, 0, 67, 44, COLORED);
          break;
        case 6:
          PaintDrawImage(SIX, 4, 0, 67, 44, COLORED);
          break;
        case 7:
          PaintDrawImage(SEVEN, 4, 0, 67, 44, COLORED);
          break;
        case 8:
          PaintDrawImage(EIGHT, 4, 0, 67, 44, COLORED);
          break;
        case 9:
          PaintDrawImage(NINE, 4, 0, 67, 44, COLORED);
          break;
      }
      EpdSetFrameMemoryXY(PaintGetImage(), 24, 39, PaintGetWidth(), PaintGetHeight());
      
    } else {
      PaintClear(UNCOLORED);
      switch (one_t) {
        case 0:
          PaintDrawImage(ZERO, 4, 0, 67, 44, COLORED);
          break;
        case 1:
          PaintDrawImage(ONE, 4, 0, 67, 44, COLORED);
          break;
        case 2:
          PaintDrawImage(TWO, 4, 0, 67, 44, COLORED);
          break;
        case 3:
          PaintDrawImage(THREE, 4, 0, 67, 44, COLORED);
          break;
        case 4:
          PaintDrawImage(FOUR, 4, 0, 67, 44, COLORED);
          break;
        case 5:
          PaintDrawImage(FIVE, 4, 0, 67, 44, COLORED);
          break;
        case 6:
          PaintDrawImage(SIX, 4, 0, 67, 44, COLORED);
          break;
        case 7:
          PaintDrawImage(SEVEN, 4, 0, 67, 44, COLORED);
          break;
        case 8:
          PaintDrawImage(EIGHT, 4, 0, 67, 44, COLORED);
          break;
        case 9:
          PaintDrawImage(NINE, 4, 0, 67, 44, COLORED);
          break;
      }
      EpdSetFrameMemoryXY(PaintGetImage(), 24, 10, PaintGetWidth(), PaintGetHeight());
     
      PaintClear(UNCOLORED);
      switch (two_t) {
        case 0:
          PaintDrawImage(ZERO, 4, 0, 67, 44, COLORED);
          break;
        case 1:
          PaintDrawImage(ONE, 4, 0, 67, 44, COLORED);
          break;
        case 2:
          PaintDrawImage(TWO, 4, 0, 67, 44, COLORED);
          break;
        case 3:
          PaintDrawImage(THREE, 4, 0, 67, 44, COLORED);
          break;
        case 4:
          PaintDrawImage(FOUR, 4, 0, 67, 44, COLORED);
          break;
        case 5:
          PaintDrawImage(FIVE, 4, 0, 67, 44, COLORED);
          break;
        case 6:
          PaintDrawImage(SIX, 4, 0, 67, 44, COLORED);
          break;
        case 7:
          PaintDrawImage(SEVEN, 4, 0, 67, 44, COLORED);
          break;
        case 8:
          PaintDrawImage(EIGHT, 4, 0, 67, 44, COLORED);
          break;
        case 9:
          PaintDrawImage(NINE, 4, 0, 67, 44, COLORED);
          break;
      }
      EpdSetFrameMemoryXY(PaintGetImage(), 24, 54, PaintGetWidth(), PaintGetHeight());
      
      PaintSetWidth(72);
      PaintSetHeight(7);
      PaintSetRotate(ROTATE_180);
      PaintClear(UNCOLORED);
      
      PaintDrawImage(POINT, 42, 0, 29, 7, COLORED);
      EpdSetFrameMemoryXY(PaintGetImage(), 24, 96, PaintGetWidth(), PaintGetHeight());
      
      
      PaintSetWidth(72);
      PaintSetHeight(19);
      PaintSetRotate(ROTATE_180);
      PaintClear(UNCOLORED);
      switch (three_t) {
        case 0:
          PaintDrawImage(ZERO_S, 42, 0, 29, 19, COLORED);
          break;
        case 1:
          PaintDrawImage(ONE_S, 42, 0, 29, 19, COLORED);
          break;
        case 2:
          PaintDrawImage(TWO_S, 42, 0, 29, 19, COLORED);
          break;
        case 3:
          PaintDrawImage(THREE_S, 42, 0, 29, 19, COLORED);
          break;
        case 4:
          PaintDrawImage(FOUR_S, 42, 0, 29, 19, COLORED);
          break;
        case 5:
          PaintDrawImage(FIVE_S, 42, 0, 29, 19, COLORED);
          break;
        case 6:
          PaintDrawImage(SIX_S, 42, 0, 29, 19, COLORED);
          break;
        case 7:
          PaintDrawImage(SEVEN_S, 42, 0, 29, 19, COLORED);
          break;
        case 8:
          PaintDrawImage(EIGHT_S, 42, 0, 29, 19, COLORED);
          break;
        case 9:
          PaintDrawImage(NINE_S, 42, 0, 29, 19, COLORED);
          break;
      }
      EpdSetFrameMemoryXY(PaintGetImage(), 24, 105, PaintGetWidth(), PaintGetHeight());
      
      PaintSetWidth(24);
      PaintSetHeight(22);
      PaintSetRotate(ROTATE_180);
      PaintClear(UNCOLORED);
      PaintDrawImage(CELSIUS, 4, 0, 16, 22, COLORED);
      EpdSetFrameMemoryXY(PaintGetImage(), 72, 98, PaintGetWidth(), PaintGetHeight());
      
      
      PaintSetWidth(24);
      PaintSetHeight(12);
      PaintSetRotate(ROTATE_180);
      PaintClear(UNCOLORED);
      
      if (last_tmp != 0) {
        if (tt > last_tmp) {
          PaintDrawImage(IMAGE_UP, 5, 0, 14, 12, COLORED);
          upTemp = true;
          downTemp = false;
        } else if (tt < last_tmp) {
          PaintDrawImage(IMAGE_DOWN, 5, 0, 14, 12, COLORED);
          upTemp = false;
          downTemp = true;
        } else {
          if (upTemp == true) {
            PaintDrawImage(IMAGE_UP, 5, 0, 14, 12, COLORED);
          }
          if (downTemp == true) {
            PaintDrawImage(IMAGE_DOWN, 5, 0, 14, 12, COLORED);
          }
        }
      }
      last_tmp = tt;
      EpdSetFrameMemoryXY(PaintGetImage(), 100, 75, PaintGetWidth(), PaintGetHeight());
      
      PaintSetWidth(24);
      PaintSetHeight(15);
      PaintSetRotate(ROTATE_180);
      PaintClear(UNCOLORED);
      PaintDrawImage(I1, 3, 0, 17, 15,  COLORED);
      EpdSetFrameMemoryXY(PaintGetImage(), 96, 56, PaintGetWidth(), PaintGetHeight());
    }
  }



void epdHumidityData(uint16 hh) {
  
  byte one_h = hh / 10;
  byte  two_h = hh % 10;
  
  
  PaintSetWidth(72);
  PaintSetHeight(44);
  PaintSetRotate(ROTATE_180);

    if (one_h == 0) {
      PaintClear(UNCOLORED);
      switch (two_h) {
        case 0:
          PaintDrawImage(ZERO, 4, 0, 67, 44, COLORED);
          break;
        case 1:
          PaintDrawImage(ONE, 4, 0, 67, 44, COLORED);
          break;
        case 2:
          PaintDrawImage(TWO, 4, 0, 67, 44, COLORED);
          break;
        case 3:
          PaintDrawImage(THREE, 4, 0, 67, 44, COLORED);
          break;
        case 4:
          PaintDrawImage(FOUR, 4, 0, 67, 44, COLORED);
          break;
        case 5:
          PaintDrawImage(FIVE, 4, 0, 67, 44, COLORED);
          break;
        case 6:
          PaintDrawImage(SIX, 4, 0, 67, 44, COLORED);
          break;
        case 7:
          PaintDrawImage(SEVEN, 4, 0, 67, 44, COLORED);
          break;
        case 8:
          PaintDrawImage(EIGHT, 4, 0, 67, 44, COLORED);
          break;
        case 9:
          PaintDrawImage(NINE, 4, 0, 67, 44, COLORED);
          break;
      }
      EpdSetFrameMemoryXY(PaintGetImage(), 22, 36, PaintGetWidth(), PaintGetHeight());
      
    } else {
      PaintClear(UNCOLORED);
      switch (one_h) {
        case 0:
          PaintDrawImage(ZERO, 4, 0, 67, 44, COLORED);
          break;
        case 1:
          PaintDrawImage(ONE, 4, 0, 67, 44, COLORED);
          break;
        case 2:
          PaintDrawImage(TWO, 4, 0, 67, 44, COLORED);
          break;
        case 3:
          PaintDrawImage(THREE, 4, 0, 67, 44, COLORED);
          break;
        case 4:
          PaintDrawImage(FOUR, 4, 0, 67, 44, COLORED);
          break;
        case 5:
          PaintDrawImage(FIVE, 4, 0, 67, 44, COLORED);
          break;
        case 6:
          PaintDrawImage(SIX, 4, 0, 67, 44, COLORED);
          break;
        case 7:
          PaintDrawImage(SEVEN, 4, 0, 67, 44, COLORED);
          break;
        case 8:
          PaintDrawImage(EIGHT, 4, 0, 67, 44, COLORED);
          break;
        case 9:
          PaintDrawImage(NINE, 4, 0, 67, 44, COLORED);
          break;
      }
      EpdSetFrameMemoryXY(PaintGetImage(), 24, 142, PaintGetWidth(), PaintGetHeight());
     
      PaintClear(UNCOLORED);
      switch (two_h) {
        case 0:
          PaintDrawImage(ZERO, 4, 0, 67, 44, COLORED);
          break;
        case 1:
          PaintDrawImage(ONE, 4, 0, 67, 44, COLORED);
          break;
        case 2:
          PaintDrawImage(TWO, 4, 0, 67, 44, COLORED);
          break;
        case 3:
          PaintDrawImage(THREE, 4, 0, 67, 44, COLORED);
          break;
        case 4:
          PaintDrawImage(FOUR, 4, 0, 67, 44, COLORED);
          break;
        case 5:
          PaintDrawImage(FIVE, 4, 0, 67, 44, COLORED);
          break;
        case 6:
          PaintDrawImage(SIX, 4, 0, 67, 44, COLORED);
          break;
        case 7:
          PaintDrawImage(SEVEN, 4, 0, 67, 44, COLORED);
          break;
        case 8:
          PaintDrawImage(EIGHT, 4, 0, 67, 44, COLORED);
          break;
        case 9:
          PaintDrawImage(NINE, 4, 0, 67, 44, COLORED);
          break;
      }
      EpdSetFrameMemoryXY(PaintGetImage(), 24, 186, PaintGetWidth(), PaintGetHeight());
      
      PaintSetWidth(24);
      PaintSetHeight(22);
      PaintSetRotate(ROTATE_180);
      PaintClear(UNCOLORED);
      PaintDrawImage(PERCENT, 4, 0, 16, 22, COLORED);
      EpdSetFrameMemoryXY(PaintGetImage(), 72, 228, PaintGetWidth(), PaintGetHeight());
      
      
      PaintSetWidth(24);
      PaintSetHeight(12);
      PaintSetRotate(ROTATE_180);
      PaintClear(UNCOLORED);
      
      if (last_hum != 0) {
        if (hh > last_hum) {
          PaintDrawImage(IMAGE_UP, 5, 0, 14, 12, COLORED);
          upHum = true;
          downHum = false;
        } else if (hh < last_hum) {
          PaintDrawImage(IMAGE_DOWN, 5, 0, 14, 12, COLORED);
          upHum = false;
          downHum = true;
        } else {
          if (upHum == true) {
            PaintDrawImage(IMAGE_UP, 5, 0, 14, 12, COLORED);
          }
          if (downHum == true) {
            PaintDrawImage(IMAGE_DOWN, 5, 0, 14, 12, COLORED);
          }
        }
      }
      last_hum = hh;
      EpdSetFrameMemoryXY(PaintGetImage(), 100, 167, PaintGetWidth(), PaintGetHeight());
      
      PaintSetWidth(24);
      PaintSetHeight(15);
      PaintSetRotate(ROTATE_180);
      PaintClear(UNCOLORED);
      PaintDrawImage(I2, 4, 0, 17, 15,  COLORED);
      EpdSetFrameMemoryXY(PaintGetImage(), 96, 185, PaintGetWidth(), PaintGetHeight());
      
  }
}



 
void epdBatteryData(uint8 b) {
  
  PaintSetWidth(22);
  PaintSetHeight(25);
  PaintSetRotate(ROTATE_0);
  PaintClear(UNCOLORED);
 
  if (b <= 0) {
    PaintDrawImage(bz, 4, 0, 16, 25, COLORED);
  }
  if (b > 0 && b <= 13) {
    PaintDrawImage(b0, 4, 0, 16, 25, COLORED);
  }
  if (b > 13 && b <= 25) {
    PaintDrawImage(b13, 4, 0, 16, 25, COLORED);
  }
  if (b > 25 && b <= 38) {
    PaintDrawImage(b38, 4, 0, 16, 25, COLORED);
  }
  if (b > 38 && b <= 50) {
    PaintDrawImage(b50, 4, 0, 16, 25, COLORED);
  }
  if (b > 50 && b <= 63) {
    PaintDrawImage(b63, 4, 0, 16, 25, COLORED);
  }
  if (b > 63 && b <= 75) {
    PaintDrawImage(b75, 4, 0, 16, 25, COLORED);
  }
  if (b > 75 && b <= 87) {
    PaintDrawImage(b87, 4, 0, 16, 25, COLORED);
  }
  if (b > 87) {
    PaintDrawImage(b100, 4, 0, 16, 25, COLORED);
  }
  EpdSetFrameMemoryXY(PaintGetImage(), 96, 26, PaintGetWidth(), PaintGetHeight());
}




void epdZigbeeStatusData() {
  PaintSetWidth(24);
  PaintSetHeight(20);
  PaintSetRotate(ROTATE_90);
  PaintClear(UNCOLORED);
  if ( bdbAttributes.bdbNodeIsOnANetwork ){
    PaintDrawImage(IMAGE_ONNETWORK, 4, 4, 16, 16, COLORED);
  } else {
    PaintDrawImage(IMAGE_OFFNETWORK, 4, 4, 16, 16, COLORED);
  }
  EpdSetFrameMemoryXY(PaintGetImage(), 96, 206, PaintGetWidth(), PaintGetHeight());
}



void epdGraphData(void){
  PaintSetWidth(20);
  PaintSetHeight(101);
  PaintSetRotate(ROTATE_270);
  PaintClear(UNCOLORED);
  int shag = 96;
  
  uint16 minActualT;
  uint16 maxActualT;
  uint16 minActualH;
  uint16 maxActualH;
  
    /* find min */
  minActualT = massDataT[0];

  for (int i = 0; i < 24; i++)
  {
      if (massDataT[i] < minActualT && massDataT[i] != 0)
          minActualT = massDataT[i];
  }

  /* find max */
  maxActualT = massDataT[0];

  for (int i = 0; i < 24; i++)
  {
      if (massDataT[i] > maxActualT && massDataT[i] !=0)
          maxActualT = massDataT[i];
  }
  
     /* find min */
  minActualH = massDataH[0];

  for (int i = 0; i < 24; i++)
  {
      if (massDataH[i] < minActualH && massDataH[i] != 0)
          minActualH = massDataH[i];
  }

  /* find max */
  maxActualH = massDataH[0];

  for (int i = 0; i < 24; i++)
  {
      if (massDataH[i] > maxActualH && massDataH[i] !=0)
          maxActualH = massDataH[i];
  }

  for(int i=0; i<24; i++)
       {
          int ch = (uint16)mapRange(minActualT, maxActualT, 0.0, 14.0, massDataT[i]);
          
          if(i == 23){
            PaintDrawVerticalLine(shag, 4, ch+2, COLORED);
            PaintDrawVerticalLine(shag+1, 4, ch+2, COLORED);
            PaintDrawVerticalLine(shag+2, 4, ch+2, COLORED);
          }else{
            PaintDrawVerticalLine(shag, 4, ch+2, COLORED);
            PaintDrawVerticalLine(shag+1, 4, ch+2, COLORED);
            PaintDrawVerticalLine(shag+2, 4, ch+2, COLORED);
            shag = shag-4;
          }
       }

  EpdSetFrameMemoryXY(PaintGetImage(), 0, 14, PaintGetWidth(), PaintGetHeight()); 
  
  
  PaintClear(UNCOLORED);
  shag = 96;

    for(int i=0; i<24; i++)
       {
          int ch = (uint16)mapRange(minActualH, maxActualH, 0.0, 14.0, massDataH[i]);
          
          if(i == 23){
            PaintDrawVerticalLine(shag, 4, ch+2, COLORED);
            PaintDrawVerticalLine(shag+1, 4, ch+2, COLORED);
            PaintDrawVerticalLine(shag+2, 4, ch+2, COLORED);
          }else{
            PaintDrawVerticalLine(shag, 4, ch+2, COLORED);
            PaintDrawVerticalLine(shag+1, 4, ch+2, COLORED);
            PaintDrawVerticalLine(shag+2, 4, ch+2, COLORED);
            shag = shag-4;
          }
       }

  EpdSetFrameMemoryXY(PaintGetImage(), 0, 140, PaintGetWidth(), PaintGetHeight()); 
  
}


void user_delay_ms(unsigned int delaytime) {
  while(delaytime--)
  {
    uint16 microSecs = 1000;
    while(microSecs--)
    {
      asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
    }
  }
}

void conveyor(int *Arr, int n, int l)
{
     int i,j,c;
     for (i=1; i<=l; i++)
     {
         c=Arr[0];
         for (j=0; j<n-1; j++) Arr[j]=Arr[j+1];
         Arr[n-1]=c;
     }
}

void graphData(uint16 temp, uint16 hum){

if(numDataT < 60){
   amountDataT = amountDataT + temp;
    numDataT++;
  }else{
    amountDataT = amountDataT + temp;
    numDataT = 0;
    
    uint16 readyDataT = amountDataT / 60;
    amountDataT = 0;
    
    if(numMassDataT < 24){
      massDataT[numMassDataT] = readyDataT;
      numMassDataT++;
    }else if(numMassDataT == 24){
      conveyor(massDataT, 24, 1); 
      massDataT[23] = readyDataT;
    }
    changeData = true;
  }

if(numDataH < 60){
   amountDataH = amountDataH + hum;
    numDataH++;
  }else{
    amountDataH = amountDataH + hum;
    numDataH = 0;
    
    uint16 readyDataH = amountDataH / 60;
    amountDataH = 0;
    
    if(numMassDataH < 24){
      massDataH[numMassDataH] = readyDataH;
      numMassDataH++;
    }else if(numMassDataH == 24){
      conveyor(massDataH, 24, 1); 
      massDataH[23] = readyDataH;
    }
    changeData = true;
  }
}