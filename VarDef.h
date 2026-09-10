#include <WiFi.h>
#include <WiFiUdp.h>
#include <stdio.h>
#include "RTClib.h"
#include <time.h>
#include <EEPROM.h>
//#include <FS.h>
#include "SPI.h"
//#include <SD.h>
#include <SdFat.h>
#include <Adafruit_ADS1X15.h>
#include <ArduinoOTA.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

Adafruit_ADS1115 ads;
//RTC Variables
RTC_DS1307 rtc;
DateTime now1,Setnow,future,diff;
DateTime ProfileStart1, ProfileEnd1;

SdFat SD;

WiFiServer wifiServer(80);

char daysOfTheWeek[7][12] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
int date_set=0,month_set=0,year_set=0;
int hour_set=0,minute_set=0;
int StartHour, EndHour, StartMin, EndMin;

// NTP config
// const IPAddress ntpServerIp(172, 34, 70, 13); Zydus Ahmdabad
const IPAddress ntpServerIp(192, 168, 17, 112);
const long gmtOffset_sec = 19800;  // IST = GMT+5:30
const int daylightOffset_sec = 0;
bool ntpSynced = false;

long AuditId=0;

unsigned long previousMillis = 0;
unsigned long previousMillis1 = 0;

bool Read_Err=false;

const long interval = 1000;
const long interval1 = 100;
int Power_Off_Timer=0;
int Logout_Timer=0;
int icount;

//LCD Variables
int i;
String OutString;
int Motor_Bits_h;
int Motor_Bits_l;
int bitArray[16];
int packetOK=0;
int Motor_PWM[4];
int Motor_PWM1[4];
int Cal_PWM;
//int Cal_PWM1;
int Start_Delay1=0;
int Start_Delay1_1=0;
int Between_Delay1=0;
int CalID=0;
String calDate="Not_Set";
String CalDueDate="Not_Set";
String calDateStr="";
String dueDateStr="";

byte temp_l;
byte temp_h;
byte Output_l;
byte Output_h;

int No_of_Runs=1;
int No_of_Runs1=1;
int Delay_between_Runs=0;
int Delay_between_Runs1=0;
//Motor Status
bool Motor_Status1=false;
//bool Motor_Status2=false;
//bool Motor_Status3=false;
//bool Motor_Status4=false;
bool Cal_Status1=false;
int Motor_Counter=0;
int Motor_Counter1=0;
int Motor_Counter2=0;
int Motor_Counter_Max=0;
int Motor_Counter_Perc;
unsigned int Motor_lts;
unsigned int Motor_lts1;

//ANALOG INPUTS
float average;
int tn;
int MTR_VTG;
//const int MTR_VTG_Pin_1 = A3;
int BAT_VTG;
int BAT_PER;
//const int BAT_VTG_Pin = A7;
int CHG_VTG;
//const int CHG_VTG_Pin = A6;

bool ChargerStatus=false;
byte LogInterval=0;

int MTR_CUR;

//Outputs
/*const int LED_Out = 41;
const int Power_Out = 17;
const int OUT1 = 15;
const int RL1 = 16;

const int BUZZ = 12;*/

//ADC
bool ADSErr=false;
int dewpoint1;

int writeback_h=0;
int writeback_m1=0;
int writeback_m2=0;
int writeback_l=0;

String SerailIn;

//Display Variables
String Str1, TempStr;
String strError;
String Str2;
String Str3;

//SD Card
File myFile;
File tempFile;
File myFile1;
File userFile;
//ESP
#define SD_CS 5
SPIClass spi = SPIClass(VSPI);
//const int chipSelect = 45;



//Device Details
String ModelNo = "AMS-G100-1BCF";
String SoftwareVer="AMS-V01";
String UnitName="UNIT-1";
String EquipmentId="250001";
String deviceID = "G00250001";
String currentDate = "";

String UnitName1="UNIT-1";
String EquipmentId1="250001";
String deviceID1 = "G00250001";

//CompanyDetails
String Company_Name="GALLEON";
String City_Name="MUMBAI";
String State_Name="MAHARASHTRA";

String Company_Name1="GALLEON";
String City_Name1="MUMBAI";
String State_Name1="MAHARASHTRA";


