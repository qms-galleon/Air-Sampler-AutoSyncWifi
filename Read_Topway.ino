void Read_Disp_Params() {
  ReadTopway(0x00,0x46);if(Read_Err==false) {Page_No=temp_l;}
  if(Page_No_Old!=Page_No) {Page_No_Old=Page_No;Logout_Timer=0;}

  if(Page_No==1 || Page_No==206 || Page_No==207) {CheckOnline();} else {if(Motor_Status1==false) {if(Page_No==1 || Page_No==207){OnlineStatus=false;}}}
  if(Page_No!=185) {RecordStatus1=false;}
  if(Page_No!=103) {RecordStatus2=false;}
  if(Page_No==118) {if(updateMemoryInfo==false) {ShowMemoryInfo();updateMemoryInfo=true;}} else {updateMemoryInfo=false;}
  if(Page_No==14) {Process_Cal();}
  else {
  ReadTopway(0x00,0x0a);if(Read_Err==false) {process_Input_Bits();} //Motor ON/OFF bit
  // Poll for "Password Expired" popup OK — user self-reset flow
  if (PasswordResetOnExpiry) {
    ReadTopway(0x01, 0x12);
    if (Read_Err==false && temp_l==0) {
      // Topway cleared the register = user pressed OK on the expired popup
      PasswordResetOnExpiry = false;
      SelfResetOnExpiry();
    }
  }
  if(Page_No==2 || Page_No==118) {QuickSample();}
  Location_Bits();
  Remark_Bits();
  UserId_Bits();
  ConfigRecipe();
  ConfigGroup();
  if(Page_No==88 || Page_No==89 || Page_No==90 || Page_No==111  || Page_No==125  || Page_No==126) {Device_Bits();}
  if(Page_No==5 || Page_No==10 || Page_No==35 || Page_No==105 || Page_No==131) {RecordStatus=false;MaxRecordCount=LocationId-1;adr_h2=0x00;adr_m2=0x04;adr_l2=0x80;ReadWriteDropdown("/LOC.csv");}  //Sampling Location
  
  if(Page_No==69 || Page_No==154 || Page_No==155 || Page_No==173 || Page_No==196) {RecordStatus=false;MaxRecordCount=GroupId-1;adr_h2=0x00;adr_m2=0x25;adr_l2=0x00;ReadWriteDropdown("/GRP.csv");}  //Group
  
  if(Page_No==204 || Page_No==138 || Page_No==146 || Page_No==194) {RecordStatus=false;MaxRecordCount=GUserId-1;adr_h2=0x00;adr_m2=0x22;adr_l2=0x80;ReadWriteDropdown("/USR.csv");}  //USER
  else if(Page_No==57 || Page_No==58) {RecordStatus=false;MaxRecordCount=LocationId-1;adr_h2=0x00;adr_m2=0x13;adr_l2=0x00;ReadWriteDropdown("/LOC.csv");}  //Sampling Location
  else if(Page_No==39 || Page_No==82) {RecordStatus=false;MaxRecordCount=RemarkId-1;adr_h2=0x00;adr_m2=0x1e;adr_l2=0x00;ReadWriteDropdown("/RMK.csv");}  //Sampling Location
  else if(Page_No==42 || Page_No==43 || Page_No==80 || Page_No==130) {RecordStatus=false;MaxRecordCount=RecipeId-1;adr_h2=0x00;adr_m2=0x0f;adr_l2=0x00;ReadWriteDropdown("/RECP.csv");}  //  
  else if(Page_No==103) {if(RecordStatus2==false) {ReadLines2();}ReadReportFile2();}//ReadReportFile
  else if(Page_No==185) {if(RecordStatus1==false) {ReadLines1();}ReadReportFile1();}//ReadReportFile
  else if(Page_No==38 || Page_No==104) {RecordStatus=false;MaxRecordCount=RemarkId-1;adr_h2=0x00;adr_m2=0x0E;adr_l2=0x80;ReadWriteDropdown("/RMK.csv");} //Remark
  }
}

