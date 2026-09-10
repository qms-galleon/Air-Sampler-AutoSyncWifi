void UserId_Bits() {
  ReadTopway(0x00,0x66);
  if(Read_Err==false) {
    for (i = 7; i >= 0; i--) {
      bitArray[i] = (temp_l >> i) & 1;  // Extract each bit from the low byte
    }
    for (int i = 7; i >= 0; i--) {
      bitArray[i + 8] = (temp_h >> i) & 1;  // Extract each bit from the high byte
    }
    if(bitArray[0]==1) {Logout_Timer=0;CheckUserConfig(1);Reset_User_Bit();} //1      Add User Next Button
    if(bitArray[1]==1) {Logout_Timer=0;CheckUserConfig(2);Reset_User_Bit();} //2      User Setting1 First Name Length Popup
    if(bitArray[2]==1) {Logout_Timer=0;CheckUserConfig(3);Reset_User_Bit();} //4      User Setting2 Department Name Length Popup
    if(bitArray[3]==1) {Logout_Timer=0;CheckUserPassword();Reset_User_Bit();} //8      User Setting3 Password Length Popup
    if(bitArray[5]==1) {Logout_Timer=0;AddUser();Reset_User_Bit();} //32      User Setting3 Password Length Popup
//    if(bitArray[6]==1) {Logout_Timer=0;CheckUserAccess();Reset_User_Bit();} //64      User Management Add Button
    if(bitArray[7]==1) {Logout_Timer=0;WriteTopway(0x00,0x74,1);Reset_User_Bit();} //128      Discard User button
    if(bitArray[8]==1) {Logout_Timer=0;Manage_User(3, "/USR.csv");Reset_User_Bit();} //256      Discard User Confirm button
    if(bitArray[9]==1) {Logout_Timer=0;CheckUserPassword1();Reset_User_Bit();} //512      Change Password
    if(bitArray[10]==1) {Logout_Timer=0;Manage_User(5, "/USR.csv");Reset_User_Bit();} //1024      Change Password
    if(bitArray[11]==1) {Logout_Timer=0;CheckUserPassword2();Reset_User_Bit();} //2048      Reset Password
    if(bitArray[12]==1) {Logout_Timer=0;CheckUserPassword3();Reset_User_Bit();} //4096      First Time Login Password Change

    if(bitArray[6]==1) {Logout_Timer=0;CheckUserConfig(6);Reset_User_Bit();} //64
    if(bitArray[13]==1) {Logout_Timer=0;CheckUserConfig(4);Reset_User_Bit();} //8192
    if(bitArray[14]==1) {Logout_Timer=0;CheckUserConfig(5);Reset_User_Bit();} //16384
    if(bitArray[15]==1) {Logout_Timer=0;Update_User();Reset_User_Bit();} //32768
    if(bitArray[4]==1) {Logout_Timer=0;CheckUserConfig(7);Reset_User_Bit();} //16 //User View
  }
}

//void CheckUserAccess() {
//  if(Group_User_Setting1=="No")  {
//    WriteTopway(0x00,0xC0,1);
//  } else {
//    DisplayPage(17);
//  }
//}

void CheckUserAccess() {
  if(Group_User_Setting1=="No")  {
    if(Group == "SuperAdmin") {
      DisplayPage(17);
    WriteTopway(0x01,0x02,1);
    } else {
        WriteTopway(0x00,0xC0,1);
    }
  } else {
    DisplayPage(17);
  }
}


void CheckUserConfig(byte ConfigParam) {
  if(ConfigParam==1) {Read_String(0x0f,0x80);if(Read_Err==false) {strGUser=Str2;}}
  else if(ConfigParam==2) {Read_String(0x10,0x00);}
  else if(ConfigParam==3) {Read_String(0x11,0x00);}
  else if(ConfigParam==4) {Read_String(0x10,0x00);}
  else if(ConfigParam==5) {Read_String(0x11,0x00);}
  else if(ConfigParam==6) {Read_String(0x22,0x80);}
  else if(ConfigParam==7) {Read_String(0x22,0x80);}
  if(Read_Err==false) {
    if(Str2.length() < 4 || Str2.length() > 20) {
      WriteTopway(0x00,0x72,1); //Length Warning Popup
    } else {
      if(ConfigParam==1) {CheckUser();}
      else if(ConfigParam==2) {DisplayPage(69);}
      else if(ConfigParam==3) {GroupPrevil1();DisplayPage(70);}
      else if(ConfigParam==4) {DisplayPage(196);}
      else if(ConfigParam==5) {DisplayPage(197);}
      else if(ConfigParam==6) {/*DisplayPage(195);*/UserSummary(195);}
      else if(ConfigParam==7) {/*DisplayPage(195);*/UserSummary(205);}
    }
  }
}

