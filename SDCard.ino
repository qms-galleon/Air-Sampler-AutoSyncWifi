void SDCard_init() {
  //SD CARD
  SdSpiConfig config(SD_CS, DEDICATED_SPI, 8000000, &spi);
//  spi.begin(18, 19, 23, SD_CS);
  if (!SD.begin(config)) {
    strError="Err";
    WriteString(0x00,0x00,0x80,"ERR");
    delay(2000);
    return;
  }
  CreateFolders();
  ConnectToWiFi();
  
}

void ConnectToWiFi() {
  String ssid, password2;
  if (!readWiFiCredentials("/WIFICRED.csv", ssid, password2)) {
    return;
  }

  bool useStaticIP = false;
  IPAddress local_IP, gateway, subnet, dns;

  // Try to read static IP configuration from staticIP.csv using SdFat
  File ipFile;
  if (ipFile.open("/staticIP.csv", O_RDONLY)) {
    char line[100];
    int len = ipFile.fgets(line, sizeof(line));
    ipFile.close();

    if (len > 0) {
      line[len - 1] = '\0';  // remove newline if present
      String dataLine = String(line);
      dataLine.trim();

      int idx1 = dataLine.indexOf(',');
      int idx2 = dataLine.indexOf(',', idx1 + 1);
      int idx3 = dataLine.indexOf(',', idx2 + 1);
      int idx4 = dataLine.indexOf(',', idx3 + 1);

      if (idx1 > 0 && idx2 > idx1 && idx3 > idx2) {
        local_IP.fromString(dataLine.substring(0, idx1));
        subnet.fromString(dataLine.substring(idx1 + 1, idx2));
        gateway.fromString(dataLine.substring(idx2 + 1, idx3));
        dns.fromString(dataLine.substring(idx3 + 1, idx4));
        
        if (WiFi.config(local_IP, gateway, subnet, dns)) {
          useStaticIP = true;
        }
      }
    }
  }

  WiFi.begin(ssid.c_str(), password2.c_str());

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
  }

  WriteString(0x00, 0x31, 0x00, WiFi.macAddress().c_str());
  
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  WriteString(0x00, 0x02, 0x00, WiFi.localIP().toString().c_str());
  WriteString(0x00, 0x2e, 0x80, WiFi.localIP().toString().c_str());
  WriteString(0x00, 0x2f, 0x00, WiFi.gatewayIP().toString().c_str());
  WriteString(0x00, 0x2f, 0x80, WiFi.subnetMask().toString().c_str());
  WriteString(0x00, 0x30, 0x00, WiFi.dnsIP().toString().c_str());
  WriteString(0x00, 0x31, 0x00, WiFi.macAddress().c_str());
  //String macAddress = WiFi.macAddress();
  Set_Bit_Icons(1, 1);
  LogActivity(useStaticIP ? "WIFI Connected (Static IP)" : "WIFI Connected (DHCP)");

  wifiServer.begin();
}

void SyncRtcFromNtpAtBoot() {
  ntpSynced = false;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("NTP skipped: Wi-Fi is not connected");
    return;
  }

  WiFiUDP ntpUdp;
  byte packet[48] = {0};
  packet[0] = 0x23; // No warning, NTP version 4, client mode.

  if (!ntpUdp.begin(2390)) {
    Serial.println("NTP failed: could not open UDP socket");
    return;
  }

  ntpUdp.beginPacket(ntpServerIp, 123);
  ntpUdp.write(packet, sizeof(packet));
  ntpUdp.endPacket();

  const unsigned long startTime = millis();
  const unsigned long timeoutMs = 3000;
  while (millis() - startTime < timeoutMs) {
    int packetSize = ntpUdp.parsePacket();
    if (packetSize >= 48 && ntpUdp.remoteIP() == ntpServerIp &&
        ntpUdp.remotePort() == 123) {
      ntpUdp.read(packet, sizeof(packet));

      uint32_t ntpSeconds = ((uint32_t)packet[40] << 24) |
                            ((uint32_t)packet[41] << 16) |
                            ((uint32_t)packet[42] << 8) |
                            (uint32_t)packet[43];
      const uint32_t ntpToUnixEpoch = 2208988800UL;
      if (ntpSeconds > ntpToUnixEpoch) {
        uint32_t localUnixTime = ntpSeconds - ntpToUnixEpoch +
                                 gmtOffset_sec + daylightOffset_sec;
        rtc.adjust(DateTime(localUnixTime));
        now1 = rtc.now();
        ntpSynced = true;
        Serial.print("RTC synchronized once from NTP server ");
        Serial.println(ntpServerIp);
        break;
      }
    }
    delay(10);
  }

  ntpUdp.stop();
  if (!ntpSynced) {
    Serial.println("NTP timeout: keeping existing RTC time");
  }
}

void CreateFolders() {
   if (!SD.exists("/GUSER")) {SD.mkdir("/GUSER");}
   if (!SD.exists("/DEVICE")) {SD.mkdir("/DEVICE");}
   if (!SD.exists("/SYNC")) {SD.mkdir("/SYNC");}
   EnsureSyncStorage();
   AuditId=ReadConfigParam1("/ActLog.csv");WriteTopway_32(0x00,0x08,AuditId-1);//WriteTopway(0x00,0xfc,AuditId); //AuditId
}

 void CheckWiFi() {
   if (WiFi.status() != WL_CONNECTED) {
     WiFiStatus=false;
     LastSyncWiFiStatus=false;
     Set_Bit_Icons(1,0);
   } else {
     if(LastSyncWiFiStatus==false) {
       NotifySyncEvent();
     }
     WiFiStatus=true;
     LastSyncWiFiStatus=true;
   }
 }


