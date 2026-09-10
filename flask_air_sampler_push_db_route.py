"""
Drop-in Flask route for ESP32 background push sync.

Use this in your existing Flask project where these already exist:
- test_sync_device Blueprint
- get_db_connection()

Firmware POST target:
  /sync_device_esp_f3_push/<serial_number>

Firmware JSON body:
{
  "device_id": "G00250001",
  "firmware": "AMS-V01",
  "records": [
    {
      "record_type": "SAMPLE",
      "record_id": "G00250001-SAMPLE-00000012",
      "source_file": "/FR.csv",
      "source_line": 12,
      "csv": "12/S000000012,1111,Single,01/10/2025 16:02:15,01/10/2025 16:03:16,OK,NO_RMK,test,100,0,1,10"
    }
  ]
}
"""

import csv
from datetime import datetime, timedelta
from io import StringIO

from flask import jsonify, request


def parse_csv_line(csv_line):
    return next(csv.reader(StringIO(csv_line or "")))


def parse_device_datetime(value):
    value = (value or "").strip()
    if not value:
        return None
    try:
        parsed = datetime.strptime(value, "%d/%m/%Y %H:%M:%S")
        return parsed - timedelta(hours=5, minutes=30)
    except Exception:
        return value


@test_sync_device.route("/sync_device_esp_f3_push/<serial_number>", methods=["POST"])
def sync_device_from_esp_f3_push(serial_number):
    conn = get_db_connection()
    cur = conn.cursor()

    inserted_counts = {"FR.csv": 0, "ActLog.csv": 0}
    updated_counts = {"FR.csv": 0, "ActLog.csv": 0}
    skipped_counts = {"FR.csv": 0, "ActLog.csv": 0}
    errors = []
    received_ids = []
    duplicate_ids = []

    try:
        payload = request.get_json(silent=True)
        if not payload:
            return jsonify({"error": "JSON body required"}), 400

        records = payload.get("records", [])
        if not isinstance(records, list):
            return jsonify({"error": "records must be list"}), 400

        # Existing device check, same intent as your previous route.
        cur.execute(
            """SELECT unit_name, equipment_id FROM device_config
               WHERE serial_number=%s
               ORDER BY id DESC LIMIT 1""",
            (serial_number,),
        )
        config_row = cur.fetchone()
        if not config_row:
            return jsonify({"error": "Device not found"}), 404

        unit_name = config_row[0] if config_row else "Unit1"
        equipment_id = config_row[1] if config_row else "EQ001"

        for record in records:
            if not isinstance(record, dict):
                continue

            record_type = str(record.get("record_type", "")).strip().upper()
            record_id = str(record.get("record_id", "")).strip()
            source_file = str(record.get("source_file", "")).strip()
            row_data = parse_csv_line(record.get("csv", ""))

            if not record_id:
                skipped_counts["FR.csv" if record_type == "SAMPLE" else "ActLog.csv"] += 1
                continue

            try:
                if record_type == "SAMPLE" or source_file.endswith("FR.csv"):
                    while len(row_data) < 12:
                        row_data.append("")

                    if row_data[0] and "/" in str(row_data[0]):
                        sample_id, sample_name = str(row_data[0]).split("/", 1)
                        sample_id = sample_id.strip() or None
                        sample_name = sample_name.strip() or None
                    else:
                        sample_id = None
                        sample_name = str(row_data[0]).strip() or None

                    if not sample_id:
                        skipped_counts["FR.csv"] += 1
                        continue

                    user_id = str(row_data[1]).strip() or None
                    mode = str(row_data[2]).strip() or None
                    startdttime = parse_device_datetime(str(row_data[3]))
                    enddttime = parse_device_datetime(str(row_data[4]))
                    status = str(row_data[5]).strip() or None
                    remark = str(row_data[6]).strip() or None
                    location_name = str(row_data[7]).strip() or None
                    volume = str(row_data[8]).strip() or None
                    start_delay = str(row_data[9]).strip() or None
                    no_of_runs = str(row_data[10]).strip() or None
                    delay_btw_runs = str(row_data[11]).strip() or None

                    cur.execute(
                        "SELECT COUNT(*) FROM sample_records WHERE serial_number=%s AND sample_id=%s",
                        (serial_number, sample_id),
                    )
                    existing = cur.fetchone()[0] > 0

                    if existing:
                        cur.execute(
                            """
                            UPDATE sample_records SET
                                sample_name=%s, user_id=%s, mode=%s, startdttime=%s,
                                enddttime=%s, status=%s, remark=%s, location_name=%s,
                                volume=%s, start_delay=%s, no_of_runs=%s,
                                delay_btw_runs=%s, header_error_no_of_runs=%s
                            WHERE serial_number=%s AND sample_id=%s
                            """,
                            [
                                sample_name, user_id, mode, startdttime, enddttime,
                                status, remark, location_name, volume, start_delay,
                                no_of_runs, delay_btw_runs, 0, serial_number, sample_id,
                            ],
                        )
                        updated_counts["FR.csv"] += 1
                        duplicate_ids.append(record_id)
                    else:
                        cur.execute(
                            """
                            INSERT INTO sample_records (
                                serial_number, sample_id, sample_name, user_id, mode,
                                startdttime, enddttime, status, remark, location_name,
                                volume, start_delay, no_of_runs, delay_btw_runs,
                                header_error_no_of_runs
                            )
                            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
                            """,
                            [
                                serial_number, sample_id, sample_name, user_id, mode,
                                startdttime, enddttime, status, remark, location_name,
                                volume, start_delay, no_of_runs, delay_btw_runs, 0,
                            ],
                        )
                        inserted_counts["FR.csv"] += 1
                        received_ids.append(record_id)

                elif record_type == "AUDIT" or source_file.endswith("ActLog.csv"):
                    while len(row_data) < 6:
                        row_data.append("")

                    user_id_log = str(row_data[1]).strip() or "NO_USER"
                    audit_type = str(row_data[2]).strip() or None
                    timestamp = str(row_data[3]).strip() or None
                    action_on = str(row_data[4]).strip() or "NA"
                    remark = str(row_data[5]).strip() or "NA"

                    cur.execute(
                        """SELECT COUNT(*) FROM audit_trail
                           WHERE user_id=%s AND audit_type=%s AND timestamp=%s AND device_name=%s""",
                        (user_id_log, audit_type, timestamp, serial_number),
                    )
                    existing = cur.fetchone()[0] > 0

                    if not existing and audit_type:
                        cur.execute(
                            """
                            INSERT INTO audit_trail
                                (user_id, audit_type, action_on, remark, timestamp, device_name)
                            VALUES (%s, %s, %s, %s, %s, %s)
                            """,
                            [user_id_log, audit_type, action_on, remark, timestamp, serial_number],
                        )
                        inserted_counts["ActLog.csv"] += 1
                        received_ids.append(record_id)
                    else:
                        skipped_counts["ActLog.csv"] += 1
                        duplicate_ids.append(record_id)

                conn.commit()

            except Exception as row_error:
                conn.rollback()
                errors.append(f"{record_id}: {str(row_error)[:150]}")

        return jsonify({
            "message": "Background push sync completed",
            "serial_number": serial_number,
            "unit_name": unit_name,
            "equipment_id": equipment_id,
            "inserted": inserted_counts,
            "updated": updated_counts,
            "skipped_duplicates": skipped_counts,
            "received_ids": received_ids,
            "duplicate_ids": duplicate_ids,
            "errors": errors[:10],
        }), 200

    except Exception as exc:
        conn.rollback()
        return jsonify({"error": str(exc)}), 500

    finally:
        cur.close()
        conn.close()
