from datetime import datetime, timezone
from pathlib import Path
import csv
import json
import os
import re

from flask import Flask, jsonify, request


app = Flask(__name__)

BASE_DIR = Path(__file__).resolve().parent
DATA_DIR = BASE_DIR / "server_received"
DATA_DIR.mkdir(exist_ok=True)

HEALTH_ROUTE = "/api/air-sampler/v1/health"
RECORDS_ROUTE = "/api/air-sampler/v1/records"
SERVER_PORT = int(os.environ.get("AIR_SAMPLER_PORT", "5002"))


def utc_now_iso():
    return datetime.now(timezone.utc).isoformat()


def safe_stamp():
    return datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S_%f")


def append_jsonl(path: Path, payload: dict):
    try:
        target_path = path
        f = target_path.open("a", encoding="utf-8")
    except PermissionError:
        target_path = path.with_name(f"{path.stem}_{safe_stamp()}_locked{path.suffix}")
        f = target_path.open("a", encoding="utf-8")

    with f:
        f.write(json.dumps(payload, ensure_ascii=False))
        f.write("\n")


def parse_csv_line(line: str):
    return next(csv.reader([line]))


def append_dict_csv(path: Path, fieldnames: list[str], row: dict):
    target_path = path
    create_file = not target_path.exists()

    try:
        f = target_path.open("a", newline="", encoding="utf-8")
    except PermissionError:
        target_path = path.with_name(f"{path.stem}_{safe_stamp()}_locked{path.suffix}")
        create_file = True
        f = target_path.open("a", newline="", encoding="utf-8")

    with f:
        writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
        if create_file:
            writer.writeheader()
        writer.writerow(row)


def load_existing_record_ids(index_path: Path):
    existing_ids = set()

    index_files = sorted(index_path.parent.glob(f"{index_path.stem}*{index_path.suffix}"))
    for candidate in index_files:
        try:
            with candidate.open("r", newline="", encoding="utf-8") as f:
                reader = csv.DictReader(f)
                for row in reader:
                    existing_ids.add(row.get("record_id", ""))
        except PermissionError:
            print(f"WARNING: {candidate.name} is locked; duplicate check skipped for this file", flush=True)

    return existing_ids


def save_sync_status(device_id: str, record: dict, status: str, server_time: str):
    append_dict_csv(
        DATA_DIR / f"{device_id}_sync_status.csv",
        [
            "server_time_utc",
            "device_id",
            "record_type",
            "record_id",
            "source_file",
            "source_line",
            "status",
        ],
        {
            "server_time_utc": server_time,
            "device_id": device_id,
            "record_type": record.get("record_type", ""),
            "record_id": record.get("record_id", ""),
            "source_file": record.get("source_file", ""),
            "source_line": record.get("source_line", ""),
            "status": status,
        },
    )


def save_sample_record(device_id: str, firmware: str, record: dict, server_time: str):
    values = parse_csv_line(record.get("csv", ""))
    while len(values) < 12:
        values.append("")

    sample_id = ""
    sample_name = values[0]
    if "/" in values[0]:
        sample_id, sample_name = values[0].split("/", 1)

    append_dict_csv(
        DATA_DIR / f"{device_id}_sample_records.csv",
        [
            "server_time_utc",
            "device_id",
            "firmware",
            "record_id",
            "source_line",
            "sample_id",
            "sample_name",
            "user_id",
            "mode",
            "sampling_start",
            "sampling_end",
            "sampling_status",
            "sampling_remark",
            "sampling_location",
            "motor_lts",
            "start_delay",
            "no_of_runs",
            "delay_between_runs",
            "raw_csv",
        ],
        {
            "server_time_utc": server_time,
            "device_id": device_id,
            "firmware": firmware,
            "record_id": record.get("record_id", ""),
            "source_line": record.get("source_line", ""),
            "sample_id": sample_id,
            "sample_name": sample_name,
            "user_id": values[1],
            "mode": values[2],
            "sampling_start": values[3],
            "sampling_end": values[4],
            "sampling_status": values[5],
            "sampling_remark": values[6],
            "sampling_location": values[7],
            "motor_lts": values[8],
            "start_delay": values[9],
            "no_of_runs": values[10],
            "delay_between_runs": values[11],
            "raw_csv": record.get("csv", ""),
        },
    )


def save_audit_record(device_id: str, firmware: str, record: dict, server_time: str):
    values = parse_csv_line(record.get("csv", ""))
    while len(values) < 6:
        values.append("")

    append_dict_csv(
        DATA_DIR / f"{device_id}_audit_records.csv",
        [
            "server_time_utc",
            "device_id",
            "firmware",
            "record_id",
            "source_line",
            "audit_id",
            "user_id",
            "activity",
            "activity_time",
            "audit_details",
            "audit_remark",
            "raw_csv",
        ],
        {
            "server_time_utc": server_time,
            "device_id": device_id,
            "firmware": firmware,
            "record_id": record.get("record_id", ""),
            "source_line": record.get("source_line", ""),
            "audit_id": values[0],
            "user_id": values[1],
            "activity": values[2],
            "activity_time": values[3],
            "audit_details": values[4],
            "audit_remark": values[5],
            "raw_csv": record.get("csv", ""),
        },
    )