void CheckUser() {
  // Check if user already exists
  bool userExists = false;
  
  if (!SD.exists("/USR.csv")) {
     // File doesn't exist, so user definitely doesn't exist.
     OpStatus=true;
     if(SoftWareConnected ==false) {DisplayPage(68);}
     return;
  }

  myFile = SD.open("/USR.csv", FILE_READ);
  if (myFile) {
    // MEMORY PROTECTION: Move String outside loop
    String line;
    line.reserve(128);

    while (myFile.available()) {
      line = myFile.readStringUntil('\n');
      
      // Extract second field (username)
      int firstComma = line.indexOf(',');
      int secondComma = line.indexOf(',', firstComma + 1);
      if (firstComma != -1 && secondComma != -1) {
        String existingUser = line.substring(firstComma + 1, secondComma);
        existingUser.trim(); // Remove spaces/newlines

        if (existingUser == strGUser) {
          userExists = true;
          break;
        }
      }
    }
    myFile.close();
  }

  if (userExists) {
    //Serial.println("User already exists. Cannot add duplicate.");
    WriteTopway(0x00,0x76,1);
    OpStatus=false;
    return; // Stop execution if user already exists
  } else {
    OpStatus=true;
    if(SoftWareConnected ==false) {DisplayPage(68);}
  }

  //End Check
}

//User Setting3 Password Check - Page 70 
void CheckUserPassword() {   
  Read_String(0x12,0x00);if(Read_Err==false) {GUserPass1=Str2;}
  Read_String(0x12,0x80);if(Read_Err==false) {GUserPass2=Str2;}
  if(GUserPass1.length() < GroupMinPassLength2 || GUserPass1.length() > 15) {
    WriteTopway(0x01,0x16,GroupMinPassLength2);
    WriteTopway(0x00,0x72,1);
    return;
  } 
  if(PasswordComplexity2=="Yes") {
    bool hasLetter = false;
    bool hasDigit = false;
    bool hasSpecialAt = false;
  
    for (int i = 0; i < GUserPass1.length(); i++) {
      char c = GUserPass1.charAt(i);
      if (isAlpha(c)) hasLetter = true;
      else if (isDigit(c)) hasDigit = true;
      else if (c == '@') hasSpecialAt = true;
    }
  
    if (!hasLetter || !hasDigit || !hasSpecialAt) {
      WriteTopway(0x00, 0x72, 3);  // Error code 3: missing required character types
      return;
    }
  }
  
  if(GUserPass1==GUserPass2) {
    DisplayPage(71);
  } else {
    WriteTopway(0x00,0x72,2);
  }
}


/**/


/**/


//Self Password Change from Setting Page 143
void CheckUserPassword1() {
  Read_String(0x23,0x00);if(Read_Err==false) {GUserPass1=Str2;}
  Read_String(0x23,0x80);if(Read_Err==false) {GUserPass2=Str2;}
  Read_String(0x24,0x00);if(Read_Err==false) {GUserPass3=Str2;}
  if(GUserPass1==Password1) {
    if(GUserPass1!=GUserPass2) {
      if(GUserPass2.length() < GroupMinPassLength1 || GUserPass2.length() > 15) {
        WriteTopway(0x01,0x16,GroupMinPassLength1);
        WriteTopway(0x00,0x72,1); //Length Popup
        return;
      } 

      if(PasswordComplexity1=="Yes") {
        bool hasLetter = false;
        bool hasDigit = false;
        bool hasSpecialAt = false;
      
        for (int i = 0; i < GUserPass2.length(); i++) {
          char c = GUserPass2.charAt(i);
          if (isAlpha(c)) hasLetter = true;
          else if (isDigit(c)) hasDigit = true;
          else if (c == '@') hasSpecialAt = true;
        }
      
        if (!hasLetter || !hasDigit || !hasSpecialAt) {
          WriteTopway(0x00, 0x72, 6);  // Error code 3: missing required character types
          return;
        }
      }
      
      if(GUserPass2==GUserPass3) {
        Manage_User(4, "/USR.csv"); //Change Password
        WriteTopway(0x00,0x72,3); //Password Changed Popup
        WriteTopway(0x00,0x74,0);
        LogActivity("SelfPasswordChanged");
        Password1=GUserPass2;
      } else {
        WriteTopway(0x00,0x72,2); //Passowd do not match Popup
      }
    } else {
      WriteTopway(0x00,0x72,5); //Matches with Recent Passowd Popup
    }
  } else {
    WriteTopway(0x00,0x72,4); //Wrong Password
  }
}

