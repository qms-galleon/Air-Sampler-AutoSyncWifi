void ProcessScreen() {
  DisplayPage(1); //Login Page
  WriteTopway(0x00,0xcc,0); //Disable Setting 2 touch screen
  WriteTopway(0x01,0x1e,0);
  WriteTopway(0x01,0x20,0);
  WriteTime();
//  WriteString(0x00,0x00,0x80,"PAJ2");
//  WriteString(0x00,0x01,0x00,"1234");
  getDeviceDetails();  //OK
  getCompanyDetails(); //OK
  getLastDeviceLogs();
  getMotorErrorParams(); //OK
  ReadSampleId(); //OK
  WriteClockParams();
  LocationId=ReadConfigParam("/LOC.csv");WriteTopway(0x00,0x6e,LocationId); //LOCATION //OK
  RemarkId=ReadConfigParam("/RMK.csv");WriteTopway(0x00,0x70,RemarkId);  //REMARK //OK
  RecipeId=ReadConfigParam("/RECP.csv");WriteTopway(0x00,0x7a,RecipeId); //RECIPE //OK
  GroupId=ReadConfigParam("/GRP.csv");WriteTopway(0x00,0x7c,GroupId); //GROUP //OK
  GUserId=ReadConfigParam("/USR.csv");WriteTopway(0x00,0x7e,GUserId); //USER //OK
  
  ShowMemoryInfo();
  CalID=ReadConfigParam("/CAL.csv");
  if(CalID==1) {
     WriteTopway(0x00,0x98,1);
  } else {
    ReadCal();
    ReadCalValidity();
  }
  WriteTemp();
}

void ShowMemoryInfo() {
  WriteTopway(0x00,0xc6,LocationId-1);
  WriteTopway(0x00,0xca,RemarkId-1);
  WriteTopway(0x00,0xc8,RecipeId-1);
  WriteTopway(0x00,0xc4,GUserId-1);
  WriteTopway(0x00,0xc2,SampleId);
}

void getCompanyDetails() {
  if (SD.exists("/DevInf.csv")) {
    // Check if CompDet exists before opening
    if(SD.exists("/CompDet.CSV")) {
        myFile = SD.open("/CompDet.CSV", FILE_READ);
        
        if(myFile) {
            // MEMORY PROTECTION: Move String outside loop
            String line;
            line.reserve(64);

            while (myFile.available()) {
              line = myFile.readStringUntil('\n');  // Read a line from the file
              line.trim();  // Remove any trailing whitespace or newline characters
          
              // Split line into values using comma as a delimiter
              pos1 = line.indexOf(',');
              pos2 = line.indexOf(',', pos1 + 1);
          
              if (pos1 == -1 || pos2 == -1) {
                continue;  // Skip malformed lines
              }
              Company_Name = line.substring(0, pos1);
              City_Name = line.substring(pos1 + 1, pos2);
              State_Name = line.substring(pos2 + 1);
            }
            myFile.close();
        }
    }
    
    WriteString(0x00,0x1b,0x00,Company_Name);WriteString(0x00,0x1e,0x80,Company_Name);
    WriteString(0x00,0x1b,0x80,City_Name);WriteString(0x00,0x1f,0x00,City_Name);
    WriteString(0x00,0x1c,0x00,State_Name);WriteString(0x00,0x1f,0x80,State_Name);
  } else {
    myFile = SD.open("/CompDet.csv", O_WRITE | O_CREAT | O_TRUNC);
    if (myFile) {
      myFile.print(Company_Name); myFile.print(",");
      myFile.print(City_Name); myFile.print(",");
      myFile.println(State_Name);
      
      myFile.flush(); // Force write
      myFile.close();
    }
  }

  Company_Name1=Company_Name;
  City_Name1=City_Name;
  State_Name1=State_Name;
    
}