//void CheckWiFi() {
//  if (WiFi.status() != WL_CONNECTED) {
//    WiFiStatus = false;
//    ntpSynced = false;  // Mark as unsynced so it re-syncs on reconnect
//    Set_Bit_Icons(1, 0);
//  } else {
//    WiFiStatus = true;
//
//    // Re-sync on first connect after drop
//    if (!ntpSynced) {
////      SyncTimeFromNTP();
//    }
//    // Periodic re-sync every NTP_SYNC_INTERVAL (e.g., every hour)
//    else if (millis() - lastNtpSync >= NTP_SYNC_INTERVAL) {
//      SyncTimeFromNTP();
//    }
//  }
//}

bool readWiFiCredentials(const char* path, String& ssid, String& password2) {
  File file = SD.open(path);
  if (!file) {
//    Serial.println("Could not open WiFi credentials file.");
    return false;
  }

  String line = file.readStringUntil('\n');
  file.close();

  int commaIndex = line.indexOf(',');
  if (commaIndex == -1) return false;

  ssid = line.substring(0, commaIndex);
  password2 = line.substring(commaIndex + 1);
  password2.trim();  // Remove any trailing newlines or spaces

//  Serial.printf("Read SSID: %s, Password: %s\n", ssid.c_str(), password.c_str());
  return true;
}

void CheckLogin() {
  Read_String(0x00, 0x80); UserId1 = Str2;
  Read_String(0x01, 0x00); Password1 = Str2;
  WriteTopway(0x00, 0xcc, 0);
  
  // Admin accounts (skip file check)
  if (UserId1 == "GService") {
    UserId = UserId1;
    Password = generatePassword();
    WriteString(0x00, 0x0e, 0x80, Password.c_str());
    

    if (Password == Password1) {
      WriteString(0x00, 0x21, 0x00, UserId1); DisplayPage(2);
      Group = "GService"; WriteTopway(0x00, 0xcc, 1);
      LogActivity("UserLogin"); getLastUserLogs();
      SetPrevil_GService();
    } else {
      LogActivity("InvalidPassword"); WriteTopway(0x00, 0x68, 55);
    }
    return;
  } else if (UserId1 == "SAdmin" || UserId1 == "SIIV") {
    if(Password1 == "1234") {
      UserId = UserId1;
      WriteString(0x00, 0x21, 0x00, UserId1); DisplayPage(2);
      Group = "SuperAdmin"; if (UserId1 == "SIIV") WriteTopway(0x00, 0xcc, 1);
      LogActivity("UserLogin"); getLastUserLogs();
      SetPrevil_SAdmin();
    } else {
      UserId = UserId1;
      LogActivity("InvalidPassword"); WriteTopway(0x00, 0x68, 55);
      UserId = "NO_USER";
    }
    return;
  }

  
  // Normal user login check
  bool loginMatched = false;
  bool loginSuccess = false;
  bool needsUpdate = false;

  myFile = SD.open("/USR.csv", FILE_READ);
  if (!myFile) return;

  // MEMORY PROTECTION: Reserve string memory outside loop
  String line;
  line.reserve(128);

  while (myFile.available()) {
    line = myFile.readStringUntil('\n');
    line.replace("\r", ""); line.trim();

    if(line.length() == 0) continue;

    int pos1 = line.indexOf(',');
    int pos2 = line.indexOf(',', pos1 + 1);
    int pos3 = line.indexOf(',', pos2 + 1);
    int pos4 = line.indexOf(',', pos3 + 1);
    int pos5 = line.indexOf(',', pos4 + 1);
    int pos6 = line.indexOf(',', pos5 + 1);
    int pos7 = line.indexOf(',', pos6 + 1);
    int pos8 = line.indexOf(',', pos7 + 1);
    int pos9 = line.indexOf(',', pos8 + 1);
    int pos10 = line.indexOf(',', pos9 + 1);

    if (pos10 == -1) continue;

    String user = line.substring(pos1 + 1, pos2);
    String pass = line.substring(pos2 + 1, pos3);
    String grp  = line.substring(pos6 + 1, pos7);
    String stat = line.substring(pos7 + 1, pos8);
    int flogbit = line.substring(pos8 + 1, pos9).toInt();
    String retry = line.substring(pos9 + 1, pos10);
    ExpDate = line.substring(pos10 + 1);
    if (user == UserId1) {
      loginMatched = true;
      Group = grp;
      GroupPrevil();
      if (pass == Password1 && stat == "Active") {
        if(flogbit==1) {
          CheckUserExpiry();
          if(PasswordExpired==false) {
            UserId = user; Password = pass; Group = grp;
            WriteString(0x00, 0x21, 0x00, UserId1); DisplayPage(2);
            LogActivity("UserLogin"); getLastUserLogs();
            if (retry != "0") needsUpdate = true;
            loginSuccess = true;
            SetPrevil();
          }
        } else {
          UserId = user; Password = pass; Group = grp;
          WriteString(0x00, 0x21, 0x00, UserId1);
          //WriteString(0x00, 0x2d, 0x00, PasswordComplexity1);
          DisplayPage(189);
        }
      }
      else if (pass == Password1 && stat == "Locked") {
        UserId = UserId1;
        LogActivity("LoginFailedUserLocked"); WriteTopway(0x00, 0x68, 22);
        UserId = "NO_USER";
      }
      else if (stat == "Discard") {    //(pass == Password1)
        UserId = UserId1;
        LogActivity("LoginFailedUserDiscarded"); WriteTopway(0x00, 0x68, 11);
        UserId = "NO_USER";
      }
      else if (pass != Password1){  // Incorrect password
        needsUpdate = true;
        int retryCount = retry.toInt() + 1;
        if (retryCount >= GroupRetryLimits1) { //3
          UserId = UserId1;
          LogActivity("InvalidLoginUserLocked"); WriteTopway(0x00, 0x68, 22);
          UserId = "NO_USER";
        } else {
          UserId = UserId1;
          LogActivity("InvalidPassword"); WriteTopway(0x00, 0x68, 55);
          UserId = "NO_USER";
        }
      }
      break;
    }
  }
  myFile.close();

  if (!loginMatched) {
    LogActivity("InvalidUserId");
    WriteTopway(0x00, 0x68, 1);
  }

  if (PasswordExpired) {
    LogActivity("InvalidLoginPasswordExpired");
    strGUser = UserId1;              // remember which user needs to reset
    PasswordResetOnExpiry = true;    // arm the self-reset flow
    WriteTopway(0x01, 0x12, 1);     // show "Password Expired" popup on Topway
  }

  // 🔁 Update retry count / lock only if needed
  if (loginMatched && needsUpdate)
    UpdateRetryOrLock(UserId1, Password1, loginSuccess);
}

