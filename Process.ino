void Motor_On_Off() {
  //digitalWrite(LED_Out, !digitalRead(LED_Out));

  // Check if the motor is supposed to be on
  if (Motor_Status1 == true) {
    Logout_Timer = 0;

    // Check if there is an initial start delay
    if (Start_Delay1 > 0) {
      Start_Delay1 -= 1;
      Motor_Counter -= 1;
      Write_Count_Down_Perc();
      Write_Count_Down();
    } else {
      // If there are runs left
      if (No_of_Runs > 0) {
        // Check if the motor should be running or in delay between runs
        if (Motor_Counter1 > 0) {
          Motor_Counter -= 1;
          Motor_Counter1 -= 1;
          Write_Count_Down_Perc();
          Write_Count_Down();
          RunMotor();
          CheckforError();
        } else {
          // Only enter delay if more runs are still pending
          if (No_of_Runs > 1) {
            // Delay between runs countdown
            if (Delay_between_Runs > 0) {
              //digitalWrite(OUT1, LOW);
              //digitalWrite(RL1, HIGH);
              Delay_between_Runs -= 1;
              Motor_Counter -= 1;
              Write_Count_Down_Perc();
              Write_Count_Down();
            } else {
              // Reset delay and decrement No_of_Runs
              Delay_between_Runs = Delay_between_Runs1; // Reset delay between runs
              No_of_Runs -= 1;
              Motor_Counter1 = Motor_Counter2; // Reset Motor Counter if needed
              RunMotor();
            }
          } else {
            // No runs left, stop the motor
            WriteString(0x00,0x03,0x80,"Completed");
            WriteTopway(0x00,0x5C,0);
            WriteTopway(0x00,0xBE,1);
            
            AuditDetails=SampleName;AuditRemark="OK";
            Log_Report("OK");            
            if(OnlineStatus==false) {LogActivity("SamplingStop");} else {AuditDetails=onlineUser + "," + AuditDetails;LogActivity("RemoteUserLogout");DisplayPage(1);}
              /*LogBatParameters();*/
              StopMotor();
              if(OnlineStatus==false) {WriteTopway(0x00,0xA0,1);}
              SampleRunString="Sample Completed";
          }
        }
      } else {
        StopMotor(); // Stop the motor after completing all runs
      }
    }
  } else {
    ErrorCounter=0;
  }
}

//Head Block 100 to 300
//Head Open 450 to 800
void CheckforError() { 
  // if(MTR_VTG==0) {if(ErrorCounter<=2) {ErrorCounter++;} else {Sample_Abort1();ErrorCounter=0;}} else {ErrorCounter=0;} //Motor Absent Error
  // if(MTR_CUR>Hblock_Min && MTR_CUR<Hblock_Max) {if(ErrorCounter1<=5) {ErrorCounter1++;} else {Sample_Abort2();ErrorCounter1=0;}} else {ErrorCounter1=0;} //Motor Head Block Error
  // if(MTR_CUR>Hopen_Min) {if(ErrorCounter2<=2) {ErrorCounter2++;} else {Sample_Abort3();ErrorCounter2=0;}} else {ErrorCounter2=0;} //Motor Head Block Error
}  // this change suggested as per call of pranil sir - for liva and zydus Amhdabad uncomment when it reuired 

void RunMotor() {
  if(Motor_Status1==true) {
    //digitalWrite(RL1,LOW);
    //analogWrite(OUT1,Motor_PWM1[0]);
  }
}

void StopMotor() {
  Motor_Status1=false;
  Set_In_Bit(1,0);
  //Buzz_Blink(1);
}

