void Device_Bits() {
  ReadTopway(0x00,0xbc);
  if(Read_Err==false) {
    for (i = 7; i >= 0; i--) {
      bitArray[i] = (temp_l >> i) & 1;  // Extract each bit from the low byte
    }
    for (int i = 7; i >= 0; i--) {
      bitArray[i + 8] = (temp_h >> i) & 1;  // Extract each bit from the high byte
    }
    if(bitArray[0]==1) {DisplayPage(89);Reset_Dev_Bit();} //                        Company Details Button  1
    if(bitArray[1]==1) {DisplayPage(90);Reset_Dev_Bit();} //                      Equipment Details Button  2
    if(bitArray[2]==1) {Set_Company_details();Reset_Dev_Bit();} //              Set Company Details Button  4
    if(bitArray[3]==1) {Set_Equipment_details();Reset_Dev_Bit();} //          Set Equipment Details Button  8
    if(bitArray[4]==1) {WriteCompanyDetails();Reset_Dev_Bit();} //         Confirm Company Details Button  16
    if(bitArray[5]==1) {WriteEquipmentDetails();Reset_Dev_Bit();} //     Confirm Equipment Details Button  32
    if(bitArray[6]==1) {DisplayPage(126);Reset_Dev_Bit();} //                  Equipment Serial No Button  64    //Page Sr No
    if(bitArray[7]==1) {Set_SerNo_details();Reset_Dev_Bit();}     //         Set Serial No Details Button  128
    if(bitArray[8]==1) {WriteEquipmentDetails();Reset_Dev_Bit();} //    Confirm Equipment Details Button  256
    if(bitArray[9]==1) {WriteMotorDetails();Reset_Dev_Bit();}     //    Confirm Equipment Details Button  512
  } 
}

void Set_Company_details() {
  Read_String(0x1e,0x80);
  Company_Name=Str2;
  Read_String(0x1f,0x00);
  City_Name=Str2;
  Read_String(0x1f,0x80);
  State_Name=Str2;
  if(Company_Name.length() < 4 || Company_Name.length() > 20 || City_Name.length() < 4 || City_Name.length() > 20 || State_Name.length() < 4 || State_Name.length() > 20) {
    WriteTopway(0x00,0x72,1); //Length Warning Popup
  } else {
    WriteTopway(0x00,0x72,2); //Confirm Details Popup
  }
}

void Set_Equipment_details() {
  Read_String(0x20,0x00);
  UnitName=Str2;
  Read_String(0x20,0x80);
  EquipmentId=Str2;
  if(UnitName.length() < 4 || UnitName.length() > 20 || EquipmentId.length() < 4 || EquipmentId.length() > 20) {
    WriteTopway(0x00,0x72,1); //Length Warning Popup
  } else {
    WriteTopway(0x00,0x72,2); //Confirm Details Popup
  }
}

void Set_SerNo_details() {
  Read_String(0x22,0x00);
  deviceID=Str2;
  if(deviceID.length() < 4 || deviceID.length() > 20) {
    WriteTopway(0x00,0x72,1); //Length Warning Popup
  } else {
    WriteTopway(0x00,0x72,2); //Confirm Details Popup
  }
}

void WriteCompanyDetails() {
  // ATOMIC WRITE STRATEGY: Write to .tmp first, then rename.
  // This prevents file corruption if power fails during write.
  const char *tmpFile = "/CompDet.tmp";
  const char *finalFile = "/CompDet.csv";

  // Clean up any previous temp file
  if (SD.exists(tmpFile)) SD.remove(tmpFile);

  // Open Temp File
  myFile = SD.open(tmpFile, O_WRITE | O_CREAT | O_TRUNC);
  
  WriteTopway(0x00,0x72,0); // Show "Saving..." or similar on UI
  
  if (myFile) {
    myFile.print(Company_Name); myFile.print(",");
    myFile.print(City_Name); myFile.print(",");
    myFile.println(State_Name);
    
    myFile.flush(); // Force write to physical card
    myFile.close(); // Close file handle
    
    // Now safely replace the old file with the new one
    if (SD.exists(finalFile)) SD.remove(finalFile);
    SD.rename(tmpFile, finalFile);
  } else {
    // If opening failed, ensure handle is closed just in case
    // and maybe log an error here
    return;
  }

  // Update UI and Variables
  WriteString(0x00,0x1b,0x00,Company_Name);
  WriteString(0x00,0x1b,0x80,City_Name);
  WriteString(0x00,0x1c,0x00,State_Name);
  
  WriteTopway(0x00,0x72,4); // Success Popup
  
  // Audit Logs
  if(Company_Name!=Company_Name1) {AuditDetails="CompanyName";AuditRemark=Company_Name1 + "->" + Company_Name;LogActivity("CompanyDetailsUpdated");}
  if(City_Name!=City_Name1) {AuditDetails="CityName";AuditRemark=City_Name1 + "->" + City_Name;LogActivity("CompanyDetailsUpdated");}
  if(State_Name!=State_Name1) {AuditDetails="StateName";AuditRemark=State_Name1 + "->" + State_Name;LogActivity("CompanyDetailsUpdated");}
}

void WriteEquipmentDetails() {
  // ATOMIC WRITE STRATEGY
  const char *tmpFile = "/DevInf.tmp";
  const char *finalFile = "/DevInf.csv";

  if (SD.exists(tmpFile)) SD.remove(tmpFile);

  myFile = SD.open(tmpFile, O_WRITE | O_CREAT | O_TRUNC);
  WriteTopway(0x00,0x72,0); 

  if (myFile) {
      myFile.print(ModelNo);      myFile.print(",");
      myFile.print(SoftwareVer);  myFile.print(",");
      myFile.print(UnitName);     myFile.print(",");
      myFile.print(EquipmentId);  myFile.print(",");
      myFile.println(deviceID);

      myFile.flush();     // Force SD write
      myFile.close();     // Ensure FAT updated

      // Safe Rename
      if (SD.exists(finalFile)) SD.remove(finalFile);
      SD.rename(tmpFile, finalFile);   // ATOMIC replace
  } else {
      return; // Failed to open file
  }

  // Restored Logic: Update Display
  WriteString(0x00,0x07,0x80,UnitName);
  WriteString(0x00,0x08,0x00,EquipmentId);
  WriteString(0x00,0x08,0x80,deviceID);
  
  WriteTopway(0x00,0x72,4); // Success Popup

  // Restored Logic: Audit Logging
  if(UnitName!=UnitName1) {AuditDetails="UnitName";AuditRemark=UnitName1 + "->" + UnitName;LogActivity("EquipmentDetailsUpdated");}
  if(EquipmentId!=EquipmentId1) {AuditDetails="EquipmentId";AuditRemark=EquipmentId1 + "->" + EquipmentId;LogActivity("EquipmentDetailsUpdated");}
  if(deviceID!=deviceID1) {AuditDetails="DeviceId";AuditRemark=deviceID1 + "->" + deviceID;LogActivity("EquipmentDetailsUpdated");}
}

void Reset_Dev_Bit() {
  WriteTopway(0x00, 0xbc, 0);
}