void CheckOnline() {
  ReadTopway(0x01,0x1c);if(Read_Err==false) {OnlineBit=temp_l;}
  ReadTopway(0x01,0x20);if(Read_Err==false) {OTA_Val=temp_l;}
  if(OnlineBit==11) {if(WiFiStatus==true) {DisplayPage(206);SampleRunString="DEVICE ONLINE";OnlineStatus=true;WriteTopway(0x01,0x1c,0);} else {WriteTopway(0x01,0x1c,44);}}
  else if(OnlineBit==22) {DisplayPage(1);OnlineStatus=false;WriteTopway(0x01,0x1c,0);}
  else if(OnlineBit==33) {DisplayPage(1);OnlineStatus=false;SoftWareConnected=false;WriteTopway(0x01,0x1c,0);}
  if(OTA_Val==10) {OTA_status=true;}
}

void ReadLines2() {
  MaxRecordCount2=SampleId-1;
  RecordCount2=MaxRecordCount2;
  RecordStatus2=true;
  /*MaxRecordCount2 = 0;
  myFile = SD.open("/FR.csv", FILE_READ);
  while (myFile.available()) {
    String line = myFile.readStringUntil('\n');
    MaxRecordCount2++;
  }
  MaxRecordCount2=MaxRecordCount2;
  RecordStatus2=true;*/
}

void ReadWriteDropdown(String Strfile1) {
  ReadLocFile(Strfile1);
  ReadTopway(0x00,0x92);
  if(Read_Err==false) {
    if(temp_l==1) {Read_String(0x14,0x00);if(Read_Err==false){WriteString(adr_h2,adr_m2,adr_l2,Str2);}}
    else if(temp_l==2) {Read_String(0x14,0x80);if(Read_Err==false){WriteString(adr_h2,adr_m2,adr_l2,Str2);}}
    else if(temp_l==3) {Read_String(0x15,0x00);if(Read_Err==false){WriteString(adr_h2,adr_m2,adr_l2,Str2);}}
    else if(temp_l==4) {Read_String(0x15,0x80);if(Read_Err==false){WriteString(adr_h2,adr_m2,adr_l2,Str2);}}
    else if(temp_l==5) {Read_String(0x16,0x00);if(Read_Err==false){WriteString(adr_h2,adr_m2,adr_l2,Str2);}}
    else if(temp_l==6) {Read_String(0x16,0x80);if(Read_Err==false){WriteString(adr_h2,adr_m2,adr_l2,Str2);}}
    else if(temp_l==7) {Read_String(0x17,0x00);if(Read_Err==false){WriteString(adr_h2,adr_m2,adr_l2,Str2);}}
    else if(temp_l==8) {Read_String(0x17,0x80);if(Read_Err==false){WriteString(adr_h2,adr_m2,adr_l2,Str2);}}
    else if(temp_l==9) {Read_String(0x18,0x00);if(Read_Err==false){WriteString(adr_h2,adr_m2,adr_l2,Str2);}}
    else if(temp_l==10) {Read_String(0x18,0x80);if(Read_Err==false){WriteString(adr_h2,adr_m2,adr_l2,Str2);}}
  }
}