void Reset_Password() {
  WriteString(0x00,0x22,0x80,strGUser);
  WriteString(0x00,0x23,0x80,GUserPass1);
  WriteString(0x00,0x24,0x00,GUserPass1);
  CheckUserPassword2();
  WriteTopway(0x00,0x72,0);
}

// Called when user presses OK on the "Password Expired" popup.
// Routes the logged-out user to the self-reset password page (147)
// so they can set a new password without admin intervention.
void SelfResetOnExpiry() {
  WriteTopway(0x01, 0x12, 0);              // clear the popup register
  PasswordExpired = false;                 // clear the expired flag
  PasswordResetOnExpiry = false;           // reset flow is now on the password page
  UserId = "NO_USER";                      // do not carry an expired-login session forward
  WriteString(0x00, 0x22, 0x80, strGUser);// pre-fill username on page 147
  WriteString(0x00, 0x23, 0x80, "");      // clear new-password field
  WriteString(0x00, 0x24, 0x00, "");      // clear confirm-password field
  DisplayPage(147);                        // go to Reset Password page
  // Page 147 confirm button calls CheckUserPassword2() → Manage_User(6)
  // which updates the password AND resets ExpDate to today in /USR.csv
}

//Reset Password - Page 84 (Page 146, 147)
void CheckUserPassword2() {
//  Read_String(0x23,0x00);if(Read_Err==false) {GUserPass1=Str2;}
  bool resetFromExpiredLogin = (Page_No == 147);
  Read_String(0x23,0x80);if(Read_Err==false) {GUserPass2=Str2;}
  Read_String(0x24,0x00);if(Read_Err==false) {GUserPass3=Str2;}
  if(GUserPass2.length() < 4 || GUserPass2.length() > 15) {
    WriteTopway(0x01,0x16,GroupMinPassLength1);
    WriteTopway(0x00,0x72,1); //Length Popup
  } else if(GUserPass2==GUserPass3) {
    Manage_User(6, "/USR.csv"); //Change Password
    delay(100);
    Read_String(0x22,0x80);
    if(Read_Err==false) {
      strGUser=Str2;
      String userFile = "/GUSER/" + strGUser + ".csv";
      if(SD.exists(userFile)) {
            Manage_User(7, userFile); // Run the same reset logic on the individual file
            delay(100); // Wait again
      }
    }
    
    WriteTopway(0x00,0x72,3); //Password Changed Popup
    WriteTopway(0x00,0x74,0);
    WriteString(0x00,0x23,0x80,"");
    WriteString(0x00,0x24,0x00,"");
    if(resetFromExpiredLogin) {
      PasswordExpired = false;
      PasswordResetOnExpiry = false;
      LoginBit = false;
      UserId = "NO_USER";
      Password = "";
      Group = "";
      Logout_Timer = 0;
      Power_Off_Timer = 0;
      WriteTopway(0x00,0xcc,0); //Disable Setting 2 touch button
      Reset_User_Cred();
      DisplayPage(1);           //Force fresh login so sample records the real user
    }
//    AuditDetails=strGroup;AuditRemark="OK";
    //LogActivity("UserPswdReset");
  } else {
    WriteTopway(0x00,0x72,2); //Passowd do not match Popup
  }
}