void process_Input_Bits() {
  for (i = 7; i >= 0; i--) {
    bitArray[i] = (temp_l >> i) & 1;  // Extract each bit from the low byte
  }
  for (int i = 7; i >= 0; i--) {
    bitArray[i + 8] = (temp_h >> i) & 1;  // Extract each bit from the high byte
  }
  if(bitArray[0]==1) {if(Page_No==1) {CheckLogin();Reset_User_Cred();} Write_In_Bit();} //Login Page 1
  if(bitArray[1]==1 && Motor_Status1==false) {Sample_Run();} //Start Sample Page 7/81
  if(bitArray[2]==1 && Motor_Status1==true) {CheckLogin1();Reset_User_Cred1();} //Abort Sample  Page 82
  if(bitArray[3]==1 && Cal_Status1==false) {if(Page_No==13) {CheckForCal();}Write_In_Bit();} //Start Calibration // Page 13
//  if(bitArray[4]==1 && Cal_Status1==true) {Cal_Status1=false;Save_Cal();} //Save Calibration Value Page 14
//  if(bitArray[5]==1 && Cal_Status1==true) {Cal_Status1=false;Exit_Cal();} //Exit Calibration Page 14
  if(bitArray[6]==1) {if(Page_No==3) {LogOut("UserLogout");} Write_In_Bit();} //User Logout
  if(bitArray[7]==1) {if(Page_No==1 || Page_No==3) {ShutDown("UserShutDown");} Write_In_Bit();} //User Shutdowm
  if(bitArray[8]==1) {Logout_Timer=0;WriteTopway(0x00,0x68,0);DisplayPage(Page_No);Write_In_Bit();} //Cancel Auto Logout, Reset Timer
  if(bitArray[9]==1) {Power_Off_Timer=0;WriteTopway(0x00,0x68,0);DisplayPage(1);Write_In_Bit();} //Cancel Autoshutdown, Reset Timer
  // if(bitArray[10]==1) {Logout_Timer=0;LogOut("AutoLogout");} //Auto Logout
  // Corrected logic for Auto Logout
  if(bitArray[10] == 1) { 
    // 1. Check if user is actually logged in before logging out
    if(UserId != "NO_USER") { 
        Logout_Timer = 0;
        LogOut("AutoLogout"); 
    }
    // 2. Clear the input bit so the loop doesn't run again
    Write_In_Bit(); 
  }
  // if(bitArray[11]==1) {ShutDown("AutoShutDown");} //Auto Shutdown
  // Corrected logic for Auto Shutdown (Bit 11)
  if(bitArray[11] == 1) {
    // 1. Check if shutdown is already in progress to prevent duplicates
    if(AutoShutdownbit == false) {
        ShutDown("AutoShutDown"); 
        AutoShutdownbit = true; // Lock the system so it doesn't log again
    }
    // 2. Clear the button signal on the screen immediately
    Write_In_Bit();
  }
  if(bitArray[12]==1) {SaveWiFiCred();Write_In_Bit();} //Connect to WiFi
  if(bitArray[13]==1) {updateClock();} //Set Date Time
  if(bitArray[14]==1) {CalButton();} //Calibration
  if(bitArray[15]==1) {SaveServerCred();Write_In_Bit();} //Connect to WiFi
//  if(bitArray[15]==1) {GetDeviceLastSample();} //Get Device Last Sample
//  if(Cal_Status1==true) {Run_Cal();}
}

void Sample_Run() {
  if(Page_No==7 || Page_No==81 || Page_No==206) {
    if(OnlineStatus==false && UserId=="NO_USER") {
      Motor_Status1=false;
      Write_In_Bit();
      WriteTopway(0x00,0x68,1);
      DisplayPage(1);
      return;
    }
    if(BAT_PER>20) {
      DisplayPage(8);
      WriteTopway(0x01,0x14,SampleId);
      WriteString(0x00,0x2c,0x80,SampleName);
      Motor_Status1=true;/*LogBatParameters();*/Motor_Bit_Loop();WriteSerial();
      AuditDetails=SampleName;AuditRemark="NA";
      if(OnlineStatus==false) {LogActivity("SamplingStart");} else {AuditDetails=onlineUser + "," + AuditDetails;LogActivity("RemoteUserLogin");SampleRunString="SAMPLING STARTED";}
    } else {
      if(OnlineStatus==false) {
      WriteTopway(0x00,0xb8,3);Write_In_Bit();//WriteTopway(0x00,0xb8,0);
      } else {
        SampleRunString="LOW BATTERY";
      }
    }
  }
}