void ReadLocFile(String Strfile1) {
  if (RecordCount * 10 > MaxRecordCount) {
    RecordCount = 0;
  }

  // Safety: Check if file exists first
  if (!SD.exists(Strfile1)) {
    return;
  }

  myFile = SD.open(Strfile1, FILE_READ);
  if (!myFile) {
    return;
  }

  ReadTopway(0x00, 0x94);
  if (Read_Err == false) {
    if (temp_l == 1) {
      if (RecordCount > 0) RecordCount--;
      WriteTopway(0x00, 0x94, 0);
    } else if (temp_l == 2) {
      if ((RecordCount + 1) * 10 < MaxRecordCount) RecordCount++;
      WriteTopway(0x00, 0x94, 0);
    }
  }

  int lineNum = 0;
  int validLineCount = 0;
  int targetStart = RecordCount * 10;
  int targetEnd = targetStart + 10;

  // MEMORY PROTECTION: Move string outside loop
  String line;
  line.reserve(128); 

  while (myFile.available() && validLineCount < targetEnd) {
    line = myFile.readStringUntil('\n');
    line.trim();

    if(line.length() == 0) continue; // Skip empty lines

    int TempId;
    String STRLOC;
    String STATUS;
    //Recipe Page no 42, 43, 80, 130 
    // User Page no 204, 138, 146, 194
    //Location Page No 5, 10, 35, 57, 58, 105, 131
    //Remark PAge No 38, 39, 82, 104
    if(Page_No==42 || Page_No==43 || Page_No==80 || Page_No==130 || Page_No==204 || Page_No==138 || Page_No==146 || Page_No==194) {
      pos1 = line.indexOf(',');
      pos2 = line.indexOf(',', pos1 + 1);
      pos3 = line.indexOf(',', pos2 + 1);
      pos4 = line.indexOf(',', pos3 + 1);
      pos5 = line.indexOf(',', pos4 + 1);
      pos6 = line.indexOf(',', pos5 + 1);
      pos7 = line.indexOf(',', pos6 + 1);
      pos8 = line.indexOf(',', pos7 + 1);
      if (pos1 == -1 || pos2 == -1 || pos3 == -1 || pos4 == -1 || pos5 == -1 || pos6 == -1 || pos7 == -1 || pos8 == -1) {
        continue;  // Skip malformed lines
      }
      TempId = line.substring(0, pos1).toInt();
      STRLOC = line.substring(pos1 + 1, pos2);
      if(Page_No==204 || Page_No==138 || Page_No==146 || Page_No==194) {STATUS = line.substring(pos7 + 1, pos8);}
      else {STATUS = line.substring(pos8 + 1);}
    } else {
      pos1 = line.indexOf(',');
      pos2 = line.indexOf(',', pos1 + 1);
      if(Page_No==69 || Page_No==154 || Page_No==155 || Page_No==173) {pos3 = line.indexOf(',', pos2 + 1);}
      if (pos1 == -1 || pos2 == -1) continue;
      TempId = line.substring(0, pos1).toInt();
      STRLOC = line.substring(pos1 + 1, pos2);
      if(Page_No==69 || Page_No==154 || Page_No==155 || Page_No==173) {STATUS = line.substring(pos2 + 1,pos3);} else {STATUS = line.substring(pos2 + 1);}
    }
    if (STATUS == "Discard") continue;

    // Only process if we are within the 10-entry page window
    if (validLineCount >= targetStart && validLineCount < targetEnd) {
      switch (validLineCount - targetStart) {
        case 0: WriteString(0x00, 0x14, 0x00, STRLOC);WriteTopway(0x00, 0xD8, TempId);SetFG(Page_No,1,1); break;
        case 1: WriteString(0x00, 0x14, 0x80, STRLOC);WriteTopway(0x00, 0xDA, TempId);SetFG(Page_No,10,1); break;
        case 2: WriteString(0x00, 0x15, 0x00, STRLOC);WriteTopway(0x00, 0xDC, TempId);SetFG(Page_No,9,1); break;
        case 3: WriteString(0x00, 0x15, 0x80, STRLOC);WriteTopway(0x00, 0xDE, TempId);SetFG(Page_No,8,1); break;
        case 4: WriteString(0x00, 0x16, 0x00, STRLOC);WriteTopway(0x00, 0xE0, TempId);SetFG(Page_No,7,1); break;
        case 5: WriteString(0x00, 0x16, 0x80, STRLOC);WriteTopway(0x00, 0xE2, TempId);SetFG(Page_No,6,1); break;
        case 6: WriteString(0x00, 0x17, 0x00, STRLOC);WriteTopway(0x00, 0xE4, TempId);SetFG(Page_No,5,1); break;
        case 7: WriteString(0x00, 0x17, 0x80, STRLOC);WriteTopway(0x00, 0xE6, TempId);SetFG(Page_No,4,1); break;
        case 8: WriteString(0x00, 0x18, 0x00, STRLOC);WriteTopway(0x00, 0xE8, TempId);SetFG(Page_No,3,1); break;
        case 9: WriteString(0x00, 0x18, 0x80, STRLOC);WriteTopway(0x00, 0xEA, TempId);SetFG(Page_No,2,1); break;
      }
    }

    validLineCount++;
  }

  // Clear any display lines if fewer than 10 valid lines were found
  int linesShown = validLineCount - (RecordCount * 10);
  for (int i = linesShown; i < 10; i++) {
    switch (i) {
      case 0: WriteString(0x00, 0x14, 0x00, "");WriteTopway(0x00, 0xD8, 0);SetFG(Page_No,1,0); break;
      case 1: WriteString(0x00, 0x14, 0x80, "");WriteTopway(0x00, 0xDA, 0);SetFG(Page_No,10,0); break;
      case 2: WriteString(0x00, 0x15, 0x00, "");WriteTopway(0x00, 0xDC, 0);SetFG(Page_No,9,0); break;
      case 3: WriteString(0x00, 0x15, 0x80, "");WriteTopway(0x00, 0xDE, 0);SetFG(Page_No,8,0); break;
      case 4: WriteString(0x00, 0x16, 0x00, "");WriteTopway(0x00, 0xE0, 0);SetFG(Page_No,7,0); break;
      case 5: WriteString(0x00, 0x16, 0x80, "");WriteTopway(0x00, 0xE2, 0);SetFG(Page_No,6,0); break;
      case 6: WriteString(0x00, 0x17, 0x00, "");WriteTopway(0x00, 0xE4, 0);SetFG(Page_No,5,0); break;
      case 7: WriteString(0x00, 0x17, 0x80, "");WriteTopway(0x00, 0xE6, 0);SetFG(Page_No,4,0); break;
      case 8: WriteString(0x00, 0x18, 0x00, "");WriteTopway(0x00, 0xE8, 0);SetFG(Page_No,3,0); break;
      case 9: WriteString(0x00, 0x18, 0x80, "");WriteTopway(0x00, 0xEA, 0);SetFG(Page_No,2,0); break;
    }
  }

  myFile.close();
}

