/*
 * Modified 17/11/2025
 * Online Sampling Audit Trail
 */
//Modified 01/11/2025
/*
 * Self User Password Change Complexity Check Added
 * User Reset through Server Added
 * Sampling End Time adjustment done
 */
//Modified 03/08/2025
//AIR_SAMPLER - TOPWAY SMART LCD - 3.5"
//Modified 15/05/2025
//1-BG - Single Channel, Basic, Graphical UI
//Auto Shutdown Modified Check

/*
  * During Sampling if User tries to Aborts. Click on abort last user logs in to sample record.
  
*/

#include "VarDef.h"
#include "VarDef1.h"

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  Serial2.setTimeout(100);
  EEPROM.begin(64);
  /*DeclareIO();
  InitIO();*/
  rtcin();
  now1 = rtc.now();
  
  //StartADS();
  delay(5000);//digitalWrite(BUZZ,LOW);
  InitSyncPrimitives();
  SDCard_init();
  SyncRtcFromNtpAtBoot();
  
  LogActivity("DevicePowerOn");ReadVtg();/*LogBatParameters();*/
  ProcessScreen();
  ReadEEP();
  StartSyncManager();
}

/*void StartADS() {
  if (!ads.begin()) {
    ADSErr=true;
  }
}*/

void rtcin() {
  if (! rtc.begin()) {
    //lcd.print("Couldn't find RTC");
    //abort();
    //strerr="Error";
  }
  if (! rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop(void) {
  if(OnlineStatus==true) {
    handleRequest1();
    if(OTA_status==true) {
      Init_OTA();
      while(1)
      {
        ArduinoOTA.handle();
      }
    }
  }
  ReadSerial();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis1 >= interval1) {  //100ms
    previousMillis1 = currentMillis;
    Read_Disp_Params();
  }
  
  if (currentMillis - previousMillis >= interval) {  //1000ms
    previousMillis = currentMillis;
    now1 = rtc.now();
    WriteTime();
    ReadVtg();
//    Read_Disp_Params();
    Motor_On_Off();
    CheckWiFi();
    LogInterval++;
    if(LogInterval==60) {
      /*LogBatParameters();*/
      LogInterval=0;
    }
    if(UserId=="NO_USER" && Motor_Status1==false) {
      if(Power_Off_Timer<Max_Poweroff_Time-30) {
        Power_Off_Timer=Power_Off_Timer+1;
      } else if(Power_Off_Timer<(Max_Poweroff_Time)) {
        Power_Off_Timer=Power_Off_Timer+1;
        WriteTopway(0x00,0x68,2);
        WriteTopway(0x00,0x56,(Max_Poweroff_Time-Power_Off_Timer));
      } else {
        ShutDown("AutoShutDown");
        AutoShutdownbit=true;
      }
    } else if(UserId=="NO_USER") {
      Power_Off_Timer=0;
      WriteTopway(0x00,0x68,0);
    } else {
      if(Logout_Timer<Max_Logout_Time-30) {
        Logout_Timer=Logout_Timer+1;
      } else if(Logout_Timer<(Max_Logout_Time)) {
        Logout_Timer=Logout_Timer+1;
        WriteTopway(0x00,0x68,3);
        WriteTopway(0x00,0x56,(Max_Logout_Time-Logout_Timer));
      } else {
        LogOut("AutoLogout");
      }
    }
  }
}

void Init_OTA() {
  ArduinoOTA.onStart([]() {
    
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    STR_ST1 = "Start updating " + type;
    WriteString(0x00,0x31,0x80,STR_ST1);
  });
  ArduinoOTA.onEnd([]() {
    STR_ST1 = "Update complete";
    WriteString(0x00,0x31,0x80,STR_ST1);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    STR_ST1 = "Progress: " + String((progress / (total / 100))) + "%";
    WriteString(0x00,0x31,0x80,STR_ST1);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if (error == OTA_AUTH_ERROR) STR_ST1 = "Auth Failed";
    else if (error == OTA_BEGIN_ERROR) STR_ST1 = "Begin Failed";
    else if (error == OTA_CONNECT_ERROR) STR_ST1 = "Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) STR_ST1 = "Receive Failed";
    else if (error == OTA_END_ERROR) STR_ST1 = "End Failed";
    STR_ST1 = "Error[" + String(error) + "]: " + STR_ST1;
    WriteString(0x00,0x31,0x80,STR_ST1);
  });

  ArduinoOTA.begin();
}

void ShutDown(String Act1) {
  AuditDetails="NA";AuditRemark="NA";
  if(AutoShutdownbit==false) {
    LogActivity(Act1);/*LogBatParameters();*/
    EEPROM.write(0,0);EEPROM.commit();
  }
  Serial2.print("SHUTDOWN");
}

void LogOut(String Act1) {
  WriteTopway(0x00,0x68,0);
  WriteTopway(0x00,0xcc,0); //Disable Setting 2 touch button
  Logout_Timer=0;
  AuditDetails="NA";AuditRemark="NA";
  LogActivity(Act1);
  Set_In_Bit(6,0);
  DisplayPage(1);
  UserId="NO_USER";
  Power_Off_Timer=0;
}

void Buzz_Blink(int i) {
  while(i>0) {
    //digitalWrite(BUZZ,HIGH);
    delay(1000);
    //digitalWrite(BUZZ,LOW);
    delay(1000);
    i=i-1;
  }
}

void ReadEEP() {
    // Initialize EEPROM

  uint8_t e0 = EEPROM.read(0);
  if (e0 > 1) {
    EEPROM.write(0, 0);
  }
  if (e0 == 1) {
    LogActivity("AbruptShutdown");
  }
  EEPROM.write(0, 1);

  uint8_t pwmVal = EEPROM.read(10);
  if (pwmVal < 20 || pwmVal > 100) {
    pwmVal = 70;
    EEPROM.write(10, pwmVal);
  }
  Motor_PWM[0] = pwmVal;
  WriteTopway(0x00, 0x00, Motor_PWM[0]);
  Cal_PWM = Motor_PWM[0];
  WriteTopway(0x00, 0x10, Cal_PWM);

  uint8_t delayVal = EEPROM.read(11);
  if (delayVal > 100) {
    delayVal = 70;
    EEPROM.write(11, delayVal);
  }
  Start_Delay1 = delayVal;

  EEPROM.commit();  // Commit all changes
}
