//void ShowAudit() {
//  if(Page_No==185) {if(RecordStatus1==false) {BuildLineIndexFile();}ReadReportFile1();}//ReadReportFile
//}

void ReadLines1() {
  MaxRecordCount1=AuditId-1;
  RecordCount1=MaxRecordCount1;
  RecordStatus1=true;
  /*
  MaxRecordCount1 = 0;
  myFile = SD.open("/ActLog.csv", FILE_READ);
  while (myFile.available()) {
    String line = myFile.readStringUntil('\n');
    MaxRecordCount1++;
  }
  MaxRecordCount1=MaxRecordCount1;
  RecordStatus1=true;*/
}

void ReadReportFile1() {
  // Check existence first to prevent hanging on open
  if (!SD.exists("/ActLog.csv")) {
     return; 
  }

  myFile = SD.open("/ActLog.csv", FILE_READ);
  if (!myFile) {
    return; // Exit if file not found
  }
  
  // Topway navigation input
  ReadTopway(0x00, 0x94);
  if (!Read_Err) {
    if (temp_l == 15) { // UP
      Logout_Timer=0;
      RecordCount1 = (RecordCount1 > 1) ? RecordCount1 - 1 : MaxRecordCount1;
      WriteTopway(0x00, 0x94, 0);
    } else if (temp_l == 240) { // DOWN
      Logout_Timer=0;
      RecordCount1 = (RecordCount1 < MaxRecordCount1) ? RecordCount1 + 1 : 1;
      WriteTopway(0x00, 0x94, 0);
    }
  }

  int lineNum = 0;
  String lineData = "";

  // MEMORY PROTECTION: Reserve memory to avoid heap fragmentation/corruption
  // This is critical for ESP32 stability when reading files in a loop.
  String line;
  line.reserve(128); 

  // Read the file line-by-line until the desired line (RecordCount)
  while (myFile.available()) {
    line = myFile.readStringUntil('\n');
    
    if (lineNum == RecordCount1-1) {
      lineData = line;
      lineData.trim(); // Remove whitespace/newlines safely here
      break;
    }
    lineNum++;
  }
  myFile.close(); // Close file immediately to release handle

  // Safety: If line is empty (end of file or error), stop here to avoid crash
  if (lineData.length() == 0) return;

  // Parse CSV
  int pos1 = lineData.indexOf(',');
  int pos2 = lineData.indexOf(',', pos1 + 1);
  int pos3 = lineData.indexOf(',', pos2 + 1);
  int pos4 = lineData.indexOf(',', pos3 + 1);
  int pos5 = lineData.indexOf(',', pos4 + 1);

  // Safety: Check if data is malformed to avoid crash on substring
  if (pos1 == -1 || pos2 == -1 || pos3 == -1 || pos4 == -1 || pos5 == -1) {
    return; 
  }

  String strAuditId1 = lineData.substring(0, pos1);
  String strSampleId1 = lineData.substring(pos1 + 1, pos2);
  String strSampleName = lineData.substring(pos2 + 1, pos3);
  String strUserId = lineData.substring(pos3 + 1, pos4);
  String strDetails = lineData.substring(pos4 + 1, pos5);
  String strRmk = lineData.substring(pos5 + 1);

  // Display
  WriteString(0x00, 0x14, 0x00, strAuditId1);
  WriteString(0x00, 0x14, 0x80, strSampleId1);
  WriteString(0x00, 0x15, 0x00, strSampleName);
  WriteString(0x00, 0x15, 0x80, strUserId);
  WriteString(0x00, 0x16, 0x00, strDetails);
  WriteString(0x00, 0x16, 0x80, strRmk);
//  WriteTopway(0x00, 0xac, RecordCount1);
//  WriteTopway(0x00, 0xae, MaxRecordCount1);
  WriteTopway_32(0x00,0x00,RecordCount1);
  WriteTopway_32(0x00,0x04,MaxRecordCount1);
}

/*void BuildLineIndexFile() {
  File csvFile = SD.open("/ActLog.csv", FILE_READ);
  // Added O_TRUNC to ensure clean file creation
  File indexFile = SD.open("/lineIndex.dat", O_WRITE | O_CREAT | O_TRUNC);

  if (!csvFile || !indexFile) {
    if(csvFile) csvFile.close();
    if(indexFile) indexFile.close();
    return;
  }

  indexFile.seek(0); // Overwrite

  while (csvFile.available()) {
    unsigned long pos = csvFile.position();
    indexFile.write((uint8_t*)&pos, sizeof(pos)); // Write 4-byte offset
    
    // Suggestion: Flush occasionally if file is huge, but usually close() is enough here
    
    // Skip to next line
    csvFile.readStringUntil('\n');
  }
  
  // Added flush for safety if this function is used
  indexFile.flush();
  
  csvFile.close();
  indexFile.close();
  MaxRecordCount1 = indexFile.position() / sizeof(unsigned long);
  RecordStatus1=true;
}

unsigned long getLineOffset(uint32_t lineNumber) {
  File indexFile = SD.open("/lineIndex.dat", FILE_READ);
  if (!indexFile) return 0;

  indexFile.seek((lineNumber - 1) * sizeof(unsigned long)); // lineNumber is 1-based

  unsigned long offset = 0;
  indexFile.read((uint8_t*)&offset, sizeof(offset));
  indexFile.close();

  return offset;
}*/
