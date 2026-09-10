void handleRequest1() {
  WiFiClient client = wifiServer.available();
  if (!client) return;

  unsigned long timeout = millis();
  String requestLine = "";
  requestLine.reserve(256); // Reserve memory for the request line

  // Wait for the HTTP request line
  while (client.connected() && millis() - timeout < 1000) {
    if (client.available()) {
      char c = client.read();
      requestLine += c;
      if (c == '\n') break; // End of request line
    }
  }

  requestLine.trim(); // Clean up
  Power_Off_Timer=0;

  // Now check and parse
  if (requestLine.indexOf("/?files") >= 0 && SoftWareConnected ==true) {
    const char* boundary = "MyBoundary123";
    const char* files[] = {"/FR.csv", "/DevInf.csv", "/GRP.csv", "/LOC.csv", "/RECP.csv", "/RMK.csv", "/USR.csv", "/CompDet.csv", "/ActLog.csv", "/staticIP.csv", "/SERVCRED.csv"};
    const int fileCount = 11;

    client.println("HTTP/1.1 200 OK");
    client.print("Content-Type: multipart/mixed; boundary=");
    client.println(boundary);
    client.println("Connection: close");
    client.println();

    for (int i = 0; i < fileCount; i++) {
      // Check existence first
      if (!SD.exists(files[i])) continue;

      File file = SD.open(files[i], FILE_READ);
      if (file) {
        client.print("--");
        client.println(boundary);
        client.println("Content-Type: text/csv");

        String filename = String(files[i]);
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash >= 0) filename = filename.substring(lastSlash + 1);

        client.print("Content-Disposition: attachment; filename=\"");
        client.print(filename);
        client.println("\"");
        client.println();

        // Efficient buffer copy
        uint8_t buf[64];
        while (file.available()) {
          int n = file.read(buf, sizeof(buf));
          if (n > 0) client.write(buf, n);
        }
        
        client.println();  // Separate parts
        file.close();
      }
    }

    client.print("--");
    client.print(boundary);
    client.println("--");
  } else if (requestLine.indexOf("/?Sample") != -1) {     //----------------------------------------------Online Sampling-----------------------------------------------------
    int startIndex = requestLine.indexOf("/?Sample");
    int endIndex = requestLine.indexOf(' ', startIndex);  // Space before HTTP/1.1
    inputData = requestLine.substring(startIndex, endIndex);
    inputData.replace("%20", " ");

    // Parse dashes
    pos1 = inputData.indexOf('-');
    pos2 = inputData.indexOf('-', pos1 + 1);
    pos3 = inputData.indexOf('-', pos2 + 1);
    pos4 = inputData.indexOf('-', pos3 + 1);
    pos5 = inputData.indexOf('-', pos4 + 1);
    pos6 = inputData.indexOf('-', pos5 + 1);
    pos7 = inputData.indexOf('-', pos6 + 1);
    pos8 = inputData.indexOf('-', pos7 + 1);
    pos9 = inputData.indexOf('-', pos8 + 1);

    if (pos9 != -1) {  // basic check
      onlineUser = inputData.substring(pos1+1, pos2);
      Mode = inputData.substring(pos2+1, pos3);
      SampleName = inputData.substring(pos3+1, pos4);
      SamplingLocation = inputData.substring(pos4+1, pos5);
      Motor_lts = inputData.substring(pos5+1, pos6).toInt();
      Start_Delay1 = inputData.substring(pos6+1, pos7).toInt();
      No_of_Runs = inputData.substring(pos7+1, pos8).toInt();
      Delay_between_Runs = inputData.substring(pos8+1, pos9).toInt();
      WriteOnlineSample();
      if (!Motor_Status1) Sample_Run();

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.print("Sampling Started ");
      client.println(inputData);
    } else {
      client.println("HTTP/1.1 400 Bad Request");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.print("Invalid Format: ");
      client.println(requestLine);
    }
  } else if (requestLine.indexOf("/?Status") >= 0) {     //----------------------------------------------Status-----------------------------------------------------
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Access-Control-Allow-Origin: *");  
    client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");  
    client.println("Access-Control-Allow-Headers: Content-Type");
    client.println("Connection: close");  
    client.println();
    if(SoftWareConnected==true) {
      client.print("DEVICE CONNECTED");
    } else if(Motor_Status1==true) {
      client.print("Sample Running");
      client.print("  ");
      client.print(Motor_Counter);
    } else {
      client.print(SampleRunString);
    }
  } else if (requestLine.indexOf("/?SampSt") >= 0) {     //----------------------------------------------Motor Running Status-----------------------------------------------------
    if(Motor_Status1==true) {
      client.println("HTTP/1.1 200 Sample Running");
    } else {
      client.println("HTTP/1.1 200 Sample Standby");
    }
    client.println("Content-Type: text/plain");
    client.println("Access-Control-Allow-Origin: *");  
    client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");  
    client.println("Access-Control-Allow-Headers: Content-Type");
    client.println("Connection: close");
    client.println();
    if(Motor_Status1==true) {
      client.print(SampleRunString);
      client.print("  ");
      client.print(Motor_Counter);
    } else {
      client.print(SampleRunString);
    }
  } else if (requestLine.indexOf("/?Abort") >= 0) {     //----------------------------------------------Sample Abort-----------------------------------------------------
    // --- ADD THESE 3 LINES ---
    String tempUser = UserId;     // Remember who was logged in on the screen
    UserId = "RemoteUser";            // Force the Audit Trail to say "RemoteUser"
    Sample_Abort();               // Run the abort
    UserId = tempUser;            // Put the screen user back
    // -------------------------
    client.println("HTTP/1.1 200 Connected to Galleon-SSS");
    client.println("Content-Type: text/plain");
    client.println("Access-Control-Allow-Origin: *");  
    client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");  
    client.println("Access-Control-Allow-Headers: Content-Type");
    client.println("Connection: close");
    client.println();
    client.print("SAMPLE ABORT");
  } else if (requestLine.indexOf("/?BatST") >= 0) {     //----------------------------------------------Battery Status-----------------------------------------------------
    if(ChargerStatus==true) {
      client.println("HTTP/1.1 200 Charger Connceted");
    } else {
      client.println("HTTP/1.1 200 Device Online");
    }
    client.println("Content-Type: text/plain");
    client.println("Access-Control-Allow-Origin: *");  
    client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");  
    client.println("Access-Control-Allow-Headers: Content-Type");
    client.println("Connection: close");
    client.println();
    if(ChargerStatus==true) {
      client.print("CHARGER CONNECTED");
    } else {
      client.print(BAT_PER);
    }
  } else if (requestLine.indexOf("/?Launch") >= 0) {     //----------------------------------------------Launch-----------------------------------------------------
    DisplayPage(207);SoftWareConnected =true;
    client.println("HTTP/1.1 200 Connected to Galleon-SSS");
    client.println("Content-Type: text/plain");
    client.println("Access-Control-Allow-Origin: *");  
    client.println("Access-Control-Allow-Methods: GET, POST, OPTIONS");  
    client.println("Access-Control-Allow-Headers: Content-Type");
    client.println("Connection: close");
    client.println();
    client.print("DEVICE ONLINE");
  } else if (requestLine.indexOf("/?LOCADD") >= 0 && SoftWareConnected ==true) {     //----------------------------------------------Location Management-----------------------------------------------------
    int startIndex = requestLine.indexOf("/?LOCADD");
    int endIndex = requestLine.indexOf(" ", startIndex);
    inputData = requestLine.substring(startIndex, endIndex);
    inputData.replace("%20", " ");
    pos1 = inputData.indexOf('-');
    pos2 = inputData.indexOf('-', pos1 + 1);
    
    if(pos1 != -1 && pos2 != -1) {
        Str2 = inputData.substring(pos1+1,pos2);
        CheckLocation("/LOC.csv");WriteTopway(0x00, 0x76, 0);WriteTopway(0x00, 0x74, 0);
        if(OpStatus==true) {ParamId=LocationId;strParam=Str2;WriteConfigParam(1,"/LOC.csv");WriteTopway(0x00,0x76,0);}
        UserId="RemoteUser"; AuditDetails=Str2; AuditRemark="Remote"; if(OpStatus==true){LogActivity("LocationCreated");} UserId="NO_USER";
    
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        if(OpStatus==true) {client.print("LOCATION ADDED");} else {client.print("LOCATION ALREADY EXIST");}
        client.println(inputData);
    }
  } else if (requestLine.indexOf("/?LOCDIS") >= 0 && SoftWareConnected ==true) {
    int startIndex = requestLine.indexOf("/?LOCDIS");
    int endIndex = requestLine.indexOf(" ", startIndex);
    inputData = requestLine.substring(startIndex, endIndex);
    inputData.replace("%20", " ");
    pos1 = inputData.indexOf('-');
    pos2 = inputData.indexOf('-', pos1 + 1);
    
    if(pos1 != -1 && pos2 != -1) {
        Str2 = inputData.substring(pos1+1,pos2);
        CheckLocation("/LOC.csv");WriteTopway(0x00, 0x76, 0);WriteTopway(0x00, 0x74, 0);
        if(OpStatus==false) {ParamId=LocationId;strParam=Str2;Discard_Location(3,"/LOC.csv");WriteTopway(0x00,0x74,0);}
        UserId="RemoteUser"; AuditDetails=Str2; AuditRemark="Remote"; if(OpStatus==false){LogActivity("LocationDiscarded");} UserId="NO_USER";
    
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        if(OpStatus==false) {client.print("LOCATION DISCARDED");} else {client.print("LOCATION DOES NOT EXIST");}
        client.println(inputData);
    }
  } else if (requestLine.indexOf("/?REMADD") >= 0 && SoftWareConnected ==true) {     //----------------------------------------------Remark Management-----------------------------------------------------
    int startIndex = requestLine.indexOf("/?REMADD");
    int endIndex = requestLine.indexOf(" ", startIndex);
    inputData = requestLine.substring(startIndex, endIndex);
    inputData.replace("%20", " ");
    pos1 = inputData.indexOf('-');
    pos2 = inputData.indexOf('-', pos1 + 1);
    
    if(pos1 != -1 && pos2 != -1) {
        Str2 = inputData.substring(pos1+1,pos2);
        CheckLocation("/RMK.csv");WriteTopway(0x00, 0x76, 0);WriteTopway(0x00, 0x74, 0);
        if(OpStatus==true) {ParamId=RemarkId;strParam=Str2;WriteConfigParam(2,"/RMK.csv");WriteTopway(0x00,0x76,0);}
        UserId="RemoteUser"; AuditDetails=Str2; AuditRemark="Remote"; if(OpStatus==true){LogActivity("RemarkCreated");} UserId="NO_USER";
    
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        if(OpStatus==true) {client.print("REMARK ADDED");} else {client.print("REMARK ALREADY EXIST");}
        client.println(inputData);
    }
  } else if (requestLine.indexOf("/?REMDIS") >= 0 && SoftWareConnected ==true) {
    int startIndex = requestLine.indexOf("/?REMDIS");
    int endIndex = requestLine.indexOf(" ", startIndex);
    inputData = requestLine.substring(startIndex, endIndex);
    inputData.replace("%20", " ");
    pos1 = inputData.indexOf('-');
    pos2 = inputData.indexOf('-', pos1 + 1);
    
    if(pos1 != -1 && pos2 != -1) {
        Str2 = inputData.substring(pos1+1,pos2);
        CheckLocation("/RMK.csv");WriteTopway(0x00, 0x76, 0);WriteTopway(0x00, 0x74, 0);
        if(OpStatus==false) {ParamId=RemarkId;strParam=Str2;Discard_Location(4,"/RMK.csv");WriteTopway(0x00,0x74,0);}
        UserId="RemoteUser"; AuditDetails=Str2; AuditRemark="Remote"; if(OpStatus==false){LogActivity("RemarkDiscarded");} UserId="NO_USER";
    
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        if(OpStatus==false) {client.print("REMARK DISCARDED");} else {client.print("REMARK DOES NOT EXIST");}
        client.println(inputData);
    }
  } else if (requestLine.indexOf("/?RECPADD") >= 0 && SoftWareConnected ==true) {     //----------------------------------------------Recipe Management-----------------------------------------------------
    int startIndex = requestLine.indexOf("/?RECPADD");
    int endIndex = requestLine.indexOf(" ", startIndex);
    inputData = requestLine.substring(startIndex, endIndex);
    inputData.replace("%20", " ");
    pos1 = inputData.indexOf('-');
    pos2 = inputData.indexOf('-', pos1 + 1);
    pos3 = inputData.indexOf('-', pos2 + 1);
    pos4 = inputData.indexOf('-', pos3 + 1);
    pos5 = inputData.indexOf('-', pos4 + 1);
    pos6 = inputData.indexOf('-', pos5 + 1);
    pos7 = inputData.indexOf('-', pos6 + 1);
    pos8 = inputData.indexOf('-', pos7 + 1);
    
    if(pos8 != -1) {
        strRecipe = inputData.substring(pos1+1,pos2);
        RecipeMode = inputData.substring(pos2+1,pos3);
        RecipeLoc = inputData.substring(pos3+1,pos4);
        RecipeVolume = inputData.substring(pos4+1,pos5).toInt();
        RecipeStartDelay = inputData.substring(pos5+1,pos6).toInt();
        RecipeNoOfRuns = inputData.substring(pos6+1,pos7).toInt();
        RecipeDelayBetRun = inputData.substring(pos7+1,pos8).toInt();
        Str2=strRecipe;
        CheckLocation("/RECP.csv");WriteTopway(0x00, 0x76, 0);WriteTopway(0x00, 0x74, 0);
        if(OpStatus==true) {AddRecipe1();WriteTopway(0x00,0x76,0);}
        UserId="RemoteUser"; AuditDetails=strRecipe; AuditRemark="Remote"; if(OpStatus==true){LogActivity("RecipeCreated");} UserId="NO_USER";
    
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        if(OpStatus==true) {client.print("RECIPE ADDED");} else {client.print("RECIPE ALREADY EXIST");}
        client.println(inputData);
    }
  } else if (requestLine.indexOf("/?RECPDIS") >= 0 && SoftWareConnected ==true) {
    int startIndex = requestLine.indexOf("/?RECPDIS");
    int endIndex = requestLine.indexOf(" ", startIndex);
    inputData = requestLine.substring(startIndex, endIndex);
    inputData.replace("%20", " ");
    pos1 = inputData.indexOf('-');
    pos2 = inputData.indexOf('-', pos1 + 1);
    
    if(pos1 != -1 && pos2 != -1) {
        Str2 = inputData.substring(pos1+1,pos2);
        CheckLocation("/RECP.csv");WriteTopway(0x00, 0x76, 0);WriteTopway(0x00, 0x74, 0);
        if(OpStatus==false) {strParam=Str2;Discard_Recipe();WriteTopway(0x00,0x74,0);}
        UserId="RemoteUser"; AuditDetails=Str2; AuditRemark="Remote"; if(OpStatus==false){LogActivity("RecipeDiscarded");} UserId="NO_USER";
    
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        if(OpStatus==false) {client.print("RECIPE DISCARDED");} else {client.print("RECIPE DOES NOT EXIST");}
        client.println(inputData);
    }
  } else if (requestLine.indexOf("/?USERADD") >= 0 && SoftWareConnected ==true) {     //----------------------------------------------User Management-----------------------------------------------------
    int startIndex = requestLine.indexOf("/?USERADD");
    int endIndex = requestLine.indexOf(" ", startIndex);
    inputData = requestLine.substring(startIndex, endIndex);
    inputData.replace("%20", " ");
    inputData.replace("%23", "#");
    pos1 = inputData.indexOf('-');
    pos2 = inputData.indexOf('-', pos1 + 1);
    pos3 = inputData.indexOf('-', pos2 + 1);
    pos4 = inputData.indexOf('-', pos3 + 1);
    pos5 = inputData.indexOf('-', pos4 + 1);
    pos6 = inputData.indexOf('-', pos5 + 1);
    pos7 = inputData.indexOf('-', pos6 + 1);
        
    if(pos7 != -1) {
        strGUser = inputData.substring(pos1+1,pos2);
        strGUser.trim();
        GUserPass1 = inputData.substring(pos2+1,pos3);
        GUserPass1.trim();
        FirstName = inputData.substring(pos3+1,pos4);
        FirstName.trim();
        LastName = inputData.substring(pos4+1,pos5);
        LastName.trim();
        GUserDept = inputData.substring(pos5+1,pos6);
        GUserDept.trim();
        GUserGroup = inputData.substring(pos6+1,pos7);
        GUserGroup.trim();
        
        CheckUser();WriteTopway(0x00, 0x76, 0);WriteTopway(0x00, 0x74, 0);
        if(OpStatus==true) {AddUser1(0);}
        UserId="RemoteUser"; AuditDetails=strGUser; AuditRemark="Remote"; if(OpStatus==true){LogActivity("UserCreated");} UserId="NO_USER";
    
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        if(OpStatus==true) {client.print("USER ADDED");} else {client.print("USER ALREADY EXIST");}
        client.println(inputData);
    }
  } else if (requestLine.indexOf("/?USERUPLOAD") >= 0 && SoftWareConnected ==true) {
    int startIndex = requestLine.indexOf("/?USERUPLOAD");
    int endIndex = requestLine.indexOf(" ", startIndex);
    inputData = requestLine.substring(startIndex, endIndex);
    inputData.replace("%20", " ");
    inputData.replace("%23", "#");
    pos1 = inputData.indexOf('-');
    pos2 = inputData.indexOf('-', pos1 + 1);
    pos3 = inputData.indexOf('-', pos2 + 1);
    pos4 = inputData.indexOf('-', pos3 + 1);
    pos5 = inputData.indexOf('-', pos4 + 1);
    pos6 = inputData.indexOf('-', pos5 + 1);
    pos7 = inputData.indexOf('-', pos6 + 1);
        
    if(pos7 != -1) {
        strGUser = inputData.substring(pos1+1,pos2);
        GUserPass1 = inputData.substring(pos2+1,pos3);
        FirstName = inputData.substring(pos3+1,pos4);
        LastName = inputData.substring(pos4+1,pos5);
        GUserDept = inputData.substring(pos5+1,pos6);
        GUserGroup = inputData.substring(pos6+1,pos7);
        
        CheckUser();WriteTopway(0x00, 0x76, 0);WriteTopway(0x00, 0x74, 0);
        if(OpStatus==true) {AddUser1(1);}
        UserId="RemoteUser"; AuditDetails=strGUser; AuditRemark="Remote"; if(OpStatus==true){LogActivity("UserUploaded");} UserId="NO_USER";
    
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        if(OpStatus==true) {client.print("USER UPLOADED");} else {client.print("USER ALREADY EXIST");}
        client.println(inputData);
    }
  } else if (requestLine.indexOf("/?USERRST") >= 0 && SoftWareConnected ==true) {
    int startIndex = requestLine.indexOf("/?USERRST");
    int endIndex = requestLine.indexOf(" ", startIndex);
    inputData = requestLine.substring(startIndex, endIndex);
    inputData.replace("%20", " ");
    inputData.replace("%23", "#");
    pos1 = inputData.indexOf('-');
    pos2 = inputData.indexOf('-', pos1 + 1);
    pos3 = inputData.indexOf('-', pos2 + 1);
        
    if(pos3 != -1) {
        strGUser = inputData.substring(pos1+1,pos2);
        strGUser.trim();
        GUserPass1 = inputData.substring(pos2+1,pos3);
        GUserPass1.trim();
        
        CheckUser();WriteTopway(0x00, 0x76, 0);WriteTopway(0x00, 0x74, 0);
        if(OpStatus==false) {
          Reset_Password();
          delay(100);
          String userFile = "/GUSER/" + strGUser + ".csv";
          if(SD.exists(userFile)) {
                Manage_User(7, userFile); // Run the same reset logic on the individual file
                delay(100); // Wait again
          }
        }
        UserId="RemoteUser"; AuditDetails=strGUser; AuditRemark="Remote"; if(OpStatus==false){LogActivity("UserPswReset");} UserId="NO_USER";
    
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        if(OpStatus==false) {client.print("USER PASSWORD RESET");} else {client.print("USER DOES NOT EXIST");}
        client.println(inputData);
    }
  } else if (requestLine.indexOf("/?USERDIS") >= 0 && SoftWareConnected ==true) {
    int startIndex = requestLine.indexOf("/?USERDIS");
    int endIndex = requestLine.indexOf(" ", startIndex);
    inputData = requestLine.substring(startIndex, endIndex);
    inputData.replace("%20", " ");
    pos1 = inputData.indexOf('-');
    pos2 = inputData.indexOf('-', pos1 + 1);
    
    if(pos1 != -1 && pos2 != -1) {
        strGUser = inputData.substring(pos1+1,pos2);

        Str2 = strGUser;

        CheckUser();WriteTopway(0x00, 0x76, 0);WriteTopway(0x00, 0x74, 0);
        if(OpStatus==false) {strParam=Str2;Discard_User();}
        UserId="RemoteUser"; AuditDetails=strGUser; AuditRemark="Remote"; if(OpStatus==false){LogActivity("UserDiscarded");} UserId="NO_USER";
    
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        if(OpStatus==false) {client.print("USER DISCARDED");} else {client.print("USER DOES NOT EXIST");}
        client.println(inputData);
    }
  } else if (requestLine.indexOf("/?GRPADD") >= 0 && SoftWareConnected ==true) {     //----------------------------------------------Group Management-----------------------------------------------------
    int startIndex = requestLine.indexOf("/?GRPADD");
    int endIndex = requestLine.indexOf(" ", startIndex);
    inputData = requestLine.substring(startIndex, endIndex);
    inputData.replace("%20", " ");
    pos1 = inputData.indexOf('-');
    pos2 = inputData.indexOf('-', pos1 + 1);
    pos3 = inputData.indexOf('-', pos2 + 1);
    pos4 = inputData.indexOf('-', pos3 + 1);
    pos5 = inputData.indexOf('-', pos4 + 1);
    pos6 = inputData.indexOf('-', pos5 + 1);
    pos7 = inputData.indexOf('-', pos6 + 1);
    pos8 = inputData.indexOf('-', pos7 + 1);
    pos9 = inputData.indexOf('-', pos8 + 1);
    pos10 = inputData.indexOf('-', pos9 + 1);
    pos11 = inputData.indexOf('-', pos10 + 1);
    pos12 = inputData.indexOf('-', pos11 + 1);
    pos13 = inputData.indexOf('-', pos12 + 1);
    pos14 = inputData.indexOf('-', pos13 + 1);
    pos15 = inputData.indexOf('-', pos14 + 1);
    pos16 = inputData.indexOf('-', pos15 + 1);
    pos17 = inputData.indexOf('-', pos16 + 1);
    pos18 = inputData.indexOf('-', pos17 + 1);
    pos19 = inputData.indexOf('-', pos18 + 1);
    pos20 = inputData.indexOf('-', pos19 + 1);
    pos21 = inputData.indexOf('-', pos20 + 1);
    
    if(pos21 != -1) {
        strGroup = inputData.substring(pos1+1,pos2);
        GroupMinPassLength = inputData.substring(pos2+1,pos3).toInt();
        GroupPasswordExpiy = inputData.substring(pos3+1,pos4).toInt();
        GroupRetryLimits = inputData.substring(pos4+1,pos5).toInt();
        PasswordComplexity = inputData.substring(pos5+1,pos6);
        LastPAsswordUnique = inputData.substring(pos6+1,pos7).toInt();
        ResetPassAuth = inputData.substring(pos7+1,pos8);
        FixedVolume = inputData.substring(pos8+1,pos9);
        Logout_Timer1 = inputData.substring(pos9+1,pos10).toInt();
        Power_Off_Timer1 = inputData.substring(pos10+1,pos11).toInt();
        Software_Access = inputData.substring(pos11+1,pos12);
        Device_Monitoring = inputData.substring(pos12+1,pos13);
        Group_User_Setting = inputData.substring(pos13+1,pos14);
        Config_Setting = inputData.substring(pos14+1,pos15);
        Sample_Data_View = inputData.substring(pos15+1,pos16);
        Audit_Trail_View = inputData.substring(pos16+1,pos17);
        Print_Config = inputData.substring(pos17+1,pos18);
        Print_Sample_Data = inputData.substring(pos18+1,pos19);
        Print_Audit_Trail = inputData.substring(pos19+1,pos20);
        Allow_Sampling = inputData.substring(pos20+1,pos21);
        Str2=strGroup;
        CheckGroup();WriteTopway(0x00, 0x76, 0);WriteTopway(0x00, 0x74, 0);
        if(OpStatus==true) {AddGroup1();}
        UserId="RemoteUser"; AuditDetails=strGroup; AuditRemark="Remote"; if(OpStatus==true){LogActivity("GroupAdded");} UserId="NO_USER";
    
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        if(OpStatus==true) {client.print("GROUP ADDED");} else {client.print("GROUP ALREADY EXIST");}
        client.println(strGroup);
    }
  } else if (requestLine.indexOf("/?GRPDIS") >= 0 && SoftWareConnected ==true) {
    int startIndex = requestLine.indexOf("/?GRPDIS");
    int endIndex = requestLine.indexOf(" ", startIndex);
    inputData = requestLine.substring(startIndex, endIndex);
    inputData.replace("%20", " ");
    pos1 = inputData.indexOf('-');
    pos2 = inputData.indexOf('-', pos1 + 1);
    
    if(pos1 != -1 && pos2 != -1) {
        Str2 = inputData.substring(pos1+1,pos2);
        CheckGroup();
        if(OpStatus==false) {strParam=Str2;Update_Group();}
        WriteTopway(0x00, 0x76, 0);WriteTopway(0x00, 0x74, 0);
        UserId="RemoteUser"; AuditDetails=Str2; AuditRemark="Remote"; if(OpStatus==false){LogActivity("GroupDiscarded");} UserId="NO_USER";
        
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        if(OpStatus==false) {client.print("GROUP DISCARDED");} else {client.print("GROUP DOES NOT EXIST");}
        client.println(inputData);
    }
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.print("Error Decoding...Request: ");
    client.println(requestLine);
  }

  delay(1);
  client.flush();
  client.stop();
}




