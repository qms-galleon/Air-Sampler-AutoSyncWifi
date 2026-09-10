void Location_Bits() {
  ReadTopway(0x00,0x5e);
  if(Read_Err==false) {
    for (i = 7; i >= 0; i--) {
      bitArray[i] = (temp_l >> i) & 1;  // Extract each bit from the low byte
    }
    for (int i = 7; i >= 0; i--) {
      bitArray[i + 8] = (temp_h >> i) & 1;  // Extract each bit from the high byte
    }
    if(bitArray[0]==1) {Logout_Timer=0;CheckConfigParam(1);Reset_Loc_Bit();} //Add Location Button   1
    if(bitArray[1]==1) {Logout_Timer=0;WriteConfigParam(1,"/LOC.csv");Reset_Loc_Bit();} //Confirm Button  2
//    if(bitArray[2]==1) {Logout_Timer=0;DisplayPage(35);Reset_Loc_Bit();} //Discard Button  4
    if(bitArray[3]==1) {Logout_Timer=0;WriteTopway(0x00,0x74,1);Reset_Loc_Bit();} //Popup 8
    if(bitArray[4]==1) {Logout_Timer=0;Discard_Location(1, "/LOC.csv");Reset_Loc_Bit();} //Prev 16 
//    if(bitArray[6]==1) {Logout_Timer=0;DisplayPage(33);Reset_Loc_Bit();} // 64    Location Management Add Button
  } 
}

//void CheckLocationAccess(byte ConfigParam) {
//  if(ConfigParam==1) {DisplayPage(33);}
//  else if(ConfigParam==2) {DisplayPage(35);}
//}

void CheckConfigParam(byte ConfigParam) {
  if(ConfigParam==1) {Read_String(0x0c,0x80);ParamId=LocationId;}
  else if(ConfigParam==2) {Read_String(0x0d,0x80);ParamId=RemarkId;}
  //else if(ConfigParam==2) {Read_String(0x0f,0x80);ParamId=GUserId;}
  if(Read_Err==false) {
    strParam=Str2;
    if(strParam.length() < 4 || strParam.length() > 20) {
      WriteTopway(0x00,0x72,1); //Length Warning Popup
    } else {
      if(ConfigParam==1) {CheckLocation("/LOC.csv");}
      if(ConfigParam==2) {CheckLocation("/RMK.csv");}
    }
  }
}

void CheckLocation(const String &Strfile1) {
  bool ConfigExists = false;

  // Check if file exists first to avoid errors
  if (!SD.exists(Strfile1)) {
     // If file doesn't exist, the config definitely doesn't exist, so we can proceed to add it.
     OpStatus=true;
     WriteTopway(0x00, 0x74, 1);
     return;
  }

  myFile = SD.open(Strfile1, O_RDONLY); // Use O_RDONLY in SdFat
  if (myFile) {
    // MEMORY PROTECTION: Move String outside loop and reserve memory
    String line;
    line.reserve(128); 
    
    while (myFile.available()) {
      char c = myFile.read();
      if (c == '\n') {
        // Process the complete line
        int pos1 = line.indexOf(',');
        int pos2 = line.indexOf(',', pos1 + 1);

        if (pos1 != -1 && pos2 != -1) {
          int fieldStart = pos1 + 1;
          int fieldLength = pos2 - fieldStart;
          String locationName = line.substring(fieldStart, fieldStart + fieldLength);
          locationName.trim();

          if (locationName.equals(Str2)) {
            ConfigExists = true;
            break;
          }
        }
        line = ""; // Clear for next line (reuse memory)
      } else {
        line += c; // Keep building the line
      }
    }
    myFile.close();
  }

  if (ConfigExists) {
    WriteTopway(0x00, 0x76, 2); // Already exists popup
    OpStatus=false;
  } else {
    if(Strfile1=="/RECP.csv") {
      OpStatus=true;
      if(SoftWareConnected ==false) {DisplayPage(56);}
    } else {
      OpStatus=true;
      WriteTopway(0x00, 0x74, 1); // Add new parameter popup
    }
  }
}

