void InitSyncPrimitives() {
  if(sdMutex == NULL) {
    sdMutex = xSemaphoreCreateMutex();
  }

  if(syncEvent == NULL) {
    syncEvent = xSemaphoreCreateBinary();
  }
}

bool SdLock(uint32_t timeoutMs) {
  if(sdMutex == NULL) {
    return true;
  }
  return xSemaphoreTake(sdMutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void SdUnlock() {
  if(sdMutex != NULL) {
    xSemaphoreGive(sdMutex);
  }
}

void NotifySyncEvent() {
  syncWakePending = true;
  if(IsSyncPausedForSampling()) {
    return;
  }
  if(syncEvent != NULL) {
    xSemaphoreGive(syncEvent);
  }
}

void MarkSyncSdQuiet(uint32_t quietMs) {
  syncSdQuietUntilMs = millis() + quietMs;
}

bool IsSyncPausedForSampling() {
  if(Motor_Status1 == true) {
    return true;
  }

  return ((long)(millis() - syncSdQuietUntilMs) < 0);
}

void StartSyncManager() {
  if(syncManagerStarted == true) {
    return;
  }

  InitSyncPrimitives();

  if(syncEvent != NULL) {
    xSemaphoreGive(syncEvent); // first wake after boot
  }

  LoadSyncState();
  SaveSyncState("BOOT");

  BaseType_t created = xTaskCreatePinnedToCore(
    SyncManagerTask,
    "SyncManager",
    SYNC_TASK_STACK_WORDS,
    NULL,
    SYNC_TASK_PRIORITY,
    &syncTaskHandle,
    SYNC_TASK_CORE
  );

  if(created == pdPASS) {
    syncManagerStarted = true;
  } else {
    syncTaskHandle = NULL;
    syncManagerStarted = false;
  }
}

void SyncManagerTask(void *parameter) {
  (void)parameter;

  for(;;) {
    if(syncEvent != NULL) {
      xSemaphoreTake(syncEvent, pdMS_TO_TICKS(SYNC_PERIODIC_WAKE_MS));
    } else {
      vTaskDelay(pdMS_TO_TICKS(SYNC_PERIODIC_WAKE_MS));
    }

    syncWakePending = false;
    syncLastWakeMs = millis();

    SyncManagerTick();
  }
}

void SyncManagerTick() {
  if(IsSyncPausedForSampling()) {
    syncState = SYNC_IDLE;
    return;
  }

  syncState = SYNC_CHECK_NETWORK;

  if(WiFi.status() != WL_CONNECTED) {
    syncState = SYNC_RETRY_BACKOFF;
    syncFailureCount++;
    SaveSyncState("WIFI_NOT_CONNECTED");
    vTaskDelay(pdMS_TO_TICKS(SYNC_RETRY_BACKOFF_MS));
    syncState = SYNC_IDLE;
    return;
  }

  syncLastAttemptMs = millis();
  SaveSyncState("OK");

  syncState = SYNC_LOAD_SERVER_CONFIG;
  if(!LoadSyncServerConfig()) {
    syncLastHealthOk = false;
    syncFailureCount++;
    SaveSyncState("SERVER_CONFIG_MISSING");
    AppendSyncError("SERVER_CONFIG_MISSING", "SERVCRED.csv missing or invalid");
    syncState = SYNC_IDLE;
    SaveSyncState("SERVER_CONFIG_MISSING");
    return;
  }
  SaveSyncState("OK");

  syncState = SYNC_HEALTH_CHECK;
  if(!SyncHealthCheck()) {
    syncLastHealthOk = false;
    syncFailureCount++;
    SaveSyncState("SERVER_HEALTH_FAILED");
    syncState = SYNC_RETRY_BACKOFF;
    vTaskDelay(pdMS_TO_TICKS(SYNC_RETRY_BACKOFF_MS));
    syncState = SYNC_IDLE;
    SaveSyncState("SERVER_HEALTH_FAILED");
    return;
  }
  syncLastHealthOk = true;
  syncLastSuccessMs = millis();
  SaveSyncState("OK");

  // Discover and upload only fresh rows. Existing CSV formats are preserved;
  // pending state and per-file cursors stay under /SYNC/.
  syncState = SYNC_LOAD_STATE;
  vTaskDelay(pdMS_TO_TICKS(10));
  SaveSyncState("OK");

  syncState = SYNC_FIND_PENDING;
  syncPendingDiscoveredLastRun = DiscoverPendingRecords();
  SaveSyncState("OK");

  syncState = SYNC_UPLOAD_PENDING;
  UploadPendingRecords(SYNC_UPLOAD_BATCH_LIMIT);
  SaveSyncState("OK");

  syncState = SYNC_IDLE;
  SaveSyncState("OK");
}

const char* SyncStateName(SyncState state) {
  switch(state) {
    case SYNC_IDLE: return "SYNC_IDLE";
    case SYNC_CHECK_NETWORK: return "SYNC_CHECK_NETWORK";
    case SYNC_LOAD_SERVER_CONFIG: return "SYNC_LOAD_SERVER_CONFIG";
    case SYNC_HEALTH_CHECK: return "SYNC_HEALTH_CHECK";
    case SYNC_LOAD_STATE: return "SYNC_LOAD_STATE";
    case SYNC_FIND_PENDING: return "SYNC_FIND_PENDING";
    case SYNC_UPLOAD_PENDING: return "SYNC_UPLOAD_PENDING";
    case SYNC_RETRY_BACKOFF: return "SYNC_RETRY_BACKOFF";
    default: return "SYNC_UNKNOWN";
  }
}

void EnsureSyncStorage() {
  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    if(!SD.exists(SYNC_DIR)) {
      SD.mkdir(SYNC_DIR);
    }
    SdUnlock();
  }

  EnsureSyncCsvFile(SYNC_STATE_FILE, SYNC_STATE_HEADER);
  EnsureSyncCsvFile(SYNC_PENDING_FILE, SYNC_PENDING_HEADER);
  EnsureSyncCsvFile(SYNC_INFLIGHT_FILE, SYNC_INFLIGHT_HEADER);
  EnsureSyncCsvFile(SYNC_ERRORS_FILE, SYNC_ERRORS_HEADER);
  EnsureSyncCsvFile(SYNC_CURSOR_FILE, SYNC_CURSOR_HEADER);
}

bool EnsureSyncCsvFile(const char* path, const char* header) {
  bool ok = false;

  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    bool createFile = !SD.exists(path);

    if(createFile) {
      File f = SD.open(path, O_WRITE | O_CREAT | O_TRUNC);
      if(f) {
        f.println(header);
        f.flush();
        f.close();
        ok = true;
      }
    } else {
      ok = true;
    }

    SdUnlock();
  }

  return ok;
}