void SetPrevil() {
  if(Group_User_Setting1=="View") {WriteTopway(0x01, 0x02, 0);} else {WriteTopway(0x01, 0x02, 1);}
  if(Group_User_Setting1=="No") {WriteTopway(0x01, 0x06, 0);} else {WriteTopway(0x01, 0x06, 1);}
  if(Config_Setting1=="View") {WriteTopway(0x01, 0x08, 0);} else {WriteTopway(0x01, 0x08, 1);}
  if(Config_Setting1=="No") {WriteTopway(0x01, 0x0a, 0);} else {WriteTopway(0x01, 0x0a, 1);}
  if(FixedVolume1=="Yes") {WriteTopway(0x00, 0xfe, 0);} else {WriteTopway(0x00, 0xfe, 1);}   //Fixed Volume
  if(Allow_Sampling1=="No") {WriteTopway(0x01, 0x0e, 0);} else {WriteTopway(0x01, 0x0e, 1);}
  if(ResetPassAuth1=="No") {WriteTopway(0x01, 0x10, 1);} else {WriteTopway(0x01, 0x10, 0);}
  if(Audit_Trail_View1=="No") {WriteTopway(0x01, 0x18, 0);} else {WriteTopway(0x01, 0x18, 1);}
  if(Sample_Data_View1=="No") {WriteTopway(0x01, 0x1a, 0);} else {WriteTopway(0x01, 0x1a, 1);}
  Max_Logout_Time=Logout_Timer2*60;
  Max_Poweroff_Time=Power_Off_Timer2*60;
}

void SetPrevil_SAdmin() {
//    WriteTopway(0x01, 0x02, 1);WriteTopway(0x01, 0x06, 1); //Group User Settong
//    WriteTopway(0x01, 0x08, 1);WriteTopway(0x01, 0x0a, 1); //Config Setting
  Group_User_Setting1="Setting";
  Config_Setting1="Setting";
  FixedVolume1="No";
  Allow_Sampling1="Yes";
  ResetPassAuth1="Yes";
  Logout_Timer2=5;
  Power_Off_Timer2=5;
  Audit_Trail_View1="All";
  Sample_Data_View1="All";
  SetPrevil();
}

void SetPrevil_GService() {
  Group_User_Setting1="View";
  Config_Setting1="View";
  FixedVolume1="No";
  Allow_Sampling1="No";
  ResetPassAuth1="Yes";
  Logout_Timer2=5;
  Power_Off_Timer2=5;
  Audit_Trail_View1="All";
  Sample_Data_View1="All";
  SetPrevil();
}

void UpdateRetryOrLock(String targetUser, String inputPassword, bool loginSuccess) {
  bool LockedSt1=false;
  myFile = SD.open("/USR.csv", FILE_READ);
  tempFile = SD.open("/temp.csv", O_WRITE | O_CREAT | O_TRUNC);
  if (!myFile || !tempFile) {
     if(myFile) myFile.close();
     if(tempFile) tempFile.close();
     return;
  }

  // MEMORY PROTECTION
  String line;
  line.reserve(128);

  while (myFile.available()) {
    line = myFile.readStringUntil('\n');
    line.replace("\r", ""); line.trim();

    int pos1 = line.indexOf(',');
    int pos2 = line.indexOf(',', pos1 + 1);
    int pos3 = line.indexOf(',', pos2 + 1);
    int pos4 = line.indexOf(',', pos3 + 1);
    int pos5 = line.indexOf(',', pos4 + 1);
    int pos6 = line.indexOf(',', pos5 + 1);
    int pos7 = line.indexOf(',', pos6 + 1);
    int pos8 = line.indexOf(',', pos7 + 1);
    int pos9 = line.indexOf(',', pos8 + 1);
    int pos10 = line.indexOf(',', pos9 + 1);

    if (pos10 == -1) {
      tempFile.println(line); continue;
    }

    String ID = line.substring(0, pos1);
    String user = line.substring(pos1 + 1, pos2);
    String pass = line.substring(pos2 + 1, pos3);
    String name = line.substring(pos3 + 1, pos4);
    String ln   = line.substring(pos4 + 1, pos5);
    String dept = line.substring(pos5 + 1, pos6);
    String grp  = line.substring(pos6 + 1, pos7);
    String stat = line.substring(pos7 + 1, pos8);
    String firstLogin = line.substring(pos8 + 1, pos9);
    String retry = line.substring(pos9 + 1, pos10);
    String dt    = line.substring(pos10 + 1);

    if (user == targetUser) {
      if (loginSuccess) {
        retry = "0";  // Reset on success
      } else {
        int retryCount = retry.toInt() + 1;
        retry = String(retryCount);
        if (retryCount >= GroupRetryLimits1) {
          stat = "Locked";
          if(retryCount==GroupRetryLimits1) {
            //LogActivity("UserLocked");
            LockedSt1=true;
          }
        }
      }
    }

    String newRow = ID + "," + user + "," + pass + "," + name + "," + ln + "," +
                    dept + "," + grp + "," + stat + "," + firstLogin + "," + retry + "," + dt;
    tempFile.println(newRow);
  }

  myFile.close(); 
  
  // ATOMIC SWAP
  tempFile.flush();
  tempFile.close();
  
  SD.remove("/USR.csv");
  SD.rename("/temp.csv", "/USR.csv");
  
  if(LockedSt1==true) {
    UserId=targetUser;
    LogActivity("UserLocked");
    UserId="NO_USER";
    LockedSt1=false;
  }
}