// static void sendTextResponse(WiFiClient& client,
//                               const char* statusLine,
//                               const char* body,
//                               bool cors = false) {
//     String resp = statusLine;
//     resp += "\r\nContent-Type: text/plain\r\nContent-Length: ";
//     resp += String(strlen(body));
//     if (cors) {
//         resp += "\r\nAccess-Control-Allow-Origin: *"
//                 "\r\nAccess-Control-Allow-Methods: GET, POST, OPTIONS"
//                 "\r\nAccess-Control-Allow-Headers: Content-Type";
//     }
//     resp += "\r\nConnection: close\r\n\r\n";
//     resp += body;
//     client.print(resp);
// }

// // --------------- Helper: Send Files (multipart) -------------
// static void sendFilesResponse(WiFiClient& client) {
//     const char* boundary = "B";   // Short boundary = less overhead per part
//     const char* files[] = {
//         "/FR.csv", "/DevInf.csv", "/GRP.csv", "/LOC.csv",
//         "/RECP.csv", "/RMK.csv", "/USR.csv", "/CompDet.csv",
//         "/ActLog.csv", "/staticIP.csv", "/SERVCRED.csv"
//     };
//     const int fileCount = 11;

//     // ---- Pre-calculate total Content-Length ----
//     // Allows HTTP client to pipeline and show progress bar
//     size_t totalSize = 0;
//     for (int i = 0; i < fileCount; i++) {
//         if (!SD.exists(files[i])) continue;
//         File f = SD.open(files[i], FILE_READ);
//         if (f) {
//             // File data + part header overhead (~80 bytes) + boundary (~6 bytes)
//             totalSize += f.size() + 90;
//             f.close();
//         }
//     }
//     // Add final closing boundary: --B--\r\n = 7 bytes
//     totalSize += 7;

