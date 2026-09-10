void ConfigRecipe() {
  ReadTopway(0x00,0x62);
  if(Read_Err==false) {
    for (i = 7; i >= 0; i--) {
      bitArray[i] = (temp_l >> i) & 1;  // Extract each bit from the low byte
    }
    for (int i = 7; i >= 0; i--) {
      bitArray[i + 8] = (temp_h >> i) & 1;  // Extract each bit from the high byte
    }
    if(bitArray[0]==1) {Logout_Timer=0;CheckRecipeConfig(1);Reset_Recipe_Bit();} //1      Add Recipe Next Button
    if(bitArray[1]==1) {Logout_Timer=0;CheckRecipeConfig(2);Reset_Recipe_Bit();} //2      Add Recipe Next Button
    if(bitArray[2]==1) {Logout_Timer=0;CheckRecipeConfig(3);Reset_Recipe_Bit();} //4      Add Recipe Next Button
    if(bitArray[3]==1) {Logout_Timer=0;AddRecipe();Reset_Recipe_Bit();} //8      Add Recipe Button
    if(bitArray[4]==1) {Logout_Timer=0;RecipeSummary(81);Reset_Recipe_Bit();} //16      Recipe Sample Summary Button
    if(bitArray[5]==1) {Logout_Timer=0;RecipeSummary(46);Reset_Recipe_Bit();} //32      View Recipe Summary Button
    if(bitArray[6]==1) {Logout_Timer=0;RecipeSummary(131);Reset_Recipe_Bit();} //64      Edit Recipe 1 
    if(bitArray[7]==1) {Logout_Timer=0;CheckRecipeConfig(4);Reset_Recipe_Bit();} //128      Edit Recipe 2 
    if(bitArray[8]==1) {Logout_Timer=0;Update_Recipe();Reset_Recipe_Bit();} //256      Edit Recipe 3 Summary Button
//    if(bitArray[9]==1) {Logout_Timer=0;DisplayPage(43);Reset_Recipe_Bit();} //512      Recipe Mgmt/Discard Recipe Summary Button
    if(bitArray[10]==1) {Logout_Timer=0;WriteTopway(0x00,0x74,1);Reset_Recipe_Bit();} //1024      Recipe Mgmt/Discard Recipe Button
    if(bitArray[11]==1) {Logout_Timer=0;Update_Recipe();Reset_Recipe_Bit();} //2048      Discard Recipe Button
  }
}

void RecipeSummary( int PageNo) {
  Read_String(0x0f,0x00);strRecipe=Str2;

  if (!SD.exists("/RECP.csv")) return;

  myFile = SD.open("/RECP.csv", FILE_READ);
  if (!myFile) {
     return;
  }

  // MEMORY PROTECTION: Move String outside loop
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
    
    if (pos1 == -1 || pos2 == -1 || pos3 == -1 || pos4 == -1 || pos5 == -1 || pos6 == -1 || pos7 == -1 || pos8 == -1) {
      continue;  // Skip malformed lines
    }

    if(strRecipe==line.substring(pos1 + 1, pos2)) {
      RecipeMode = (pos3 != -1) ? line.substring(pos2 + 1, pos3) : "";
      RecipeLoc = (pos4 != -1) ? line.substring(pos3 + 1, pos4) : "";
      RecipeVolume = (pos5 != -1) ? line.substring(pos4 + 1, pos5).toInt() : 0;
      RecipeStartDelay = (pos6 != -1) ? line.substring(pos5 + 1, pos6).toInt() : 0;
      RecipeNoOfRuns = (pos7 != -1) ? line.substring(pos6 + 1, pos7).toInt() : 0;
      RecipeDelayBetRun = (pos8 != -1) ? line.substring(pos7 + 1, pos8).toInt() : 0;
      //SamplingRemark="Recipe Sample";

      WriteString(0x00,0x03,0x00,RecipeMode);
      WriteString(0x00,0x04,0x80,RecipeLoc);
      WriteTopway(0x00,0x08,RecipeVolume);
      WriteTopway(0x00,0x34,RecipeStartDelay);
      WriteTopway(0x00,0x36,RecipeNoOfRuns);
      WriteTopway(0x00,0x38,RecipeDelayBetRun);
      DisplayPage(PageNo);
      myFile.close();
      return;
    }
  }
  myFile.close();
  
}