void Process_Cal() {
  ReadTopway(0x00,0x12);
  if(Read_Err==false) {
    if(temp_l==3 && Cal_Status1==true) {if(Cal_PWM<100){Cal_PWM=Cal_PWM+1;WriteTopway(0x00,0x10,Cal_PWM);WriteTopway(0x00,0x12,0);}} //Increment Cal Value
    if(temp_l==6 && Cal_Status1==true) {if(Cal_PWM>20) {Cal_PWM=Cal_PWM-1;WriteTopway(0x00,0x10,Cal_PWM);WriteTopway(0x00,0x12,0);}} //Decrement Cal Vale
    if(temp_l==12 && Cal_Status1==true) {Cal_Status1=false;Save_Cal();} //Save Calibration Value Page 14
    if(temp_l==24 && Cal_Status1==true) {Cal_Status1=false;Exit_Cal();} //Exit Calibration Page 
    if(Cal_Status1==true) {Run_Cal();}
  }
}

void CalButton() {
  ReadCal();
  DisplayPage(13);
  Write_In_Bit();
}

void CheckForCal() {
  if(UserId=="GService" || UserId=="SIIV") {
    DisplayPage(14);
    Cal_Status1=true;Write_In_Bit();
  } else {
    WriteTopway(0x00,0xC0,1);
    Write_In_Bit();
  }
}

void SaveWiFiCred() {
  Read_String(0x05,0x80);wifiSSID=Str2;
  Read_String(0x06,0x00);wifiPWD=Str2;
  
  // ATOMIC WRITE: Write to temp file first
  const char *tmpFile = "/WIFICRED.tmp";
  const char *finalFile = "/WIFICRED.csv";

  if (SD.exists(tmpFile)) SD.remove(tmpFile);
  
  myFile = SD.open(tmpFile, O_WRITE | O_CREAT | O_TRUNC);
  
  if (myFile) {
    myFile.print(wifiSSID); myFile.print(",");
    myFile.println(wifiPWD);
    
    myFile.flush(); // Force write
    myFile.close(); // Close handle
    
    // Rename to final
    if (SD.exists(finalFile)) SD.remove(finalFile);
    SD.rename(tmpFile, finalFile);
  }

  AuditDetails=wifiSSID;AuditRemark="OK";
  LogActivity("Wi-FiNameSet");
  WriteTopway(0x00,0x68,4);
}

void SaveServerCred() {
  Read_String(0x2d,0x80);ServerIP=Str2;
  Read_String(0x2e,0x00);SeverPort=Str2;
  
  // ATOMIC WRITE
  const char *tmpFile = "/SERVCRED.tmp";
  const char *finalFile = "/SERVCRED.csv";

  if (SD.exists(tmpFile)) SD.remove(tmpFile);

  myFile = SD.open(tmpFile, O_WRITE | O_CREAT | O_TRUNC);
  if (myFile) {
    myFile.print(ServerIP); myFile.print(",");
    myFile.println(SeverPort);
    
    myFile.flush(); // Force write
    myFile.close(); 
    
    if (SD.exists(finalFile)) SD.remove(finalFile);
    SD.rename(tmpFile, finalFile);
  }

  AuditDetails=ServerIP;AuditRemark="OK";
  LogActivity("ServerDetSet");
  WriteTopway(0x00,0x68,4);
}

void SaveIPCred() {
  Read_String(0x2e,0x80);strStaticIP=Str2;
  Read_String(0x2f,0x00);strGateWay=Str2;
  Read_String(0x2f,0x80);strSubnet=Str2;
  Read_String(0x30,0x00);strDNS=Str2;
  Read_String(0x31,0x00);strMAC=Str2;
  
  // ATOMIC WRITE
  const char *tmpFile = "/staticIP.tmp";
  const char *finalFile = "/staticIP.csv";

  if (SD.exists(tmpFile)) SD.remove(tmpFile);

  myFile = SD.open(tmpFile, O_WRITE | O_CREAT | O_TRUNC);
  if (myFile) {
    myFile.print(strStaticIP); myFile.print(",");
    myFile.print(strSubnet); myFile.print(",");
    myFile.print(strGateWay); myFile.print(",");
    myFile.print(strDNS); myFile.print(",");
    myFile.println(strMAC);
    
    myFile.flush(); // Force write
    myFile.close();
    
    if (SD.exists(finalFile)) SD.remove(finalFile);
    SD.rename(tmpFile, finalFile);
  }

  AuditDetails=ServerIP;AuditRemark="OK";
  LogActivity("staticIPDetSet");
  WriteTopway(0x00,0x68,4);
}