void LoadSyncState() {
  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    File f = SD.open(SYNC_STATE_FILE, FILE_READ);
    if(f) {
      f.readStringUntil('\n');
      String row = f.readStringUntil('\n');
      row.trim();
      f.close();

      if(row.length() > 0) {
        int p1 = row.indexOf(',');
        int p2 = row.indexOf(',', p1 + 1);
        int p3 = row.indexOf(',', p2 + 1);
        int p4 = row.indexOf(',', p3 + 1);
        int p5 = row.indexOf(',', p4 + 1);

        if(p1 != -1 && p2 != -1 && p3 != -1 && p4 != -1 && p5 != -1) {
          syncLastSuccessMs = row.substring(0, p1).toInt();
          syncLastAttemptMs = row.substring(p1 + 1, p2).toInt();
          syncFailureCount = row.substring(p4 + 1, p5).toInt();
        }
      }
    }
    SdUnlock();
  }
}

void SaveSyncState(const char* lastError) {
  const char* tmpPath = "/SYNC/state.tmp";

  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    if(!SD.exists(SYNC_DIR)) {
      SD.mkdir(SYNC_DIR);
    }
    if(SD.exists(tmpPath)) {
      SD.remove(tmpPath);
    }

    File f = SD.open(tmpPath, O_WRITE | O_CREAT | O_TRUNC);
    if(f) {
      f.println(SYNC_STATE_HEADER);
      f.print(syncLastSuccessMs); f.print(",");
      f.print(syncLastAttemptMs); f.print(",");
      f.print(SyncStateName(syncState)); f.print(",");
      f.print(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED"); f.print(",");
      f.print(syncFailureCount); f.print(",");
      f.println(lastError);
      f.flush();
      f.close();

      bool renamed = false;
      if(SD.exists(SYNC_STATE_FILE)) {
        SD.remove(SYNC_STATE_FILE);
      }
      renamed = SD.rename(tmpPath, SYNC_STATE_FILE);

      if(!renamed) {
        File direct = SD.open(SYNC_STATE_FILE, O_WRITE | O_CREAT | O_TRUNC);
        if(direct) {
          direct.println(SYNC_STATE_HEADER);
          direct.print(syncLastSuccessMs); direct.print(",");
          direct.print(syncLastAttemptMs); direct.print(",");
          direct.print(SyncStateName(syncState)); direct.print(",");
          direct.print(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED"); direct.print(",");
          direct.print(syncFailureCount); direct.print(",");
          direct.println(lastError);
          direct.flush();
          direct.close();
        }

        if(SD.exists(tmpPath)) {
          SD.remove(tmpPath);
        }
      }
    }

    SdUnlock();
  }
}

void AppendSyncError(const char* errorCode, const char* message) {
  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    File f = SD.open(SYNC_ERRORS_FILE, O_WRITE | O_CREAT | O_APPEND);
    if(f) {
      f.print(millis()); f.print(",");
      f.print(SyncStateName(syncState)); f.print(",");
      f.print(errorCode); f.print(",");
      f.println(message);
      f.flush();
      f.close();
    }
    SdUnlock();
  }
}

