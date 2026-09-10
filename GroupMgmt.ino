void ConfigGroup() {
  ReadTopway(0x00,0x64);
  if(Read_Err==false) {
    for (i = 7; i >= 0; i--) {
      bitArray[i] = (temp_l >> i) & 1;  // Extract each bit from the low byte
    }
    for (int i = 7; i >= 0; i--) {
      bitArray[i + 8] = (temp_h >> i) & 1;  // Extract each bit from the high byte
    }
    if(bitArray[0]==1) {Logout_Timer=0;CheckGrpConfig(1);Reset_Group_Bit();} //1      Add Group Next Button
//    if(bitArray[1]==1) {CheckGrpConfig(2);Reset_Group_Bit();} //2      Add Group Next Button
//    if(bitArray[2]==1) {CheckGrpConfig(3);Reset_Group_Bit();} //4      Add Group Next Button
    if(bitArray[3]==1) {Logout_Timer=0;AddGroup();Reset_Group_Bit();} //8      Add Group Button
//    if(bitArray[4]==1) {GroupSummary(81);Reset_Group_Bit();} //16      Group Sample Summary Button
    if(bitArray[5]==1) {Logout_Timer=0;GroupSummary(169);Reset_Group_Bit();} //32      View Group Summary Button
    if(bitArray[6]==1) {Logout_Timer=0;GroupSummary(174);Reset_Group_Bit();} //64      Edit Group 1 
//    if(bitArray[7]==1) {CheckGrpConfig(4);Reset_Group_Bit();} //128      Edit Group 2 
    if(bitArray[8]==1) {Logout_Timer=0;Update_Group();Reset_Group_Bit();} //256      Edit Group 3 Summary Button
    if(bitArray[9]==1) {Logout_Timer=0;DisplayPage(155);Reset_Group_Bit();} //512      Group Mgmt/Discard Group Summary Button
    if(bitArray[10]==1) {Logout_Timer=0;WriteTopway(0x00,0x74,1);Reset_Group_Bit();} //1024      Group Mgmt/Discard Group Button
    if(bitArray[11]==1) {Logout_Timer=0;Update_Group();Reset_Group_Bit();} //2048      Discard Group Confirm Button
  }
}