void Reset_User_Cred() {
  WriteString(0x00, 0x00, 0x80, "");
  WriteString(0x00, 0x01, 0x00, "");
}

void Reset_User_Cred1() {
  WriteString(0x00, 0x1d, 0x00, "");
  WriteString(0x00, 0x1d, 0x80, "");
  WriteString(0x00, 0x1e, 0x00, "");
}

void CheckLogin1() {
  String ActUserId;
  ActUserId = UserId;

  Read_String(0x1d,0x00);UserId1=Str2;
  Read_String(0x1d,0x80);Password1=Str2;
  Read_String(0x1e,0x00);SamplingRemark=Str2;
  
  if(UserId1=="GService") {
    UserId=UserId1;
    Password=generatePassword();
    WriteString(0x00, 0x0e, 0x80, Password.c_str());
    if(Password==Password1) {
      Sample_Abort();SamplingRemark="NO_RMK";
      UserId = ActUserId;
      return;
    }
    LogActivity("AbortFailed");
    WriteTopway(0x00,0x68,1);
    UserId = ActUserId;
  } else if(UserId1=="SAdmin" && Password1 == "1234") {
    UserId = UserId1;
    Sample_Abort();SamplingRemark="NO_RMK";
    UserId = ActUserId;
    return;
    LogActivity("AbortFailed");
    WriteTopway(0x00,0x68,1);
    UserId = ActUserId;
  } else {
    myFile = SD.open("/USR.csv", FILE_READ);
    if (!myFile) {
      
    }
  
    // MEMORY PROTECTION
    String line;
    line.reserve(128);

    while (myFile.available()) {
      line = myFile.readStringUntil('\n');  // Read a line from the file
      line.trim();  // Remove any trailing whitespace or newline characters
  
      // Split line into username and password
      pos1 = line.indexOf(',');
      pos2 = line.indexOf(',', pos1 + 1);
      pos3 = line.indexOf(',', pos2 + 1);
      pos4 = line.indexOf(',', pos3 + 1);
      pos5 = line.indexOf(',', pos4 + 1);
      pos6 = line.indexOf(',', pos5 + 1);
      pos7 = line.indexOf(',', pos6 + 1);
      pos8 = line.indexOf(',', pos7 + 1);
      pos9 = line.indexOf(',', pos8 + 1);
      pos10 = line.indexOf(',', pos9 + 1);
      
      if (pos1 == -1 || pos2 == -1 || pos3 == -1 || pos4 == -1 || pos5 == -1 || pos6 == -1 || pos7 == -1 || pos8 == -1 || pos9 == -1 || pos10 == -1) {
        continue;  // Skip malformed lines
      }
      UserId = line.substring(pos1 + 1, pos2);
      Password = line.substring(pos2 + 1, pos3);
      Group = line.substring(pos6 + 1, pos7);
      
      if(UserId==UserId1 && Password==Password1) {
        Sample_Abort();SamplingRemark="NO_RMK";
        UserId = ActUserId;
        return;
      }
    }
    myFile.close();
    UserId = "NA";
    LogActivity("AbortFailed");
    WriteTopway(0x00,0x68,1);
    UserId = ActUserId;
  }
  SamplingRemark="NO_RMK";
  Set_In_Bit(2,0);
}

void CheckPrevil() {
  bool UserCreation;
  bool UserEdit;
  bool UserView;
  
  if(Group=="Admin") {
    UserCreation=true;
    UserEdit=true;
  }
}

void CheckFirstLogin() {
  UserSrNo = line.substring(0, pos1).toInt();
  UserFirstName = line.substring(pos3 + 1, pos4);
  UserLastName = line.substring(pos4 + 1, pos5);
  UserDept = line.substring(pos5 + 1, pos6);
  UserGroup = line.substring(pos6 + 1, pos7);
  UserStatus = line.substring(pos7 + 1, pos8);
  LoginBit = (line.substring(pos8 + 1, pos9) == "1");
  RetryLimits = (byte) line.substring(pos9 + 1, pos10).toInt();
  ExpDate = line.substring(pos10 + 1, pos11);
}