bool LoadSyncServerConfig() {
  bool loaded = false;

  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    File f = SD.open("/SERVCRED.csv", FILE_READ);
    if(f) {
      String row = f.readStringUntil('\n');
      row.trim();
      f.close();

      int commaPos = row.indexOf(',');
      if(commaPos > 0) {
        ServerIP = row.substring(0, commaPos);
        SeverPort = row.substring(commaPos + 1);
        ServerIP.trim();
        SeverPort.trim();

        if(ServerIP.length() > 0 && SeverPort.toInt() > 0) {
          loaded = true;
        }
      }
    }
    SdUnlock();
  }

  return loaded;
}

bool SyncHealthCheck() {
  if(ServerIP.length() == 0 || SeverPort.toInt() <= 0) {
    AppendSyncError("SERVER_CONFIG_INVALID", "Server IP or port is empty");
    return false;
  }

  WiFiClient client;
  client.setTimeout(SYNC_HTTP_RESPONSE_TIMEOUT_MS);

  uint16_t port = (uint16_t)SeverPort.toInt();
  if(!client.connect(ServerIP.c_str(), port, SYNC_TCP_CONNECT_TIMEOUT_MS)) {
    AppendSyncError("SERVER_CONNECT_FAILED", "Health TCP connect failed");
    client.stop();
    return false;
  }

  String request = "GET /api/air-sampler/v1/health?device_id=";
  request += deviceID;
  request += " HTTP/1.1\r\nHost: ";
  request += ServerIP;
  request += "\r\nX-Device-Id: ";
  request += deviceID;
  request += "\r\nX-Firmware-Version: ";
  request += SoftwareVer;
  request += "\r\nConnection: close\r\n\r\n";

  client.print(request);

  unsigned long startMs = millis();
  String statusLine = "";
  statusLine.reserve(64);

  while((client.connected() || client.available()) && (millis() - startMs < SYNC_HTTP_RESPONSE_TIMEOUT_MS)) {
    while(client.available()) {
      char c = client.read();
      if(c == '\n') {
        statusLine.trim();
        bool ok = statusLine.indexOf("200") > 0;
        client.stop();
        if(!ok) {
          AppendSyncError("SERVER_HEALTH_NOT_OK", statusLine.c_str());
        }
        return ok;
      }
      if(statusLine.length() < 63 && c != '\r') {
        statusLine += c;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  client.stop();
  AppendSyncError("SERVER_HEALTH_TIMEOUT", "No health response status line");
  return false;
}

String BuildSyncRecordId(const char* recordType, unsigned long recordNumber) {
  char numberPart[12];
  snprintf(numberPart, sizeof(numberPart), "%08lu", recordNumber);

  String recordId = deviceID;
  recordId += "-";
  recordId += recordType;
  recordId += "-";
  recordId += numberPart;
  return recordId;
}

unsigned long GetLastPendingRecordNumber(const char* recordType) {
  unsigned long lastRecordNumber = 0;

  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    File f = SD.open(SYNC_PENDING_FILE, FILE_READ);
    if(f) {
      f.readStringUntil('\n'); // header

      while(f.available()) {
        String row = f.readStringUntil('\n');
        row.trim();
        if(row.length() == 0) continue;

        int p1 = row.indexOf(',');
        int p2 = row.indexOf(',', p1 + 1);
        if(p1 == -1 || p2 == -1) continue;

        String rowType = row.substring(0, p1);
        if(rowType != recordType) continue;

        String recordId = row.substring(p1 + 1, p2);
        int lastDash = recordId.lastIndexOf('-');
        if(lastDash == -1) continue;

        unsigned long recordNumber = recordId.substring(lastDash + 1).toInt();
        if(recordNumber > lastRecordNumber) {
          lastRecordNumber = recordNumber;
        }
      }

      f.close();
    }
    SdUnlock();
  }

  return lastRecordNumber;
}

unsigned long DiscoverPendingRecords() {
  EnsureSyncCsvFile(SYNC_PENDING_FILE, SYNC_PENDING_HEADER);
  EnsureSyncCsvFile(SYNC_CURSOR_FILE, SYNC_CURSOR_HEADER);

  unsigned long cursorRecord = 0;
  unsigned long cursorLine = 0;
  bool sampleCursorKnown = GetSyncCursor("SAMPLE", &cursorRecord, &cursorLine);
  bool auditCursorKnown = GetSyncCursor("AUDIT", &cursorRecord, &cursorLine);

  if(!sampleCursorKnown || !auditCursorKnown) {
    ResetPendingQueue();
  }

  unsigned long discovered = 0;
  discovered += DiscoverPendingFromFile("SAMPLE", "/FR.csv", true);
  discovered += DiscoverPendingFromFile("AUDIT", "/ActLog.csv", false);

  // Append-only operational CSV files. Credential and internal sync files are
  // intentionally not listed here and are never sent to the server.
  const char* extraSources[][2] = {
    {"DLS",  "/DLS.csv"},
    {"BAT",  "/BatParam.csv"},
    {"CAL",  "/CAL.csv"},
    {"GROUP", "/GRP.csv"},
    {"LOCATION", "/LOC.csv"},
    {"REMARK", "/RMK.csv"},
    {"RECIPE", "/RECP.csv"},
    {"DEVICE", "/DevInf.csv"},
    {"COMPONENT", "/CompDet.csv"}
  };

  const uint8_t extraSourceCount = sizeof(extraSources) / sizeof(extraSources[0]);
  for(uint8_t i = 0; i < extraSourceCount; i++) {
    discovered += DiscoverPendingFromLineFile(extraSources[i][0], extraSources[i][1]);
  }

  return discovered;
}

unsigned long DiscoverPendingFromLineFile(const char* recordType, const char* sourceFile) {
  unsigned long discovered = 0;
  unsigned long lastKnownRecord = 0;
  unsigned long lastKnownLine = 0;
  unsigned long sourceLine = 0;
  unsigned long maxSourceLine = 0;

  if(recordType == NULL || sourceFile == NULL) {
    return 0;
  }

  // First sight of a file creates a baseline. Existing rows are not uploaded;
  // only rows appended after this cursor are considered fresh.
  if(!GetSyncCursor(recordType, &lastKnownRecord, &lastKnownLine)) {
    if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
      File source = SD.open(sourceFile, FILE_READ);
      if(source) {
        while(source.available()) {
          String row = source.readStringUntil('\n');
          sourceLine++;
          row.trim();
          if(row.length() > 0) {
            maxSourceLine = sourceLine;
          }
        }
        source.close();
      }
      SdUnlock();
    }
    SaveSyncCursor(recordType, sourceFile, maxSourceLine, maxSourceLine);
    return 0;
  }

  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    File source = SD.open(sourceFile, FILE_READ);
    File pending = SD.open(SYNC_PENDING_FILE, O_WRITE | O_CREAT | O_APPEND);

    if(source && pending) {
      while(source.available()) {
        String row = source.readStringUntil('\n');
        sourceLine++;
        row.trim();
        if(row.length() == 0) {
          continue;
        }

        maxSourceLine = sourceLine;
        if(sourceLine <= lastKnownLine) {
          continue;
        }

        String recordId = BuildSyncRecordId(recordType, sourceLine);
        pending.print(recordType); pending.print(",");
        pending.print(recordId); pending.print(",");
        pending.print(sourceFile); pending.print(",");
        pending.print(sourceLine); pending.print(",PENDING,0,0\n");
        discovered++;
      }
      pending.flush();
    }

    if(source) source.close();
    if(pending) pending.close();
    SdUnlock();
  }

  if(maxSourceLine > lastKnownLine) {
    SaveSyncCursor(recordType, sourceFile, maxSourceLine, maxSourceLine);
  }

  return discovered;
}