//First Time Login - Page 189
void CheckUserPassword3() {
  Read_String(0x23,0x00);if(Read_Err==false) {GUserPass1=Str2;}
  Read_String(0x23,0x80);if(Read_Err==false) {GUserPass2=Str2;}
  Read_String(0x24,0x00);if(Read_Err==false) {GUserPass3=Str2;}
  if(GUserPass1==Password1) {
    if(GUserPass1!=GUserPass2) {
      if(GUserPass2.length() < GroupMinPassLength1 || GUserPass2.length() > 15) {
        WriteTopway(0x01,0x16,GroupMinPassLength1);
        WriteTopway(0x00,0x72,1); //Length Popup
        return;
      }
  
      if(PasswordComplexity1=="Yes") {
      bool hasLetter = false;
      bool hasDigit = false;
      bool hasSpecialAt = false;
    
      for (int i = 0; i < GUserPass2.length(); i++) {
        char c = GUserPass2.charAt(i);
        if (isAlpha(c)) hasLetter = true;
        else if (isDigit(c)) hasDigit = true;
        else if (c == '@') hasSpecialAt = true;
      }
    
      if (!hasLetter || !hasDigit || !hasSpecialAt) {
        WriteTopway(0x00, 0x72, 5);  // Error code 3: missing required character types
        return;
      }
    }
      
      if(GUserPass2==GUserPass3) {
        Manage_User(4, "/USR.csv"); //Change Password
        WriteTopway(0x00,0x72,3); //Password Changed Popup
        WriteTopway(0x00,0x74,0);
      } else {
        WriteTopway(0x00,0x72,2); //Passowd do not match Popup
      }
    } else {
      WriteTopway(0x00,0x72,6); //Matches with Recent Passowd Popup
    }
  } else {
    WriteTopway(0x00,0x72,4); //Wrong Password
  }
}

void UserSummary( int PageNo) {
  String STRUSR1;
  Read_String(0x22,0x80);STRUSR1=Str2;
  
  if (!SD.exists("/USR.csv")) return;

  myFile = SD.open("/USR.csv", FILE_READ);
  if (!myFile) {
     return;
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

    if(STRUSR1==line.substring(pos1 + 1, pos2)) {
      String ID = line.substring(0, pos1);
      String STRPWD = line.substring(pos2 + 1, pos3);
      String STRNAME = line.substring(pos3 + 1, pos4);
      String STRLN = line.substring(pos4 + 1, pos5);
      String STRDEPT = line.substring(pos5 + 1, pos6);
      String STRGRP = line.substring(pos6 + 1, pos7);
      String STATUS = line.substring(pos7 + 1, pos8);
      String STRFLOG = line.substring(pos8 + 1, pos9);
      String STRRETRY = line.substring(pos9 + 1, pos10);
      String STRDT = line.substring(pos10 + 1);

      WriteString(0x00,0x30,0x80,ID);
      WriteString(0x00,0x0f,0x80,STRUSR1);
      WriteString(0x00,0x10,0x00,STRNAME);
      WriteString(0x00,0x10,0x80,STRLN);
      WriteString(0x00,0x11,0x00,STRDEPT);
      WriteString(0x00,0x25,0x00,STRGRP);
      DisplayPage(PageNo);
      myFile.close();
      return;
    }
  }
  myFile.close();
  
}

