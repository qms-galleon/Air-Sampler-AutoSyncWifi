# Firmware Changes

Issue fixed: after a user password expired, the self password reset flow allowed the user to continue without a clean login session. Samples could then be recorded with `NO_USER`, and the auto-shutdown timer could later trigger because the firmware still considered the device logged out.

## 1. Expired-password self reset now stays logged out

File: `UserManagement.ino`

### Before

```cpp
void SelfResetOnExpiry() {
  WriteTopway(0x01, 0x12, 0);              // clear the popup register
  PasswordExpired = false;                 // clear the expired flag
  WriteString(0x00, 0x22, 0x80, strGUser);// pre-fill username on page 147
  WriteString(0x00, 0x23, 0x80, "");      // clear new-password field
  WriteString(0x00, 0x24, 0x00, "");      // clear confirm-password field
  DisplayPage(147);                        // go to Reset Password page
}
```

### After

```cpp
void SelfResetOnExpiry() {
  WriteTopway(0x01, 0x12, 0);              // clear the popup register
  PasswordExpired = false;                 // clear the expired flag
  PasswordResetOnExpiry = false;           // reset flow is now on the password page
  UserId = "NO_USER";                      // do not carry an expired-login session forward
  WriteString(0x00, 0x22, 0x80, strGUser);// pre-fill username on page 147
  WriteString(0x00, 0x23, 0x80, "");      // clear new-password field
  WriteString(0x00, 0x24, 0x00, "");      // clear confirm-password field
  DisplayPage(147);                        // go to Reset Password page
}
```

Reason: when password is expired, firmware must not keep any partial/expired user session active.

## 2. After expiry reset, force user back to login page

File: `UserManagement.ino`

### Before

```cpp
void CheckUserPassword2() {
//  Read_String(0x23,0x00);if(Read_Err==false) {GUserPass1=Str2;}
  Read_String(0x23,0x80);if(Read_Err==false) {GUserPass2=Str2;}
  Read_String(0x24,0x00);if(Read_Err==false) {GUserPass3=Str2;}

  ...

    WriteTopway(0x00,0x72,3); //Password Changed Popup
    WriteTopway(0x00,0x74,0);
    WriteString(0x00,0x23,0x80,"");
    WriteString(0x00,0x24,0x00,"");
//    AuditDetails=strGroup;AuditRemark="OK";
    //LogActivity("UserPswdReset");
}
```

### After

```cpp
void CheckUserPassword2() {
//  Read_String(0x23,0x00);if(Read_Err==false) {GUserPass1=Str2;}
  bool resetFromExpiredLogin = (Page_No == 147);
  Read_String(0x23,0x80);if(Read_Err==false) {GUserPass2=Str2;}
  Read_String(0x24,0x00);if(Read_Err==false) {GUserPass3=Str2;}

  ...

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
}
```

Reason: once password reset succeeds from the expiry page, user must login again. This ensures `UserId` is recorded properly in sample records.

## 3. Block local sample start when no user is logged in

File: `Process.ino`

### Before

```cpp
void Sample_Run() {
  if(Page_No==7 || Page_No==81 || Page_No==206) {
    if(BAT_PER>20) {
      DisplayPage(8);
      WriteTopway(0x01,0x14,SampleId);
      WriteString(0x00,0x2c,0x80,SampleName);
      Motor_Status1=true;/*LogBatParameters();*/Motor_Bit_Loop();WriteSerial();
      AuditDetails=SampleName;AuditRemark="NA";
      if(OnlineStatus==false) {LogActivity("SamplingStart");} else {AuditDetails=onlineUser + "," + AuditDetails;LogActivity("RemoteUserLogin");SampleRunString="SAMPLING STARTED";}
    }
  }
}
```

### After

```cpp
void Sample_Run() {
  if(Page_No==7 || Page_No==81 || Page_No==206) {
    if(OnlineStatus==false && UserId=="NO_USER") {
      Motor_Status1=false;
      Write_In_Bit();
      WriteTopway(0x00,0x68,1);
      DisplayPage(1);
      return;
    }
    if(BAT_PER>20) {
      DisplayPage(8);
      WriteTopway(0x01,0x14,SampleId);
      WriteString(0x00,0x2c,0x80,SampleName);
      Motor_Status1=true;/*LogBatParameters();*/Motor_Bit_Loop();WriteSerial();
      AuditDetails=SampleName;AuditRemark="NA";
      if(OnlineStatus==false) {LogActivity("SamplingStart");} else {AuditDetails=onlineUser + "," + AuditDetails;LogActivity("RemoteUserLogin");SampleRunString="SAMPLING STARTED";}
    }
  }
}
```