//     // ---- Send HTTP header in ONE write ----
//     String header = "HTTP/1.1 200 OK\r\nContent-Type: multipart/mixed; boundary=";
//     header += boundary;
//     header += "\r\nContent-Length: ";
//     header += String(totalSize);
//     header += "\r\nConnection: close\r\n\r\n";
//     client.print(header);

//     // ---- 8KB static buffer — avoids stack overflow, maximises SD read ----
//     static uint8_t buf[8192];

//     for (int i = 0; i < fileCount; i++) {
//         if (!SD.exists(files[i])) continue;

//         File file = SD.open(files[i], FILE_READ);
//         if (!file) continue;

//         // Extract filename (strip leading slash)
//         String fname = String(files[i]);
//         fname = fname.substring(fname.lastIndexOf('/') + 1);

//         // Part header — ONE write
//         String partHeader = "--";
//         partHeader += boundary;
//         partHeader += "\r\nContent-Type: text/csv\r\nContent-Disposition: attachment; filename=\"";
//         partHeader += fname;
//         partHeader += "\"\r\n\r\n";
//         client.print(partHeader);

//         // Stream file in 8KB chunks
//         while (file.available()) {
//             size_t bytesRead = file.read(buf, sizeof(buf));
//             if (bytesRead > 0) {
//                 size_t sent = 0;
//                 while (sent < bytesRead) {
//                     int n = client.write(buf + sent, bytesRead - sent);
//                     if (n <= 0) {           // Client disconnected mid-transfer
//                         file.close();
//                         return;
//                     }
//                     sent += n;
//                 }
//             }
//             yield();    // Feed watchdog timer — critical for 50MB transfer
//         }

