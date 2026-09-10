# ESP32 Air Sampler Sync API Contract

Date: 2026-09-03

Purpose: define the server contract required for automatic ESP32-initiated background synchronization.

Important: the current firmware mainly exposes inbound HTTP endpoints from the ESP32. This contract defines the additional server-side API needed for robust automatic background sync. If an existing backend already provides equivalent endpoints, map these concepts to those endpoints instead of creating conflicting APIs.

## 1. Current observed ESP32 endpoints

The existing firmware currently supports inbound requests to the ESP32, mainly in `OnlineS.ino`.

Observed active endpoints:

- `GET /?files`
- `GET /?Sample-...`
- `GET /?Status`
- `GET /?SampSt`
- `GET /?Abort`
- `GET /?BatST`
- `GET /?Launch`
- `GET /?LOCADD-...`
- `GET /?LOCDIS-...`
- `GET /?REMADD-...`
- `GET /?REMDIS-...`
- `GET /?RECPADD-...`
- `GET /?RECPDIS-...`
- `GET /?USERADD-...`
- `GET /?USERUPLOAD-...`
- `GET /?USERRST-...`
- `GET /?USERDIS-...`
- `GET /?GRPADD-...`
- `GET /?GRPDIS-...`

Older/alternate handler in `hndleWF.ino` includes:

- `GET /file`
- `GET /AUDIT`
- `GET /LOCADD`
- `GET /LOCDIS`

These are device-hosted endpoints. Automatic sync requires server-hosted endpoints that ESP32 can call.

## 2. Required server properties

The server must support:

- idempotent upload
- duplicate detection
- bounded responses
- retry-safe ACK
- server-to-device update discovery
- file download with validation metadata
- ACK for imported server updates

The server must not create duplicate records when the ESP32 retries after a lost response.

## 3. Device identity

Every request should include:

```text
device_id
equipment_id
unit_name
firmware_version
```

Current firmware fields:

- `deviceID`
- `EquipmentId`
- `UnitName`
- `SoftwareVer`

Recommended transport:

- query params for simple GET status calls
- multipart fields or HTTP headers for uploads

Example headers:

```http
X-Device-Id: G00250001
X-Equipment-Id: 250001
X-Unit-Name: UNIT-1
X-Firmware-Version: AMS-V01
```

## 4. Record identity

Each synced record must include stable `record_id`.

Format:

```text
<device_id>-<record_type>-<record_number>
```

Examples:

```text
G00250001-SAMPLE-00000123
G00250001-AUDIT-00000456
G00250001-USER-00000007
```

Server rule:

```text
if record_id does not exist:
  insert
else:
  skip or update only if version is newer
```

## 5. Endpoint: health check

### Request

```http
GET /api/air-sampler/v1/health?device_id=G00250001
```

### Success response

```json
{
  "ok": true,
  "server_time": "2026-09-03T12:00:00+05:30",
  "min_supported_firmware": "AMS-V01"
}
```

### ESP32 behavior

- If success: continue sync.
- If timeout/failure: retry later with backoff.
- Do not block sampling.

## 6. Endpoint: upload records

Use multipart/form-data.

Do not require the ESP32 to load an entire large CSV into RAM.

### Request

```http
POST /api/air-sampler/v1/sync/upload
Content-Type: multipart/form-data; boundary=...
X-Device-Id: G00250001
```

Multipart fields:

```text
device_id
firmware_version
batch_id
record_type
first_record_id
last_record_id
file_name
file_chunk or records_file
```

For phase 1, the ESP32 can send a small batch of CSV rows as a temporary file/part.

### Example CSV payload

For sample records:

```csv
record_id,device_id,record_type,source_file,source_line,payload_csv
G00250001-SAMPLE-00000123,G00250001,SAMPLE,/FR.csv,123,"123/S000000123,user1,Single,..."
```

Alternative: send original CSV fields as normal columns. This is preferred if backend can parse by record type.

### Success response

```json
{
  "ok": true,
  "batch_id": "G00250001-20260903-0001",
  "accepted": 10,
  "duplicates": 2,
  "failed": 0,
  "last_acked_record_id": "G00250001-SAMPLE-00000134",
  "server_sync_token": "opaque-token"
}
```

### Partial success response

```json
{
  "ok": false,
  "retry": true,
  "accepted": 8,
  "duplicates": 0,
  "failed": 2,
  "failed_records": [
    {
      "record_id": "G00250001-SAMPLE-00000133",
      "reason": "invalid timestamp"
    }
  ]
}
```

### ESP32 behavior

- On success: persist ACK state.
- On timeout/lost response: do not mark synced; retry same batch.
- On duplicate response: treat duplicates as already synced.
- On partial failure: persist accepted records, retry failed records only if safe.

## 7. Endpoint: upload file snapshot

For mutable/config files where row-level change tracking is not available.

### Request

```http
POST /api/air-sampler/v1/sync/file
Content-Type: multipart/form-data; boundary=...
X-Device-Id: G00250001
```

Multipart fields:

```text
device_id
file_type
file_name
file_version
file_hash
records_file
```

Example file types:

