# ESP32 Air Sampler Sync Architecture

Date: 2026-09-03

Purpose: design the production-safe automatic background synchronization system before code implementation.

This document is intentionally additive. It does not require rewriting the Arduino sketch or migrating to ESP-IDF.

## 1. Design goals

The device must remain offline-first.

Sampling, motor/pump control, UI, sensors, audit logging, and local CSV storage must continue even when:

- Wi-Fi is unavailable
- Internet/server is unavailable
- HTTP request times out
- sync fails halfway
- power is interrupted
- ESP32 restarts
- the same record is uploaded again

Synchronization must never be a prerequisite for sampling.

## 2. Current architecture summary

The existing firmware is a single Arduino ESP32 sketch split across `.ino` files.

Current loop ownership:

- `loop()` runs the main scheduler.
- `Read_Disp_Params()` polls Topway display and UI bits.
- `Motor_On_Off()` advances sampling/motor state every second.
- `handleRequest1()` handles inbound HTTP requests when `OnlineStatus == true`.
- SD CSV writes happen directly from many functions.
- Global state is shared across all modules.

The current server behavior is mostly device-as-server: PC/software connects to ESP32 endpoints like `/?files`, `/?Sample`, `/?LOCADD`, etc.

The new background sync requirement introduces device-as-client behavior: ESP32 should initiate upload/download automatically.

## 3. Proposed task model

Keep the existing Arduino `loop()` as the critical main loop.

Add only one low-priority FreeRTOS task:

```text
Main Arduino Loop
  priority: normal/high by default
  owns: sampling, motor timing, display polling, existing UI actions

SyncManager Task
  priority: low
  owns: automatic background upload/download/retry
```

Do not move sampling into a new task in the first implementation phase. That reduces risk because the existing motor and display code depends heavily on globals.

## 4. Responsibility split

### Main loop keeps ownership of

- sampling start/stop
- motor countdown
- motor absent/head block/head open safety aborts
- Topway display read/write
- login/session/user UI flow
- local CSV append for sample/audit records
- existing inbound software control behavior

### SyncManager owns

- deciding when sync should run
- checking Wi-Fi/server readiness
- reading pending local records/files
- uploading records/files in bounded chunks
- receiving server ACK
- writing persistent sync state
- checking for server-side updates
- downloading server files to temp files
- validating downloaded files
- atomically importing valid downloaded files
- retry/backoff on failure

## 5. FreeRTOS objects

### SD mutex

One global mutex should protect all SD filesystem access:

```cpp
SemaphoreHandle_t sdMutex;
```

Rule:

```cpp
if (xSemaphoreTake(sdMutex, timeoutTicks) == pdTRUE) {
  // SD-only operation
  xSemaphoreGive(sdMutex);
}
```

Important:

- Hold SD mutex only while using SD.
- Do not hold SD mutex while waiting for Wi-Fi/server/HTTP.
- Do not use shared global `File myFile`, `tempFile`, or `userFile` from SyncManager.
- Prefer local `File` objects inside sync code.

### Sync event semaphore

Use one binary semaphore/event to wake SyncManager:

```cpp
SemaphoreHandle_t syncEvent;
```

Signals:

- Wi-Fi connected/reconnected
- new local sample/audit data written
- periodic interval expired
- retry backoff expired
- manual/service sync request if later needed

### Optional queue

For phase 1, avoid a complex queue. Existing firmware already writes records to CSV synchronously.

For later improvement, a queue can send “new record available” metadata:

```text
record_type
record_id
file_path
created_at
```

## 6. SyncManager state machine

Recommended states:

```text
SYNC_IDLE
  |
  v
SYNC_CHECK_NETWORK
  |
  v
SYNC_LOAD_STATE
  |
  v
SYNC_FIND_PENDING
  |
  v
SYNC_UPLOAD
  |
  v
SYNC_WAIT_ACK
  |
  +--> failure --> SYNC_RETRY_BACKOFF --> SYNC_IDLE
  |
  v
SYNC_CHECK_SERVER_UPDATES
  |
  v
SYNC_DOWNLOAD
  |
  v
SYNC_VALIDATE
  |
  v
SYNC_IMPORT
  |
  v
SYNC_ACK_SERVER
  |
  v
SYNC_SAVE_STATE
  |
  v
SYNC_IDLE
```

No state may contain an infinite loop.

Every network operation must have a timeout.

## 7. Persistent sync state

Sync state must survive reboot.

Recommended files:

| File | Purpose |
|---|---|
| `/SYNC/state.csv` | last successful sync, current server version, retry metadata |
| `/SYNC/pending.csv` | records/files known to be pending upload |
| `/SYNC/inflight.csv` | upload started but not yet ACKed |
| `/SYNC/errors.csv` | bounded sync error log |
| `/SYNC/download.tmp` | temporary server download |
| `/SYNC/import.tmp` | parsed/validated temporary import file if needed |

Create `/SYNC` folder during SD initialization.

Do not mark records synced until server ACK is received and sync state is safely written.

## 8. Record identity and idempotency

Every uploaded record must have stable identity.

Recommended identity format:

```text
<device_id>-<record_type>-<record_number>
```

Examples:

```text
G00250001-SAMPLE-00000123
G00250001-AUDIT-00000456
G00250001-USER-00000007
G00250001-GROUP-00000003
```