void WriteConfigParam(byte ConfigParam, String Strfile1) {
  myFile = SD.open(Strfile1, O_RDWR | O_CREAT | O_APPEND);
  
//  if (!SD.exists(Strfile1)) {
//    myFile = SD.open(Strfile1, FILE_WRITE);
//  } else {
//    myFile = SD.open(Strfile1, O_RDWR);
//  }
  
  if (myFile) {
    myFile.print(ParamId); myFile.print(",");
    myFile.print(strParam); myFile.print(",");
    myFile.println("Active");
    
    myFile.flush(); // Critical: Force write
    myFile.close();
  }
  if(ConfigParam==1) {AuditDetails=strParam;AuditRemark="NA";LogActivity("LocationCreated");LocationId++;WriteTopway(0x00,0x6e,LocationId);WriteTopway(0x00,0x76,1);/*Add Location Popup*/}
  else if(ConfigParam==2) {AuditDetails=strParam;AuditRemark="NA";LogActivity("RemarkCreated");RemarkId++;WriteTopway(0x00,0x70,RemarkId);WriteTopway(0x00,0x76,1);/*Add Remark Popup*/}
}

void Discard_Location(byte ConfigParam, String Strfile1) {
  String STRLOC1;
  if(ConfigParam==1) {Read_String(0x04,0x80);}
  else if(ConfigParam==2) {Read_String(0x1e,0x00);}
  
  if(Read_Err==false) {
    STRLOC1=Str2;
    WriteString(0x00,0x21,0x80,STRLOC1);
    
    // ATOMIC UPDATE Strategy
    const char* tempFileName = "/temp.csv";
    if(SD.exists(tempFileName)) SD.remove(tempFileName);

    tempFile = SD.open(tempFileName, O_WRITE | O_CREAT | O_TRUNC);
    myFile = SD.open(Strfile1, O_READ);
    
    if (!myFile || !tempFile) {
       if(myFile) myFile.close();
       if(tempFile) tempFile.close();
       return; 
    }

    char line[128];
    while (myFile.fgets(line, sizeof(line))) {
      String row = String(line);
      row.trim();  // Remove \n or \r\n
  
      // Split the CSV row
      pos1 = row.indexOf(',');
      pos2 = row.indexOf(',', pos1 + 1);
  
      if (pos1 == -1 || pos2 == -1) {
        // Malformed row, write it as is
        tempFile.println(row);
        continue;
      }
  
      String ID = row.substring(0, pos1);
      String STRLOC = row.substring(pos1 + 1, pos2);
      String STATUS = row.substring(pos2 + 1);
  
      if (STRLOC == STRLOC1 && STATUS == "Active") {
        STATUS = "Discard";
      }
  
      String newRow = ID + "," + STRLOC + "," + STATUS;
      tempFile.println(newRow);
    }
    
    myFile.close();
    
    // Flush and Close temp file BEFORE renaming
    tempFile.flush();
    tempFile.close();
  
    if(SD.exists(Strfile1)) SD.remove(Strfile1);
    SD.rename(tempFileName, Strfile1);
    
    WriteTopway(0x00,0x74,2); 
    if(ConfigParam==1 || ConfigParam==3) {AuditDetails=STRLOC1;AuditRemark="NA";LogActivity("LocationDiscarded");}
    if(ConfigParam==2 || ConfigParam==4) {AuditDetails=STRLOC1;AuditRemark="NA";LogActivity("RemarkDiscarded");}
  }
}

void Status_Location() {
  
}

void Reset_Loc_Bit() {
  WriteTopway(0x00, 0x5e, 0);
}

void Set_Loc_Bit(byte j, byte val) { 
    ReadTopway(0x00, 0x5e); // Read current values into temp_h and temp_l
    if(Read_Err==false) {
      if (j < 8) {
          if (val == 1) {Output_l = temp_l | (1 << j);} else {Output_l = temp_l & ~(1 << j);}
          Output_h = temp_h;
      } else {
          j = j - 8;
          if (val == 1) {Output_h = temp_h | (1 << j);} else {Output_h = temp_h & ~(1 << j);}
          Output_l = temp_l;
      }
      WriteTopway_byte(0x00, 0x5e, Output_h, Output_l);
    }
}
