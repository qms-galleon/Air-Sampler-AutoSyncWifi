# ESP32 Air Sampler Firmware Audit

Date: 2026-09-03

Scope: audit of the existing Arduino ESP32 firmware before adding production hardening and automatic background synchronization. No architecture changes are included in this audit document.

## 1. Project structure

The firmware is an Arduino ESP32 sketch split across multiple `.ino` files with shared globals in `VarDef.h`.

| File | Main responsibility |
|---|---|
| `AS_ESP_BCF_OTA_11_HeadOp_MotorAb_ntp.ino` | `setup()`, `loop()`, RTC init, OTA mode, logout/shutdown timers |
| `VarDef.h` | Includes, global objects, global state, constants |
| `VarDef1.h` | Empty/minimal |
| `Process.ino` | motor/sample execution, input bit processing, calibration, Wi-Fi/server credential save |
| `SDCard.ino` | SD init, Wi-Fi connect/check, NTP boot sync, login, audit/sample logging, last sample reads, calibration validity |
| `Read_Topway.ino` | Topway display polling, page state, dropdown/report reads, serial packet decode |
| `Write_Topway.ino` | Topway writes, time/countdown display writes |
| `Screens.ino` | boot screen setup, counters, company details, CSV count helpers |
| `OnlineS.ino` | active HTTP/Wi-Fi server request handler for online software operations |
| `hndleWF.ino` | older/alternate HTTP request handler, mostly similar to file/audit/location endpoints |
| `UserManagement.ino` | user add/edit/discard, password change/reset, password-expiry self reset |
| `GroupMgmt.ino` | group add/edit/discard, group permissions, group privilege load |
| `RecipeManagement.ino` | recipe add/edit/discard and recipe summary |
| `LocationManagement.ino` | location/remark add/discard helper logic |
| `RemarkManagement.ino` | remark UI bit handler |
| `DeviceDetails.ino` | company/equipment/serial details |
| `ErrorParams.ino` | motor/head-block/head-open thresholds |
| `Audit.ino` | audit report pagination/indexing |
| `Quick.ino` | quick sample, last sample repeat, online sample parameter writes |
| `ReadSensorVal.ino` | voltage/current display and battery/charger popups |
| `changes.md` | prior password-expiry bug-fix notes |

## 2. Runtime architecture

Current firmware is mostly single-threaded.

### Boot flow

1. `setup()`
2. Serial ports start.
3. EEPROM starts.
4. RTC initializes.
5. 5 second startup delay.
6. `SDCard_init()`
7. `SyncRtcFromNtpAtBoot()`
8. `LogActivity("DevicePowerOn")`
9. `ReadVtg()`
10. `ProcessScreen()`
11. `ReadEEP()`

### Main loop flow

`loop()` currently does:

1. If `OnlineStatus == true`, call `handleRequest1()`.
2. If OTA is requested, enter OTA mode with an infinite `while(1)` loop.
3. Read serial input from motor/battery controller using `ReadSerial()`.
4. Every 100 ms call `Read_Disp_Params()`.
5. Every 1000 ms:
   - update RTC/display time
   - read voltage/current values
   - run `Motor_On_Off()`
   - check Wi-Fi state
   - manage auto logout/auto shutdown timers

## 3. Sampling and motor flow

### Local sample start

1. Topway input bit is read in `Read_Disp_Params()`.
2. `process_Input_Bits()` detects sample start bit.
3. `Sample_Run()` validates page and battery.
4. `DisplayPage(8)` changes to run page.
5. `Motor_Status1 = true`.
6. `Motor_Bit_Loop()` reads motor/sample parameters from display registers.
7. `WriteSerial()` sends motor command to the secondary controller over `Serial2`.
8. `LogActivity("SamplingStart")`.

### Motor run

`Motor_On_Off()` executes every second while `Motor_Status1 == true`.

It handles:

- initial start delay
- run countdown
- delay between runs
- run count decrement
- motor output through `RunMotor()`
- motor absent/head block/head open checks through `CheckforError()`
- completion logging through `Log_Report("OK")`
- stop using `StopMotor()`

### Abort/error paths