void Sample_Abort() {
  Motor_Status1=false;Write_In_Bit();StopMotor();
  Serial2.print("ABORT");
  WriteString(0x00,0x03,0x80,"Sample Aborted");
  WriteTopway(0x00,0x5C,0); //Sample Abort Touch Disable
  if(OnlineStatus==false) {
    DisplayPage(2);
  } else {
    DisplayPage(206);
    SampleRunString="OnlineUserAbort";
  }
  AuditDetails=SampleName;AuditRemark="UserAbort";
  Log_Report("SampleAbort");
  LogActivity("SamplingAbort");/*LogBatParameters();*/
  WriteTopway(0x00,0xA0,1);
}

void Sample_Abort1() {
  Motor_Status1=false;StopMotor();
  Serial2.print("ABORT");
  WriteString(0x00,0x03,0x80,"Sample Aborted");
  WriteTopway(0x00,0x5C,0); //Sample Abort Touch Disable
//  DisplayPage(2);
  AuditDetails=SampleName;AuditRemark="Motor Absent";
  Log_Report("MotorAbsent");
  
  if(OnlineStatus==false) {
    WriteTopway(0x00,0xA0,1); //Sample Run Home Enable
    WriteTopway(0x00,0xBA,1);
  } else {
    AuditDetails=onlineUser + "," + AuditDetails;
    DisplayPage(206);
  }
  LogActivity("MotorAbsentAbort");/*LogBatParameters();*/
  SampleRunString="MotorAbsentAbort";
}

void Sample_Abort2() {
  Motor_Status1=false;StopMotor();
  Serial2.print("ABORT");
  WriteString(0x00,0x03,0x80,"Sample Aborted");
  WriteTopway(0x00,0x5C,0); //Sample Abort Touch Disable
//  DisplayPage(2);
  AuditDetails=SampleName;AuditRemark="Head Block";
  Log_Report("HeadBlock");
  
  if(OnlineStatus==false) {
    WriteTopway(0x00,0xA0,1); //Sample Run Home Enable
    WriteTopway(0x00,0xBA,2);
  } else {
    AuditDetails=onlineUser + "," + AuditDetails;
    SampleRunString="HeadBlockAbort";
    DisplayPage(206);
  }
  LogActivity("HeadBlockAbort");/*LogBatParameters();*/
  SampleRunString="HeadBlockAbort";
}

void Sample_Abort3() {
  Motor_Status1=false;StopMotor();
  Serial2.print("ABORT");
  WriteString(0x00,0x03,0x80,"Sample Aborted");
  WriteTopway(0x00,0x5C,0); //Sample Abort Touch Disable
//  DisplayPage(2);
  AuditDetails=SampleName;AuditRemark="Head Open";
  Log_Report("HeadOpen");
  
  if(OnlineStatus==false) {
    WriteTopway(0x00,0xA0,1); //Sample Run Home Enable
    WriteTopway(0x00,0xBA,3);
  } else {
    AuditDetails=onlineUser + "," + AuditDetails;
    SampleRunString="HeadOpenAbort";
    DisplayPage(206);
  }
  LogActivity("HeadOpenAbort");/*LogBatParameters();*/
  SampleRunString="HeadOpenAbort";
}

void ReadSerial() {
  if (Serial2.available()) {
    inputString = Serial2.readString(); // get the new byte:
    stringComplete = true;
    CheckInput();
    IPEnable=true;
  }
}

void CheckInput() {
  if (stringComplete) {
    if (inputString.startsWith("BATPARAM")) {
      Read_Bat_Param();
    } else {
      //WriteString(0x00,0x05,0x00,inputString);
    }

   String inputString = "";
    stringComplete = false; // clear the string:
  }
}