void ReadReportFile2() {
  if (!SD.exists("/FR.csv")) return; // Safety check

  myFile = SD.open("/FR.csv", FILE_READ);
  if (!myFile) {
    return; // Exit if file not found
  }

  // Read user input from display
  ReadTopway(0x00, 0x94);
  if (Read_Err == false) {
    if (temp_l == 15) {
      Logout_Timer=0;
      if (RecordCount2 > 1) {RecordCount2--;} else {RecordCount2=MaxRecordCount2;} // UP
      WriteTopway(0x00, 0x94, 0);
    } else if (temp_l == 240) {
      Logout_Timer=0;
      if (RecordCount2 < MaxRecordCount2) {RecordCount2++;} else {RecordCount2=1;} // DOWN
      WriteTopway(0x00, 0x94, 0);
    }
  }

  int lineNum = 0;
  String lineData = "";

  // MEMORY PROTECTION: Move String outside loop
  String line;
  line.reserve(128);

  // Read the file line-by-line until the desired line (RecordCount)
  while (myFile.available()) {
    line = myFile.readStringUntil('\n');
    if (lineNum == RecordCount2-1) {
      lineData = line;
      break;
    }
    lineNum++;
  }

  myFile.close(); // Done reading

  // Parse the line and extract the second field
  pos1 = lineData.indexOf('/');
  pos2 = lineData.indexOf(',', pos1 + 1);
  pos3 = lineData.indexOf(',', pos2 + 1);
  pos4 = lineData.indexOf(',', pos3 + 1);
  pos5 = lineData.indexOf(',', pos4 + 1);
  pos6 = lineData.indexOf(',', pos5 + 1);
  pos7 = lineData.indexOf(',', pos6 + 1);
  pos8 = lineData.indexOf(',', pos7 + 1);
  pos9 = lineData.indexOf(',', pos8 + 1);
  pos10 = lineData.indexOf(',', pos9 + 1);
  pos11 = lineData.indexOf(',', pos10 + 1);
  pos12 = lineData.indexOf(',', pos11 + 1);
  
  String strSampleId1 = (pos1 != -1) ? lineData.substring(0, pos1) : "";
  String strSampleName = (pos2 != -1) ? lineData.substring(pos1 + 1, pos2) : "";
  String strUserId = (pos3 != -1) ? lineData.substring(pos2 + 1, pos3) : "";
  String strSampleMode = (pos4 != -1) ? lineData.substring(pos3 + 1, pos4) : "";
  String strSampleStart = (pos5 != -1) ? lineData.substring(pos4 + 1, pos5) : "";
  String strSampleEnd = (pos6 != -1) ? lineData.substring(pos5 + 1, pos6) : "";
  String strSampleStatus = (pos7 != -1) ? lineData.substring(pos6 + 1, pos7) : "";
  String strSampleRemark = (pos8 != -1) ? lineData.substring(pos7 + 1, pos8) : "NO_RMK";
  String strSampleLocation = (pos9 != -1) ? lineData.substring(pos8 + 1, pos9) : "NO_LOC";
  int RVolume = (pos10 != -1) ? lineData.substring(pos9 + 1, pos10).toInt() : 0;
  int RStartDelay = (pos11 != -1) ? lineData.substring(pos10 + 1, pos11).toInt() : 0;
  int RNoofSamples = (pos12 != -1) ? lineData.substring(pos11 + 1, pos12).toInt() : 0;
  int RDelaybetwRuns = lineData.substring(pos12 + 1).toInt();
  WriteString(0x00,0x14,0x00,strUserId);
  WriteString(0x00,0x14,0x80,strSampleId1);
  WriteString(0x00,0x15,0x00,strSampleName);
  WriteString(0x00,0x15,0x80,strSampleMode);
  WriteString(0x00,0x16,0x00,strSampleStart);
  WriteString(0x00,0x16,0x80,strSampleEnd);
  WriteString(0x00,0x17,0x00,strSampleStatus);
  WriteString(0x00,0x17,0x80,strSampleRemark);
  WriteString(0x00,0x18,0x00,strSampleLocation);
  WriteTopway(0x00,0xa4,RVolume);
  WriteTopway(0x00,0xa6,RStartDelay);
  WriteTopway(0x00,0xa8,RNoofSamples);
  WriteTopway(0x00,0xaa,RDelaybetwRuns);
  WriteTopway(0x00,0xac,RecordCount2);
  WriteTopway(0x00,0xae,MaxRecordCount2);
}