void WriteTemp() {
  WriteString(0x00,0x14,0x00,"");
  WriteString(0x00,0x14,0x80,"");
  WriteString(0x00,0x15,0x00,"");
  WriteString(0x00,0x15,0x80,"");
  WriteString(0x00,0x16,0x00,"");
  WriteString(0x00,0x16,0x80,"");
  WriteString(0x00,0x17,0x00,"");
  WriteString(0x00,0x17,0x80,"");
  WriteString(0x00,0x18,0x00,"");
  WriteString(0x00,0x18,0x80,"");
  
}

void WriteClockParams() {
  date_set=now1.day();
  month_set=now1.month();
  year_set=now1.year();
  hour_set=now1.hour();
  minute_set=now1.minute();
  WriteTopway(0x00,0x3C,date_set);
  WriteTopway(0x00,0x3E,month_set);
  WriteTopway(0x00,0x40,year_set);
  WriteTopway(0x00,0x42,hour_set);
  WriteTopway(0x00,0x44,minute_set);
}

int ReadConfigParam(String Strfile1) {
  int ParamId = 1;  // Default to 1

  if (!SD.exists(Strfile1)) {
    // Create the file if it doesn't exist
    myFile = SD.open(Strfile1, FILE_WRITE);
    if (myFile) {
      //myFile.println(); // Optional: write header or just create empty file
      myFile.close();
    }
    return ParamId;  // Return 1 since file is empty
  }

  // If file exists, read the last line
  myFile = SD.open(Strfile1, O_RDONLY);  // Open file for reading
  if (myFile) {
    String lastLine = readLastLine();
    myFile.close();

    // Parse the first field from the last line
    int commaIndex = lastLine.indexOf(',');
    ParamId = (commaIndex != -1) 
              ? lastLine.substring(0, commaIndex).toInt() + 1 
              : lastLine.toInt() + 1;
  }

  return ParamId;
}

long ReadConfigParam1(String Strfile1) {
  long ParamId = 1;  // Default to 1

  if (!SD.exists(Strfile1)) {
    // Create the file if it doesn't exist
    myFile = SD.open(Strfile1, FILE_WRITE);
    if (myFile) {
      //myFile.println(); // Optional: write header or just create empty file
      myFile.close();
    }
    return ParamId;  // Return 1 since file is empty
  }

  // If file exists, read the last line
  myFile = SD.open(Strfile1, O_RDONLY);  // Open file for reading
  if (myFile) {
    String lastLine = readLastLine();
    myFile.close();

    // Parse the first field from the last line
    int commaIndex = lastLine.indexOf(',');
    ParamId = (commaIndex != -1) 
              ? lastLine.substring(0, commaIndex).toInt() + 1 
              : lastLine.toInt() + 1;
  }

  return ParamId;
}

String readLastLine() {
  uint32_t fileSize = myFile.size();
  if (fileSize == 0) return "";  // Return empty string if file is empty

  String line = "";
  // MEMORY PROTECTION
  line.reserve(64); 
  
  uint32_t pos = fileSize - 1;
  int count = 0;
  
  // Seek backward from the last byte to find the last line
  // Added limit check to prevent infinite loop if something goes wrong
  while (pos >= 0 && count < 200) { 
    myFile.seek(pos);
    char c = myFile.read();

    // Stop when a newline is found, but avoid reading a trailing empty line
    if (c == '\n' && pos != fileSize - 1) {
      break; 
    }

    // Prepend the character (as we are reading backwards)
    // Note: Prepending strings (line = c + line) is generally bad for memory
    // but unavoidable here without a complex buffer logic. 
    // Ideally, buffer this, but for short lines this is acceptable.
    if(c != '\n' && c != '\r') {
        line = c + line; 
    }
    
    if (pos == 0) break; // Stop at start of file
    pos--;
    count++;
  }

  return line;  // Return the last line
}

/*String readLastLine() {
  int fileSize = myFile.size();
  if (fileSize == 0) return "";

  String line = "";
  for (int i = fileSize - 2; i >= 0; i--) { // Start near end
    myFile.seek(i);
    char c = myFile.read();
    if (c == '\n') break; // Found last newline, start reading next line
    line = c + line;
  }
  return line;
}*/
