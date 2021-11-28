
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
//#include "zcl_lighting.h"
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

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

extern bool requestNewTrustCenterLinkKey;
byte zclApp_TaskID;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
bool firstLoop = true;
bool changeData = false;
static uint8 currentSensorsReadingPhase = 0;
int16 colorPrint = 0x00;  // 00 - black on white, ff -  white on black
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





/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclApp_HandleKeys(byte shift, byte keys);
static void zclApp_Report(void);

static void zclApp_ReadSensors(void);
static void zclApp_TempHumiSens(void);

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
        osal_start_reload_timer(zclApp_TaskID, APP_REPORT_EVT, APP_REPORT_DELAY);
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
        osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 1000);
        }else{
          osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 2000);
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
    HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
    switch (currentSensorsReadingPhase++) {  
    case 0:
      if(startWork <= 5){
        startWork++;
        zclBattery_Report();
        pushBut = true;
      }
      
      if(startWork == 6){
      sendBattCount++;
      if(sendBattCount == 3){
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
        osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 200);
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



static void zclApp_Report(void) { osal_start_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT, 200); }


/****************************************************************************
****************************************************************************/


void EpdRefresh(void)
{
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
 
  epdTemperatureData(zclApp_Temperature_Sensor_MeasuredValue);
  epdHumidityData(zclApp_HumiditySensor_MeasuredValue);
  epdZigbeeStatusData();
  epdBatteryData(zclBattery_PercentageRemainig/2);

  if(loopCount == 0){
  loopCount = 5;
  }else{
  }
  
  EpdDisplayFramePartial(); 
  
  EpdSleep(); 

  changeData = false;
  }
}


void EpdStart(void)
{
EpdInitFull();

    EpdSetFrameMemory(LOGO);
    EpdDisplayFrame();
    user_delay_ms(2000);
    
    EpdSetFrameMemory(ESPEC);
    EpdDisplayFrame();
    user_delay_ms(6000);
 
    first = true;
    EpdSleep(); 
}



void epdTemperatureData(uint16 tt) {
  
//temperature
  char temp_string[] = {'0', '0', '.', '0', '0', ' ', ' ','^', 'C','\0'};
  temp_string[0] = tt / 1000 % 10 + '0';
  temp_string[1] = tt / 100 % 10 + '0';
  temp_string[3] = tt / 10 % 10 + '0';
  temp_string[4] = tt % 10 + '0';
  
  PaintSetWidth(16);
  PaintSetHeight(110);
  PaintSetRotate(ROTATE_90);
  PaintClear(UNCOLORED);
  PaintDrawStringAt(0, 0, temp_string, &Font16, COLORED);
  EpdSetFrameMemoryXY(PaintGetImage(), 79, 90, PaintGetWidth(), PaintGetHeight()); 
  }



void epdHumidityData(uint16 hh) {
 
  //humidity
  char hum_string[] = {'0', '0', '.', '0', '0', ' ', ' ','%', ' ','\0'};
  hum_string[0] = hh / 1000 % 10 + '0';
  hum_string[1] = hh / 100 % 10 + '0';
  hum_string[3] = hh / 10 % 10 + '0';
  hum_string[4] = hh % 10 + '0';
  
  PaintSetWidth(16);
  PaintSetHeight(110);
  PaintSetRotate(ROTATE_90);
  PaintClear(UNCOLORED);
  PaintDrawStringAt(0, 0, hum_string, &Font16, COLORED);

  EpdSetFrameMemoryXY(PaintGetImage(), 63, 90, PaintGetWidth(), PaintGetHeight()); 
}



 
void epdBatteryData(uint8 b) {

  //percentage
  char perc_string[] = {'0', '0', '0', '%', '\0'};
  perc_string[0] = zclBattery_PercentageRemainig/2 / 100 % 10 + '0';
  perc_string[1] = zclBattery_PercentageRemainig/2 / 10 % 10 + '0';
  perc_string[2] = zclBattery_PercentageRemainig/2 % 10 + '0';
  
  PaintSetWidth(16);
  PaintSetHeight(48);
  PaintSetRotate(ROTATE_90);
  PaintClear(UNCOLORED);
  PaintDrawStringAt(0, 0, perc_string, &Font16, COLORED);

  EpdSetFrameMemoryXY(PaintGetImage(), 47, 90, PaintGetWidth(), PaintGetHeight()); 
}




void epdZigbeeStatusData() {

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