Reason: this is a safety guard. If any screen path reaches sample start without login, sample is refused and user is sent to login.

## 4. Auto-shutdown no longer runs while sampling is active

File: `AS_ESP_BCF_OTA_11_HeadOp_MotorAb_ntp.ino`

### Before

```cpp
if(UserId=="NO_USER") {
  if(Power_Off_Timer<Max_Poweroff_Time-30) {
    Power_Off_Timer=Power_Off_Timer+1;
  } else if(Power_Off_Timer<(Max_Poweroff_Time)) {
    Power_Off_Timer=Power_Off_Timer+1;
    WriteTopway(0x00,0x68,2);
    WriteTopway(0x00,0x56,(Max_Poweroff_Time-Power_Off_Timer));
  } else {
    ShutDown("AutoShutDown");
    AutoShutdownbit=true;
  }
} else {
  if(Logout_Timer<Max_Logout_Time-30) {
    Logout_Timer=Logout_Timer+1;
  }
}
```

### After

```cpp
if(UserId=="NO_USER" && Motor_Status1==false) {
  if(Power_Off_Timer<Max_Poweroff_Time-30) {
    Power_Off_Timer=Power_Off_Timer+1;
  } else if(Power_Off_Timer<(Max_Poweroff_Time)) {
    Power_Off_Timer=Power_Off_Timer+1;
    WriteTopway(0x00,0x68,2);
    WriteTopway(0x00,0x56,(Max_Poweroff_Time-Power_Off_Timer));
  } else {
    ShutDown("AutoShutDown");
    AutoShutdownbit=true;
  }
} else if(UserId=="NO_USER") {
  Power_Off_Timer=0;
  WriteTopway(0x00,0x68,0);
} else {
  if(Logout_Timer<Max_Logout_Time-30) {
    Logout_Timer=Logout_Timer+1;
  }
}
```

Reason: even if `UserId` becomes `NO_USER`, firmware must not auto-shutdown during an active motor/sample run.

## Expected behavior after fix

1. User login attempt detects expired password.
2. Firmware opens password reset page.
3. User resets password.
4. Firmware clears session and returns to login page.
5. User logs in again with new password.
6. Sample start records the correct user ID.
7. Auto-shutdown does not trigger during active sampling.

---

# Background Sync Hardening - Step 7

Step 7 added the first outbound network foundation for background sync. This only checks whether the configured server is reachable. It does not upload samples, does not change pending record status, and does not hold the SD-card mutex during network I/O.

## 5. Sync manager now loads saved server settings

Files: `VarDef.h`, `SyncManager.ino`

### Before

```cpp
// Sync manager had Wi-Fi state checks and pending record discovery,
// but it did not reload /SERVCRED.csv before trying server work.
```

### After

```cpp
bool LoadSyncServerConfig() {
  bool loaded = false;

  if(SdLock(SD_LOCK_CRITICAL_TIMEOUT_MS)) {
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
```

Reason: server IP and port are saved by the existing UI into `/SERVCRED.csv`, so the background sync task must reload them safely before network checks.

## 6. Sync manager now performs bounded server health check

Files: `VarDef.h`, `SyncManager.ino`

### Before

```cpp
// No outbound server health check existed in the background sync flow.
// Sync stopped after Wi-Fi and pending-record foundation work.
```

### After

```cpp
const uint32_t SYNC_TCP_CONNECT_TIMEOUT_MS = 3000;
const uint32_t SYNC_HTTP_RESPONSE_TIMEOUT_MS = 5000;

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
```

Reason: the sync task can now prove the server is reachable using a short TCP/HTTP timeout before later upload steps are added. This prevents long blocking network waits from affecting sampling. On success, `syncLastSuccessMs` is updated so `/SYNC/state.csv` shows the latest successful health contact.

## 7. Sync state machine includes server config and health states