unsigned long DiscoverPendingFromFile(const char* recordType, const char* sourceFile, bool sampleFile) {
  unsigned long discovered = 0;
  unsigned long lastKnownRecord = 0;
  unsigned long lastKnownLine = 0;
  unsigned long sourceLine = 0;
  unsigned long maxRecordNumber = 0;
  unsigned long maxSourceLine = 0;

  if(!GetSyncCursor(recordType, &lastKnownRecord, &lastKnownLine)) {
    ScanSourceLatest(sourceFile, sampleFile, &maxRecordNumber, &maxSourceLine);
    SaveSyncCursor(recordType, sourceFile, maxRecordNumber, maxSourceLine);
    return 0;
  }

  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    if(!SD.exists(sourceFile)) {
      SdUnlock();
      return 0;
    }

    File source = SD.open(sourceFile, FILE_READ);
    File pending = SD.open(SYNC_PENDING_FILE, O_WRITE | O_CREAT | O_APPEND);

    if(source && pending) {
      while(source.available()) {
        String row = source.readStringUntil('\n');
        sourceLine++;
        row.trim();
        if(row.length() == 0) continue;

        int firstComma = row.indexOf(',');
        if(firstComma == -1) continue;

        unsigned long recordNumber = 0;
        if(sampleFile) {
          int slashPos = row.indexOf('/');
          if(slashPos == -1 || slashPos > firstComma) continue;
          recordNumber = row.substring(0, slashPos).toInt();
        } else {
          recordNumber = row.substring(0, firstComma).toInt();
        }

        if(recordNumber > maxRecordNumber) {
          maxRecordNumber = recordNumber;
          maxSourceLine = sourceLine;
        }

        if(recordNumber == 0 || recordNumber <= lastKnownRecord) continue;

        String recordId = BuildSyncRecordId(recordType, recordNumber);
        pending.print(recordType); pending.print(",");
        pending.print(recordId); pending.print(",");
        pending.print(sourceFile); pending.print(",");
        pending.print(sourceLine); pending.print(",");
        pending.print("PENDING"); pending.print(",");
        pending.print(0); pending.print(",");
        pending.println(0);

        discovered++;
      }

      pending.flush();
    }

    if(source) source.close();
    if(pending) pending.close();
    SdUnlock();
  }

  if(maxRecordNumber > lastKnownRecord) {
    SaveSyncCursor(recordType, sourceFile, maxRecordNumber, maxSourceLine);
  }

  return discovered;
}