The identity must be derived from existing stable fields, not generated newly on retry.

Suggested mapping:

| Record type | Source file | Stable number |
|---|---|---|
| SAMPLE | `/FR.csv` | `SampleId` before `/` |
| AUDIT | `/ActLog.csv` | `AuditId` |
| USER | `/USR.csv` | user ID field |
| GROUP | `/GRP.csv` | group ID field |
| LOCATION | `/LOC.csv` | location ID field |
| REMARK | `/RMK.csv` | remark ID field |
| RECIPE | `/RECP.csv` | recipe ID field |
| CALIBRATION | `/CAL.csv` | calibration ID field |
| DEVICE | `/DevInf.csv` | device ID plus version/hash |

For append-only logs, sync can track last ACKed record number.

For mutable master/config files, sync should upload complete row snapshots with stable IDs and updated state/status.

## 9. Local data handling strategy

### Append logs

Files:

- `/FR.csv`
- `/ActLog.csv`
- `/BatParam.csv`
- `/CAL.csv`

Preferred sync method:

- read from last ACKed record number
- upload only new rows
- retry same rows if ACK is missing
- server deduplicates by record ID

### Mutable/config data

Files:

- `/USR.csv`
- `/GRP.csv`
- `/LOC.csv`
- `/RMK.csv`
- `/RECP.csv`
- `/CompDet.csv`
- `/DevInf.csv`
- `/SERVCRED.csv`
- `/staticIP.csv`
- `/Motor.CSV`

Preferred sync method:

- compute lightweight file version/hash
- upload changed file snapshot or changed rows
- server applies by stable identity
- server returns latest accepted version

## 10. SD locking rules

All SD use must be protected once SyncManager exists.

Rules:

1. Take `sdMutex` before any `SD.open`, `SD.exists`, `SD.remove`, `SD.rename`, `SD.mkdir`, `File.read`, or `File.write`.
2. Release `sdMutex` immediately after SD work.
3. Never perform HTTP connect/read/write while holding `sdMutex`.
4. Never use global `myFile`, `tempFile`, `myFile1`, or `userFile` in SyncManager.
5. Existing main-loop SD functions should be wrapped gradually with helper macros/functions.
6. Temp filenames must be unique by module. Avoid shared `/temp.csv` once concurrency is introduced.

Example helper concept:

```cpp
bool SdLock(uint32_t timeoutMs);
void SdUnlock();
```

## 11. Network behavior

SyncManager should use outbound HTTP client requests.

Timeout requirements:

| Operation | Recommended timeout |
|---|---|
| Wi-Fi connect/reconnect attempt | 5-10 seconds, non-blocking in sync task |
| TCP connect | 3-5 seconds |
| HTTP upload write chunk | bounded, retry if stalled |
| HTTP response wait | 5-10 seconds |
| file download | bounded by per-chunk progress timeout |

Failures must move to retry/backoff, not reboot.

## 12. Retry/backoff

Recommended backoff:

```text
first failure: 30 seconds
second failure: 60 seconds
third failure: 2 minutes
then: cap at 15 minutes
```

Reset backoff after successful sync.

Do not continuously hammer server.

## 13. Server-to-device file import

Safe import sequence:

```text
GET update metadata
  |
download to /SYNC/<file>.download.tmp
  |
validate file type, size, CSV columns, required active rows
  |
write /SYNC/<file>.validated.tmp
  |
backup current file if practical
  |
replace target file
  |
reload counters/permissions if needed
  |
ACK server
```

If validation fails, keep current valid file.

Never delete existing config before the new file is fully downloaded and validated.

## 14. Watchdog plan

Add watchdog after sync task is stable.

Recommended approach:

- main loop feeds watchdog only after critical periodic operations run
- SyncManager task has its own progress/heartbeat
- network offline is not a watchdog fault
- sync timeout moves to retry state
- if watchdog reset occurs, log recovery event after boot

Persistent event example:

```text
SYSTEM_RECOVERY,WATCHDOG,previous_state=SYNC_UPLOAD
```

## 15. Diagnostics

Add optional production diagnostics:

- Wi-Fi connect/disconnect count
- sync started
- upload started/success/failure
- download started/success/failure
- SD lock timeout
- SD open/write/rename failure
- ESP free heap
- ESP min free heap
- SyncManager stack high-water mark

Debug logging must be disableable to avoid SD wear.

## 16. Implementation sequence

Recommended code implementation order:

1. Add sync config constants and `/SYNC` folder creation.
2. Add SD mutex helper functions.
3. Wrap only the most critical SD writes first:
   - `Log_Report()`
   - `LogActivity()`
   - `Log_DLS()`
   - `Log_ULS()`
4. Add persistent sync state file helpers.
5. Add SyncManager task skeleton with no upload yet.
6. Add pending discovery for `/FR.csv` and `/ActLog.csv`.
7. Add outbound upload for append logs with server ACK.
8. Add file version/hash for config files.
9. Add server-to-device download/import.
10. Add watchdog and diagnostics.

## 17. First code milestone

The first safe code milestone should not upload data yet.

It should only:

- create `/SYNC`
- create `sdMutex`
- create `syncEvent`
- start a low-priority SyncManager task
- log/track state without touching existing CSV formats
- prove sampling still runs normally