/*
 * Mode
 * SampleName           Read_String(0x01,0x80);
 * SamplingLocation    Read_String(0x04,0x80);
 * Motor_lts           ReadTopway(0x00,0x08);
 * Start_Delay1        ReadTopway(0x00,0x34);
 * No_of_Runs          ReadTopway(0x00,0x36);
 * Delay_between_Runs  ReadTopway(0x00,0x38);
 */

void Read_Bat_Param() {
    WriteString(0x00,0x05,0x00,inputString);
    pos1 = inputString.indexOf(',');
    pos2 = inputString.indexOf(',', pos1 + 1);
    pos3 = inputString.indexOf(',', pos2 + 1);
    pos4 = inputString.indexOf(',', pos3 + 1);

    if (pos1 == -1 || pos2 == -1 || pos3 == -1 || pos4 == -1) {
      //Serial.println("Malformed line, skipping...");
      return;  // Skip malformed lines
    }

    BAT_VTG = inputString.substring(pos1 + 1, pos2).toInt();
    BAT_PER = map(BAT_VTG, 1250,1550,0,100);
    BAT_PER = constrain(BAT_PER,0,100);
    CHG_VTG = inputString.substring(pos2 + 1, pos3).toInt();
    MTR_VTG = inputString.substring(pos3 + 1, pos4).toInt();
    MTR_CUR = inputString.substring(pos4 + 1, pos5).toInt();
}

void SendSerial() {
//  Strfile = "/ACTLOG.csv"; // Specify the file name

  // Open the file for reading
  userFile = SD.open(Strfile, FILE_READ);
  if (!userFile) {
    return;
  }

  // Define a buffer for reading file data
  const size_t bufferSize = 128; // Adjust based on available memory
  char buffer[bufferSize];

  // Read and send the entire file
  while (userFile.available()) {
    size_t bytesRead = userFile.readBytes(buffer, bufferSize); // Read chunk of data
    Serial2.write(buffer, bytesRead); // Send the data to Serial2
  }

  // Close the file
  userFile.close();
}

void WriteSerial() {
  String strSamp;
  strSamp=Mode + "-" + SampleName + "-" + SamplingLocation + "-" + String(Motor_lts) + "-" + String(Start_Delay1) + "-" + String(No_of_Runs) + "-" + String(Delay_between_Runs) + "-" + String(Motor_PWM[0]);
  Serial2.print(strSamp);//delay(2000);
}

void Motor_Bit_Loop() {
  Motor_Status1=true;
  WriteTopway(0x00,0xA0,0);
  ReadTopway(0x00,0x00); //Motor PWM
  Motor_PWM[0] = (temp_h << 8) | temp_l;
  Motor_PWM1[0] = map(Motor_PWM[0], 0, 100, 0, 255);
  ReadTopway(0x00,0x34);
  Start_Delay1=(temp_h << 8) | temp_l;
  ReadTopway(0x00,0x36); //No of Runs
  No_of_Runs = (temp_h << 8) | temp_l;
  ReadTopway(0x00,0x38); //Delay between Runs
  Delay_between_Runs = (temp_h << 8) | temp_l;
  Delay_between_Runs1 = Delay_between_Runs;
  ReadTopway(0x00,0x08);
  Motor_lts = (temp_h << 8) | temp_l;
  Read_String(0x01,0x80);
  SampleName=Str2; //Check
  Read_String(0x04,0x80);
  SamplingLocation=Str2;
  Motor_Counter1 = Motor_lts*60/100/No_of_Runs;
  Motor_Counter2 = Motor_Counter1;
  Motor_Counter = Motor_Counter1*No_of_Runs + Start_Delay1 + Delay_between_Runs*(No_of_Runs-1);
  Motor_Counter_Max=Motor_Counter;
  if(Motor_Counter_Max!=0) {
    Motor_Counter_Perc=Motor_Counter/Motor_Counter_Max;
  } else {
    Motor_Counter_Perc=0;
  }
  Write_Count_Down();
  WriteString(0x00,0x03,0x80,"Sample Abort");
  LogParams();
  
}