- `Sample_Abort()` for user/remote abort
- `Sample_Abort1()` for motor absent
- `Sample_Abort2()` for head block
- `Sample_Abort3()` for head open

Each abort path logs sample report plus audit activity.

## 4. Display/UI architecture

Topway display communication is through serial packets.

Read functions:

- `ReadTopway()`
- `Read_String()`
- `processPacket()`
- `processPacket2()`

Write functions:

- `WriteTopway()`
- `WriteTopway_byte()`
- `WriteTopway_32()`
- `WriteString()`
- `DisplayPage()`

UI state is controlled through:

- `Page_No`
- page-specific bit registers
- shared `bitArray[16]`

Risk: display read/write calls are synchronous and use global temporary variables (`temp_l`, `temp_h`, `Str2`, `Read_Err`). If background tasks are added later, Topway serial access should remain on one task or be protected.

## 5. Data and CSV storage

The firmware uses SdFat with one global `SdFat SD`.

### Global File objects

Defined in `VarDef.h`:

- `File myFile`
- `File tempFile`
- `File myFile1`
- `File userFile`

There are also local `File` objects in some functions, especially HTTP file sending and calibration reading.

### Main CSV/config files observed

| File | Purpose | Current format notes |
|---|---|---|
| `/FR.csv` | full sample report | `SampleId/SampleName,UserId,Mode,Start,End,Status,Remark,Location,Volume,StartDelay,NoOfRuns,DelayBetweenRuns` |
| `/DLS.csv` | last device sample | same sample fields as latest sample |
| `/<UserId>-LS.csv` | last sample for one user | same sample fields as latest sample |
| `/ActLog.csv` | audit/activity trail | `AuditId,UserId,Activity,ActivityTime,AuditDetails,AuditRemark` |
| `/USR.csv` | users | `Id,User,Password,FirstName,LastName,Dept,Group,Status,FirstLoginFlag,Retry,PasswordDate` |
| `/GUSER/<User>.csv` | per-user detail copy | same user fields |
| `/GRP.csv` | groups/permissions | long permission row with group timers, flags, print/view/access settings |
| `/LOC.csv` | locations | `Id,Name,Status` |
| `/RMK.csv` | remarks | `Id,Name,Status` |
| `/RECP.csv` | recipes | `Id,Recipe,Mode,Location,Volume,StartDelay,NoOfRuns,DelayBetweenRuns,Status` |
| `/CAL.csv` | calibration | `CalId,Pwm,CalDate,DueDate` |
| `/CompDet.csv` | company details | `Company,City,State` |
| `/DevInf.csv` | device details | `ModelNo,SoftwareVer,UnitName,EquipmentId,DeviceId` |
| `/WIFICRED.csv` | Wi-Fi credentials | `SSID,Password` |
| `/SERVCRED.csv` | server details | `ServerIP,Port` |
| `/staticIP.csv` | static network config | `IP,Subnet,Gateway,DNS,MAC` |
| `/Motor.CSV` | motor/head thresholds | `Hblock_Min,Hblock_Max,Hopen_Min,Hopen_Max` |
| `/BatParam.csv` | battery parameter log | `Time,BAT_VTG,BAT_PER,CHG_VTG` |
| `/lineIndex.dat` | audit pagination index | binary/line index helper |

## 6. SD access audit

SD access is spread across many files:

- `SDCard.ino`
- `Process.ino`
- `Screens.ino`
- `OnlineS.ino`
- `hndleWF.ino`
- `UserManagement.ino`
- `GroupMgmt.ino`
- `RecipeManagement.ino`
- `LocationManagement.ino`
- `DeviceDetails.ino`
- `ErrorParams.ino`
- `Audit.ino`
- `Read_Topway.ino`

Good existing practices:

- Many important updates use temp files and rename.
- Most write paths call `flush()` and `close()`.
- Many read paths check file open success.
- File upload endpoints stream files with small buffers instead of loading full files into RAM.

Current risks:

- No SD mutex exists.
- Global `File` objects are reused by unrelated functions.
- Multiple files use the same temp filename, especially `/temp.csv`; this is dangerous if any SD operation is moved to another task.
- Some `SD.remove()` then `SD.rename()` replacements can lose valid data if power fails between remove and rename.
- Some config creation helpers silently create empty files, which may hide missing-data problems.
- Some SD failures return silently, which can lose audit visibility.
- No central SD error logging exists.