void GroupSummary( int PageNo) {
  Read_String(0x25,0x00);strGroup=Str2;

  if (!SD.exists("/GRP.csv")) return;

  myFile = SD.open("/GRP.csv", FILE_READ);
  if (!myFile) return;

  // MEMORY PROTECTION: Reserve string space outside loop
  String line;
  line.reserve(256); // Reserve enough for long permission strings

  while (myFile.available()) {
    line = myFile.readStringUntil('\n');  // Read a line from the file
    line.trim();  // Remove any trailing whitespace or newline characters

    if (line.length() == 0) continue;

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
    pos11 = line.indexOf(',', pos10 + 1);
    pos12 = line.indexOf(',', pos11 + 1);
    pos13 = line.indexOf(',', pos12 + 1);
    pos14 = line.indexOf(',', pos13 + 1);
    pos15 = line.indexOf(',', pos14 + 1);
    pos16 = line.indexOf(',', pos15 + 1);
    pos17 = line.indexOf(',', pos16 + 1);
    pos18 = line.indexOf(',', pos17 + 1);
    pos19 = line.indexOf(',', pos18 + 1);
    pos20 = line.indexOf(',', pos19 + 1);
    pos21 = line.indexOf(',', pos20 + 1);
    
    if (pos1 == -1 || pos2 == -1 || pos3 == -1 || pos4 == -1 || pos5 == -1 || pos6 == -1 || pos7 == -1 || pos8 == -1) {
      continue;  // Skip malformed lines
    }

    if(strGroup==line.substring(pos1 + 1, pos2)) {

      GroupMinPassLength = (pos4 != -1) ? line.substring(pos3 + 1, pos4).toInt() : 0;
      GroupPasswordExpiy = (pos5 != -1) ? line.substring(pos4 + 1, pos5).toInt() : 0;
      GroupRetryLimits = (pos6 != -1) ? line.substring(pos5 + 1, pos6).toInt() : 0;  
      PasswordComplexity = (pos7 != -1) ? line.substring(pos6 + 1, pos7) : "";
      LastPAsswordUnique = (pos8 != -1) ? line.substring(pos7 + 1, pos8).toInt() : 0;
      ResetPassAuth = (pos9 != -1) ? line.substring(pos8 + 1, pos9) : "";
      FixedVolume = (pos10 != -1) ? line.substring(pos9 + 1, pos10) : "";
      Logout_Timer1 = (pos11 != -1) ? line.substring(pos10 + 1, pos11).toInt() : 0;
      Power_Off_Timer1 = (pos12 != -1) ? line.substring(pos11 + 1, pos12).toInt() : 0;

      Software_Access = (pos13 != -1) ? line.substring(pos12 + 1, pos13) : "";
      Device_Monitoring = (pos14 != -1) ? line.substring(pos13 + 1, pos14) : "";
      Group_User_Setting = (pos15 != -1) ? line.substring(pos14 + 1, pos15) : "";
      Config_Setting = (pos16 != -1) ? line.substring(pos15 + 1, pos16) : "";
      Sample_Data_View = (pos17 != -1) ? line.substring(pos16 + 1, pos17) : "";
      Audit_Trail_View = (pos18 != -1) ? line.substring(pos17 + 1, pos18) : "";
      Print_Config = (pos19 != -1) ? line.substring(pos18 + 1, pos19) : "";
      Print_Sample_Data = (pos20 != -1) ? line.substring(pos19 + 1, pos20) : "";
      Print_Audit_Trail = (pos21 != -1) ? line.substring(pos20 + 1, pos21) : "";
      Allow_Sampling = (pos21 != -1) ? line.substring(pos21 + 1) : "";

      WriteTopway(0x00,0xf0,GroupMinPassLength);
      WriteTopway(0x00,0xf8,GroupPasswordExpiy);
      WriteTopway(0x00,0xf2,GroupRetryLimits);
      WriteString(0x00,0x25,0x80,PasswordComplexity);
      WriteTopway(0x00,0xfa,LastPAsswordUnique);
      WriteString(0x00,0x26,0x80,ResetPassAuth);
      WriteString(0x00,0x27,0x00,FixedVolume);
      WriteTopway(0x00,0xf4,Logout_Timer1);
      WriteTopway(0x00,0xf6,Power_Off_Timer1);

      WriteString(0x00,0x27,0x80,Software_Access);
      WriteString(0x00,0x28,0x00,Device_Monitoring);
      WriteString(0x00,0x28,0x80,Group_User_Setting);
      WriteString(0x00,0x29,0x00,Config_Setting);
      WriteString(0x00,0x29,0x80,Sample_Data_View);
      WriteString(0x00,0x2a,0x00,Audit_Trail_View);
      WriteString(0x00,0x2a,0x80,Print_Config);
      WriteString(0x00,0x2b,0x00,Print_Sample_Data);
      WriteString(0x00,0x2b,0x80,Print_Audit_Trail);
      WriteString(0x00,0x2c,0x00,Allow_Sampling);

      DisplayPage(PageNo);
      myFile.close();
      return;
    }
  }
  myFile.close();
}

void CheckGrpConfig(byte ConfigParam) {
  if(ConfigParam==1) {Read_String(0x25,0x00);}
//  else if(ConfigParam==2) {Read_String(0x13,0x00);} //Recipe Location
//  else if(ConfigParam==3) {Read_String(0x13,0x00);}
//  else if(ConfigParam==4) {Read_String(0x03,0x00);}
  if(Read_Err==false) {
    if(Str2.length() < 4 || Str2.length() > 20) {
      WriteTopway(0x00,0x72,1); //Length Warning Popup
    } else {
      if(ConfigParam==1) {CheckGroup();}
//      else if(ConfigParam==2) {DisplayPage(60);}
//      else if(ConfigParam==3) {DisplayPage(59);}
//      else if(ConfigParam==4) {if(Str2=="Single") {WriteTopway(0x00,0x8c,1);WriteTopway(0x00,0x8e,0);DisplayPage(133);} else {DisplayPage(132);}}
    }
  }
}

void CheckGroup() {
  bool ConfigExists = false;
  
  if (!SD.exists("/GRP.csv")) {
     // If file doesn't exist, we can't find the group, so it doesn't exist.
     // Skip straight to creating default logic.
     OpStatus=true;
     if(SoftWareConnected ==false) {
       SetGroupDefault();
       DisplayPage(150);
     }
     return;
  }

  myFile = SD.open("/GRP.csv", O_RDONLY); 
  if (myFile) {
    // Memory efficient line reading instead of char-by-char
    String line;
    line.reserve(128);

    while (myFile.available()) {
      line = myFile.readStringUntil('\n');
      line.trim();
      
      if (line.length() == 0) continue;

      int pos1 = line.indexOf(',');
      int pos2 = line.indexOf(',', pos1 + 1);

      if (pos1 != -1 && pos2 != -1) {
          // Extract Group Name safely
          String RecipeName = line.substring(pos1 + 1, pos2);
          RecipeName.trim();

          if (RecipeName.equals(Str2)) {
            ConfigExists = true;
            break;
          }
      }
    }
    myFile.close();
  }

  if (ConfigExists) {
    WriteTopway(0x00, 0x76, 2); // Already exists popup
    OpStatus=false;
    return;
  } else {
    OpStatus=true;
    if(SoftWareConnected ==false) {
      SetGroupDefault();
      DisplayPage(150);
    }
  }
}