/**/
void Update_User() {
  String STRLOC1;
  String ID;
  String StrUsr1;
  String STRPWD;
  String STRNAME;
  String STRLN;
  String STRDEPT;
  String STRGRP;
  String STATUS;
  String STRFLOG;
  String STRRETRY;
  String STRDT;
  
  String STRNAME2;
  String STRLN2;
  String STRDEPT2;
  String STRGRP2;
  
  
  String STRNAME3;
  String STRLN3;
  String STRDEPT3;
  String STRGRP3;
  
      
  Read_String(0x0f,0x80);
  if(Read_Err==false) {
    STRLOC1=Str2;
    WriteString(0x00,0x21,0x80,STRLOC1);
    
    // ATOMIC WRITE Strategy
    const char *tmpName = "/temp.csv";
    const char *fileName = "/USR.csv";
    if(SD.exists(tmpName)) SD.remove(tmpName);

    tempFile = SD.open(tmpName, O_WRITE | O_CREAT | O_TRUNC);
    myFile = SD.open(fileName, O_READ);
    
    if (!myFile || !tempFile) {
        if(myFile) myFile.close();
        if(tempFile) tempFile.close();
        return;
    }
    
    // MEMORY PROTECTION
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
      pos9 = row.indexOf(',', pos8 + 1);
      pos10 = row.indexOf(',', pos9 + 1);
      if (pos1 == -1 || pos2 == -1 || pos3 == -1 || pos4 == -1 || pos5 == -1 || pos6 == -1 || pos7 == -1 || pos8 == -1 || pos9 == -1 || pos10 == -1) {
        // Malformed row, write it as is
        tempFile.println(row);
        continue;
      }

      ID = row.substring(0, pos1);
      StrUsr1 = row.substring(pos1 + 1, pos2);
      STRPWD = row.substring(pos2 + 1, pos3);
      STRNAME = row.substring(pos3 + 1, pos4);
      STRLN = row.substring(pos4 + 1, pos5);
      STRDEPT = row.substring(pos5 + 1, pos6);
      STRGRP = row.substring(pos6 + 1, pos7);
      STATUS = row.substring(pos7 + 1, pos8);
      STRFLOG = row.substring(pos8 + 1, pos9);
      STRRETRY = row.substring(pos9 + 1, pos10);
      STRDT = row.substring(pos10 + 1);
  
      

      String newRow = "";
      if (StrUsr1 == STRLOC1) {
        if(Page_No==43) {
          STATUS = "Discard";
          WriteString(0x00,0x21,0x80,STRLOC1);
          newRow = ID + "," + StrUsr1 + "," + STRPWD + "," + STRNAME + "," + STRLN + "," + STRDEPT + "," + STRGRP + "," + STATUS + "," + STRFLOG + "," + STRRETRY + "," + STRDT;
        } else {
          STRNAME2 = STRNAME;
          STRLN2 = STRLN;
          STRDEPT2 = STRDEPT;
          STRGRP2 = STRGRP;
          
          
          Read_String(0x10,0x00);if(Read_Err==false) {STRNAME3=Str2;}
          Read_String(0x10,0x80);if(Read_Err==false) {STRLN3=Str2;}
          Read_String(0x11,0x00);if(Read_Err==false) {STRDEPT3=Str2;}
          Read_String(0x25,0x00);if(Read_Err==false) {STRGRP3=Str2;}
          
          newRow = ID + "," + StrUsr1 + "," + STRPWD + "," + STRNAME3 + "," + STRLN3 + "," + STRDEPT3 + "," + STRGRP3 + "," + STATUS + "," + STRFLOG + "," + STRRETRY + "," + STRDT;
        }
      } else {
        newRow = ID + "," + StrUsr1 + "," + STRPWD + "," + STRNAME + "," + STRLN + "," + STRDEPT + "," + STRGRP + "," + STATUS + "," + STRFLOG + "," + STRRETRY + "," + STRDT;
      }
      tempFile.println(newRow);
    }
    myFile.close();
    
    // Critical: Flush and Close temp
    tempFile.flush();
    tempFile.close();
  
    SD.remove(fileName);
    SD.rename(tmpName, fileName);
    
    if(Page_No==43) {
      AuditDetails=STRLOC1;AuditRemark="NA";
      LogActivity("UserDiscarded");
      WriteTopway(0x00,0x74,2);
    } else {
      if(STRNAME3!=STRNAME2) {AuditDetails=STRLOC1 + "-FirstName";AuditRemark=STRNAME2 + "->" + STRNAME3;LogActivity("UserEdited");}
      if(STRLN3!=STRLN2) {AuditDetails=STRLOC1 + "-LastName";AuditRemark=STRLN2 + "->" + STRLN3;LogActivity("UserEdited");}
      if(STRDEPT3!=STRDEPT2) {AuditDetails=STRLOC1 + "-Dept";AuditRemark=STRDEPT2 + "->" + STRDEPT3;LogActivity("UserEdited");}
      if(STRGRP3!=STRGRP2) {AuditDetails=STRLOC1 + "-Group";AuditRemark=STRGRP2 + "->" + STRGRP3;LogActivity("UserEdited");}
      WriteTopway(0x00,0x76,1);
    }
  }
}

/**/