## 7. Wi-Fi, HTTP, OTA, and NTP audit

### Wi-Fi

`ConnectToWiFi()` reads `/WIFICRED.csv`, optionally loads static IP, calls `WiFi.begin()`, then waits up to 10 seconds for connection. It starts `wifiServer` after successful connection.

`CheckWiFi()` only updates `WiFiStatus`; it does not reconnect by itself.

### HTTP/server behavior

`OnlineS.ino` implements `handleRequest1()` using `WiFiServer`/`WiFiClient`.

Observed endpoints:

- `/?files`
- `/?Sample-...`
- `/?Status`
- `/?SampSt`
- `/?Abort`
- `/?BatST`
- `/?Launch`
- `/?LOCADD-...`
- `/?LOCDIS-...`
- `/?REMADD-...`
- `/?REMDIS-...`
- `/?RECPADD-...`
- `/?RECPDIS-...`
- `/?USERADD-...`
- `/?USERUPLOAD-...`
- `/?USERRST-...`
- `/?USERDIS-...`
- `/?GRPADD-...`
- `/?GRPDIS-...`

`hndleWF.ino` contains an older/alternate handler with:

- `GET /file`
- `GET /AUDIT`
- `GET /LOCADD`
- `GET /LOCDIS`

Current server model is inbound request/response: external software connects to the ESP32. The pasted production-hardening request asks for ESP32-initiated automatic background sync, which is a new behavior and needs an API contract before implementation.

### OTA

When `OTA_status == true`, the firmware calls `Init_OTA()` and then enters an infinite OTA loop:

```cpp
while(1) {
  ArduinoOTA.handle();
}
```

Risk: once OTA mode starts, normal sampling/UI/logging loop is not serviced.

### NTP

`SyncRtcFromNtpAtBoot()` uses UDP to sync RTC once at boot. It has bounded timeout. Periodic NTP sync code exists only as commented code.

## 8. EEPROM audit

EEPROM is used in `ReadEEP()`:

- byte `0` appears to track clean/abrupt shutdown.
- byte `10` stores motor PWM.
- byte `11` stores start delay.

Risk: abrupt shutdown detection only distinguishes a simple marker and does not persist active state such as sync progress or pending upload status.

## 9. Blocking delays and loops

Observed blocking behavior:

- `setup()` has `delay(5000)`.
- `ConnectToWiFi()` waits up to 10 seconds, with `delay(500)` loop.
- `SyncRtcFromNtpAtBoot()` waits up to about 2 seconds.
- HTTP request handlers wait up to 1 second for request data.
- OTA mode uses infinite loop and stops normal firmware loop.
- Several password/user reset paths use `delay(100)`.
- Calibration save/exit sends repeated serial commands with delays.
- `Buzz_Blink()` can block for multiple seconds.
- Several full-file read loops read CSV files line by line in the UI path.

Most blocking operations currently happen on the main Arduino loop. For 24/7 hardening, sync/network actions must not add new main-loop blocking.

## 10. String and memory audit

The firmware uses many global and local Arduino `String` objects.

Positive points:

- Several functions call `reserve()` for request/line buffers.
- File upload uses byte buffers.
- No JSON library use was found.

Risks:

- Heavy `String` concatenation is used for CSV rows, HTTP parsing, dates, audit messages, and serial commands.
- `OnlineS.ino` and management modules repeatedly create temporary `String` instances.
- Dynamic `String` use can fragment heap during long 24/7 operation.
- No current heap monitoring exists using `ESP.getFreeHeap()` or minimum free heap.

## 11. Race risks if FreeRTOS tasks are added

The current firmware is not task-safe.

Shared global state used across modules includes:

- `UserId`, `Password`, `Group`
- `SampleId`, `SampleName`, `Mode`, sample timing fields
- `Motor_Status1`, motor counters, run counters
- `OnlineStatus`, `SoftWareConnected`, `WiFiStatus`
- `AuditDetails`, `AuditRemark`
- `Str1`, `Str2`, `Str3`, `pos1` through `pos21`
- `myFile`, `tempFile`, `userFile`
- `Page_No`, `bitArray`