void getDeviceDetails() {

  const char *finalFile = "/DevInf.csv";
  const char *tempFile  = "/DevInf.tmp";

  char line[128] = {0};

  /* ================= READ FILE ================= */

  myFile = SD.open(finalFile, FILE_READ);

  if (!myFile) {
    // ---------- FILE MISSING → CREATE SAFELY ----------

    if (SD.exists(tempFile)) SD.remove(tempFile);

    myFile = SD.open(tempFile, O_WRITE | O_CREAT | O_TRUNC);
    if (myFile) {
      noInterrupts();
      myFile.print(ModelNo);        myFile.print(",");
      myFile.print(SoftwareVer);   myFile.print(",");
      myFile.print(UnitName);      myFile.print(",");
      myFile.print(EquipmentId);   myFile.print(",");
      myFile.println(deviceID);
      
      myFile.flush();
      myFile.close();
      interrupts();

      if (SD.exists(finalFile)) SD.remove(finalFile);
      SD.rename(tempFile, finalFile);
    }

    goto DISPLAY_UPDATE;
  }

  {
  /* ---------- READ FIRST LINE ONLY ---------- */
  size_t len = myFile.readBytesUntil('\n', line, sizeof(line) - 1);
  myFile.close();

  if (len < 10) goto LOAD_DEFAULTS;   // too short → corrupted

  /* ================= PARSE CSV ================= */

  char *p1 = strtok(line, ",");
  char *p2 = strtok(NULL, ",");
  char *p3 = strtok(NULL, ",");
  char *p4 = strtok(NULL, ",");
  char *p5 = strtok(NULL, ",");

  if (!p1 || !p2 || !p3 || !p4 || !p5) goto LOAD_DEFAULTS;

  ModelNo     = p1;
  SoftwareVer = p2;
  UnitName    = p3;
  EquipmentId = p4;
  deviceID    = p5;

  }

  goto DISPLAY_UPDATE;

  /* ================= DEFAULTS ================= */

LOAD_DEFAULTS:
  ModelNo     = "AMS-G100-1BCF";
  SoftwareVer = "AMS-V01";
  UnitName    = "UNIT-1";
  EquipmentId = "250001";
  deviceID    = "G00250001";

  /* ================= DISPLAY ================= */

DISPLAY_UPDATE:
  WriteString(0x00, 0x06, 0x80, ModelNo);
  WriteString(0x00, 0x07, 0x00, SoftwareVer);
  WriteString(0x00, 0x07, 0x80, UnitName);
  WriteString(0x00, 0x08, 0x00, EquipmentId);
  WriteString(0x00, 0x08, 0x80, deviceID);
  WriteString(0x00, 0x20, 0x00, UnitName);
  WriteString(0x00, 0x20, 0x80, EquipmentId);

  UnitName1    = UnitName;
  EquipmentId1 = EquipmentId;
  deviceID1    = deviceID;
}

void getLastDeviceLogs() {
  if (!SD.exists("/DLS.csv")) return;

  myFile = SD.open("/DLS.csv", FILE_READ);
  if (!myFile) {
    return;
  }

  String line = myFile.readStringUntil('\n');  // Read a line from the file
  line.trim();  // Remove any trailing whitespace or newline characters

  // Split line into values using comma as a delimiter
  pos1 = line.indexOf('/');
  pos2 = line.indexOf(',', pos1 + 1);
  pos3 = line.indexOf(',', pos2 + 1);
  pos4 = line.indexOf(',', pos3 + 1);
  pos5 = line.indexOf(',', pos4 + 1);
  pos6 = line.indexOf(',', pos5 + 1);
  pos7 = line.indexOf(',', pos6 + 1);
  pos8 = line.indexOf(',', pos7 + 1);
  pos9 = line.indexOf(',', pos8 + 1);
  pos10 = line.indexOf(',', pos9 + 1);
  pos11 = line.indexOf(',', pos10 + 1);
  pos12 = line.indexOf(',', pos11 + 1);

//  if (pos1 == -1 || pos2 == -1 || pos3 == -1 || pos4 == -1 || pos5 == -1 || pos6 == -1 || pos7 == -1 || pos8 == -1 || pos9 == -1 || pos10 == -1 || pos11 == -1 || pos1 == -1 || pos13 == -1) {
//    return;  // Skip malformed lines
//  }

  LastSampleId = (pos1 != -1) ? line.substring(0, pos1) : "";
  LastSampleName = (pos2 != -1) ? line.substring(pos1 + 1, pos2) : "";
  LastUserId = (pos3 != -1) ? line.substring(pos2 + 1, pos3) : "";
  LastSampleMode = (pos4 != -1) ? line.substring(pos3 + 1, pos4) : "";
  LastSampleStart = (pos5 != -1) ? line.substring(pos4 + 1, pos5) : "";
  LastSampleEnd = (pos6 != -1) ? line.substring(pos5 + 1, pos6) : "";
  LastSampleStatus = (pos7 != -1) ? line.substring(pos6 + 1, pos7) : "";
  LastSampleRemark = (pos8 != -1) ? line.substring(pos7 + 1, pos8) : "NO_RMK";
  LastSampleLocation = (pos9 != -1) ? line.substring(pos8 + 1, pos9) : "NO_LOC";
  LastVolume = (pos10 != -1) ? line.substring(pos9 + 1, pos10).toInt() : 0;
  LastStartDelay = (pos11 != -1) ? line.substring(pos10 + 1, pos11).toInt() : 0;
  LastNoofSamples = (pos12 != -1) ? line.substring(pos11 + 1, pos12).toInt() : 0;
  LastDelaybetwRuns = line.substring(pos12 + 1).toInt();
  
  myFile.close();
  WriteString(0x00,0x09,0x00,LastUserId);
  WriteString(0x00,0x09,0x80,LastSampleId);
  WriteString(0x00,0x0A,0x00,LastSampleName);
  WriteString(0x00,0x0A,0x80,LastSampleMode);
  WriteString(0x00,0x0B,0x00,LastSampleStart);
  WriteString(0x00,0x0B,0x80,LastSampleEnd);
  WriteString(0x00,0x0C,0x00,LastSampleStatus);
}