void SetGroupDefault() {
  WriteTopway(0x00, 0xf0, 4); //Min Password Length
  WriteTopway(0x00, 0xf8, 30); //Password Expiry
  WriteTopway(0x00, 0xf2, 3); //Retry Limits
  WriteString(0x00,0x25,0x80,"No"); //Password Complexity
  WriteTopway(0x00, 0xfa, 1); //Last Password Unique
  
  WriteString(0x00,0x26,0x80,"No"); //Reset Password Authority
  WriteString(0x00,0x27,0x00,"No"); //Fixed Volume
  WriteTopway(0x00, 0xf4, 5); //Auto Logout
  WriteTopway(0x00, 0xf6, 5); //Auto Shutdown
  WriteString(0x00,0x27,0x80,"No"); //Software Access

  WriteString(0x00,0x28,0x00,"No"); //Device Monitoring
  WriteString(0x00,0x28,0x80,"No"); //Group User Setting
  WriteString(0x00,0x29,0x00,"No"); //Config Setting
  WriteString(0x00,0x29,0x80,"No"); //View Sample Data
  WriteString(0x00,0x2a,0x00,"No"); //View Audit Trail Data

  WriteString(0x00,0x2a,0x80,"No"); //Print Config
  WriteString(0x00,0x2b,0x00,"No"); //Print Sample Data
  WriteString(0x00,0x2b,0x80,"No"); //Print Audit Trail
  WriteString(0x00,0x2c,0x00,"No"); //Allow Sampling
}

void AddGroup() {
  Read_String(0x25,0x00);if(Read_Err==false) {strGroup=Str2;}
  ReadTopway(0x00,0xf0);if(Read_Err==false) {GroupMinPassLength=(temp_h << 8) | temp_l;}
  ReadTopway(0x00,0xf8);if(Read_Err==false) {GroupPasswordExpiy=(temp_h << 8) | temp_l;}
  ReadTopway(0x00,0xf2);if(Read_Err==false) {GroupRetryLimits=(temp_h << 8) | temp_l;}
  Read_String(0x25,0x80);if(Read_Err==false) {PasswordComplexity=Str2;}
  ReadTopway(0x00,0xfa);if(Read_Err==false) {LastPAsswordUnique=(temp_h << 8) | temp_l;}
  Read_String(0x26,0x80);if(Read_Err==false) {ResetPassAuth=Str2;}
  Read_String(0x27,0x00);if(Read_Err==false) {FixedVolume=Str2;}
  ReadTopway(0x00,0xf4);if(Read_Err==false) {Logout_Timer1=(temp_h << 8) | temp_l;}
  ReadTopway(0x00,0xf6);if(Read_Err==false) {Power_Off_Timer1=(temp_h << 8) | temp_l;}
  Read_String(0x27,0x80);if(Read_Err==false) {Software_Access=Str2;}
  Read_String(0x28,0x00);if(Read_Err==false) {Device_Monitoring=Str2;}
  Read_String(0x28,0x80);if(Read_Err==false) {Group_User_Setting=Str2;}
  Read_String(0x29,0x00);if(Read_Err==false) {Config_Setting=Str2;}
  Read_String(0x29,0x80);if(Read_Err==false) {Sample_Data_View=Str2;}
  Read_String(0x2a,0x00);if(Read_Err==false) {Audit_Trail_View=Str2;}
  Read_String(0x2a,0x80);if(Read_Err==false) {Print_Config=Str2;}
  Read_String(0x2b,0x00);if(Read_Err==false) {Print_Sample_Data=Str2;}
  Read_String(0x2b,0x80);if(Read_Err==false) {Print_Audit_Trail=Str2;}
  Read_String(0x2c,0x00);if(Read_Err==false) {Allow_Sampling=Str2;}

  AddGroup1();
  WriteTopway(0x00,0x76,1);
}