void CheckRecipeConfig(byte ConfigParam) {
  if(ConfigParam==1) {Read_String(0x0f,0x00);}
  else if(ConfigParam==2) {Read_String(0x13,0x00);}
  else if(ConfigParam==3) {Read_String(0x13,0x00);}
  else if(ConfigParam==4) {Read_String(0x03,0x00);}
  if(Read_Err==false) {
    if(Str2.length() < 4 || Str2.length() > 20) {
      WriteTopway(0x00,0x72,1); //Length Warning Popup
    } else {
      if(ConfigParam==1) {CheckLocation("/RECP.csv");}
      else if(ConfigParam==2) {DisplayPage(60);}
      else if(ConfigParam==3) {DisplayPage(59);}
      else if(ConfigParam==4) {if(Str2=="Single") {WriteTopway(0x00,0x8c,1);WriteTopway(0x00,0x8e,0);DisplayPage(133);} else {DisplayPage(132);}}
    }
  }
}

void CheckRecipe() {
  bool ConfigExists = false;

  if (!SD.exists("/RECP.csv")) {
     // File doesn't exist, proceed to create
     DisplayPage(56);
     return;
  }

  myFile = SD.open("/RECP.csv", O_RDONLY); // Use O_RDONLY in SdFat
  if (myFile) {
    // MEMORY PROTECTION: Use readStringUntil instead of char-by-char accumulation
    String line;
    line.reserve(128); 

    while (myFile.available()) {
      line = myFile.readStringUntil('\n');
      line.trim();
      
      if (line.length() == 0) continue;

      int pos1 = line.indexOf(',');
      int pos2 = line.indexOf(',', pos1 + 1);

      if (pos1 != -1 && pos2 != -1) {
        int fieldStart = pos1 + 1;
        int fieldLength = pos2 - fieldStart;
        String RecipeName = line.substring(fieldStart, fieldStart + fieldLength);
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
  } else {
    DisplayPage(56);
  }
}

void AddRecipe() {
  Read_String(0x0f,0x00);if(Read_Err==false) {strRecipe=Str2;}
  Read_String(0x13,0x80);if(Read_Err==false) {RecipeMode=Str2;}
  Read_String(0x13,0x00);if(Read_Err==false) {RecipeLoc=Str2;}
  ReadTopway(0x00,0x88);if(Read_Err==false) {RecipeVolume=(temp_h << 8) | temp_l;}
  ReadTopway(0x00,0x8a);if(Read_Err==false) {RecipeStartDelay=(temp_h << 8) | temp_l;}
  ReadTopway(0x00,0x8c);if(Read_Err==false) {RecipeNoOfRuns=(temp_h << 8) | temp_l;}
  ReadTopway(0x00,0x8e);if(Read_Err==false) {RecipeDelayBetRun=(temp_h << 8) | temp_l;}
  myFile = SD.open("/RECP.csv", O_RDWR | O_CREAT | O_APPEND);

  if (myFile) {
    myFile.print(RecipeId); myFile.print(",");
    myFile.print(strRecipe); myFile.print(",");
    myFile.print(RecipeMode); myFile.print(",");
    myFile.print(RecipeLoc); myFile.print(",");
    myFile.print(RecipeVolume); myFile.print(",");
    myFile.print(RecipeStartDelay); myFile.print(",");
    myFile.print(RecipeNoOfRuns); myFile.print(",");
    myFile.print(RecipeDelayBetRun); myFile.print(",");
    myFile.println("Active");
    
    myFile.flush(); // Critical: Force write
    myFile.close();
  }
  AuditDetails=strRecipe;AuditRemark="OK";
  LogActivity("RecipeCreated");
  RecipeId++;
  WriteTopway(0x00,0x7a,RecipeId);
  WriteTopway(0x00,0x76,1);
}

void AddRecipe1() {
  myFile = SD.open("/RECP.csv", O_RDWR | O_CREAT | O_APPEND);

  if (myFile) {
    myFile.print(RecipeId); myFile.print(",");
    myFile.print(strRecipe); myFile.print(",");
    myFile.print(RecipeMode); myFile.print(",");
    myFile.print(RecipeLoc); myFile.print(",");
    myFile.print(RecipeVolume); myFile.print(",");
    myFile.print(RecipeStartDelay); myFile.print(",");
    myFile.print(RecipeNoOfRuns); myFile.print(",");
    myFile.print(RecipeDelayBetRun); myFile.print(",");
    myFile.println("Active");
    
    myFile.flush(); // Critical: Force write
    myFile.close();
  }
  AuditDetails=strRecipe;AuditRemark="OK";
  LogActivity("RecipeCreated");
  RecipeId++;
  WriteTopway(0x00,0x7a,RecipeId);
}

void Update_Recipe() {
  String STRLOC1;
  Read_String(0x0f,0x00);
  if(Read_Err==false) {
    STRLOC1=Str2;
    WriteString(0x00,0x21,0x80,STRLOC1);
    
    // ATOMIC UPDATE Strategy
    const char *tmpName = "/temp.csv";
    const char *fileName = "/RECP.csv";
    
    if (SD.exists(tmpName)) SD.remove(tmpName);
    tempFile = SD.open(tmpName, O_WRITE | O_CREAT | O_TRUNC);
    
    myFile = SD.open(fileName, O_READ);
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
      pos3 = row.indexOf(',', pos2 + 1);
      pos4 = row.indexOf(',', pos3 + 1);
      pos5 = row.indexOf(',', pos4 + 1);
      pos6 = row.indexOf(',', pos5 + 1);
      pos7 = row.indexOf(',', pos6 + 1);
      pos8 = row.indexOf(',', pos7 + 1);
  
      if (pos1 == -1 || pos2 == -1 || pos3 == -1 || pos4 == -1 || pos5 == -1 || pos6 == -1 || pos7 == -1 || pos8 == -1) {
        // Malformed row, write it as is
        tempFile.println(row);
        continue;
      }
  
      String ID = row.substring(0, pos1);
      strRecipe = row.substring(pos1 + 1, pos2);
      RecipeMode = (pos3 != -1) ? row.substring(pos2 + 1, pos3) : "";
      RecipeLoc = (pos4 != -1) ? row.substring(pos3 + 1, pos4) : "";
      RecipeVolume = (pos5 != -1) ? row.substring(pos4 + 1, pos5).toInt() : 0;
      RecipeStartDelay = (pos6 != -1) ? row.substring(pos5 + 1, pos6).toInt() : 0;
      RecipeNoOfRuns = (pos7 != -1) ? row.substring(pos6 + 1, pos7).toInt() : 0;
      RecipeDelayBetRun = (pos8 != -1) ? row.substring(pos7 + 1, pos8).toInt() : 0;
      String STATUS = row.substring(pos8 + 1);
//      String STRLOC = row.substring(pos1 + 1, pos2);
//      String STATUS = row.substring(pos2 + 1);

      String newRow = "";
      if (strRecipe == STRLOC1) {
        if(Page_No==43) {
          STATUS = "Discard";
          WriteString(0x00,0x21,0x80,STRLOC1);
          newRow = ID + "," + strRecipe + "," + RecipeMode + "," + RecipeLoc + "," + RecipeVolume + "," + RecipeStartDelay + "," + RecipeNoOfRuns + "," + RecipeDelayBetRun + "," + STATUS;
        } else {
          RecipeMode1=RecipeMode;
          RecipeLoc1=RecipeLoc;
          RecipeVolume1=RecipeVolume;
          RecipeStartDelay1=RecipeStartDelay;
          RecipeNoOfRuns1=RecipeNoOfRuns;
          RecipeDelayBetRun1=RecipeDelayBetRun;
          Read_String(0x03,0x00);if(Read_Err==false) {RecipeMode2=Str2;}
          Read_String(0x04,0x80);if(Read_Err==false) {RecipeLoc2=Str2;}
          ReadTopway(0x00,0x08);if(Read_Err==false) {RecipeVolume2=(temp_h << 8) | temp_l;}
          ReadTopway(0x00,0x34);if(Read_Err==false) {RecipeStartDelay2=(temp_h << 8) | temp_l;}
          ReadTopway(0x00,0x36);if(Read_Err==false) {RecipeNoOfRuns2=(temp_h << 8) | temp_l;}
          ReadTopway(0x00,0x38);if(Read_Err==false) {RecipeDelayBetRun2=(temp_h << 8) | temp_l;}
          newRow = ID + "," + strRecipe + "," + RecipeMode2 + "," + RecipeLoc2 + "," + RecipeVolume2 + "," + RecipeStartDelay2 + "," + RecipeNoOfRuns2 + "," + RecipeDelayBetRun2 + "," + STATUS;
        }
      } else {
        newRow = ID + "," + strRecipe + "," + RecipeMode + "," + RecipeLoc + "," + RecipeVolume + "," + RecipeStartDelay + "," + RecipeNoOfRuns + "," + RecipeDelayBetRun + "," + STATUS;
      }
      tempFile.println(newRow);
    }
    myFile.close();
    
    // Critical: Flush before close
    tempFile.flush();
    tempFile.close();
  
    SD.remove(fileName);
    SD.rename(tmpName, fileName);
    
    if(Page_No==43) {
      AuditDetails=STRLOC1;AuditRemark="NA";
      LogActivity("RecipeDiscarded");
      WriteTopway(0x00,0x74,2);
    } else {
      if(RecipeMode2!=RecipeMode1) {AuditDetails=STRLOC1 + "-Run";AuditRemark=RecipeMode1 + "->" + RecipeMode2;LogActivity("RecipeEdited");}
      if(RecipeLoc2!=RecipeLoc1) {AuditDetails=STRLOC1 + "-Location";AuditRemark=RecipeLoc1 + "->" + RecipeLoc2;LogActivity("RecipeEdited");}
      if(RecipeVolume2!=RecipeVolume1) {AuditDetails=STRLOC1 + "-Volume";AuditRemark=String(RecipeVolume1) + "->" + String(RecipeVolume2);LogActivity("RecipeEdited");}
      if(RecipeStartDelay2!=RecipeStartDelay1) {AuditDetails=STRLOC1 + "-StartDelay";AuditRemark=String(RecipeStartDelay1) + "->" + String(RecipeStartDelay2);LogActivity("RecipeEdited");}
      if(RecipeNoOfRuns2!=RecipeNoOfRuns1) {AuditDetails=STRLOC1 + "-NoOfRuns";AuditRemark=String(RecipeNoOfRuns1) + "->" + String(RecipeNoOfRuns2);LogActivity("RecipeEdited");}
      if(RecipeDelayBetRun2!=RecipeDelayBetRun1) {AuditDetails=STRLOC1 + "-DelBetwRun";AuditRemark=String(RecipeDelayBetRun1) + "->" + String(RecipeDelayBetRun2);LogActivity("RecipeEdited");}
      WriteTopway(0x00,0x76,1);
    }
  }
}

void Discard_Recipe() {
  String STRLOC1;
  STRLOC1=Str2;
  WriteString(0x00,0x21,0x80,STRLOC1);
  
  const char *tmpName = "/temp.csv";
  const char *fileName = "/RECP.csv";

  if (SD.exists(tmpName)) SD.remove(tmpName);
  
  tempFile = SD.open(tmpName, O_WRITE | O_CREAT | O_TRUNC);
  myFile = SD.open(fileName, O_READ);
  
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
    pos3 = row.indexOf(',', pos2 + 1);
    pos4 = row.indexOf(',', pos3 + 1);
    pos5 = row.indexOf(',', pos4 + 1);
    pos6 = row.indexOf(',', pos5 + 1);
    pos7 = row.indexOf(',', pos6 + 1);
    pos8 = row.indexOf(',', pos7 + 1);
    if (pos8 == -1) {
      // Malformed row, write it as is
      tempFile.println(row);
      continue;
    }

    String ID = row.substring(0, pos1);
    strRecipe = row.substring(pos1 + 1, pos2);
    RecipeMode = (pos3 != -1) ? row.substring(pos2 + 1, pos3) : "";
    RecipeLoc = (pos4 != -1) ? row.substring(pos3 + 1, pos4) : "";
    RecipeVolume = (pos5 != -1) ? row.substring(pos4 + 1, pos5).toInt() : 0;
    RecipeStartDelay = (pos6 != -1) ? row.substring(pos5 + 1, pos6).toInt() : 0;
    RecipeNoOfRuns = (pos7 != -1) ? row.substring(pos6 + 1, pos7).toInt() : 0;
    RecipeDelayBetRun = (pos8 != -1) ? row.substring(pos7 + 1, pos8).toInt() : 0;
    String STATUS = row.substring(pos8 + 1);
      
    if (strRecipe == STRLOC1 && STATUS == "Active") {
      STATUS = "Discard";
    }
    
    String newRow = ID + "," + strRecipe + "," + RecipeMode + "," + RecipeLoc + "," + RecipeVolume + "," + RecipeStartDelay + "," + RecipeNoOfRuns + "," + RecipeDelayBetRun + "," + STATUS;
    tempFile.println(newRow);
  }
  myFile.close();
  
  // Critical: Flush before close
  tempFile.flush();
  tempFile.close();

  SD.remove(fileName);
  SD.rename(tmpName, fileName); 
  
  AuditDetails=STRLOC1;AuditRemark="NA";LogActivity("RecipeDiscarded");
 
}

void Reset_Recipe_Bit() {
  WriteTopway(0x00, 0x62, 0);
}

void Set_Recipe_Bit(byte j, byte val) { 
    ReadTopway(0x00, 0x62); // Read current values into temp_h and temp_l
    if(Read_Err==false) {
      if (j < 8) {
          if (val == 1) {Output_l = temp_l | (1 << j);} else {Output_l = temp_l & ~(1 << j);}
          Output_h = temp_h;
      } else {
          j = j - 8;
          if (val == 1) {Output_h = temp_h | (1 << j);} else {Output_h = temp_h & ~(1 << j);}
          Output_l = temp_l;
      }
      WriteTopway_byte(0x00, 0x62, Output_h, Output_l);
    }
}