void getLastUserLogs() {
  String UserFileName="/" + UserId + "-LS.csv";
  if (!SD.exists(UserFileName)) return;

  myFile = SD.open(UserFileName, FILE_READ);
  if (!myFile) {
    return;
  }

  String line = myFile.readStringUntil('\n');  // Read a line from the file
  line.trim();  // Remove any trailing whitespace or newline characters

  // Split line into values using comma as a delimiter
  pos1 = line.indexOf('/');
  pos2 = line.indexOf(',', pos1 + 1);
  pos3 = line.indexOf(',', pos2 + 1);
  pos4 = line.indexOf(',', pos3 + 1);
  pos5 = line.indexOf(',', pos4 + 1);
  pos6 = line.indexOf(',', pos5 + 1);
  pos7 = line.indexOf(',', pos6 + 1);
  pos8 = line.indexOf(',', pos7 + 1);
  pos9 = line.indexOf(',', pos8 + 1);
  pos10 = line.indexOf(',', pos9 + 1);
  pos11 = line.indexOf(',', pos10 + 1);
  pos12 = line.indexOf(',', pos11 + 1);

//  if (pos1 == -1 || pos2 == -1 || pos3 == -1 || pos4 == -1 || pos5 == -1 || pos6 == -1 || pos7 == -1 || pos8 == -1 || pos9 == -1 || pos10 == -1 || pos11 == -1 || pos1 == -1 || pos13 == -1) {
//    return;  // Skip malformed lines
//  }

  UserLastSampleMode = (pos4 != -1) ? line.substring(pos3 + 1, pos4) : "";
  UserLastSampleLocation = (pos9 != -1) ? line.substring(pos8 + 1, pos9) : "NO_LOC";
  UserLastVolume = (pos10 != -1) ? line.substring(pos9 + 1, pos10).toInt() : 0;
  UserLastStartDelay = (pos11 != -1) ? line.substring(pos10 + 1, pos11).toInt() : 0;
  UserLastNoofSamples = (pos12 != -1) ? line.substring(pos11 + 1, pos12).toInt() : 0;
  UserLastDelaybetwRuns = line.substring(pos12 + 1).toInt();
  myFile.close();
}

void getTime() {
  Str1="";
  if(now1.day()<10) {Str1='0';}
  Str1=Str1+now1.day()+"/";
  if(now1.month()<10) {Str1+='0';}
  Str1=Str1+now1.month()+"/"+now1.year();
  currentDate=Str1;
  Str1+=" ";
  if(now1.hour()<10) {Str1+='0';}
  Str1=Str1+now1.hour()+":";
  if(now1.minute()<10) {Str1+='0';}
  Str1=Str1+now1.minute()+":";
  if(now1.second()<10) {Str1+='0';}
  Str1=Str1+now1.second();
}

void getTime1() {
  DateTime futureTime = now1 + TimeSpan(Motor_Counter_Max);
  Str3 = "";
  if(futureTime.day() < 10) Str3 += '0';
  Str3 += String(futureTime.day()) + "/";
  if(futureTime.month() < 10) Str3 += '0';
  Str3 += String(futureTime.month()) + "/";
  Str3 += String(futureTime.year()) + " ";
  if(futureTime.hour() < 10) Str3 += '0';
  Str3 += String(futureTime.hour()) + ":";
  if(futureTime.minute() < 10) Str3 += '0';
  Str3 += String(futureTime.minute()) + ":";
  if(futureTime.second() < 10) Str3 += '0';
  Str3 += String(futureTime.second());
}

void LogActivity(String strAct) {
  if(UserId=="SIIV") {
    AuditDetails="NA";AuditRemark="NA";
    return;
  }
  MarkSyncSdQuiet();
  getTime();
  ActivityTime=Str1;
  if(ActivityTime!="165/165/165 165:165:85") {
    Activity=strAct;
    if(SdLock(SD_LOCK_CRITICAL_TIMEOUT_MS)) {
      myFile = SD.open("/ActLog.csv", O_RDWR | O_CREAT | O_APPEND);
      if (myFile) {
        myFile.print(AuditId);myFile.print(",");
        myFile.print(UserId);myFile.print(",");
        myFile.print(Activity);myFile.print(",");
        myFile.print(ActivityTime);myFile.print(",");
        myFile.print(AuditDetails);myFile.print(",");
        myFile.println(AuditRemark);
        
        myFile.flush(); // Force write
        myFile.close();
        
        AuditId++;
        WriteTopway_32(0x00,0x08,AuditId-1);
        //WriteTopway(0x00,0xfc,AuditId);
        AuditDetails="NA";AuditRemark="NA";
        MarkSyncSdQuiet();
        NotifySyncEvent();
      }
      SdUnlock();
    }
  }
}