Files: `VarDef.h`, `SyncManager.ino`

### Before

```cpp
enum SyncState {
  SYNC_IDLE,
  SYNC_CHECK_NETWORK,
  SYNC_LOAD_STATE,
  SYNC_FIND_PENDING,
  SYNC_RETRY_BACKOFF
};
```

### After

```cpp
enum SyncState {
  SYNC_IDLE,
  SYNC_CHECK_NETWORK,
  SYNC_LOAD_SERVER_CONFIG,
  SYNC_HEALTH_CHECK,
  SYNC_LOAD_STATE,
  SYNC_FIND_PENDING,
  SYNC_RETRY_BACKOFF
};
```

Reason: failed server config and failed server health are now visible in `/SYNC/state.csv` and `/SYNC/errors.csv`, making field debugging easier without changing sample CSV formats.

---

# Background Sync Hardening - SD Stuck Rectification

After testing, `/SYNC` files were created correctly, but sample completion could still collide with background sync SD access. This patch makes sync pause around sample/audit SD writes and cleans temporary state files more safely.

## 8. Background sync now pauses during sampling and SD log writes

Files: `VarDef.h`, `SyncManager.ino`, `SDCard.ino`

### Before

```cpp
void NotifySyncEvent() {
  syncWakePending = true;
  if(syncEvent != NULL) {
    xSemaphoreGive(syncEvent);
  }
}
```

### After

```cpp
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
```

Reason: sample completion writes `/FR.csv`, `/DLS.csv`, user last-sample CSV, and `/ActLog.csv`. Sync must not wake immediately and compete for SD access while those writes are finishing.

## 9. Sample and audit logs mark SD quiet window

File: `SDCard.ino`

### Before

```cpp
NotifySyncEvent();
```

### After

```cpp
MarkSyncSdQuiet();
NotifySyncEvent();
```

Reason: after a sample or audit row is written, background sync waits briefly before touching SD. This reduces SD-card stuck/freezing risk after one sample completes.

## 10. Sync state file write has fallback cleanup

File: `SyncManager.ino`

### Before

```cpp
if(SD.exists(SYNC_STATE_FILE)) {
  SD.remove(SYNC_STATE_FILE);
}
SD.rename(tmpPath, SYNC_STATE_FILE);
```

### After

```cpp
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
```

Reason: if FAT rename fails or old `state.csv` is damaged, firmware now tries a direct rewrite and removes stale `state.tmp`.

## 11. Flask listener added for ESP32 sync testing

File: `flask_air_sampler_sync_server.py`

### Before

```text
No local Python listener file existed in this firmware folder.
```

### After

```python
@app.get("/api/air-sampler/v1/health")
def health():
    device_id = request.args.get("device_id", "")
    return jsonify({
        "ok": True,
        "message": "air sampler sync server running",
        "device_id": device_id,
        "server_time_utc": utc_now_iso(),
    }), 200
```

Reason: Step 7 firmware health check now has a matching Flask endpoint. The file also includes `/api/air-sampler/v1/records` for the next upload step.

---

# Background Sync Upload - Step 8

Step 8 adds actual pending-record upload after the server health check passes. Upload is intentionally limited to one record per sync wake to reduce ESP32 RAM pressure and avoid disturbing sampling.

## 12. Sync state machine now uploads pending rows

Files: `VarDef.h`, `SyncManager.ino`

### Before

```cpp
syncState = SYNC_FIND_PENDING;
syncPendingDiscoveredLastRun = DiscoverPendingRecords();
SaveSyncState("OK");

syncState = SYNC_IDLE;
SaveSyncState("OK");
```

### After

```cpp
syncState = SYNC_FIND_PENDING;
syncPendingDiscoveredLastRun = DiscoverPendingRecords();
SaveSyncState("OK");

syncState = SYNC_UPLOAD_PENDING;
UploadPendingRecords(SYNC_UPLOAD_BATCH_LIMIT);
SaveSyncState("OK");

syncState = SYNC_IDLE;
SaveSyncState("OK");
```

Reason: after pending rows are discovered from `/FR.csv` and `/ActLog.csv`, firmware now attempts background upload to the configured Flask server.

## 13. One-record JSON POST upload added