- `USERS`
- `GROUPS`
- `LOCATIONS`
- `REMARKS`
- `RECIPES`
- `DEVICE_INFO`
- `COMPANY_DETAILS`
- `MOTOR_LIMITS`

### Success response

```json
{
  "ok": true,
  "file_name": "USR.csv",
  "file_hash": "sha256-or-crc32",
  "server_version": "42",
  "duplicates": 0
}
```

## 8. Endpoint: check server updates

### Request

```http
GET /api/air-sampler/v1/sync/updates?device_id=G00250001&known_token=opaque-token
```

### Success response

```json
{
  "ok": true,
  "updates_available": true,
  "server_sync_token": "opaque-token-2",
  "files": [
    {
      "file_type": "USERS",
      "file_name": "USR.csv",
      "version": "43",
      "hash": "crc32-or-sha256",
      "size": 1234,
      "download_url": "/api/air-sampler/v1/sync/download?device_id=G00250001&file=USR.csv&version=43"
    }
  ]
}
```

### ESP32 behavior

- If no updates: return to idle.
- If updates exist: download one at a time.
- Do not delete current local file before validation succeeds.

## 9. Endpoint: download update file

### Request

```http
GET /api/air-sampler/v1/sync/download?device_id=G00250001&file=USR.csv&version=43
```

### Success response

Headers:

```http
Content-Type: text/csv
X-File-Type: USERS
X-File-Name: USR.csv
X-File-Version: 43
X-File-Hash: crc32-or-sha256
X-File-Size: 1234
```

Body:

```csv
...
```

### ESP32 behavior

1. Download to `/SYNC/USR.csv.download.tmp`.
2. Check size limit.
3. Verify hash if provided.
4. Validate CSV columns and required values.
5. Replace `/USR.csv` only after validation.
6. ACK import result.

## 10. Endpoint: ACK imported update

### Request

```http
POST /api/air-sampler/v1/sync/ack
Content-Type: application/json
```

Body:

```json
{
  "device_id": "G00250001",
  "file_name": "USR.csv",
  "file_type": "USERS",
  "version": "43",
  "hash": "crc32-or-sha256",
  "status": "IMPORTED",
  "message": "OK"
}
```

### Failure ACK example

```json
{
  "device_id": "G00250001",
  "file_name": "USR.csv",
  "file_type": "USERS",
  "version": "43",
  "hash": "crc32-or-sha256",
  "status": "REJECTED",
  "message": "CSV validation failed"
}
```

## 11. Error handling contract

Server should return structured errors:

```json
{
  "ok": false,
  "error_code": "DUPLICATE_BATCH",
  "retry": false,
  "message": "Batch already processed"
}
```

Recommended error codes:

| Code | Retry? | Meaning |
|---|---:|---|
| `SERVER_BUSY` | yes | retry with backoff |
| `TIMEOUT` | yes | retry later |
| `BAD_REQUEST` | no | firmware payload invalid |
| `UNAUTHORIZED_DEVICE` | no | device not registered |
| `DUPLICATE_RECORD` | no | record already accepted |
| `DUPLICATE_BATCH` | no | batch already processed |
| `INVALID_FILE` | no | downloaded/uploaded file invalid |
| `UNSUPPORTED_FIRMWARE` | no | firmware too old |

## 12. Timeouts and response sizes

Server responses should be small enough for ESP32 memory.

Recommended:

- JSON response under 2 KB.
- CSV download file size declared before or in headers.
- Upload ACK response includes only summary plus failed IDs.
- No large HTML responses.

## 13. Security notes

Minimum recommended API security:

- register known `device_id`
- use shared API token per device or signed request
- do not put user passwords in URL query strings
- prefer HTTPS if hardware/network supports it
- if HTTPS is not possible, document LAN-only deployment assumption

The current firmware stores passwords in plaintext CSV. This contract does not fix that, but server sync must treat those files as sensitive.

## 14. Compatibility with existing firmware data

Do not require changing existing CSV row formats for phase 1.

Sync metadata should be stored separately under `/SYNC`.

Examples:

- `/SYNC/state.csv`
- `/SYNC/pending.csv`
- `/SYNC/inflight.csv`

This protects existing UI/report parsers from format changes.

## 15. Open questions before implementation

These must be confirmed before final outbound sync code:

1. What is the actual server base URL/IP and port?
2. Does the existing backend already have upload/download endpoints?
3. Does server expect complete CSV files or row batches?
4. Can server deduplicate by `record_id`?
5. Can server return JSON, or must responses be plain text?
6. What is maximum expected file size for `/FR.csv` and `/ActLog.csv`?
7. Should configuration changes from server overwrite local changes, merge, or require conflict handling?
8. Is HTTPS required, or is this LAN-only?

## 16. Minimal phase-1 API

For first implementation, only these endpoints are required:

```text
GET  /api/air-sampler/v1/health
POST /api/air-sampler/v1/sync/upload
GET  /api/air-sampler/v1/sync/updates
GET  /api/air-sampler/v1/sync/download
POST /api/air-sampler/v1/sync/ack
```

If backend cannot be changed yet, implement SyncManager skeleton and persistent state first, then wire endpoints once server behavior is confirmed.