//         client.print("\r\n");   // Part separator
//         file.close();
//     }

//     // Final closing boundary
//     client.print("--");
//     client.print(boundary);
//     client.print("--\r\n");
// }

// // ============================================================
// //  Main Handler
// // ============================================================
// void handleRequest1() {
//     WiFiClient client = wifiServer.available();
//     if (!client) return;

//     // ✅ Disable Nagle — sends TCP packets immediately, no batching delay
//     client.setNoDelay(true);
//     client.setTimeout(500);     // Don't wait more than 500ms for slow clients

//     Power_Off_Timer = 0;

//     // ✅ Read request line in one call — faster than char-by-char
//     String requestLine = "";
//     requestLine.reserve(256);

//     unsigned long timeout = millis();
//     while (client.connected() && (millis() - timeout < 500)) {
//         if (client.available()) {
//             requestLine = client.readStringUntil('\n');
//             requestLine.trim();
//             break;
//         }
//     }

//     // ✅ Early exit on empty request — prevents wasted processing
//     if (requestLine.length() == 0) {
//         client.stop();
//         return;
//     }

//     // ============================================================
//     //  Route: /?files  — Download all CSV files
//     // ============================================================
//     if (requestLine.indexOf("/?files") >= 0 && SoftWareConnected == true) {
//         sendFilesResponse(client);