void AddGroup1() {
  myFile = SD.open("/GRP.csv", O_RDWR | O_CREAT | O_APPEND);

  if (myFile) {
    myFile.print(GroupId); myFile.print(",");
    myFile.print(strGroup); myFile.print(",");
    myFile.print("Active"); myFile.print(",");
    myFile.print(GroupMinPassLength); myFile.print(",");
    myFile.print(GroupPasswordExpiy); myFile.print(",");
    myFile.print(GroupRetryLimits); myFile.print(",");
    myFile.print(PasswordComplexity); myFile.print(",");
    myFile.print(LastPAsswordUnique); myFile.print(",");
    myFile.print(ResetPassAuth); myFile.print(",");
    myFile.print(FixedVolume); myFile.print(",");
    myFile.print(Logout_Timer1); myFile.print(",");
    myFile.print(Power_Off_Timer1); myFile.print(",");
    myFile.print(Software_Access); myFile.print(",");
    myFile.print(Device_Monitoring); myFile.print(",");
    myFile.print(Group_User_Setting); myFile.print(",");
    myFile.print(Config_Setting); myFile.print(",");
    myFile.print(Sample_Data_View); myFile.print(",");
    myFile.print(Audit_Trail_View); myFile.print(",");
    myFile.print(Print_Config); myFile.print(",");
    myFile.print(Print_Sample_Data); myFile.print(",");
    myFile.print(Print_Audit_Trail); myFile.print(",");
    myFile.println(Allow_Sampling);
    
    myFile.flush(); // Critical: Force write before close
    myFile.close();
  }
  AuditDetails=strGroup;AuditRemark="OK";
  LogActivity("GroupAdded");
  GroupId++;
  WriteTopway(0x00,0x7c,GroupId);
}