void ResetPendingQueue() {
  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    File f = SD.open(SYNC_PENDING_FILE, O_WRITE | O_CREAT | O_TRUNC);
    if(f) {
      f.println(SYNC_PENDING_HEADER);
      f.flush();
      f.close();
    }
    SdUnlock();
  }
}

bool GetSyncCursor(const char* recordType, unsigned long *lastRecordNumber, unsigned long *lastSourceLine) {
  bool found = false;

  if(lastRecordNumber != NULL) {
    *lastRecordNumber = 0;
  }
  if(lastSourceLine != NULL) {
    *lastSourceLine = 0;
  }

  if(recordType == NULL) {
    return false;
  }

  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    File f = SD.open(SYNC_CURSOR_FILE, FILE_READ);
    if(f) {
      f.readStringUntil('\n'); // header

      while(f.available()) {
        String row = f.readStringUntil('\n');
        row.trim();
        if(row.length() == 0) continue;

        int p1 = row.indexOf(',');
        int p2 = row.indexOf(',', p1 + 1);
        int p3 = row.indexOf(',', p2 + 1);
        if(p1 == -1 || p2 == -1 || p3 == -1) continue;

        String rowType = row.substring(0, p1);
        rowType.trim();
        if(rowType != recordType) continue;

        if(lastRecordNumber != NULL) {
          *lastRecordNumber = row.substring(p2 + 1, p3).toInt();
        }
        if(lastSourceLine != NULL) {
          *lastSourceLine = row.substring(p3 + 1).toInt();
        }
        found = true;
        break;
      }
      f.close();
    }
    SdUnlock();
  }

  return found;
}