void LogBatParameters() {
  getTime();
  ActivityTime=Str1;
  myFile = SD.open("/BatParam.csv", O_RDWR | O_CREAT | O_APPEND);
 
  if (myFile) {
    myFile.print(ActivityTime);myFile.print(",");
    myFile.print(BAT_VTG);myFile.print(",");
    myFile.print(BAT_PER);myFile.print(",");
    myFile.println(CHG_VTG);
    
    myFile.flush();
    myFile.close();
  } else {
    //Serial.println("error opening dataLog.csv");
  }
}

void Log_Report(String Status1) {
  if(UserId=="SIIV" || (OnlineStatus==true && onlineUser=="SIIV")) {
    // updateSampleID(); Change as per pranil sir call
    SamplingRemark="NO_RMK";
    if(OnlineStatus==true) {UserId="NO_USER";}
    return;
  }
  MarkSyncSdQuiet();
  if(OnlineStatus==true) {SamplingRemark="OnlineSampling";UserId=onlineUser;}
  SamplingStatus=Status1;
  if(SamplingStatus=="OK") {
    SamplingEnd=Str3;
  } else {
    getTime();SamplingEnd=Str1;
  }
  
  if(SdLock(SD_LOCK_CRITICAL_TIMEOUT_MS)) {
    myFile = SD.open("/FR.csv", O_RDWR | O_CREAT | O_APPEND);
    
    if (myFile) {
      myFile.print(SampleId);myFile.print("/");
      myFile.print(SampleName);myFile.print(",");
      myFile.print(UserId);myFile.print(",");
      myFile.print(Mode);myFile.print(","); //LogParams
      myFile.print(SamplingStart);myFile.print(","); //LogParams
      myFile.print(SamplingEnd);myFile.print(","); //
      myFile.print(SamplingStatus);myFile.print(","); //
      myFile.print(SamplingRemark);myFile.print(",");
      myFile.print(SamplingLocation);myFile.print(",");
      myFile.print(Motor_lts);myFile.print(",");//LogParams
      myFile.print(Start_Delay1_1);myFile.print(","); //LogParams
      myFile.print(No_of_Runs1);myFile.print(","); //LogParams
      myFile.println(Delay_between_Runs1);//myFile.print(",");
      //myFile.println(ActivityTime); //Samplnig Error
      
      myFile.flush();
      myFile.close();
      //Serial.println("done.");
    }
    SdUnlock();
  }
  Log_DLS();
  Log_ULS();
  updateSampleID();
  SamplingRemark="NO_RMK";
  if(OnlineStatus==true) {UserId="NO_USER";}
  MarkSyncSdQuiet();
  NotifySyncEvent();
}

void Log_DLS() { 
  const char *tmpFile = "/DLS.tmp";
  const char *finalFile = "/DLS.csv";

  if(SdLock(SD_LOCK_CRITICAL_TIMEOUT_MS)) {
    if (SD.exists(tmpFile)) SD.remove(tmpFile);

    // ATOMIC WRITE STRATEGY
    myFile = SD.open(tmpFile, O_WRITE | O_CREAT | O_TRUNC);

    if (myFile) {
      myFile.print(SampleId); myFile.print("/");
      myFile.print(SampleName); myFile.print(",");
      myFile.print(UserId); myFile.print(",");
      myFile.print(Mode); myFile.print(",");
      myFile.print(SamplingStart); myFile.print(",");
      myFile.print(SamplingEnd); myFile.print(",");
      myFile.print(SamplingStatus); myFile.print(",");
      myFile.print(SamplingRemark); myFile.print(",");
      myFile.print(SamplingLocation); myFile.print(",");
      myFile.print(Motor_lts); myFile.print(",");
      myFile.print(Start_Delay1_1); myFile.print(",");
      myFile.print(No_of_Runs1); myFile.print(",");
      myFile.println(Delay_between_Runs1);
      
      myFile.flush();
      myFile.close();
      
      if (SD.exists(finalFile)) SD.remove(finalFile);
      SD.rename(tmpFile, finalFile);
    }
    SdUnlock();
  }
  getLastDeviceLogs();
}

void Log_ULS() { 
  // Build file name
  String UserFileName = "/" + UserId + "-LS.csv";
  String TempFileName = "/" + UserId + "-LS.tmp";

  if(SdLock(SD_LOCK_CRITICAL_TIMEOUT_MS)) {
    if (SD.exists(TempFileName)) SD.remove(TempFileName);

    // ATOMIC WRITE STRATEGY
    myFile = SD.open(TempFileName, O_WRITE | O_CREAT | O_TRUNC);

    if (myFile) {
      myFile.print(SampleId); myFile.print("/");
      myFile.print(SampleName); myFile.print(",");
      myFile.print(UserId); myFile.print(",");
      myFile.print(Mode); myFile.print(",");
      myFile.print(SamplingStart); myFile.print(",");
      myFile.print(SamplingEnd); myFile.print(",");
      myFile.print(SamplingStatus); myFile.print(",");
      myFile.print(SamplingRemark); myFile.print(",");
      myFile.print(SamplingLocation); myFile.print(",");
      myFile.print(Motor_lts); myFile.print(",");
      myFile.print(Start_Delay1_1); myFile.print(",");
      myFile.print(No_of_Runs1); myFile.print(",");
      myFile.println(Delay_between_Runs1); 

      myFile.flush();
      myFile.close();
      
      if (SD.exists(UserFileName)) SD.remove(UserFileName);
      SD.rename(TempFileName, UserFileName);
    }
    SdUnlock();
  }
  getLastUserLogs();
}