//Report Variables
//Activity Log
String UserId1="";
String Password1="";
String UserId="NO_USER";
String Password="";
String Activity="DevicePowerOn";
String ActivityTime;
int UserSrNo;
String UserFirstName="";
String UserLastName="";
String UserDept="";
String UserGroup="";
String UserStatus="";
bool LoginBit=false;
byte RetryLimits=0;
String ExpDate="";
bool PasswordExpired=false;
bool PasswordResetOnExpiry=false;  // true while waiting for user to self-reset after expiry popup

//
String line;
int pos1;
int pos2;
int pos3;
int pos4;
int pos5;
int pos6;
int pos7;
int pos8;
int pos9;
int pos10;
int pos11;
int pos12;
int pos13;
int pos14;
int pos15;
int pos16;
int pos17;
int pos18;
int pos19;
int pos20;
int pos21;

//Device Logs
String LastUserId="NA";
String LastSampleId="NA";
String LastSampleName="NA";
String LastSampleMode="NA";
String LastSampleStart="NA";
String LastSampleEnd="NA";
String LastSampleStatus="NA";
String LastSampleRemark="NA";
String LastSampleLocation="NA";
int LastVolume=0;
int LastStartDelay=0;
int LastNoofSamples=0;
int LastDelaybetwRuns=0;

String UserLastSampleMode="NA";
String UserLastSampleLocation="NA";
int UserLastVolume=0;
int UserLastStartDelay=0;
int UserLastNoofSamples=0;
int UserLastDelaybetwRuns=0;

//Sample Data Report
long SampleId=0;
String strSampleId;
String SampleName;
String Mode;
String SamplingStart;
String SamplingEnd;
String SamplingStatus;
String SamplingRemark="NO_RMK";
String SamplingLocation;
String SamplingErr;
String Strfile="/Login.csv";
String inputString;
bool stringComplete;

//ESP 
String IpAddress="";
bool IPEnable=false;

int Page_No=0;
int Page_No_Old=0;
//Config
int ParamId;
String strParam="";
int LocationId=0;
String strLocation="";
String LocationStatus="Active";

int RemarkId=0;
String strRemark="";
String RemarkStatus="Active";

int RecipeId=0;
String strRecipe="";
String RecipeLoc="NO_LOC";
String RecipeMode="";
int RecipeVolume=0;
int RecipeStartDelay=0;
int RecipeNoOfRuns=1;
int RecipeDelayBetRun=0;
String RecipeStatus="Active";

String strRecipe1="";
String RecipeLoc1="NO_LOC";
String RecipeMode1="";
int RecipeVolume1=0;
int RecipeStartDelay1=0;
int RecipeNoOfRuns1=1;
int RecipeDelayBetRun1=0;
//String RecipeStatus1="Active";

String strRecipe2="";
String RecipeLoc2="NO_LOC";
String RecipeMode2="";
int RecipeVolume2=0;
int RecipeStartDelay2=0;
int RecipeNoOfRuns2=1;
int RecipeDelayBetRun2=0;
//String RecipeStatus1="Active";

int GroupId=0;
String strGroup="";
String GroupStatus="Active";
int GroupMinPassLength=4;
int GroupPasswordExpiy=90;
int GroupRetryLimits=3;
String PasswordComplexity="No";
int LastPAsswordUnique=1;
String ResetPassAuth="No";
String FixedVolume="No";
int Logout_Timer1=600;
int Power_Off_Timer1=600;
String Software_Access="No";
String Device_Monitoring="No";
String Group_User_Setting="No";
String Config_Setting="No";
String Sample_Data_View="No";
String Audit_Trail_View="No";
String Print_Config="No";
String Print_Sample_Data="No";
String Print_Audit_Trail="No";
String Allow_Sampling="No";

String GroupStatus1="Active";
int GroupMinPassLength1=4;
int GroupPasswordExpiy1=90;
int GroupRetryLimits1=3;
String PasswordComplexity1="No";
int LastPAsswordUnique1=1;
String ResetPassAuth1="No";
String FixedVolume1="No";
int Logout_Timer2=600;
int Power_Off_Timer2=600;
String Software_Access1="No";
String Device_Monitoring1="No";
String Group_User_Setting1="No";
String Config_Setting1="No";
String Sample_Data_View1="No";
String Audit_Trail_View1="No";
String Print_Config1="No";
String Print_Sample_Data1="No";
String Print_Audit_Trail1="No";
String Allow_Sampling1="No";

int GroupMinPassLength2=4;
String PasswordComplexity2="No";