Files: `VarDef.h`, `SyncManager.ino`

### Before

```text
Firmware only checked server health. It did not send pending data to the listener.
```

### After

```cpp
String request = "POST ";
request += SYNC_RECORDS_PATH;
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
```

Reason: the ESP32 now posts one pending record at a time to `/api/air-sampler/v1/records` using the existing `ServerIP` and `SeverPort` from `/SERVCRED.csv`.

## 14. Uploaded rows are marked in pending.csv

File: `SyncManager.ino`

### Before

```csv
record_type,record_id,source_file,source_line,status,attempts,last_attempt_ms
SAMPLE,G00250001-SAMPLE-00000001,/FR.csv,1,PENDING,0,0
```

### After

```csv
record_type,record_id,source_file,source_line,status,attempts,last_attempt_ms
SAMPLE,G00250001-SAMPLE-00000001,/FR.csv,1,UPLOADED,1,123456
```

Reason: successfully acknowledged records are not uploaded again. If upload fails due network/server timeout, the row remains `PENDING` with increased attempt count so it can retry later.

## 15. Pending file rewrite recovery added

File: `SyncManager.ino`

### Before

```cpp
if(!SD.rename(tmpPath, SYNC_PENDING_FILE)) {
  renameFailed = true;
}
```

### After

```cpp
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
```

Reason: if FAT rename fails, firmware rebuilds `pending.csv` from `pending.tmp` instead of losing pending status data.

---

# Flask Listener Hardening - Proper Received Data and Status

After ESP32 upload testing, records were reaching Flask, but the server originally stored only raw JSON/index rows. If `G00250001_record_index.csv` was open in Excel, Flask returned `500`, causing the ESP32 to correctly retry the same pending row.

## 16. Server now writes parsed sample/audit CSV files

File: `flask_air_sampler_sync_server.py`

### Before

```text
Server stored the raw uploaded record in:
- <device_id>_records.jsonl
- <device_id>_record_index.csv
```

### After

```text
Server also writes:
- <device_id>_sample_records.csv
- <device_id>_audit_records.csv
- <device_id>_sync_status.csv
```

Reason: received sample data is now available in proper columns such as `sample_id`, `sample_name`, `user_id`, `sampling_start`, `sampling_end`, `sampling_status`, `sampling_location`, and motor/run values.

## 17. Server sync status is now recorded

File: `flask_air_sampler_sync_server.py`

### Before

```text
Server had no separate status file showing whether a device record was received or duplicate.
```

### After

```text
<device_id>_sync_status.csv

server_time_utc,device_id,record_type,record_id,source_file,source_line,status
```

Reason: firmware status is stored on SD in `/SYNC/pending.csv`, while server-side receive status is stored in `<device_id>_sync_status.csv`.

## 18. Server handles locked CSV files without breaking device sync

File: `flask_air_sampler_sync_server.py`

### Before

```text
If <device_id>_record_index.csv was open in Excel, Flask could raise PermissionError and return HTTP 500.
```

### After

```text
If a CSV file is locked, Flask writes a timestamped fallback file:

<original_name>_YYYYMMDD_HHMMSS_microseconds_locked.csv
```

Reason: the ESP32 should not keep retrying forever just because a server CSV is open in another program.

## 19. Duplicate check includes locked fallback index files

File: `flask_air_sampler_sync_server.py`

### Before

```text
Duplicate detection only checked:
<device_id>_record_index.csv
```

### After

```text
Duplicate detection checks:
<device_id>_record_index*.csv
```

Reason: if the main index CSV is open in Excel, the server writes a timestamped `_locked.csv` fallback. Future duplicate checks now also read those fallback files.

---

# Fresh-Only Background Sync - Step 9

Step 9 changes background sync from backlog upload to fresh-data upload. On first run, firmware records the latest existing sample/audit positions as a baseline and clears old pending backlog. After that, only new rows created after the baseline are queued and uploaded.

## 20. Cursor file added for fresh-only row discovery

Files: `VarDef.h`, `SyncManager.ino`

### Before

```cpp
unsigned long lastKnownRecord = GetLastPendingRecordNumber(recordType);
```

If `/SYNC/pending.csv` was empty, firmware started from record `1` and uploaded old `/FR.csv` rows.

