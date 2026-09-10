void handleRequest() {
  WiFiClient client = wifiServer.available();
  if (!client) return;

//  Serial.println("Client connected.");
  unsigned long timeout = millis();
  String request = "";
  
  // MEMORY PROTECTION: Reserve space to prevent fragmentation
  request.reserve(512);

  // Wait up to 1 second for data from client
  while (client.connected() && millis() - timeout < 1000) {
    if (client.available()) {
      char c = client.read();
      request += c;
      if (c == '\n' && request.endsWith("\r\n\r\n")) {
        break;  // End of headers
      }
    }
  }

  request.trim();

  if (request.indexOf("GET /file") >= 0) {
    const char* boundary = "MyBoundary123";
    const char* files[] = {"/FR.csv", "/DevInf.csv"};
    const int fileCount = 2;

    client.println("HTTP/1.1 200 OK");
    client.print("Content-Type: multipart/mixed; boundary=");
    client.println(boundary);
    client.println("Connection: close");
    client.println();

    for (int i = 0; i < fileCount; i++) {
      // Safety check
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

        // OPTIMIZATION: Use buffer instead of byte-by-byte for stability
        uint8_t buf[128];
        while (file.available()) {
          int n = file.read(buf, sizeof(buf));
          if (n > 0) {
            client.write(buf, n);
          }
        }
        client.println();  // Separate parts
        file.close();
      }
    }

    client.print("--");
    client.print(boundary);
    client.println("--");
  } else if (request.indexOf("GET /AUDIT") >= 0) {
    const char* boundary = "MyBoundary123";
    const char* files[] = {"/ACTLOG.csv", "/DevInf.csv"};
    const int fileCount = 2;

    client.println("HTTP/1.1 200 OK");
    client.print("Content-Type: multipart/mixed; boundary=");
    client.println(boundary);
    client.println("Connection: close");
    client.println();

    for (int i = 0; i < fileCount; i++) {
      // Safety check
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

        // OPTIMIZATION: Use buffer instead of byte-by-byte
        uint8_t buf[128];
        while (file.available()) {
          int n = file.read(buf, sizeof(buf));
          if (n > 0) {
            client.write(buf, n);
          }
        }
        client.println();  // Separate parts
        file.close();
      }
    }

    client.print("--");
    client.print(boundary);
    client.println("--");
  } else if (request.indexOf("GET /LOCADD") >= 0) {
    int startIndex = request.indexOf("/LOCADD") + 4;
    int endIndex = request.indexOf(" ", startIndex);
    inputData = request.substring(startIndex, endIndex);
    inputData.replace("%20", " ");
    pos1 = inputData.indexOf('-');
    Str2 = inputData.substring(pos1+1);
    CheckLocation("/LOC.csv");WriteTopway(0x00, 0x76, 0);WriteTopway(0x00, 0x74, 0);
    if(OpStatus==true) {ParamId=LocationId;strParam=Str2;WriteConfigParam(1,"/LOC.csv");WriteTopway(0x00,0x76,0);}

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    if(OpStatus==true) {client.println("LOCATION ADDED");} else {client.println("LOCATION ALREADY EXIST");}
    client.println(inputData);
  } else if (request.indexOf("GET /LOCDIS") >= 0) {
    int startIndex = request.indexOf("/LOCDIS") + 4;
    int endIndex = request.indexOf(" ", startIndex);
    inputData = request.substring(startIndex, endIndex);
    inputData.replace("%20", " ");
    pos1 = inputData.indexOf('-');
    Str2 = inputData.substring(pos1+1);
    CheckLocation("/LOC.csv");WriteTopway(0x00, 0x76, 0);WriteTopway(0x00, 0x74, 0);
    if(OpStatus==true) {ParamId=LocationId;strParam=Str2;Discard_Location(3,"/LOC.csv");WriteTopway(0x00,0x74,0);}

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
//    if(OpStatus==true) {client.println("LOCATION DISCARDED");} else {client.println("LOCATION DOES NOT EXIST");}
    client.println(inputData);
  } else {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println(inputData);
  }

  delay(1);            // Ensure data is sent
  client.flush();      // Clear buffer
  client.stop();       // Close connection
//  Serial.println("Client disconnected.");
}