//String GroupStatus1="Active";
int GroupMinPassLength3=4;
int GroupPasswordExpiy3=90;
int GroupRetryLimits3=3;
String PasswordComplexity3="No";
int LastPAsswordUnique3=1;
String ResetPassAuth3="No";
String FixedVolume3="No";
int Logout_Timer3=600;
int Power_Off_Timer3=600;
String Software_Access3="No";
String Device_Monitoring3="No";
String Group_User_Setting3="No";
String Config_Setting3="No";
String Sample_Data_View3="No";
String Audit_Trail_View3="No";
String Print_Config3="No";
String Print_Sample_Data3="No";
String Print_Audit_Trail3="No";
String Allow_Sampling3="No";

String GroupStatus4="Active";
int GroupMinPassLength4=4;
int GroupPasswordExpiy4=90;
int GroupRetryLimits4=3;
String PasswordComplexity4="No";
int LastPAsswordUnique4=1;
String ResetPassAuth4="No";
String FixedVolume4="No";
int Logout_Timer4=600;
int Power_Off_Timer4=600;
String Software_Access4="No";
String Device_Monitoring4="No";
String Group_User_Setting4="No";
String Config_Setting4="No";
String Sample_Data_View4="No";
String Audit_Trail_View4="No";
String Print_Config4="No";
String Print_Sample_Data4="No";
String Print_Audit_Trail4="No";
String Allow_Sampling4="No";

int Max_Logout_Time=600;
int Max_Poweroff_Time=600;

String Group;

int GUserId=0;
String strGUser="";
String GUserStatus="Active";
String FirstName="";
String LastName="";
String GUserDept="";
String GUserGroup="";
String GUserPass1="";
String GUserPass2="";
String GUserPass3="";

byte adr_h2, adr_m2, adr_l2;

int RecordCount=1;
long RecordCount1=1;
long RecordCount2=1;
int MaxRecordCount=0;
long MaxRecordCount1=0;
long MaxRecordCount2=0;

bool RecordStatus=false;
bool RecordStatus1=false;
bool RecordStatus2=false;

int batPopCounter=0;

//Error Parameters
int ErrorCounter=0;
int ErrorCounter1=0;
int ErrorCounter2=0;
int Hblock_Min=100;
int Hblock_Max=300;
int Hopen_Min=450;
int Hopen_Max=800;

bool CalStatus=false;



String inputData = "";
bool OpStatus=false;

String AuditDetails="NA";
String AuditRemark="NA";

String wifiSSID;
String wifiPWD;

String ServerIP;
String SeverPort;

byte OnlineBit=0;
bool OnlineStatus=false;
String onlineUser="NO_USER";
bool SoftWareConnected=false;
bool WiFiStatus=false;
// Wi-Fi reconnect state. Reconnect attempts are deliberately non-blocking so
// sampling, SD logging, and the display loop continue while the AP is absent.
const unsigned long WIFI_RECONNECT_INTERVAL_MS = 10000;
unsigned long wifiLastReconnectAttemptMs = 0;
bool wifiReconnectAttempted = false;

bool AutoShutdownbit=false;

String strStaticIP="";
String strGateWay="";
String strSubnet="";
String strDNS="";
String strMAC="";

bool OTA_status=false;
int OTA_Val=0;

String STR_ST1="";

String SampleRunString="";

bool updateMemoryInfo=false;

// Production sync foundation
enum SyncState {
  SYNC_IDLE,
  SYNC_CHECK_NETWORK,
  SYNC_LOAD_SERVER_CONFIG,
  SYNC_HEALTH_CHECK,
  SYNC_LOAD_STATE,
  SYNC_FIND_PENDING,
  SYNC_UPLOAD_PENDING,
  SYNC_RETRY_BACKOFF
};

SemaphoreHandle_t sdMutex = NULL;
SemaphoreHandle_t syncEvent = NULL;
TaskHandle_t syncTaskHandle = NULL;