void Update_Group() {
  String STRLOC1;
  Read_String(0x25,0x00);
  if(Read_Err==false) {
    STRLOC1=Str2;
    WriteString(0x00,0x21,0x80,STRLOC1);
    
    // ATOMIC UPDATE: Write to temp, then rename
    const char* tmpName = "/temp.csv";
    const char* originalName = "/GRP.csv";

    if (SD.exists(tmpName)) SD.remove(tmpName);
    tempFile = SD.open(tmpName, O_WRITE | O_CREAT | O_TRUNC);
    
    myFile = SD.open(originalName, O_READ);
    if (!myFile) {
        if(tempFile) tempFile.close();
        return;
    }
    
    // Memory Protection
    String row; 
    row.reserve(512); // Reserve space for large rows

    while (myFile.available()) {
      row = myFile.readStringUntil('\n');
      row.trim();  // Remove \n or \r\n
  
      if (row.length() == 0) continue;

      // Split the CSV row
      pos1 = row.indexOf(',');
      pos2 = row.indexOf(',', pos1 + 1);
      pos3 = row.indexOf(',', pos2 + 1);
      pos4 = row.indexOf(',', pos3 + 1);
      pos5 = row.indexOf(',', pos4 + 1);
      pos6 = row.indexOf(',', pos5 + 1);
      pos7 = row.indexOf(',', pos6 + 1);
      pos8 = row.indexOf(',', pos7 + 1);
      pos9 = row.indexOf(',', pos8 + 1);
      pos10 = row.indexOf(',', pos9 + 1);
      pos11 = row.indexOf(',', pos10 + 1);
      pos12 = row.indexOf(',', pos11 + 1);
      pos13 = row.indexOf(',', pos12 + 1);
      pos14 = row.indexOf(',', pos13 + 1);
      pos15 = row.indexOf(',', pos14 + 1);
      pos16 = row.indexOf(',', pos15 + 1);
      pos17 = row.indexOf(',', pos16 + 1);
      pos18 = row.indexOf(',', pos17 + 1);
      pos19 = row.indexOf(',', pos18 + 1);
      pos20 = row.indexOf(',', pos19 + 1);
      pos21 = row.indexOf(',', pos20 + 1);
  
      if (pos1 == -1 || pos2 == -1 || pos3 == -1 || pos4 == -1 || pos5 == -1 || pos6 == -1 || pos7 == -1 || pos8 == -1) {
        // Malformed row, write it as is
        tempFile.println(row);
        continue;
      }

      String ID = row.substring(0, pos1);
      strGroup = row.substring(pos1 + 1, pos2);
      String STATUS = (pos3 != -1) ? row.substring(pos2 + 1, pos3) : "";
      GroupMinPassLength = (pos4 != -1) ? row.substring(pos3 + 1, pos4).toInt() : 0;
      GroupPasswordExpiy = (pos5 != -1) ? row.substring(pos4 + 1, pos5).toInt() : 0;
      GroupRetryLimits = (pos6 != -1) ? row.substring(pos5 + 1, pos6).toInt() : 0;  
      PasswordComplexity = (pos7 != -1) ? row.substring(pos6 + 1, pos7) : "";
      LastPAsswordUnique = (pos8 != -1) ? row.substring(pos7 + 1, pos8).toInt() : 0;
      ResetPassAuth = (pos9 != -1) ? row.substring(pos8 + 1, pos9) : "";
      FixedVolume = (pos10 != -1) ? row.substring(pos9 + 1, pos10) : "";
      Logout_Timer1 = (pos11 != -1) ? row.substring(pos10 + 1, pos11).toInt() : 0;
      Power_Off_Timer1 = (pos12 != -1) ? row.substring(pos11 + 1, pos12).toInt() : 0;

      Software_Access = (pos13 != -1) ? row.substring(pos12 + 1, pos13) : "";
      Device_Monitoring = (pos14 != -1) ? row.substring(pos13 + 1, pos14) : "";
      Group_User_Setting = (pos15 != -1) ? row.substring(pos14 + 1, pos15) : "";
      Config_Setting = (pos16 != -1) ? row.substring(pos15 + 1, pos16) : "";
      Sample_Data_View = (pos17 != -1) ? row.substring(pos16 + 1, pos17) : "";
      Audit_Trail_View = (pos18 != -1) ? row.substring(pos17 + 1, pos18) : "";
      Print_Config = (pos19 != -1) ? row.substring(pos18 + 1, pos19) : "";
      Print_Sample_Data = (pos20 != -1) ? row.substring(pos19 + 1, pos20) : "";
      Print_Audit_Trail = (pos21 != -1) ? row.substring(pos20 + 1, pos21) : "";
      Allow_Sampling = (pos21 != -1) ? row.substring(pos21 + 1) : "";
      
      String newRow = "";
      if (strGroup == STRLOC1) {
        if(Page_No==155 || Page_No==207) {
          STATUS = "Discard";
          WriteString(0x00,0x21,0x80,STRLOC1);
          newRow = ID + "," + strGroup + "," + STATUS + "," + GroupMinPassLength + "," + GroupPasswordExpiy + "," + GroupRetryLimits + "," + PasswordComplexity + "," + LastPAsswordUnique + "," + ResetPassAuth + "," + FixedVolume + "," + Logout_Timer1 + "," + Power_Off_Timer1 + "," + Software_Access + "," + Device_Monitoring + "," + Group_User_Setting + "," + Config_Setting + "," + Sample_Data_View + "," + Audit_Trail_View + "," + Print_Config + "," + Print_Sample_Data + "," + Print_Audit_Trail + "," + Allow_Sampling;
        } else {
          GroupMinPassLength3=GroupMinPassLength;
          GroupPasswordExpiy3=GroupPasswordExpiy;
          GroupRetryLimits3=GroupRetryLimits;
          PasswordComplexity3 = PasswordComplexity;
          LastPAsswordUnique3 = LastPAsswordUnique;
          ResetPassAuth3 = ResetPassAuth;
          FixedVolume3 = FixedVolume;
          Logout_Timer3 = Logout_Timer1;
          Power_Off_Timer3 = Power_Off_Timer1;

          Software_Access3 = Software_Access;
          Device_Monitoring3 = Device_Monitoring;
          Group_User_Setting3 = Group_User_Setting;
          Config_Setting3 = Config_Setting;
          Sample_Data_View3 = Sample_Data_View;
          Audit_Trail_View3 = Audit_Trail_View;
          Print_Config3 = Print_Config;
          Print_Sample_Data3 = Print_Sample_Data;
          Print_Audit_Trail3 = Print_Audit_Trail;
          Allow_Sampling3 = Allow_Sampling;
          
//          Read_String(0x25,0x00);if(Read_Err==false) {strGroup=Str2;}
          ReadTopway(0x00,0xf0);if(Read_Err==false) {GroupMinPassLength4=(temp_h << 8) | temp_l;}
          ReadTopway(0x00,0xf8);if(Read_Err==false) {GroupPasswordExpiy4=(temp_h << 8) | temp_l;}
          ReadTopway(0x00,0xf2);if(Read_Err==false) {GroupRetryLimits4=(temp_h << 8) | temp_l;}
          Read_String(0x25,0x80);if(Read_Err==false) {PasswordComplexity4=Str2;}
          ReadTopway(0x00,0xfa);if(Read_Err==false) {LastPAsswordUnique4=(temp_h << 8) | temp_l;}
          Read_String(0x26,0x80);if(Read_Err==false) {ResetPassAuth4=Str2;}
          Read_String(0x27,0x00);if(Read_Err==false) {FixedVolume4=Str2;}
          ReadTopway(0x00,0xf4);if(Read_Err==false) {Logout_Timer4=(temp_h << 8) | temp_l;}
          ReadTopway(0x00,0xf6);if(Read_Err==false) {Power_Off_Timer4=(temp_h << 8) | temp_l;}
          Read_String(0x27,0x80);if(Read_Err==false) {Software_Access4=Str2;}
          Read_String(0x28,0x00);if(Read_Err==false) {Device_Monitoring4=Str2;}
          Read_String(0x28,0x80);if(Read_Err==false) {Group_User_Setting4=Str2;}
          Read_String(0x29,0x00);if(Read_Err==false) {Config_Setting4=Str2;}
          Read_String(0x29,0x80);if(Read_Err==false) {Sample_Data_View4=Str2;}
          Read_String(0x2a,0x00);if(Read_Err==false) {Audit_Trail_View4=Str2;}
          Read_String(0x2a,0x80);if(Read_Err==false) {Print_Config4=Str2;}
          Read_String(0x2b,0x00);if(Read_Err==false) {Print_Sample_Data4=Str2;}
          Read_String(0x2b,0x80);if(Read_Err==false) {Print_Audit_Trail4=Str2;}
          Read_String(0x2c,0x00);if(Read_Err==false) {Allow_Sampling4=Str2;}
          newRow = ID + "," + strGroup + "," + STATUS + "," + GroupMinPassLength4 + "," + GroupPasswordExpiy4 + "," + GroupRetryLimits4 + "," + PasswordComplexity4 + "," + LastPAsswordUnique4 + "," + ResetPassAuth4 + "," + FixedVolume4 + "," + Logout_Timer4 + "," + Power_Off_Timer4 + "," + Software_Access4 + "," + Device_Monitoring4 + "," + Group_User_Setting4 + "," + Config_Setting4 + "," + Sample_Data_View4 + "," + Audit_Trail_View4 + "," + Print_Config4 + "," + Print_Sample_Data4 + "," + Print_Audit_Trail4 + "," + Allow_Sampling4;
        }
        
      } else {
        newRow = ID + "," + strGroup + "," + STATUS + "," + GroupMinPassLength + "," + GroupPasswordExpiy + "," + GroupRetryLimits + "," + PasswordComplexity + "," + LastPAsswordUnique + "," + ResetPassAuth + "," + FixedVolume + "," + Logout_Timer1 + "," + Power_Off_Timer1 + "," + Software_Access + "," + Device_Monitoring + "," + Group_User_Setting + "," + Config_Setting + "," + Sample_Data_View + "," + Audit_Trail_View + "," + Print_Config + "," + Print_Sample_Data + "," + Print_Audit_Trail + "," + Allow_Sampling;
      }
      tempFile.println(newRow);
    }
    myFile.close();
    
    // Finalize update safely
    tempFile.flush();
    tempFile.close();
  
    if (SD.exists(originalName)) SD.remove(originalName);
    SD.rename(tmpName, originalName);

    if(Page_No==155 || Page_No==207) {
      AuditDetails=STRLOC1;AuditRemark="NA";
      LogActivity("GroupDiscarded");
      WriteTopway(0x00,0x74,2);
    } else {
      if(GroupMinPassLength4!=GroupMinPassLength3) {AuditDetails=STRLOC1 + "-MinPassLength";AuditRemark=String(GroupMinPassLength3) + "->" + String(GroupMinPassLength4);LogActivity("GroupEdited");}
      if(GroupPasswordExpiy4!=GroupPasswordExpiy3) {AuditDetails=STRLOC1 + "-GroupPasswordExpiy";AuditRemark=String(GroupPasswordExpiy3) + "->" + String(GroupPasswordExpiy4);LogActivity("GroupEdited");}
      if(GroupRetryLimits4!=GroupRetryLimits3) {AuditDetails=STRLOC1 + "-GroupRetryLimits";AuditRemark=String(GroupRetryLimits3) + "->" + String(GroupRetryLimits4);LogActivity("GroupEdited");}
      
      if(PasswordComplexity4!=PasswordComplexity3) {AuditDetails=STRLOC1 + "-PasswordComplexity";AuditRemark=PasswordComplexity3 + "->" + PasswordComplexity4;LogActivity("GroupEdited");}
      
      if(LastPAsswordUnique4!=LastPAsswordUnique3) {AuditDetails=STRLOC1 + "-LastPAsswordUnique";AuditRemark=String(LastPAsswordUnique3) + "->" + String(LastPAsswordUnique4);LogActivity("GroupEdited");}
      
      if(ResetPassAuth4!=ResetPassAuth3) {AuditDetails=STRLOC1 + "-ResetPassAuth";AuditRemark=ResetPassAuth3 + "->" + ResetPassAuth4;LogActivity("GroupEdited");}
      if(FixedVolume4!=FixedVolume3) {AuditDetails=STRLOC1 + "-FixedVolume";AuditRemark=FixedVolume3 + "->" + FixedVolume4;LogActivity("GroupEdited");}
      
      if(Logout_Timer4!=Logout_Timer3) {AuditDetails=STRLOC1 + "-Logout_Timer4";AuditRemark=String(Logout_Timer3) + "->" + String(Logout_Timer4);LogActivity("GroupEdited");}
      if(Power_Off_Timer4!=Power_Off_Timer3) {AuditDetails=STRLOC1 + "-PowerOffTimer";AuditRemark=String(Power_Off_Timer3) + "->" + String(Power_Off_Timer4);LogActivity("GroupEdited");}

      if(Software_Access4!=Software_Access3) {AuditDetails=STRLOC1 + "-SoftwareAccess";AuditRemark=Software_Access3 + "->" + Software_Access4;LogActivity("GroupEdited");}
      if(Device_Monitoring4!=Device_Monitoring3) {AuditDetails=STRLOC1 + "-DeviceMonitoring";AuditRemark=Device_Monitoring3 + "->" + Device_Monitoring4;LogActivity("GroupEdited");}
      if(Group_User_Setting4!=Group_User_Setting3) {AuditDetails=STRLOC1 + "-GroupUserSetting";AuditRemark=Group_User_Setting3 + "->" + Group_User_Setting4;LogActivity("GroupEdited");}
      if(Config_Setting4!=Config_Setting3) {AuditDetails=STRLOC1 + "-ConfigSetting";AuditRemark=Config_Setting3 + "->" + Config_Setting4;LogActivity("GroupEdited");}
      if(Sample_Data_View4!=Sample_Data_View3) {AuditDetails=STRLOC1 + "-SampleDataView";AuditRemark=Sample_Data_View3 + "->" + Sample_Data_View4;LogActivity("GroupEdited");}
      if(Audit_Trail_View4!=Audit_Trail_View3) {AuditDetails=STRLOC1 + "-AuditTrailView";AuditRemark=Audit_Trail_View3 + "->" + Audit_Trail_View4;LogActivity("GroupEdited");}
      if(Print_Config4!=Print_Config3) {AuditDetails=STRLOC1 + "-PrintConfig";AuditRemark=Print_Config3 + "->" + Print_Config4;LogActivity("GroupEdited");}
      if(Print_Sample_Data4!=Print_Sample_Data3) {AuditDetails=STRLOC1 + "-PrintSampleData";AuditRemark=Print_Sample_Data3 + "->" + Print_Sample_Data4;LogActivity("GroupEdited");}
      if(Print_Audit_Trail4!=Print_Audit_Trail3) {AuditDetails=STRLOC1 + "-PrintAuditTrail";AuditRemark=Print_Audit_Trail3 + "->" + Print_Audit_Trail4;LogActivity("GroupEdited");}
      if(Allow_Sampling4!=Allow_Sampling3) {AuditDetails=STRLOC1 + "-Allow_Sampling";AuditRemark=Allow_Sampling3 + "->" + Allow_Sampling4;LogActivity("GroupEdited");}
      
      WriteTopway(0x00,0x76,1);
    }
  }
}