void ReadSampleId() {
  SampleId = ReadConfigParam1("/FR.csv");
  if(SampleId>99999) {SampleName="S000" + String(SampleId);}
  else if(SampleId>9999) {SampleName="S0000" + String(SampleId);}
  else if(SampleId>999) {SampleName="S00000" + String(SampleId);}
  else if(SampleId>99) {SampleName="S000000" + String(SampleId);}
  else if(SampleId>9) {SampleName="S0000000" + String(SampleId);}
  else {SampleName="S00000000" + String(SampleId);}
  
  WriteTopway(0x00,0x32,SampleId);
  WriteString(0x00,0x01,0x80,SampleName);
}

void updateSampleID() {
  SampleId+=1;
  if(SampleId>99999) {SampleName="S000" + String(SampleId);}
  else if(SampleId>9999) {SampleName="S0000" + String(SampleId);}
  else if(SampleId>999) {SampleName="S00000" + String(SampleId);}
  else if(SampleId>99) {SampleName="S000000" + String(SampleId);}
  else if(SampleId>9) {SampleName="S0000000" + String(SampleId);}
  else {SampleName="S00000000" + String(SampleId);}
  WriteTopway(0x00,0x32,SampleId);
  WriteString(0x00,0x01,0x80,SampleName);
}

  String generatePassword() {
    String data = deviceID + currentDate;
    String password = "";

    unsigned long hash = 5381;  // Simple hash function
    for (int i = 0; i < data.length(); i++) {
      hash = ((hash << 5) + hash) + data[i];  // hash * 33 + char
    }

    // Generate a 6-character alphanumeric password
    for (int i = 0; i < 6; i++) {
      char c = (hash >> (i * 5)) % 62;  // Use bits from hash to get values
      if (c < 10) {
        password += char('0' + c);  // Numbers 0-9
      } else if (c < 36) {
        password += char('A' + (c - 10));  // Uppercase A-Z
      } else {
        password += char('a' + (c - 36));  // Lowercase a-z
      }
    }

    return password;
  }

void ReadCal() {
//  now1 = rtc.now();

  File myFile = SD.open("/CAL.csv", FILE_READ);
  if (!myFile) {
//    Serial.println("Failed to open CAL.csv");
    return;
  }

  String lastLine = "";
  // MEMORY PROTECTION: Reduce fragmentation
  lastLine.reserve(64); 
  
  while (myFile.available()) {
    String temp = myFile.readStringUntil('\n');  // Store the last line
    temp.trim();
    if(temp.length() > 0) {
        lastLine = temp;
    }
  }
  myFile.close();

  if (lastLine.length() == 0) {
//    Serial.println("CAL.csv is empty.");
    return;
  }

  // Split the line by comma
  pos1 = lastLine.indexOf(',');
  pos2 = lastLine.indexOf(',', pos1 + 1);
  pos3 = lastLine.indexOf(',', pos2 + 1);

  String paramIdStr = lastLine.substring(0, pos1);
//  String calPwmStr  = lastLine.substring(pos1 + 1, pos2);
  calDateStr = lastLine.substring(pos2 + 1, pos3);
  dueDateStr = lastLine.substring(pos3 + 1);
  WriteString(0x00,0x1a,0x00,calDateStr);
  WriteString(0x00,0x1a,0x80,dueDateStr);
}

void ReadCalValidity() {
  // Convert DueDate to DateTime
  int d = dueDateStr.substring(0, 2).toInt();
  int m = dueDateStr.substring(3, 5).toInt();
  int y = dueDateStr.substring(6).toInt();

  DateTime dueDate(y, m, d);
  TimeSpan diff = dueDate - now1;

  if (diff.totalseconds() < 0) {
    WriteTopway(0x00,0x98,2); //Calibration Expired Popup
    
  } else if (diff.days() < 10) {
    WriteTopway(0x00,0x9a,diff.days());
    WriteTopway(0x00,0x98,3); //Calibration will expire in X days Popup
    CalStatus=true;
  } else {
    CalStatus=true;
  }
}

void CheckUserExpiry() {
  int d = ExpDate.substring(0, 2).toInt();
  int m = ExpDate.substring(3, 5).toInt();
  int y = ExpDate.substring(6).toInt();

  DateTime dueDate(y, m, d);
  TimeSpan diff = now1 - dueDate;
  if (diff.days() >= GroupPasswordExpiy1) {
      PasswordExpired=true;
  } else {
    PasswordExpired=false;
  }
}


// Update for NTP

//void SyncTimeFromNTP() {
//  if (WiFi.status() != WL_CONNECTED) return;
//
//  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
//
//  struct tm timeinfo;
//  int retries = 0;
//  
//  // Wait up to 5 seconds for NTP to respond
//  while (!getLocalTime(&timeinfo) && retries < 10) {
//    delay(500);
//    retries++;
//  }
//
//  if (retries < 10) {
//    // Valid time received — push to RTC
//    rtc.adjust(DateTime(
//      timeinfo.tm_year + 1900,
//      timeinfo.tm_mon + 1,
//      timeinfo.tm_mday,
//      timeinfo.tm_hour,
//      timeinfo.tm_min,
//      timeinfo.tm_sec
//    ));
//    ntpSynced = true;
//    lastNtpSync = millis();
//
//    Serial.printf("NTP Synced: %02d/%02d/%04d %02d:%02d:%02d\n",
//      timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900,
//      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
//
//    LogActivity("NTPTimeSynced");
//  } else {
//    Serial.println("NTP sync failed — keeping existing RTC time");
//  }
//}