void ReadTopway(byte adr_h, byte adr_l) {
  Read_Err=false;
  ClearSerialData();
  Serial.write(0xaa);  // packet head
  Serial.write(0x3e);  // VP_N16 read command
  Serial.write((byte) 0x00); // VP_N16 address (High byte)
  Serial.write(0x08);  // VP_N16 address (Low byte)
  Serial.write((byte) adr_h); 
  Serial.write(adr_l);
  EndPacket();
  unsigned long startTime = millis();
  unsigned long timeout = 50;  // 1 second timeout
  byte receivedData[8];
  int index = 0;

  OutString = "";

  // Read 8 bytes from Serial1 within the timeout
  while (index < 8 && millis() - startTime < timeout) {
    if (Serial.available()) {
      receivedData[index++] = Serial.read();
    }
  }
  // Check if we received the full packet
  if (index < 8) {
    OutString = "Timeout waiting for full packet";
    return;  // Exit if we didn't receive the full packet
  }
  processPacket(receivedData);
}

void ClearSerialData() {
  while (Serial.available() > 0) {
    Serial.read();  // Discard incoming data
  }
}

void processPacket(byte* data) {
  if (data[0] == 0xAA && data[1] == 0x3E && data[4] == 0xCC && data[5] == 0x33 && data[6] == 0xC3 && data[7] == 0x3C) {
    temp_h = data[2];temp_l = data[3];
  } else {
    Read_Err=true;
  }
}

void Read_String(byte adr_h, byte adr_l) {
  // Send command to read from VP_STR
  Read_Err=false;
  ClearSerialData();
  Serial.write(0xaa);  // packet head
  Serial.write(0x43);  // VP_STR read command
  Serial.write((byte) 0x00); // VP_STR address (High byte)
  Serial.write((byte) 0x00);  // VP_STR address (Low byte)
  Serial.write((byte) adr_h); 
  Serial.write((byte) adr_l);
  EndPacket();

  const int maxBytes = 134; 
  unsigned long startTime = millis();
  unsigned long timeout = 100;  // 1 second timeout
  // Create an array to hold the incoming packet (8 bytes)
  byte receivedData[134];
  int index = 0;
  
  // Clear OutString before starting to accumulate new output
  OutString = "";

  // Read 8 bytes from Serial1 within the timeout
  while (millis() - startTime < timeout) {
    if (Serial.available()) {
      receivedData[index++] = Serial.read();
    }
    if (index >= maxBytes) break;
  }
  if (index < 7) {
    return;
  }
  
  int startIndex = 2;  // Start at the 3rd byte
  int endIndex = index - 6;  // End at the byte before the last 5 bytes

  Str2="";
      // Extract Str2 (high byte and low byte)
  for (i = 2; i <= endIndex; i++) {
    Str2 += (char)receivedData[i];
  }
  processPacket2(receivedData);
}

void processPacket2(byte* data) {
  if (data[0] == 0xAA && data[1] == 0x43) {
//    WriteString(0x00,0x01,0x80,Str2);
  } else {
    Str2="Invalid Data...";
    Read_Err=true;
//    WriteString(0x00,0x01,0x80,Str2);
  }
}