void Reset_Group_Bit() {
  WriteTopway(0x00, 0x64, 0);
}

void Set_Group_Bit(byte j, byte val) { 
    ReadTopway(0x00, 0x64); // Read current values into temp_h and temp_l
    if(Read_Err==false) {
      if (j < 8) {
          if (val == 1) {Output_l = temp_l | (1 << j);} else {Output_l = temp_l & ~(1 << j);}
          Output_h = temp_h;
      } else {
          j = j - 8;
          if (val == 1) {Output_h = temp_h | (1 << j);} else {Output_h = temp_h & ~(1 << j);}
          Output_l = temp_l;
      }
      WriteTopway_byte(0x00, 0x64, Output_h, Output_l);
    }
}

void GroupPrevil() {
//  Read_String(0x25,0x00);strGroup=Str2;

  myFile = SD.open("/GRP.csv", FILE_READ);
  if (!myFile) {
     return;
  }
  
  // MEMORY PROTECTION: Reserve string space outside loop
  String line;
  line.reserve(256);

  while (myFile.available()) {
    line = myFile.readStringUntil('\n');  // Read a line from the file
    line.trim();  // Remove any trailing whitespace or newline characters
    
    if (line.length() == 0) continue;

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
    pos11 = line.indexOf(',', pos10 + 1);
    pos12 = line.indexOf(',', pos11 + 1);
    pos13 = line.indexOf(',', pos12 + 1);
    pos14 = line.indexOf(',', pos13 + 1);
    pos15 = line.indexOf(',', pos14 + 1);
    pos16 = line.indexOf(',', pos15 + 1);
    pos17 = line.indexOf(',', pos16 + 1);
    pos18 = line.indexOf(',', pos17 + 1);
    pos19 = line.indexOf(',', pos18 + 1);
    pos20 = line.indexOf(',', pos19 + 1);
    pos21 = line.indexOf(',', pos20 + 1);
    
    if (pos1 == -1 || pos2 == -1 || pos3 == -1 || pos4 == -1 || pos5 == -1 || pos6 == -1 || pos7 == -1 || pos8 == -1) {
      continue;  // Skip malformed lines
    }

    if(Group==line.substring(pos1 + 1, pos2)) {

      GroupStatus1 = (pos3 != -1) ? line.substring(pos2 + 1, pos3) : "";
      GroupMinPassLength1 = (pos4 != -1) ? line.substring(pos3 + 1, pos4).toInt() : 0;
      GroupPasswordExpiy1 = (pos5 != -1) ? line.substring(pos4 + 1, pos5).toInt() : 0;
      GroupRetryLimits1 = (pos6 != -1) ? line.substring(pos5 + 1, pos6).toInt() : 0;  
      PasswordComplexity1 = (pos7 != -1) ? line.substring(pos6 + 1, pos7) : "";
      LastPAsswordUnique1 = (pos8 != -1) ? line.substring(pos7 + 1, pos8).toInt() : 0;
      ResetPassAuth1 = (pos9 != -1) ? line.substring(pos8 + 1, pos9) : "";
      FixedVolume1 = (pos10 != -1) ? line.substring(pos9 + 1, pos10) : "";
      Logout_Timer2 = (pos11 != -1) ? line.substring(pos10 + 1, pos11).toInt() : 0;
      Power_Off_Timer2 = (pos12 != -1) ? line.substring(pos11 + 1, pos12).toInt() : 0;

      Software_Access1 = (pos13 != -1) ? line.substring(pos12 + 1, pos13) : "";
      Device_Monitoring1 = (pos14 != -1) ? line.substring(pos13 + 1, pos14) : "";
      Group_User_Setting1 = (pos15 != -1) ? line.substring(pos14 + 1, pos15) : "";
      Config_Setting1 = (pos16 != -1) ? line.substring(pos15 + 1, pos16) : "";
      Sample_Data_View1 = (pos17 != -1) ? line.substring(pos16 + 1, pos17) : "";
      Audit_Trail_View1 = (pos18 != -1) ? line.substring(pos17 + 1, pos18) : "";
      Print_Config1 = (pos19 != -1) ? line.substring(pos18 + 1, pos19) : "";
      Print_Sample_Data1 = (pos20 != -1) ? line.substring(pos19 + 1, pos20) : "";
      Print_Audit_Trail1 = (pos21 != -1) ? line.substring(pos20 + 1, pos21) : "";
      Allow_Sampling1 = (pos21 != -1) ? line.substring(pos21 + 1) : "";
      myFile.close();
      return;
    }
  }
  myFile.close();
  
}