void SaveSyncCursor(const char* recordType, const char* sourceFile, unsigned long lastRecordNumber, unsigned long lastSourceLine) {
  const char* tmpPath = "/SYNC/cursors.tmp";
  bool rowUpdated = false;
  bool targetWritten = false;

  if(recordType == NULL || sourceFile == NULL) {
    return;
  }

  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    File source = SD.open(SYNC_CURSOR_FILE, FILE_READ);
    File target = SD.open(tmpPath, O_WRITE | O_CREAT | O_TRUNC);

    if(target) {
      target.println(SYNC_CURSOR_HEADER);

      if(source) {
        source.readStringUntil('\n'); // header
        while(source.available()) {
          String row = source.readStringUntil('\n');
          String originalRow = row;
          row.trim();
          if(row.length() == 0) continue;

          int p1 = row.indexOf(',');
          if(p1 == -1) continue;

          String rowType = row.substring(0, p1);
          rowType.trim();
          if(rowType == recordType) {
            target.print(recordType); target.print(",");
            target.print(sourceFile); target.print(",");
            target.print(lastRecordNumber); target.print(",");
            target.println(lastSourceLine);
            rowUpdated = true;
          } else {
            target.println(originalRow);
          }
        }
      }

      if(!rowUpdated) {
        target.print(recordType); target.print(",");
        target.print(sourceFile); target.print(",");
        target.print(lastRecordNumber); target.print(",");
        target.println(lastSourceLine);
      }

      target.flush();
      target.close();
      targetWritten = true;
    }

    if(source) source.close();

    if(targetWritten) {
      if(SD.exists(SYNC_CURSOR_FILE)) {
        SD.remove(SYNC_CURSOR_FILE);
      }
      SD.rename(tmpPath, SYNC_CURSOR_FILE);
    } else if(SD.exists(tmpPath)) {
      SD.remove(tmpPath);
    }

    SdUnlock();
  }
}

bool ScanSourceLatest(const char* sourceFile, bool sampleFile, unsigned long *lastRecordNumber, unsigned long *lastSourceLine) {
  bool scanned = false;
  unsigned long sourceLine = 0;

  if(lastRecordNumber != NULL) {
    *lastRecordNumber = 0;
  }
  if(lastSourceLine != NULL) {
    *lastSourceLine = 0;
  }

  if(sourceFile == NULL) {
    return false;
  }

  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    File source = SD.open(sourceFile, FILE_READ);
    if(source) {
      while(source.available()) {
        String row = source.readStringUntil('\n');
        sourceLine++;
        row.trim();
        if(row.length() == 0) continue;

        int firstComma = row.indexOf(',');
        if(firstComma == -1) continue;

        unsigned long recordNumber = 0;
        if(sampleFile) {
          int slashPos = row.indexOf('/');
          if(slashPos == -1 || slashPos > firstComma) continue;
          recordNumber = row.substring(0, slashPos).toInt();
        } else {
          recordNumber = row.substring(0, firstComma).toInt();
        }

        if(recordNumber > 0) {
          if(lastRecordNumber != NULL) {
            *lastRecordNumber = recordNumber;
          }
          if(lastSourceLine != NULL) {
            *lastSourceLine = sourceLine;
          }
          scanned = true;
        }
      }
      source.close();
    }
    SdUnlock();
  }

  return scanned;
}

String JsonEscape(String value) {
  String escaped = "";
  escaped.reserve(value.length() + 8);

  for(unsigned int i = 0; i < value.length(); i++) {
    char c = value.charAt(i);
    if(c == '"') {
      escaped += "\\\"";
    } else if(c == '\\') {
      escaped += "\\\\";
    } else if(c == '\n') {
      escaped += "\\n";
    } else if(c == '\r') {
      escaped += "\\r";
    } else if(c == '\t') {
      escaped += "\\t";
    } else {
      escaped += c;
    }
  }

  return escaped;
}

bool ParsePendingCsvRow(String row, SyncPendingRecord *record) {
  row.trim();
  if(row.length() == 0 || record == NULL) {
    return false;
  }

  int p1 = row.indexOf(',');
  int p2 = row.indexOf(',', p1 + 1);
  int p3 = row.indexOf(',', p2 + 1);
  int p4 = row.indexOf(',', p3 + 1);
  int p5 = row.indexOf(',', p4 + 1);
  int p6 = row.indexOf(',', p5 + 1);

  if(p1 == -1 || p2 == -1 || p3 == -1 || p4 == -1 || p5 == -1 || p6 == -1) {
    return false;
  }

  record->recordType = row.substring(0, p1);
  record->recordId = row.substring(p1 + 1, p2);
  record->sourceFile = row.substring(p2 + 1, p3);
  record->sourceLine = row.substring(p3 + 1, p4).toInt();
  record->status = row.substring(p4 + 1, p5);
  record->attempts = row.substring(p5 + 1, p6).toInt();
  record->lastAttemptMs = row.substring(p6 + 1).toInt();
  record->csvRow = "";

  record->recordType.trim();
  record->recordId.trim();
  record->sourceFile.trim();
  record->status.trim();

  return (record->recordType.length() > 0 &&
          record->recordId.length() > 0 &&
          record->sourceFile.length() > 0 &&
          record->sourceLine > 0);
}

