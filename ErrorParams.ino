void getMotorErrorParams() {
  const char* fileName = "/Motor.CSV";

  // Check if file exists first. If not, create defaults safely.
  if (!SD.exists(fileName)) {
    WriteTopway(0x00, 0xce, Hblock_Min);
    WriteTopway(0x00, 0xd0, Hblock_Max);
    WriteTopway(0x00, 0xd2, Hopen_Min);
    WriteTopway(0x00, 0xd4, Hopen_Max);

    myFile = SD.open(fileName, O_WRITE | O_CREAT | O_TRUNC);
    if (myFile) {
      myFile.print(Hblock_Min); myFile.print(",");
      myFile.print(Hblock_Max); myFile.print(",");
      myFile.print(Hopen_Min); myFile.print(",");
      myFile.println(Hopen_Max);
      
      myFile.flush(); // Force write
      myFile.close();
    }
    return;
  }

  // File exists, open for reading
  myFile = SD.open(fileName, FILE_READ);
  if (!myFile) return;

  // MEMORY PROTECTION: Reserve memory to prevent fragmentation
  String line;
  line.reserve(64);

  while (myFile.available()) {
    line = myFile.readStringUntil('\n'); 
    line.trim(); 

    if (line.length() == 0) continue; // Skip empty lines

    // Split line into values using comma as a delimiter
    int pos1 = line.indexOf(',');
    int pos2 = line.indexOf(',', pos1 + 1);
    int pos3 = line.indexOf(',', pos2 + 1);

    if (pos1 == -1 || pos2 == -1 || pos3 == -1) {
      continue;  // Skip malformed lines
    }

    Hblock_Min = line.substring(0, pos1).toInt();
    Hblock_Max = line.substring(pos1 + 1, pos2).toInt();
    Hopen_Min = line.substring(pos2 + 1, pos3).toInt();
    Hopen_Max = line.substring(pos3 + 1).toInt();
  }
  myFile.close();
  
  // Update Display
  WriteTopway(0x00, 0xce, Hblock_Min);
  WriteTopway(0x00, 0xd0, Hblock_Max);
  WriteTopway(0x00, 0xd2, Hopen_Min);
  WriteTopway(0x00, 0xd4, Hopen_Max);
}

void WriteMotorDetails() {
  // Read current values from Topway Display
  ReadTopway(0x00, 0xce); if(Read_Err==false) {Hblock_Min = (temp_h << 8) | temp_l;}
  ReadTopway(0x00, 0xd0); if(Read_Err==false) {Hblock_Max = (temp_h << 8) | temp_l;}
  ReadTopway(0x00, 0xd2); if(Read_Err==false) {Hopen_Min = (temp_h << 8) | temp_l;}
  ReadTopway(0x00, 0xd4); if(Read_Err==false) {Hopen_Max = (temp_h << 8) | temp_l;}
  
  // ATOMIC WRITE STRATEGY
  const char *tmpFile = "/Motor.tmp";
  const char *finalFile = "/Motor.CSV";

  // Clean previous temp file if it exists
  if (SD.exists(tmpFile)) SD.remove(tmpFile);

  // Open Temp File
  myFile = SD.open(tmpFile, O_WRITE | O_CREAT | O_TRUNC);
  
  if (myFile) {
    myFile.print(Hblock_Min); myFile.print(",");
    myFile.print(Hblock_Max); myFile.print(",");
    myFile.print(Hopen_Min); myFile.print(",");
    myFile.println(Hopen_Max);
    
    myFile.flush(); // Critical: Force write to SD card
    myFile.close(); // Critical: Ensure FAT update
    
    // Safely replace the old file
    if (SD.exists(finalFile)) SD.remove(finalFile);
    SD.rename(tmpFile, finalFile);
  } else {
    // If we failed to open/create file, exit to avoid false success msg
    return; 
  }

  LogActivity("MotorDetailsUpdated");
  WriteTopway(0x00, 0x72, 4); // Success Popup
}

/*void SetBG(int PageNo, int strID, int ColorID) { //0 - Black, 1 - White, 2 - Red, 3 - Green, 4 - Orange
  BGCmd();
  Serial.write((byte) PageNo);
  Serial.write((byte) strID);
  if(ColorID==0) {Serial.write((byte) 0xff);Serial.write((byte) 0xff);Serial.write((byte) 0xff);}
  else if(ColorID==1) {Serial.write((byte) 0x00);Serial.write((byte) 0x00);Serial.write((byte) 0x00);}
  else if(ColorID==2) {Serial.write((byte) 0x00);Serial.write((byte) 0xf0);Serial.write((byte) 0x00);}
  else if(ColorID==3) {Serial.write((byte) 0x00);Serial.write((byte) 0x0f);Serial.write((byte) 0x00);}
  else if(ColorID==4) {Serial.write((byte) 0x00);Serial.write((byte) 0xff);Serial.write((byte) 0x00);}
  EndPacket();
}

void SetFG(int PageNo, int strID, int ColorID) { //0 - Black, 1 - White, 2 - Red, 3 - Green, 4 - Orange
  FGCmd();
  Serial.write((byte) PageNo);
  Serial.write((byte) strID);
  if(ColorID==0) {Serial.write((byte) 0xff);Serial.write((byte) 0xff);Serial.write((byte) 0xff);}
  else if(ColorID==1) {Serial.write((byte) 0x00);Serial.write((byte) 0x00);Serial.write((byte) 0x00);}
  else if(ColorID==2) {Serial.write((byte) 0x00);Serial.write((byte) 0xf0);Serial.write((byte) 0x00);}
  else if(ColorID==3) {Serial.write((byte) 0x00);Serial.write((byte) 0x0f);Serial.write((byte) 0x00);}
  else if(ColorID==4) {Serial.write((byte) 0x00);Serial.write((byte) 0xff);Serial.write((byte) 0x00);}
  EndPacket();
}

void BGCmd() {
  Serial.write(0xaa);
  Serial.write(0x7e);// write command
  Serial.write((byte) 0x00); //00 - Str, 01 - N16/N32
  Serial.write((byte) 0x00);
}

void FGCmd() {
  Serial.write(0xaa);
  Serial.write(0x7f);// VP_N16 write command
  Serial.write((byte) 0x00);
  Serial.write((byte) 0x00);
}*/