//     // ============================================================
//     //  Route: /?Sample — Online Sampling
//     // ============================================================
//     } else if (requestLine.indexOf("/?Sample") != -1) {
//         int startIndex = requestLine.indexOf("/?Sample");
//         int endIndex   = requestLine.indexOf(' ', startIndex);
//         inputData      = requestLine.substring(startIndex, endIndex);
//         inputData.replace("%20", " ");

//         pos1 = inputData.indexOf('-');
//         pos2 = inputData.indexOf('-', pos1 + 1);
//         pos3 = inputData.indexOf('-', pos2 + 1);
//         pos4 = inputData.indexOf('-', pos3 + 1);
//         pos5 = inputData.indexOf('-', pos4 + 1);
//         pos6 = inputData.indexOf('-', pos5 + 1);
//         pos7 = inputData.indexOf('-', pos6 + 1);
//         pos8 = inputData.indexOf('-', pos7 + 1);
//         pos9 = inputData.indexOf('-', pos8 + 1);

//         if (pos9 != -1) {
//             onlineUser          = inputData.substring(pos1 + 1, pos2);
//             Mode                = inputData.substring(pos2 + 1, pos3);
//             SampleName          = inputData.substring(pos3 + 1, pos4);
//             SamplingLocation    = inputData.substring(pos4 + 1, pos5);
//             Motor_lts           = inputData.substring(pos5 + 1, pos6).toInt();
//             Start_Delay1        = inputData.substring(pos6 + 1, pos7).toInt();
//             No_of_Runs          = inputData.substring(pos7 + 1, pos8).toInt();
//             Delay_between_Runs  = inputData.substring(pos8 + 1, pos9).toInt();
//             WriteOnlineSample();
//             if (!Motor_Status1) Sample_Run();

//             String body = "Sampling Started " + inputData;
//             sendTextResponse(client, "HTTP/1.1 200 OK", body.c_str());
//         } else {
//             String body = "Invalid Format: " + requestLine;
//             sendTextResponse(client, "HTTP/1.1 400 Bad Request", body.c_str());
//         }

//     // ============================================================
//     //  Route: /?Status — Device Status
//     // ============================================================
//     } else if (requestLine.indexOf("/?Status") >= 0) {
//         String body;
//         if (SoftWareConnected == true) {
//             body = "DEVICE CONNECTED";
//         } else if (Motor_Status1 == true) {
//             body = "Sample Running  " + String(Motor_Counter);
//         } else {
//             body = SampleRunString;
//         }
//         sendTextResponse(client, "HTTP/1.1 200 OK", body.c_str(), true);

//     // ============================================================
//     //  Route: /?SampSt — Motor Running Status
//     // ============================================================
//     } else if (requestLine.indexOf("/?SampSt") >= 0) {
//         const char* statusLine = (Motor_Status1 == true)
//             ? "HTTP/1.1 200 Sample Running"
//             : "HTTP/1.1 200 Sample Standby";

//         String body;
//         if (Motor_Status1 == true) {
//             body = SampleRunString + "  " + String(Motor_Counter);
//         } else {
//             body = SampleRunString;
//         }
//         sendTextResponse(client, statusLine, body.c_str(), true);

//     // ============================================================
//     //  Route: /?Abort — Sample Abort
//     // ============================================================
//     } else if (requestLine.indexOf("/?Abort") >= 0) {
//         String tempUser = UserId;
//         UserId = "RemoteUser";
//         Sample_Abort();
//         UserId = tempUser;
//         sendTextResponse(client, "HTTP/1.1 200 Connected to Galleon-SSS", "SAMPLE ABORT", true);

//     // ============================================================
//     //  Route: /?BatST — Battery Status
//     // ============================================================
//     } else if (requestLine.indexOf("/?BatST") >= 0) {
//         const char* statusLine = (ChargerStatus == true)
//             ? "HTTP/1.1 200 Charger Connected"
//             : "HTTP/1.1 200 Device Online";

//         String body = (ChargerStatus == true) ? "CHARGER CONNECTED" : String(BAT_PER);
//         sendTextResponse(client, statusLine, body.c_str(), true);

//     // ============================================================
//     //  Route: /?Launch — Launch / Connect Software
//     // ============================================================
//     } else if (requestLine.indexOf("/?Launch") >= 0) {
//         DisplayPage(207);
//         SoftWareConnected = true;
//         sendTextResponse(client, "HTTP/1.1 200 Connected to Galleon-SSS", "DEVICE ONLINE", true);

//     // ============================================================
//     //  Route: /?LOCADD — Add Location
//     // ============================================================
//     } else if (requestLine.indexOf("/?LOCADD") >= 0 && SoftWareConnected == true) {
//         int startIndex = requestLine.indexOf("/?LOCADD");
//         int endIndex   = requestLine.indexOf(" ", startIndex);
//         inputData      = requestLine.substring(startIndex, endIndex);
//         inputData.replace("%20", " ");
//         pos1 = inputData.indexOf('-');
//         pos2 = inputData.indexOf('-', pos1 + 1);

//         if (pos1 != -1 && pos2 != -1) {
//             Str2 = inputData.substring(pos1 + 1, pos2);
//             CheckLocation("/LOC.csv");
//             WriteTopway(0x00, 0x76, 0);
//             WriteTopway(0x00, 0x74, 0);
//             if (OpStatus == true) {
//                 ParamId = LocationId;
//                 strParam = Str2;
//                 WriteConfigParam(1, "/LOC.csv");
//                 WriteTopway(0x00, 0x76, 0);
//             }
//             UserId = "RemoteUser";
//             AuditDetails = Str2;
//             AuditRemark = "Remote";
//             if (OpStatus == true) LogActivity("LocationCreated");
//             UserId = "NO_USER";

//             String body = (OpStatus == true)
//                 ? "LOCATION ADDED" + inputData
//                 : "LOCATION ALREADY EXIST" + inputData;
//             sendTextResponse(client, "HTTP/1.1 200 OK", body.c_str());
//         }