bool ReadCsvLineAt(const char* sourceFile, unsigned long targetLine, String *outRow) {
  bool found = false;
  unsigned long currentLine = 0;

  if(outRow == NULL || sourceFile == NULL || targetLine == 0) {
    return false;
  }

  *outRow = "";

  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    File source = SD.open(sourceFile, FILE_READ);
    if(source) {
      while(source.available()) {
        String row = source.readStringUntil('\n');
        currentLine++;
        if(currentLine == targetLine) {
          row.trim();
          *outRow = row;
          found = (row.length() > 0);
          break;
        }
      }
      source.close();
    }
    SdUnlock();
  }

  return found;
}

bool LoadNextPendingRecord(SyncPendingRecord *record) {
  if(record == NULL) {
    return false;
  }

  bool loaded = false;

  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    File pending = SD.open(SYNC_PENDING_FILE, FILE_READ);
    if(pending) {
      pending.readStringUntil('\n'); // header

      while(pending.available()) {
        String row = pending.readStringUntil('\n');
        SyncPendingRecord candidate;
        if(!ParsePendingCsvRow(row, &candidate)) {
          continue;
        }

        if(candidate.status == "PENDING") {
          *record = candidate;
          loaded = true;
          break;
        }
      }

      pending.close();
    }
    SdUnlock();
  }

  if(!loaded) {
    return false;
  }

  if(!ReadCsvLineAt(record->sourceFile.c_str(), record->sourceLine, &record->csvRow)) {
    UpdatePendingRecordStatus(record->recordId.c_str(), "FAILED", record->attempts + 1);
    AppendSyncError("SOURCE_ROW_MISSING", record->recordId.c_str());
    return false;
  }

  return true;
}

// Preserve CSV column order as a JSON string array, including quoted commas.
String SyncJsonFields(const String& row) {
  String result = "[";
  String field = "";
  bool quoted = false;
  for(unsigned int i = 0; i < row.length(); i++) {
    char c = row[i];
    if(c == '"') {
      if(quoted && i + 1 < row.length() && row[i + 1] == '"') {
        field += '"';
        i++;
      } else {
        quoted = !quoted;
      }
    } else if(c == ',' && !quoted) {
      result += "\"" + JsonEscape(field) + "\",";
      field = "";
    } else {
      field += c;
    }
  }
  result += "\"" + JsonEscape(field) + "\"]";
  return result;
}