void LogParams() {
  if(No_of_Runs>1) {Mode="Multi";No_of_Runs1=No_of_Runs;} else {Mode="Single";No_of_Runs1=1;}
  Motor_lts1=Motor_lts;
  getTime();getTime1();SamplingStart=Str1;
  Start_Delay1_1=Start_Delay1;  
}

void Run_Cal() {
  String strCalVal;
  strCalVal = "CAL-" + String(Cal_PWM);
  Serial2.print(strCalVal);
}

void Save_Cal() {
//  Write_In_Bit();
  WriteTopway(0x00,0x12,0);
  Write_Cal_File();
  EEPROM.write(10,Cal_PWM);EEPROM.commit();
  Motor_PWM[0]=EEPROM.read(10);
  WriteTopway(0x00,0x00,Motor_PWM[0]);
  StopMotor();
  Serial2.print("STOP");delay(100);Serial2.print("STOP");delay(100);Serial2.print("STOP");
  AuditDetails="GService";AuditRemark=String(Motor_PWM[0]);
  LogActivity("CalibrationSave");
  DisplayPage(2);
}

void Exit_Cal() {
//  Write_In_Bit();
  WriteTopway(0x00,0x12,0);
  Cal_PWM=Motor_PWM[0];
  StopMotor();
  WriteTopway(0x00,0x10,Cal_PWM);
  Serial2.print("STOP");delay(100);Serial2.print("STOP");delay(100);Serial2.print("STOP");
  DisplayPage(2);
}

void Write_Cal_File() {
  now1 = rtc.now();  // Get current date and time

  // Format Calibration Date as dd/mm/yyyy
  String calDate = "";
  if (now1.day() < 10) calDate += '0';
  calDate += String(now1.day()) + "/";
  if (now1.month() < 10) calDate += '0';
  calDate += String(now1.month()) + "/";
  calDate += String(now1.year());

  // Calculate Due Date (Cal Date + 365 days)
  DateTime dueDateObj = now1 + TimeSpan(364, 0, 0, 0);  // Add 365 days
  String CalDueDate = "";
  if (dueDateObj.day() < 10) CalDueDate += '0';
  CalDueDate += String(dueDateObj.day()) + "/";
  if (dueDateObj.month() < 10) CalDueDate += '0';
  CalDueDate += String(dueDateObj.month()) + "/";
  CalDueDate += String(dueDateObj.year());

  // Write to CAL.csv
  myFile = SD.open("/CAL.csv", O_RDWR | O_CREAT | O_APPEND);
  
  if (myFile) {
    myFile.print(CalID); myFile.print(",");
    myFile.print(Cal_PWM); myFile.print(",");
    myFile.print(calDate); myFile.print(",");
    myFile.println(CalDueDate);
    
    myFile.flush(); // Critical: Force write to SD card
    myFile.close(); // Critical: Ensure FAT table update
    
    CalID++;
  }
}

void Write_In_Bit() {
  WriteTopway(0x00, 0x0a, 0);
}

void Set_In_Bit(byte j, byte val) { 
    ReadTopway(0x00, 0x0a); // Read current values into temp_h and temp_l
    if(Read_Err==false) {
      if (j < 8) {
          if (val == 1) {Output_l = temp_l | (1 << j);} else {Output_l = temp_l & ~(1 << j);}
          Output_h = temp_h;
      } else {
          j = j - 8;
          if (val == 1) {Output_h = temp_h | (1 << j);} else {Output_h = temp_h & ~(1 << j);}
          Output_l = temp_l;
      }
      WriteTopway_byte(0x00, 0x0a, Output_h, Output_l);
    }
}

void Set_Bit_Icons(byte j, byte val) { 
    ReadTopway(0x00, 0x84); // Read current values into temp_h and temp_l
    if(Read_Err==false) {
      if (j < 8) {
          if (val == 1) {Output_l = temp_l | (1 << j);} else {Output_l = temp_l & ~(1 << j);}
          Output_h = temp_h;
      } else {
          j = j - 8;
          if (val == 1) {Output_h = temp_h | (1 << j);} else {Output_h = temp_h & ~(1 << j);}
          Output_l = temp_l;
      }
      WriteTopway_byte(0x00, 0x84, Output_h, Output_l);
    }
}