If a SyncManager task is added, it must not call existing SD/file functions directly without locking and without isolating shared globals.

Highest race-risk functions:

- `LogActivity()`
- `Log_Report()`
- `Log_DLS()`
- `Log_ULS()`
- `UpdateRetryOrLock()`
- `Manage_User()`
- `Update_User()`
- `Update_Group()`
- `Update_Recipe()`
- `Discard_Location()`
- `handleRequest1()`
- `Read_Disp_Params()`

## 12. Power-loss corruption risks

Most append logs are vulnerable to partial trailing lines if power fails mid-write:

- `/FR.csv`
- `/ActLog.csv`
- `/BatParam.csv`
- `/CAL.csv`

Most temp-file replacements are safer, but still have a vulnerable window where old file is removed before temp rename:

- `/USR.csv`
- `/GRP.csv`
- `/RECP.csv`
- `/LOC.csv`
- `/RMK.csv`
- `/DLS.csv`
- `/<UserId>-LS.csv`
- `/DevInf.csv`
- `/CompDet.csv`
- `/WIFICRED.csv`
- `/SERVCRED.csv`
- `/staticIP.csv`
- `/Motor.CSV`

For production sync, do not mark a record synced until server ACK is durable.

## 13. Network failure impact

Current inbound HTTP handler has bounded request-line wait, but file transfer and client writes may still consume main-loop time while sending CSV files.

Current design risks:

- `/ ?files` endpoint can stream many files while main loop is occupied.
- No retry/backoff exists for outbound sync because outbound sync is not implemented yet.
- No automatic reconnect after Wi-Fi loss.
- Existing server behavior is mostly device-as-server, not device-as-client.
- Server response loss/idempotent sync is not addressed.

Sampling must stay higher priority than any future sync task.

## 14. Security and access notes

Observed sensitive or weak areas:

- Built-in `SAdmin`/`SIIV` password is hardcoded as `1234`.
- Wi-Fi credentials are stored plaintext in `/WIFICRED.csv`.
- User passwords are stored plaintext in `/USR.csv` and `/GUSER/<User>.csv`.
- HTTP endpoints appear unauthenticated once `SoftWareConnected == true`.
- `GET` query strings carry user/password/config fields.

This audit does not change security behavior, but production hardening should document these risks.

## 15. Current strengths

- Sampling logic is already local/offline-first.
- CSV records are locally stored.
- Many writes are flushed and closed.
- Several config updates use temp files.
- Online file transfer streams chunks instead of loading full files.
- NTP boot sync has a bounded timeout.
- Prior password-expiry/no-user sampling glitch has been patched separately.

## 16. Key gaps before production background sync

1. No SD mutex.
2. No persistent sync state.
3. No SyncManager state machine.
4. No outbound HTTP upload/download implementation.
5. No idempotent sync ID scheme for all record types.
6. No server API contract for device-initiated sync.
7. No automatic Wi-Fi reconnect/backoff.
8. No watchdog integration.
9. No heap/stack diagnostics.
10. No central SD error/audit logging.
11. No task-safe wrapper around shared globals.
12. No safe import/validation path for downloaded server files.

## 17. Recommended next step

Do not implement all hardening at once.

Recommended sequence:

1. Add minimal production foundation:
   - `ProductionConfig`
   - SD mutex helpers
   - sync event flags
   - diagnostic logging wrapper
2. Add `SYNC_ARCHITECTURE.md` and `SYNC_API_CONTRACT.md`.
3. Implement persistent sync state files.
4. Add low-priority FreeRTOS SyncManager skeleton.
5. Add outbound upload with idempotent record IDs.
6. Add safe server-to-device download/import.
7. Add watchdog and heap diagnostics.
8. Add production test plan and run compile/field tests.

## 18. Immediate caution for implementation

Any new background task must avoid:

- calling SD functions without a mutex
- reusing global `myFile`/`tempFile`/`userFile`
- using global parser variables like `Str2` and `pos1..pos21`
- holding SD lock while doing network I/O
- changing existing CSV row format without a compatibility plan
- blocking sampling on Wi-Fi/server status