void AddUser() {
  Read_String(0x0f,0x80);if(Read_Err==false) {strGUser=Str2;}
  Read_String(0x10,0x00);if(Read_Err==false) {FirstName=Str2;}
  Read_String(0x10,0x80);if(Read_Err==false) {LastName=Str2;}
  Read_String(0x11,0x00);if(Read_Err==false) {GUserDept=Str2;}
  Read_String(0x25,0x00);if(Read_Err==false) {GUserGroup=Str2;} //0x11,0x80
  Read_String(0x12,0x00);if(Read_Err==false) {GUserPass1=Str2;}
  String strDate = "";
  myFile = SD.open("/USR.csv", O_RDWR | O_CREAT | O_APPEND);
  
  if (myFile) {
//    myFile.seekEnd();
    now1 = rtc.now();
    if (now1.day() < 10) strDate += '0';
    strDate += String(now1.day()) + "/";
    if (now1.month() < 10) strDate += '0';
    strDate += String(now1.month()) + "/";
    strDate += String(now1.year());
    
    myFile.print(GUserId); myFile.print(",");
    myFile.print(strGUser); myFile.print(",");
    myFile.print(GUserPass1); myFile.print(",");
    myFile.print(FirstName); myFile.print(",");
    myFile.print(LastName); myFile.print(",");
    myFile.print(GUserDept); myFile.print(",");
    myFile.print(GUserGroup); myFile.print(",");
    myFile.print("Active"); myFile.print(",");
    myFile.print("0"); myFile.print(",");
    myFile.print("0"); myFile.print(",");
    myFile.println(strDate);
    
    myFile.flush(); // Critical: Force write
    myFile.close();
  }

  //User File
  String userFile = "/GUSER/" + strGUser + ".csv";
  myFile = SD.open(userFile, O_RDWR | O_CREAT | O_APPEND);
  
  if (myFile) {
    myFile.print(GUserId); myFile.print(",");
    myFile.print(strGUser); myFile.print(",");
    myFile.print(GUserPass1); myFile.print(",");
    myFile.print(FirstName); myFile.print(",");
    myFile.print(LastName); myFile.print(",");
    myFile.print(GUserDept); myFile.print(",");
    myFile.print(GUserGroup); myFile.print(",");
    myFile.print("Active"); myFile.print(",");
    myFile.print("0"); myFile.print(",");
    myFile.print("0"); myFile.print(",");
    myFile.println(strDate);
    
    myFile.flush(); // Critical: Force write
    myFile.close();
  }
  
//  LogActivity("UserCreated");
  AuditDetails=strGUser;AuditRemark="OK";LogActivity("UserCreated");
  GUserId++;
  WriteTopway(0x00,0x7e,GUserId);
  WriteTopway(0x00,0x76,1);
}

void AddUser1(bool flbool) {
  strGUser.trim(); 
  if(strGUser.length() == 0) return; // Safety check

  String strDate = "";
  now1 = rtc.now();
  if (now1.day() < 10) strDate += '0';
  strDate += String(now1.day()) + "/";
  if (now1.month() < 10) strDate += '0';
  strDate += String(now1.month()) + "/";
  strDate += String(now1.year());

  myFile = SD.open("/USR.csv", O_RDWR | O_CREAT | O_APPEND);
  
  if (myFile) {
    myFile.print(GUserId); myFile.print(",");
    myFile.print(strGUser); myFile.print(",");
    myFile.print(GUserPass1); myFile.print(",");
    myFile.print(FirstName); myFile.print(",");
    myFile.print(LastName); myFile.print(",");
    myFile.print(GUserDept); myFile.print(",");
    myFile.print(GUserGroup); myFile.print(",");
    myFile.print("Active"); myFile.print(",");
    if(flbool) {
      myFile.print("1"); myFile.print(","); //First Time Login 1 for upload user
    } else {
      myFile.print("0"); myFile.print(","); //First Time Login 1 for upload user
    }
    
    myFile.print("0"); myFile.print(","); //Retry Limit
    myFile.println(strDate);
    
    myFile.flush(); // Critical
    myFile.close();
  } else {
    return; 
  }

  delay(100);

  //User File
  if (!SD.exists("/GUSER")) {
    SD.mkdir("/GUSER");
    delay(50);
  }

  String userFile = "/GUSER/" + strGUser + ".csv";
  myFile = SD.open(userFile, O_RDWR | O_CREAT | O_APPEND);
  
  if (myFile) {
    myFile.print(GUserId); myFile.print(",");
    myFile.print(strGUser); myFile.print(",");
    myFile.print(GUserPass1); myFile.print(",");
    myFile.print(FirstName); myFile.print(",");
    myFile.print(LastName); myFile.print(",");
    myFile.print(GUserDept); myFile.print(",");
    myFile.print(GUserGroup); myFile.print(",");
    myFile.print("Active"); myFile.print(",");
    myFile.print("0"); myFile.print(",");
    myFile.print("0"); myFile.print(",");
    myFile.println(strDate);
    
    myFile.flush(); // Critical
    myFile.close();
  }
  
//  LogActivity("UserCreated");
  AuditDetails=strGUser;AuditRemark="OK";LogActivity("UserCreated");
  GUserId++;
  WriteTopway(0x00,0x7e,GUserId);
}