//     // ============================================================
//     //  Route: /?LOCDIS — Discard Location
//     // ============================================================
//     } else if (requestLine.indexOf("/?LOCDIS") >= 0 && SoftWareConnected == true) {
//         int startIndex = requestLine.indexOf("/?LOCDIS");
//         int endIndex   = requestLine.indexOf(" ", startIndex);
//         inputData      = requestLine.substring(startIndex, endIndex);
//         inputData.replace("%20", " ");
//         pos1 = inputData.indexOf('-');
//         pos2 = inputData.indexOf('-', pos1 + 1);

//         if (pos1 != -1 && pos2 != -1) {
//             Str2 = inputData.substring(pos1 + 1, pos2);
//             CheckLocation("/LOC.csv");
//             WriteTopway(0x00, 0x76, 0);
//             WriteTopway(0x00, 0x74, 0);
//             if (OpStatus == false) {
//                 ParamId = LocationId;
//                 strParam = Str2;
//                 Discard_Location(3, "/LOC.csv");
//                 WriteTopway(0x00, 0x74, 0);
//             }
//             UserId = "RemoteUser";
//             AuditDetails = Str2;
//             AuditRemark = "Remote";
//             if (OpStatus == false) LogActivity("LocationDiscarded");
//             UserId = "NO_USER";

//             String body = (OpStatus == false)
//                 ? "LOCATION DISCARDED" + inputData
//                 : "LOCATION DOES NOT EXIST" + inputData;
//             sendTextResponse(client, "HTTP/1.1 200 OK", body.c_str());
//         }

//     // ============================================================
//     //  Route: /?REMADD — Add Remark
//     // ============================================================
//     } else if (requestLine.indexOf("/?REMADD") >= 0 && SoftWareConnected == true) {
//         int startIndex = requestLine.indexOf("/?REMADD");
//         int endIndex   = requestLine.indexOf(" ", startIndex);
//         inputData      = requestLine.substring(startIndex, endIndex);
//         inputData.replace("%20", " ");
//         pos1 = inputData.indexOf('-');
//         pos2 = inputData.indexOf('-', pos1 + 1);

//         if (pos1 != -1 && pos2 != -1) {
//             Str2 = inputData.substring(pos1 + 1, pos2);
//             CheckLocation("/RMK.csv");
//             WriteTopway(0x00, 0x76, 0);
//             WriteTopway(0x00, 0x74, 0);
//             if (OpStatus == true) {
//                 ParamId = RemarkId;
//                 strParam = Str2;
//                 WriteConfigParam(2, "/RMK.csv");
//                 WriteTopway(0x00, 0x76, 0);
//             }
//             UserId = "RemoteUser";
//             AuditDetails = Str2;
//             AuditRemark = "Remote";
//             if (OpStatus == true) LogActivity("RemarkCreated");
//             UserId = "NO_USER";

//             String body = (OpStatus == true)
//                 ? "REMARK ADDED" + inputData
//                 : "REMARK ALREADY EXIST" + inputData;
//             sendTextResponse(client, "HTTP/1.1 200 OK", body.c_str());
//         }

//     // ============================================================
//     //  Route: /?REMDIS — Discard Remark
//     // ============================================================
//     } else if (requestLine.indexOf("/?REMDIS") >= 0 && SoftWareConnected == true) {
//         int startIndex = requestLine.indexOf("/?REMDIS");
//         int endIndex   = requestLine.indexOf(" ", startIndex);
//         inputData      = requestLine.substring(startIndex, endIndex);
//         inputData.replace("%20", " ");
//         pos1 = inputData.indexOf('-');
//         pos2 = inputData.indexOf('-', pos1 + 1);

//         if (pos1 != -1 && pos2 != -1) {
//             Str2 = inputData.substring(pos1 + 1, pos2);
//             CheckLocation("/RMK.csv");
//             WriteTopway(0x00, 0x76, 0);
//             WriteTopway(0x00, 0x74, 0);
//             if (OpStatus == false) {
//                 ParamId = RemarkId;
//                 strParam = Str2;
//                 Discard_Location(4, "/RMK.csv");
//                 WriteTopway(0x00, 0x74, 0);
//             }
//             UserId = "RemoteUser";
//             AuditDetails = Str2;
//             AuditRemark = "Remote";
//             if (OpStatus == false) LogActivity("RemarkDiscarded");
//             UserId = "NO_USER";

//             String body = (OpStatus == false)
//                 ? "REMARK DISCARDED" + inputData
//                 : "REMARK DOES NOT EXIST" + inputData;
//             sendTextResponse(client, "HTTP/1.1 200 OK", body.c_str());
//         }

//     // ============================================================
//     //  Route: /?RECPADD — Add Recipe
//     // ============================================================
//     } else if (requestLine.indexOf("/?RECPADD") >= 0 && SoftWareConnected == true) {
//         int startIndex = requestLine.indexOf("/?RECPADD");
//         int endIndex   = requestLine.indexOf(" ", startIndex);
//         inputData      = requestLine.substring(startIndex, endIndex);
//         inputData.replace("%20", " ");
//         pos1  = inputData.indexOf('-');
//         pos2  = inputData.indexOf('-', pos1 + 1);
//         pos3  = inputData.indexOf('-', pos2 + 1);
//         pos4  = inputData.indexOf('-', pos3 + 1);
//         pos5  = inputData.indexOf('-', pos4 + 1);
//         pos6  = inputData.indexOf('-', pos5 + 1);
//         pos7  = inputData.indexOf('-', pos6 + 1);
//         pos8  = inputData.indexOf('-', pos7 + 1);

//         if (pos8 != -1) {
//             strRecipe        = inputData.substring(pos1 + 1, pos2);
//             RecipeMode       = inputData.substring(pos2 + 1, pos3);
//             RecipeLoc        = inputData.substring(pos3 + 1, pos4);
//             RecipeVolume     = inputData.substring(pos4 + 1, pos5).toInt();
//             RecipeStartDelay = inputData.substring(pos5 + 1, pos6).toInt();
//             RecipeNoOfRuns   = inputData.substring(pos6 + 1, pos7).toInt();
//             RecipeDelayBetRun = inputData.substring(pos7 + 1, pos8).toInt();
//             Str2 = strRecipe;
//             CheckLocation("/RECP.csv");
//             WriteTopway(0x00, 0x76, 0);
//             WriteTopway(0x00, 0x74, 0);
//             if (OpStatus == true) {
//                 AddRecipe1();
//                 WriteTopway(0x00, 0x76, 0);
//             }
//             UserId = "RemoteUser";
//             AuditDetails = strRecipe;
//             AuditRemark = "Remote";
//             if (OpStatus == true) LogActivity("RecipeCreated");
//             UserId = "NO_USER";

//             String body = (OpStatus == true)
//                 ? "RECIPE ADDED" + inputData
//                 : "RECIPE ALREADY EXIST" + inputData;
//             sendTextResponse(client, "HTTP/1.1 200 OK", body.c_str());
//         }

//     // ============================================================
//     //  Route: /?RECPDIS — Discard Recipe
//     // ============================================================
//     } else if (requestLine.indexOf("/?RECPDIS") >= 0 && SoftWareConnected == true) {
//         int startIndex = requestLine.indexOf("/?RECPDIS");
//         int endIndex   = requestLine.indexOf(" ", startIndex);
//         inputData      = requestLine.substring(startIndex, endIndex);
//         inputData.replace("%20", " ");
//         pos1 = inputData.indexOf('-');
//         pos2 = inputData.indexOf('-', pos1 + 1);