const uint32_t SD_LOCK_DEFAULT_TIMEOUT_MS = 250;
const uint32_t SD_LOCK_CRITICAL_TIMEOUT_MS = 2000;
const uint32_t SYNC_TASK_STACK_WORDS = 4096;
const UBaseType_t SYNC_TASK_PRIORITY = 1;
const BaseType_t SYNC_TASK_CORE = 0;
const uint32_t SYNC_PERIODIC_WAKE_MS = 60000;
const uint32_t SYNC_RETRY_BACKOFF_MS = 30000;
const uint32_t SYNC_TCP_CONNECT_TIMEOUT_MS = 3000;
const uint32_t SYNC_HTTP_RESPONSE_TIMEOUT_MS = 5000;
const uint32_t SYNC_SD_LOCK_TIMEOUT_MS = 100;
const uint32_t SYNC_SD_QUIET_MS = 5000;
const uint8_t SYNC_UPLOAD_BATCH_LIMIT = 1;
const char* SYNC_RECORDS_PATH = "/sync_device_esp_f3_push/";

const char* SYNC_DIR = "/SYNC";
const char* SYNC_STATE_FILE = "/SYNC/state.csv";
const char* SYNC_PENDING_FILE = "/SYNC/pending.csv";
const char* SYNC_INFLIGHT_FILE = "/SYNC/inflight.csv";
const char* SYNC_ERRORS_FILE = "/SYNC/errors.csv";
const char* SYNC_CURSOR_FILE = "/SYNC/cursors.csv";

const char* SYNC_STATE_HEADER = "last_success_ms,last_attempt_ms,last_state,wifi_status,failure_count,last_error";
const char* SYNC_PENDING_HEADER = "record_type,record_id,source_file,source_line,status,attempts,last_attempt_ms";
const char* SYNC_INFLIGHT_HEADER = "batch_id,record_type,first_record_id,last_record_id,source_file,status,attempt_ms";
const char* SYNC_ERRORS_HEADER = "time_ms,state,error_code,message";
const char* SYNC_CURSOR_HEADER = "record_type,source_file,last_record_number,last_source_line";

struct SyncPendingRecord {
  String recordType;
  String recordId;
  String sourceFile;
  unsigned long sourceLine;
  String status;
  unsigned long attempts;
  unsigned long lastAttemptMs;
  String csvRow;
};

volatile SyncState syncState = SYNC_IDLE;
volatile bool syncManagerStarted = false;
volatile bool syncWakePending = false;
unsigned long syncLastWakeMs = 0;
unsigned long syncLastAttemptMs = 0;
unsigned long syncLastSuccessMs = 0;
unsigned long syncFailureCount = 0;
unsigned long syncPendingDiscoveredLastRun = 0;
volatile unsigned long syncSdQuietUntilMs = 0;
bool LastSyncWiFiStatus = false;
bool syncLastHealthOk = false;

void InitSyncPrimitives();
bool SdLock(uint32_t timeoutMs = SD_LOCK_DEFAULT_TIMEOUT_MS);
void SdUnlock();
void NotifySyncEvent();
void MarkSyncSdQuiet(uint32_t quietMs = SYNC_SD_QUIET_MS);
bool IsSyncPausedForSampling();
void StartSyncManager();
void SyncManagerTask(void *parameter);
void SyncManagerTick();
const char* SyncStateName(SyncState state);
void EnsureSyncStorage();
bool EnsureSyncCsvFile(const char* path, const char* header);
void LoadSyncState();
void SaveSyncState(const char* lastError);
void AppendSyncError(const char* errorCode, const char* message);
bool LoadSyncServerConfig();
bool SyncHealthCheck();
String BuildSyncRecordId(const char* recordType, unsigned long recordNumber);
unsigned long GetLastPendingRecordNumber(const char* recordType);
unsigned long DiscoverPendingRecords();
unsigned long DiscoverPendingFromFile(const char* recordType, const char* sourceFile, bool sampleFile);
void ResetPendingQueue();
bool GetSyncCursor(const char* recordType, unsigned long *lastRecordNumber, unsigned long *lastSourceLine);
void SaveSyncCursor(const char* recordType, const char* sourceFile, unsigned long lastRecordNumber, unsigned long lastSourceLine);
bool ScanSourceLatest(const char* sourceFile, bool sampleFile, unsigned long *lastRecordNumber, unsigned long *lastSourceLine);
String JsonEscape(String value);
bool ParsePendingCsvRow(String row, SyncPendingRecord *record);
bool ReadCsvLineAt(const char* sourceFile, unsigned long targetLine, String *outRow);
bool LoadNextPendingRecord(SyncPendingRecord *record);
bool UploadPendingRecord(SyncPendingRecord *record);
bool UpdatePendingRecordStatus(const char* recordId, const char* newStatus, unsigned long attempts);
unsigned long UploadPendingRecords(uint8_t maxRecords);