void Discard_User() {
  String STRLOC1;
  STRLOC1=Str2;
  
  // ATOMIC WRITE Strategy
  const char *tmpName = "/temp.csv";
  const char *fileName = "/USR.csv";
  if(SD.exists(tmpName)) SD.remove(tmpName);

  tempFile = SD.open(tmpName, O_WRITE | O_CREAT | O_TRUNC);
  myFile = SD.open(fileName, O_READ);
  
  if (!myFile || !tempFile) {
      if(myFile) myFile.close();
      if(tempFile) tempFile.close();
      return;
  }
  
  // MEMORY PROTECTION
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
    pos9 = row.indexOf(',', pos8 + 1);
    pos10 = row.indexOf(',', pos9 + 1);
    if (pos10 == -1) {
      // Malformed row, write it as is
      tempFile.println(row);
      continue;
    }
        
    String ID = row.substring(0, pos1);
    String STRLOC = row.substring(pos1 + 1, pos2);
    String STRPWD = (pos3 != -1) ? row.substring(pos2 + 1, pos3) : "";
    String STRNAME = (pos4 != -1) ? row.substring(pos3 + 1, pos4) : "";
    String STRLN = (pos5 != -1) ? row.substring(pos4 + 1, pos5) : "";
    String STRDEPT = (pos6 != -1) ? row.substring(pos5 + 1, pos6) : "";
    String STRGRP = (pos7 != -1) ? row.substring(pos6 + 1, pos7) : "";
    String STATUS = (pos8 != -1) ? row.substring(pos7 + 1, pos8) : "";
    String STRFLOG = (pos9 != -1) ? row.substring(pos8 + 1, pos9) : "";
    String STRRETRY = (pos10 != -1) ? row.substring(pos9 + 1, pos10) : "";
    String STRDT = (pos10 != -1) ? row.substring(pos10 + 1) : "";
      
    if (STRLOC == STRLOC1) {
      STATUS = "Discard";
    }
    
    String newRow = ID + "," + STRLOC + "," + STRPWD + "," + STRNAME + "," + STRLN + "," + STRDEPT + "," + STRGRP + "," + STATUS + "," + STRFLOG + "," + STRRETRY + "," + STRDT;
    
    tempFile.println(newRow);
  }
  myFile.close();
  
  // Critical: Flush and Close temp
  tempFile.flush();
  tempFile.close();

  SD.remove(fileName);
  SD.rename(tmpName, fileName); 
  AuditDetails=STRLOC1;AuditRemark="NA";LogActivity("UserDiscarded");
 
}