//         if (pos1 != -1 && pos2 != -1) {
//             Str2 = inputData.substring(pos1 + 1, pos2);
//             CheckLocation("/RECP.csv");
//             WriteTopway(0x00, 0x76, 0);
//             WriteTopway(0x00, 0x74, 0);
//             if (OpStatus == false) {
//                 strParam = Str2;
//                 Discard_Recipe();
//                 WriteTopway(0x00, 0x74, 0);
//             }
//             UserId = "RemoteUser";
//             AuditDetails = Str2;
//             AuditRemark = "Remote";
//             if (OpStatus == false) LogActivity("RecipeDiscarded");
//             UserId = "NO_USER";

//             String body = (OpStatus == false)
//                 ? "RECIPE DISCARDED" + inputData
//                 : "RECIPE DOES NOT EXIST" + inputData;
//             sendTextResponse(client, "HTTP/1.1 200 OK", body.c_str());
//         }

//     // ============================================================
//     //  Route: /?USERADD — Add User
//     // ============================================================
//     } else if (requestLine.indexOf("/?USERADD") >= 0 && SoftWareConnected == true) {
//         int startIndex = requestLine.indexOf("/?USERADD");
//         int endIndex   = requestLine.indexOf(" ", startIndex);
//         inputData      = requestLine.substring(startIndex, endIndex);
//         inputData.replace("%20", " ");
//         inputData.replace("%23", "#");
//         pos1 = inputData.indexOf('-');
//         pos2 = inputData.indexOf('-', pos1 + 1);
//         pos3 = inputData.indexOf('-', pos2 + 1);
//         pos4 = inputData.indexOf('-', pos3 + 1);
//         pos5 = inputData.indexOf('-', pos4 + 1);
//         pos6 = inputData.indexOf('-', pos5 + 1);
//         pos7 = inputData.indexOf('-', pos6 + 1);

//         if (pos7 != -1) {
//             strGUser   = inputData.substring(pos1 + 1, pos2); strGUser.trim();
//             GUserPass1 = inputData.substring(pos2 + 1, pos3); GUserPass1.trim();
//             FirstName  = inputData.substring(pos3 + 1, pos4); FirstName.trim();
//             LastName   = inputData.substring(pos4 + 1, pos5); LastName.trim();
//             GUserDept  = inputData.substring(pos5 + 1, pos6); GUserDept.trim();
//             GUserGroup = inputData.substring(pos6 + 1, pos7); GUserGroup.trim();

//             CheckUser();
//             WriteTopway(0x00, 0x76, 0);
//             WriteTopway(0x00, 0x74, 0);
//             if (OpStatus == true) AddUser1(0);
//             UserId = "RemoteUser";
//             AuditDetails = strGUser;
//             AuditRemark = "Remote";
//             if (OpStatus == true) LogActivity("UserCreated");
//             UserId = "NO_USER";

//             String body = (OpStatus == true)
//                 ? "USER ADDED" + inputData
//                 : "USER ALREADY EXIST" + inputData;
//             sendTextResponse(client, "HTTP/1.1 200 OK", body.c_str());
//         }

//     // ============================================================
//     //  Route: /?USERUPLOAD — Upload User
//     // ============================================================
//     } else if (requestLine.indexOf("/?USERUPLOAD") >= 0 && SoftWareConnected == true) {
//         int startIndex = requestLine.indexOf("/?USERUPLOAD");
//         int endIndex   = requestLine.indexOf(" ", startIndex);
//         inputData      = requestLine.substring(startIndex, endIndex);
//         inputData.replace("%20", " ");
//         inputData.replace("%23", "#");
//         pos1 = inputData.indexOf('-');
//         pos2 = inputData.indexOf('-', pos1 + 1);
//         pos3 = inputData.indexOf('-', pos2 + 1);
//         pos4 = inputData.indexOf('-', pos3 + 1);
//         pos5 = inputData.indexOf('-', pos4 + 1);
//         pos6 = inputData.indexOf('-', pos5 + 1);
//         pos7 = inputData.indexOf('-', pos6 + 1);

//         if (pos7 != -1) {
//             strGUser   = inputData.substring(pos1 + 1, pos2);
//             GUserPass1 = inputData.substring(pos2 + 1, pos3);
//             FirstName  = inputData.substring(pos3 + 1, pos4);
//             LastName   = inputData.substring(pos4 + 1, pos5);
//             GUserDept  = inputData.substring(pos5 + 1, pos6);
//             GUserGroup = inputData.substring(pos6 + 1, pos7);

//             CheckUser();
//             WriteTopway(0x00, 0x76, 0);
//             WriteTopway(0x00, 0x74, 0);
//             if (OpStatus == true) AddUser1(1);
//             UserId = "RemoteUser";
//             AuditDetails = strGUser;
//             AuditRemark = "Remote";
//             if (OpStatus == true) LogActivity("UserUploaded");
//             UserId = "NO_USER";

//             String body = (OpStatus == true)
//                 ? "USER UPLOADED" + inputData
//                 : "USER ALREADY EXIST" + inputData;
//             sendTextResponse(client, "HTTP/1.1 200 OK", body.c_str());
//         }

//     // ============================================================
//     //  Route: /?USERRST — Reset User Password
//     // ============================================================
//     } else if (requestLine.indexOf("/?USERRST") >= 0 && SoftWareConnected == true) {
//         int startIndex = requestLine.indexOf("/?USERRST");
//         int endIndex   = requestLine.indexOf(" ", startIndex);
//         inputData      = requestLine.substring(startIndex, endIndex);
//         inputData.replace("%20", " ");
//         inputData.replace("%23", "#");
//         pos1 = inputData.indexOf('-');
//         pos2 = inputData.indexOf('-', pos1 + 1);
//         pos3 = inputData.indexOf('-', pos2 + 1);

//         if (pos3 != -1) {
//             strGUser   = inputData.substring(pos1 + 1, pos2); strGUser.trim();
//             GUserPass1 = inputData.substring(pos2 + 1, pos3); GUserPass1.trim();

//             CheckUser();
//             WriteTopway(0x00, 0x76, 0);
//             WriteTopway(0x00, 0x74, 0);
//             if (OpStatus == false) {
//                 Reset_Password();
//                 delay(100);
//                 String userFile = "/GUSER/" + strGUser + ".csv";
//                 if (SD.exists(userFile)) {
//                     Manage_User(7, userFile);
//                     delay(100);
//                 }
//             }
//             UserId = "RemoteUser";
//             AuditDetails = strGUser;
//             AuditRemark = "Remote";
//             if (OpStatus == false) LogActivity("UserPswReset");
//             UserId = "NO_USER";

//             String body = (OpStatus == false)
//                 ? "USER PASSWORD RESET" + inputData
//                 : "USER DOES NOT EXIST" + inputData;
//             sendTextResponse(client, "HTTP/1.1 200 OK", body.c_str());
//         }

//     // ============================================================
//     //  Route: /?USERDIS — Discard User
//     // ============================================================
//     } else if (requestLine.indexOf("/?USERDIS") >= 0 && SoftWareConnected == true) {
//         int startIndex = requestLine.indexOf("/?USERDIS");
//         int endIndex   = requestLine.indexOf(" ", startIndex);
//         inputData      = requestLine.substring(startIndex, endIndex);
//         inputData.replace("%20", " ");
//         pos1 = inputData.indexOf('-');
//         pos2 = inputData.indexOf('-', pos1 + 1);