bool UploadPendingRecord(SyncPendingRecord *record) {
  if(record == NULL) {
    return false;
  }

  if(ServerIP.length() == 0 || SeverPort.toInt() <= 0) {
    AppendSyncError("UPLOAD_CONFIG_INVALID", "Server IP or port is empty");
    return false;
  }

  String body = "{\"serial_number\":\"";
  body += JsonEscape(deviceID);
  body += "\",\"device_id\":\"";
  body += JsonEscape(deviceID);
  body += "\",\"firmware\":\"";
  body += JsonEscape(SoftwareVer);
  body += "\",\"records\":[{\"record_type\":\"";
  body += JsonEscape(record->recordType);
  body += "\",\"record_id\":\"";
  body += JsonEscape(record->recordId);
  body += "\",\"source_file\":\"";
  body += JsonEscape(record->sourceFile);
  body += "\",\"source_line\":";
  body += String(record->sourceLine);
  body += ",\"fields\":";
  body += SyncJsonFields(record->csvRow);
  body += ",\"csv\":\"";
  body += JsonEscape(record->csvRow);
  body += "\"}]}";

  WiFiClient client;
  client.setTimeout(SYNC_HTTP_RESPONSE_TIMEOUT_MS);

  uint16_t port = (uint16_t)SeverPort.toInt();
  if(!client.connect(ServerIP.c_str(), port, SYNC_TCP_CONNECT_TIMEOUT_MS)) {
    AppendSyncError("UPLOAD_CONNECT_FAILED", record->recordId.c_str());
    client.stop();
    return false;
  }

  String request = "POST ";
  request += SYNC_RECORDS_PATH;
  request += deviceID;
  request += " HTTP/1.1\r\nHost: ";
  request += ServerIP;
  request += "\r\nX-Device-Id: ";
  request += deviceID;
  request += "\r\nX-Firmware-Version: ";
  request += SoftwareVer;
  request += "\r\nContent-Type: application/json\r\nContent-Length: ";
  request += String(body.length());
  request += "\r\nConnection: close\r\n\r\n";
  request += body;

  client.print(request);

  unsigned long startMs = millis();
  String statusLine = "";
  statusLine.reserve(64);
  bool firstLine = true;
  bool httpOk = false;
  bool recordAck = false;

  while((client.connected() || client.available()) && (millis() - startMs < SYNC_HTTP_RESPONSE_TIMEOUT_MS)) {
    while(client.available()) {
      char c = client.read();
      if(c == '\n') {
        statusLine.trim();
        if(firstLine) {
          httpOk = statusLine.startsWith("HTTP/1.1 200 ") || statusLine.startsWith("HTTP/1.0 200 ");
          firstLine = false;
        } else if(statusLine.length() == 0) {
          client.stop();
          if(!httpOk || !recordAck) AppendSyncError("UPLOAD_ACK_MISSING", record->recordId.c_str());
          return httpOk && recordAck;
        } else {
          int colon = statusLine.indexOf(':');
          if(colon > 0) {
            String name = statusLine.substring(0, colon);
            String value = statusLine.substring(colon + 1);
            value.trim();
            if(name.equalsIgnoreCase("X-Ack-Record-Id") && value == record->recordId) recordAck = true;
          }
        }
        statusLine = "";
        continue;
      }
      if(statusLine.length() < 255 && c != '\r') {
        statusLine += c;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  client.stop();
  AppendSyncError("UPLOAD_TIMEOUT", record->recordId.c_str());
  return false;
}

bool UpdatePendingRecordStatus(const char* recordId, const char* newStatus, unsigned long attempts) {
  const char* tmpPath = "/SYNC/pending.tmp";
  bool updated = false;
  bool renameFailed = false;

  if(recordId == NULL || newStatus == NULL) {
    return false;
  }

  if(SdLock(SYNC_SD_LOCK_TIMEOUT_MS)) {
    File source = SD.open(SYNC_PENDING_FILE, FILE_READ);
    File target = SD.open(tmpPath, O_WRITE | O_CREAT | O_TRUNC);

    if(source && target) {
      String header = source.readStringUntil('\n');
      header.trim();
      if(header.length() == 0) {
        header = SYNC_PENDING_HEADER;
      }
      target.println(header);

      while(source.available()) {
        String row = source.readStringUntil('\n');
        SyncPendingRecord current;
        if(ParsePendingCsvRow(row, &current) && current.recordId == recordId) {
          target.print(current.recordType); target.print(",");
          target.print(current.recordId); target.print(",");
          target.print(current.sourceFile); target.print(",");
          target.print(current.sourceLine); target.print(",");
          target.print(newStatus); target.print(",");
          target.print(attempts); target.print(",");
          target.println(millis());
          updated = true;
        } else {
          row.trim();
          if(row.length() > 0) {
            target.println(row);
          }
        }
      }

      target.flush();
    }

    if(source) source.close();
    if(target) target.close();

    if(updated) {
      if(SD.exists(SYNC_PENDING_FILE)) {
        SD.remove(SYNC_PENDING_FILE);
      }
      if(!SD.rename(tmpPath, SYNC_PENDING_FILE)) {
        renameFailed = true;
        File recovered = SD.open(SYNC_PENDING_FILE, O_WRITE | O_CREAT | O_TRUNC);
        File tmp = SD.open(tmpPath, FILE_READ);
        if(recovered && tmp) {
          while(tmp.available()) {
            recovered.write(tmp.read());
          }
          recovered.flush();
        }
        if(tmp) tmp.close();
        if(recovered) recovered.close();
        if(SD.exists(tmpPath)) {
          SD.remove(tmpPath);
        }
      }
    } else if(SD.exists(tmpPath)) {
      SD.remove(tmpPath);
    }

    SdUnlock();
  }

  if(renameFailed) {
    AppendSyncError("PENDING_RENAME_FAILED", recordId);
  }

  return updated;
}

unsigned long UploadPendingRecords(uint8_t maxRecords) {
  unsigned long uploaded = 0;

  for(uint8_t i = 0; i < maxRecords; i++) {
    if(IsSyncPausedForSampling()) {
      break;
    }

    SyncPendingRecord record;
    if(!LoadNextPendingRecord(&record)) {
      break;
    }

    bool ok = UploadPendingRecord(&record);
    if(ok) {
      UpdatePendingRecordStatus(record.recordId.c_str(), "UPLOADED", record.attempts + 1);
      syncLastSuccessMs = millis();
      uploaded++;
    } else {
      UpdatePendingRecordStatus(record.recordId.c_str(), "PENDING", record.attempts + 1);
      syncFailureCount++;
      break;
    }
  }

  return uploaded;
}