### After

```cpp
const char* SYNC_CURSOR_FILE = "/SYNC/cursors.csv";
const char* SYNC_CURSOR_HEADER = "record_type,source_file,last_record_number,last_source_line";
```

Firmware now stores:

```csv
record_type,source_file,last_record_number,last_source_line
SAMPLE,/FR.csv,3,3
AUDIT,/ActLog.csv,150,150
```

Reason: `pending.csv` is only the upload queue. `cursors.csv` is the discovery checkpoint that decides which source CSV rows are new.

## 21. First cursor creation skips old backlog

File: `SyncManager.ino`

### Before

```text
No cursor existed, so old FR.csv / ActLog.csv rows were treated as pending.
```

### After

```cpp
if(!GetSyncCursor(recordType, &lastKnownRecord, &lastKnownLine)) {
  ScanSourceLatest(sourceFile, sampleFile, &maxRecordNumber, &maxSourceLine);
  SaveSyncCursor(recordType, sourceFile, maxRecordNumber, maxSourceLine);
  return 0;
}
```

Reason: first run becomes the baseline. New sample/audit rows after this point are the only rows queued for background upload.

## 22. Pending queue reset on fresh-sync migration

File: `SyncManager.ino`

### Before

```text
Old pending rows could remain and upload before the new sample.
```

### After

```cpp
if(!sampleCursorKnown || !auditCursorKnown) {
  ResetPendingQueue();
}
```

Reason: when fresh-only sync is first activated, old unsent queue data is cleared so the next completed sample is the first uploaded sample.

## 23. Firmware POST endpoint changed to serial-number route

Files: `VarDef.h`, `SyncManager.ino`

### Before

```text
POST /api/air-sampler/v1/records
```

### After

```text
POST /sync_device_esp_f3_push/<deviceID>
```

Reason: this matches the database route style of your previous server code, where data is inserted against the serial number.

## 26. REST API expanded for fresh operational CSV rows

Files: `SyncManager.ino`, `VarDef.h`, `flask_air_sampler_sync_server.py`

### Before

```text
The background API discovered fresh rows only from FR.csv and ActLog.csv.
Other operational CSV files were not sent.
```

### After

```text
Fresh rows are tracked with an independent line cursor for DLS.csv, BatParam.csv,
CAL.csv, GRP.csv, LOC.csv, RMK.csv, RECP.csv, DevInf.csv, and CompDet.csv.
The Flask listener accepts the same POST endpoint and stores each additional source
as a raw CSV record while retaining duplicate protection by record_id.
```

Reason: the ESP32 now sends only rows appended after each file's saved cursor. Existing rows are baselined on first discovery, so old SD-card data is not uploaded. `SERVCRED.csv`, `WIFICRED.csv`, `staticIP.csv`, `/USR.csv`, and `/SYNC/*` remain excluded because they contain credentials, passwords, configuration secrets, or internal sync state.

The database route also stores non-sample/non-audit rows in `sync_csv_records` with a unique `(serial_number, record_id)` key, so the firmware receives an acknowledgement only after the row is stored or confirmed as a duplicate.

## 24. Database push-route example added

File: `flask_air_sampler_push_db_route.py`

### Before

```text
Your previous route pulled multipart CSV files from the ESP32 using requests.get().
```

### After

```text
New route accepts ESP32 background POST JSON:
/sync_device_esp_f3_push/<serial_number>
```

Reason: the ESP32 can now push only fresh rows in the background. The server inserts/updates `sample_records` and inserts/skips duplicate `audit_trail` rows against the provided serial number.

## 25. Automatic Wi-Fi reconnection and live Wi-Fi icon state

Files: `VarDef.h`, `SDCard.ino`

### Before

```text
CheckWiFi() only detected a lost connection and turned the Wi-Fi icon off. It did not attempt to reconnect, so Wi-Fi stayed unavailable until a restart or manual reconnect.
```

### After

```cpp
if (!connected) {
  Set_Bit_Icons(1, 0);
  WiFi.reconnect();
}
```

Reason: the firmware now retries Wi-Fi every 10 seconds without blocking the sampling loop. The Wi-Fi icon turns off on the disconnect transition and turns on after the connection is restored; the local server and background sync are also notified after recovery.