//         if (pos1 != -1 && pos2 != -1) {
//             strGUser = inputData.substring(pos1 + 1, pos2);
//             Str2 = strGUser;
//             CheckUser();
//             WriteTopway(0x00, 0x76, 0);
//             WriteTopway(0x00, 0x74, 0);
//             if (OpStatus == false) {
//                 strParam = Str2;
//                 Discard_User();
//             }
//             UserId = "RemoteUser";
//             AuditDetails = strGUser;
//             AuditRemark = "Remote";
//             if (OpStatus == false) LogActivity("UserDiscarded");
//             UserId = "NO_USER";

//             String body = (OpStatus == false)
//                 ? "USER DISCARDED" + inputData
//                 : "USER DOES NOT EXIST" + inputData;
//             sendTextResponse(client, "HTTP/1.1 200 OK", body.c_str());
//         }

//     // ============================================================
//     //  Route: /?GRPADD — Add Group
//     // ============================================================
//     } else if (requestLine.indexOf("/?GRPADD") >= 0 && SoftWareConnected == true) {
//         int startIndex = requestLine.indexOf("/?GRPADD");
//         int endIndex   = requestLine.indexOf(" ", startIndex);
//         inputData      = requestLine.substring(startIndex, endIndex);
//         inputData.replace("%20", " ");
//         pos1  = inputData.indexOf('-');
//         pos2  = inputData.indexOf('-', pos1  + 1);
//         pos3  = inputData.indexOf('-', pos2  + 1);
//         pos4  = inputData.indexOf('-', pos3  + 1);
//         pos5  = inputData.indexOf('-', pos4  + 1);
//         pos6  = inputData.indexOf('-', pos5  + 1);
//         pos7  = inputData.indexOf('-', pos6  + 1);
//         pos8  = inputData.indexOf('-', pos7  + 1);
//         pos9  = inputData.indexOf('-', pos8  + 1);
//         pos10 = inputData.indexOf('-', pos9  + 1);
//         pos11 = inputData.indexOf('-', pos10 + 1);
//         pos12 = inputData.indexOf('-', pos11 + 1);
//         pos13 = inputData.indexOf('-', pos12 + 1);
//         pos14 = inputData.indexOf('-', pos13 + 1);
//         pos15 = inputData.indexOf('-', pos14 + 1);
//         pos16 = inputData.indexOf('-', pos15 + 1);
//         pos17 = inputData.indexOf('-', pos16 + 1);
//         pos18 = inputData.indexOf('-', pos17 + 1);
//         pos19 = inputData.indexOf('-', pos18 + 1);
//         pos20 = inputData.indexOf('-', pos19 + 1);
//         pos21 = inputData.indexOf('-', pos20 + 1);

//         if (pos21 != -1) {
//             strGroup             = inputData.substring(pos1  + 1, pos2);
//             GroupMinPassLength   = inputData.substring(pos2  + 1, pos3).toInt();
//             GroupPasswordExpiy   = inputData.substring(pos3  + 1, pos4).toInt();
//             GroupRetryLimits     = inputData.substring(pos4  + 1, pos5).toInt();
//             PasswordComplexity   = inputData.substring(pos5  + 1, pos6);
//             LastPAsswordUnique   = inputData.substring(pos6  + 1, pos7).toInt();
//             ResetPassAuth        = inputData.substring(pos7  + 1, pos8);
//             FixedVolume          = inputData.substring(pos8  + 1, pos9);
//             Logout_Timer1        = inputData.substring(pos9  + 1, pos10).toInt();
//             Power_Off_Timer1     = inputData.substring(pos10 + 1, pos11).toInt();
//             Software_Access      = inputData.substring(pos11 + 1, pos12);
//             Device_Monitoring    = inputData.substring(pos12 + 1, pos13);
//             Group_User_Setting   = inputData.substring(pos13 + 1, pos14);
//             Config_Setting       = inputData.substring(pos14 + 1, pos15);
//             Sample_Data_View     = inputData.substring(pos15 + 1, pos16);
//             Audit_Trail_View     = inputData.substring(pos16 + 1, pos17);
//             Print_Config         = inputData.substring(pos17 + 1, pos18);
//             Print_Sample_Data    = inputData.substring(pos18 + 1, pos19);
//             Print_Audit_Trail    = inputData.substring(pos19 + 1, pos20);
//             Allow_Sampling       = inputData.substring(pos20 + 1, pos21);
//             Str2 = strGroup;
//             CheckGroup();
//             WriteTopway(0x00, 0x76, 0);
//             WriteTopway(0x00, 0x74, 0);
//             if (OpStatus == true) AddGroup1();
//             UserId = "RemoteUser";
//             AuditDetails = strGroup;
//             AuditRemark = "Remote";
//             if (OpStatus == true) LogActivity("GroupAdded");
//             UserId = "NO_USER";

//             String body = (OpStatus == true)
//                 ? "GROUP ADDED" + strGroup
//                 : "GROUP ALREADY EXIST" + strGroup;
//             sendTextResponse(client, "HTTP/1.1 200 OK", body.c_str());
//         }

//     // ============================================================
//     //  Route: /?GRPDIS — Discard Group
//     // ============================================================
//     } else if (requestLine.indexOf("/?GRPDIS") >= 0 && SoftWareConnected == true) {
//         int startIndex = requestLine.indexOf("/?GRPDIS");
//         int endIndex   = requestLine.indexOf(" ", startIndex);
//         inputData      = requestLine.substring(startIndex, endIndex);
//         inputData.replace("%20", " ");
//         pos1 = inputData.indexOf('-');
//         pos2 = inputData.indexOf('-', pos1 + 1);

//         if (pos1 != -1 && pos2 != -1) {
//             Str2 = inputData.substring(pos1 + 1, pos2);
//             CheckGroup();
//             if (OpStatus == false) {
//                 strParam = Str2;
//                 Update_Group();
//             }
//             WriteTopway(0x00, 0x76, 0);
//             WriteTopway(0x00, 0x74, 0);
//             UserId = "RemoteUser";
//             AuditDetails = Str2;
//             AuditRemark = "Remote";
//             if (OpStatus == false) LogActivity("GroupDiscarded");
//             UserId = "NO_USER";

//             String body = (OpStatus == false)
//                 ? "GROUP DISCARDED" + inputData
//                 : "GROUP DOES NOT EXIST" + inputData;
//             sendTextResponse(client, "HTTP/1.1 200 OK", body.c_str());
//         }

//     // For ntp 
// //    }
// //    else if (requestLine.indexOf("/?NTPSync") >= 0) {
// //  SyncTimeFromNTP();
// //  client.println("HTTP/1.1 200 OK");
// //  client.println("Content-Type: text/plain");
// //  client.println("Access-Control-Allow-Origin: *");
// //  client.println("Connection: close");
// //  client.println();
// //  if (ntpSynced) {
// //    now1 = rtc.now();
// //    char buf[32];
// //    snprintf(buf, sizeof(buf), "SYNCED %02d/%02d/%04d %02d:%02d:%02d",
// //      now1.day(), now1.month(), now1.year(),
// //      now1.hour(), now1.minute(), now1.second());
// //    client.println(buf);
// //  } else {
// //    client.println("NTP_SYNC_FAILED");
// //  }

//     // ============================================================
//     //  Route: 404 — Unknown request
//     // ============================================================
//     } else {
//         String body = "Error Decoding...Request: " + requestLine;
//         sendTextResponse(client, "HTTP/1.1 404 Not Found", body.c_str());
//     }

//     // ✅ No client.flush() here — it causes ~200ms stall
//     //    client.write() already pushes data
//     client.stop();
// }