void Manage_User(byte ConfigParam, String Strfile1) {
  String STRLOC1;
  if(ConfigParam==1) {Read_String(0x04,0x80);}
  else if(ConfigParam==2) {Read_String(0x1e,0x00);}
  else if(ConfigParam==3) {Read_String(0x22,0x80);}
  else if(ConfigParam==4) {Read_String(0x21,0x00);}
  else if(ConfigParam==5) {Read_String(0x22,0x80);}
  else if(ConfigParam==6) {Read_String(0x22,0x80);}
  else if(ConfigParam==7) {Read_String(0x22,0x80);}
  if(Read_Err==false) {
    STRLOC1=Str2;
    WriteString(0x00,0x21,0x80,STRLOC1);
    
    // ATOMIC WRITE Strategy
    const char *tmpName = "/temp.csv";
    if(SD.exists(tmpName)) SD.remove(tmpName);

    tempFile = SD.open(tmpName, O_WRITE | O_CREAT | O_TRUNC);
    myFile = SD.open(Strfile1, O_READ);
    
    if (!myFile || !tempFile) {
        if(myFile) myFile.close();
        if(tempFile) tempFile.close();
        return;
    }
    
    // MEMORY PROTECTION
    char line[128];
    while (myFile.fgets(line, sizeof(line))) {
      String row = String(line);
      row.trim();  // Remove \n or \r\n
  
      // Split the CSV row
      pos1 = row.indexOf(',');
      pos2 = row.indexOf(',', pos1 + 1);
      if(ConfigParam==3 || ConfigParam==4 || ConfigParam==6 || ConfigParam==7) {
        pos3 = row.indexOf(',', pos2 + 1);
        pos4 = row.indexOf(',', pos3 + 1);
        pos5 = row.indexOf(',', pos4 + 1);
        pos6 = row.indexOf(',', pos5 + 1);
        pos7 = row.indexOf(',', pos6 + 1);
        pos8 = row.indexOf(',', pos7 + 1);
        pos9 = row.indexOf(',', pos8 + 1);
        pos10 = row.indexOf(',', pos9 + 1);
      }
      if (pos1 == -1 || pos2 == -1) {
        // Malformed row, write it as is
        tempFile.println(row);
        continue;
      }
  
      String ID = row.substring(0, pos1);
      String STRLOC = row.substring(pos1 + 1, pos2);
      String STATUS; // = row.substring(pos2 + 1);
      String STRPWD;
      String STRNAME;
      String STRLN;
      String STRDEPT;
      String STRGRP;
      String STRFLOG;
      String STRRETRY;
      String STRDT;
      if(ConfigParam==3 || ConfigParam==4 || ConfigParam==6 || ConfigParam==7) {
        STRPWD = row.substring(pos2 + 1, pos3);
        STRNAME = row.substring(pos3 + 1, pos4);
        STRLN = row.substring(pos4 + 1, pos5);
        STRDEPT = row.substring(pos5 + 1, pos6);
        STRGRP = row.substring(pos6 + 1, pos7);
        STATUS = row.substring(pos7 + 1, pos8);
        STRFLOG = row.substring(pos8 + 1, pos9);
        STRRETRY = row.substring(pos9 + 1, pos10);
        STRDT = row.substring(pos10 + 1);
      } else {
        STATUS = row.substring(pos2 + 1);
      }
//      if (STRLOC == STRLOC1 && STATUS == "Active") {
//        STATUS = "Discard";
//      }
      if(ConfigParam==4) {
        if(STRLOC == STRLOC1) {STRPWD=GUserPass2;STRFLOG="1";}
      } else if(ConfigParam==5) {
        if(STRLOC == STRLOC1) {STRPWD="1234";STRFLOG="0";STATUS = "Active";STRRETRY="0";} //Default Password
      } else if(ConfigParam==6 || ConfigParam==7) {
        if(STRLOC == STRLOC1) {STRPWD=GUserPass2;STRFLOG="1";STATUS = "Active";STRRETRY="0";getTime();STRDT=Str1;} //Reset Password
      } else if(STRLOC == STRLOC1) { //&& STATUS == "Active"
        STATUS = "Discard";
      }
      String newRow = ID + "," + STRLOC + "," + STRPWD + "," + STRNAME + "," + STRLN + "," + STRDEPT + "," + STRGRP + "," + STATUS + "," + STRFLOG + "," + STRRETRY + "," + STRDT;
      tempFile.println(newRow);
    }
    myFile.close();
    
    // Critical: Flush and Close temp
    tempFile.flush();
    tempFile.close();
    
    if(ConfigParam==6) {AuditDetails=STRLOC1;AuditRemark="OK";LogActivity("UserPswReset");}
    if(ConfigParam==3) {AuditDetails=STRLOC1;AuditRemark="OK";LogActivity("UserDiscarded");}
    SD.remove(Strfile1);
    SD.rename("/temp.csv", Strfile1);
    
    WriteTopway(0x00,0x74,2);  
  }
}

void Reset_User_Bit() {
  WriteTopway(0x00, 0x66, 0);
}

void Set_User_Bit(byte j, byte val) { 
    ReadTopway(0x00, 0x66); // Read current values into temp_h and temp_l
    if(Read_Err==false) {
      if (j < 8) {
          if (val == 1) {Output_l = temp_l | (1 << j);} else {Output_l = temp_l & ~(1 << j);}
          Output_h = temp_h;
      } else {
          j = j - 8;
          if (val == 1) {Output_h = temp_h | (1 << j);} else {Output_h = temp_h & ~(1 << j);}
          Output_l = temp_l;
      }
      WriteTopway_byte(0x00, 0x66, Output_h, Output_l);
    }
}
