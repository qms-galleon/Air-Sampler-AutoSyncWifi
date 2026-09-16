# JSON listener setup

Run on the server PC:

```powershell
python -m pip install Flask
$env:AIR_SAMPLER_PORT = '5002'
python json_sync_listener.py
```

Set SD `/SERVCRED.csv` to `SERVER_LAN_IP,5002` with your actual server IP.
Allow inbound TCP 5002. Flash the updated ESP32. Run this listener instead of
the older listener on that port. `0.0.0.0` is the server bind address, not the
address to enter in the device.

ESP32 initiates `POST /sync_device_esp_f3_push/G00250001` with JSON:

```json
{
  "serial_number": "G00250001",
  "device_id": "G00250001",
  "firmware": "2.0.1",
  "records": [{
    "record_type": "LOCATION",
    "record_id": "G00250001-LOCATION-00000002",
    "source_file": "/LOC.csv",
    "source_line": 2,
    "fields": ["2", "Room A", "Active"],
    "csv": "2,Room A,Active"
  }]
}
```

`fields` contains JSON strings in source column order. `csv` retains the original
row for compatibility. Serial is firmware `deviceID`; URL and body must match.

Records persist in `json_sync.sqlite3`, table `records`, uniquely identified by
`(serial_number, record_id)`. `payload_json` contains the complete record.
Identical retries are acknowledged as duplicates; different data for an existing
ID returns 409. Only HTTP 200 plus the matching `X-Ack-Record-Id` header lets the
ESP32 mark `/SYNC/pending.csv` UPLOADED. Older listeners must implement that
header after durable storage to work with this firmware.

Discovery still uses existing fresh-row cursors and runs while sampling is idle.
First baseline skips history. Line cursors detect appended rows, not changes to
existing rows or replacement of one-line files such as DLS.csv. Full change
tracking for rewritten files is not part of this transport update. Credential
files remain excluded. The listener is for LAN testing, without TLS or device
authentication.

After baselining, complete a new sample, verify its serial and JSON in SQLite,
then disconnect/reconnect Wi-Fi and confirm retries do not duplicate records.
Firmware compilation and ESP32 hardware testing are still required.