def save_generic_record(device_id: str, firmware: str, record: dict, server_time: str):
    """Store fresh non-sample CSV rows without changing their raw format."""
    source_file = str(record.get("source_file", "unknown.csv"))
    source_name = re.sub(r"[^A-Za-z0-9_.-]+", "_", source_file.strip("/")) or "unknown.csv"
    append_dict_csv(
        DATA_DIR / f"{device_id}_{source_name}_records.csv",
        [
            "server_time_utc",
            "device_id",
            "firmware",
            "record_type",
            "record_id",
            "source_file",
            "source_line",
            "raw_csv",
        ],
        {
            "server_time_utc": server_time,
            "device_id": device_id,
            "firmware": firmware,
            "record_type": record.get("record_type", ""),
            "record_id": record.get("record_id", ""),
            "source_file": source_file,
            "source_line": record.get("source_line", ""),
            "raw_csv": record.get("csv", ""),
        },
    )


@app.before_request
def log_request():
    print(
        f"[{utc_now_iso()}] {request.remote_addr} {request.method} {request.path} "
        f"query={dict(request.args)}",
        flush=True,
    )


@app.get("/")
@app.get("/health")
@app.get("/api/health")
@app.get(HEALTH_ROUTE)
def health():
    device_id = request.args.get("device_id", "")
    return jsonify({
        "ok": True,
        "message": "air sampler sync server running",
        "device_id": device_id,
        "server_time_utc": utc_now_iso(),
    }), 200


@app.route("/<path:any_path>", methods=["GET", "POST", "PUT", "PATCH", "DELETE"])
def catch_all(any_path):
    return jsonify({
        "ok": False,
        "error": "unknown route",
        "path": "/" + any_path,
        "expected_health_routes": [
            "/",
            "/health",
            "/api/health",
            HEALTH_ROUTE,
        ],
        "expected_records_route": RECORDS_ROUTE,
    }), 404


@app.post(RECORDS_ROUTE)
@app.post("/sync_device_esp_f3_push/<serial_number>")
def receive_records(serial_number=None):
    """
    Accepts JSON from firmware background sync.

    Expected future JSON shape:
    {
      "device_id": "G00250001",
      "firmware": "2.0.1",
      "records": [
        {
          "record_type": "SAMPLE",
          "record_id": "G00250001-SAMPLE-00000001",
          "source_file": "/FR.csv",
          "source_line": 2,
          "csv": "1/SampleName,user,..."
        }
      ]
    }
    """
    payload = request.get_json(silent=True)
    if payload is None:
        return jsonify({"ok": False, "error": "JSON body required"}), 400

    device_id = str(serial_number or payload.get("device_id", "UNKNOWN")).strip() or "UNKNOWN"
    firmware = str(payload.get("firmware", "")).strip()
    records = payload.get("records", [])
    if not isinstance(records, list):
        return jsonify({"ok": False, "error": "records must be a list"}), 400

    received_ids = []
    duplicate_ids = []
    jsonl_path = DATA_DIR / f"{device_id}_records.jsonl"
    index_path = DATA_DIR / f"{device_id}_record_index.csv"

    existing_ids = load_existing_record_ids(index_path)

    for record in records:
        if not isinstance(record, dict):
            continue

        record_id = str(record.get("record_id", "")).strip()
        if not record_id:
            continue

        if record_id in existing_ids:
            duplicate_ids.append(record_id)
            save_sync_status(device_id, record, "DUPLICATE", utc_now_iso())
            continue

        server_time = utc_now_iso()
        received_ids.append(record_id)
        existing_ids.add(record_id)

        append_dict_csv(
            index_path,
            [
                "server_time_utc",
                "device_id",
                "record_type",
                "record_id",
                "source_file",
                "source_line",
            ],
            {
                "server_time_utc": server_time,
                "device_id": device_id,
                "record_type": record.get("record_type", ""),
                "record_id": record_id,
                "source_file": record.get("source_file", ""),
                "source_line": record.get("source_line", ""),
            },
        )

        append_jsonl(jsonl_path, {
            "server_time_utc": server_time,
            "device_id": device_id,
            "firmware": firmware,
            "record": record,
        })

        record_type = str(record.get("record_type", "")).strip().upper()
        try:
            if record_type == "SAMPLE":
                save_sample_record(device_id, firmware, record, server_time)
            elif record_type == "AUDIT":
                save_audit_record(device_id, firmware, record, server_time)
            else:
                save_generic_record(device_id, firmware, record, server_time)
            save_sync_status(device_id, record, "RECEIVED", server_time)
        except Exception as exc:
            save_sync_status(device_id, record, f"PARSE_ERROR:{exc}", server_time)

    return jsonify({
        "ok": True,
        "received_ids": received_ids,
        "duplicate_ids": duplicate_ids,
        "received_count": len(received_ids),
        "duplicate_count": len(duplicate_ids),
    }), 200


if __name__ == "__main__":
    print("Air Sampler sync server starting", flush=True)
    print(f"Health URL: http://0.0.0.0:{SERVER_PORT}{HEALTH_ROUTE}", flush=True)
    print(f"Records URL: http://0.0.0.0:{SERVER_PORT}{RECORDS_ROUTE}", flush=True)
    app.run(host="0.0.0.0", port=SERVER_PORT, debug=False, threaded=True, use_reloader=False)