void GroupPrevil1() {
//  Read_String(0x25,0x00);strGroup=Str2;
  Read_String(0x25,0x00);if(Read_Err==false) {GUserGroup=Str2;}
  myFile = SD.open("/GRP.csv", FILE_READ);
  if (!myFile) {
     return;
  }

  // MEMORY PROTECTION: Reserve string space outside loop
  String line;
  line.reserve(256);

  while (myFile.available()) {
    line = myFile.readStringUntil('\n');  // Read a line from the file
    line.trim();  // Remove any trailing whitespace or newline characters
    
    if (line.length() == 0) continue;

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
    pos11 = line.indexOf(',', pos10 + 1);
    pos12 = line.indexOf(',', pos11 + 1);
    pos13 = line.indexOf(',', pos12 + 1);
    pos14 = line.indexOf(',', pos13 + 1);
    pos15 = line.indexOf(',', pos14 + 1);
    pos16 = line.indexOf(',', pos15 + 1);
    pos17 = line.indexOf(',', pos16 + 1);
    pos18 = line.indexOf(',', pos17 + 1);
    pos19 = line.indexOf(',', pos18 + 1);
    pos20 = line.indexOf(',', pos19 + 1);
    pos21 = line.indexOf(',', pos20 + 1);
    
    if (pos1 == -1 || pos2 == -1 || pos3 == -1 || pos4 == -1 || pos5 == -1 || pos6 == -1 || pos7 == -1 || pos8 == -1) {
      continue;  // Skip malformed lines
    }

    if(GUserGroup==line.substring(pos1 + 1, pos2)) {
      GroupMinPassLength2 = (pos4 != -1) ? line.substring(pos3 + 1, pos4).toInt() : 0;
      PasswordComplexity2 = (pos7 != -1) ? line.substring(pos6 + 1, pos7) : "";
      myFile.close();
      return;
    }
  }
  myFile.close();
}
