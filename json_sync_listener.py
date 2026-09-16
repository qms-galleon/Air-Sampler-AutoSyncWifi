"""Run: python json_sync_listener.py (Flask required)."""
import json
import os
import re
import sqlite3
from pathlib import Path

from flask import Flask, jsonify, request

app = Flask(__name__)
app.config['MAX_CONTENT_LENGTH'] = 65536
app.config['SYNC_DB'] = str(Path(__file__).with_name('json_sync.sqlite3'))
SOURCES = {'/FR.csv', '/ActLog.csv', '/DLS.csv', '/BatParam.csv', '/CAL.csv',
           '/GRP.csv', '/LOC.csv', '/RMK.csv', '/RECP.csv', '/DevInf.csv', '/CompDet.csv'}


@app.get('/api/air-sampler/v1/health')
def health():
    return jsonify(ok=True)


@app.post('/sync_device_esp_f3_push/<serial_number>')
def receive(serial_number):
    data = request.get_json(silent=True)
    if not re.fullmatch(r'[A-Za-z0-9_-]{1,64}', serial_number):
        return jsonify(ok=False, error='Invalid serial number'), 400
    if not isinstance(data, dict) or data.get('serial_number') != serial_number:
        return jsonify(ok=False, error='Body serial_number must match URL'), 400
    if data.get('device_id', serial_number) != serial_number:
        return jsonify(ok=False, error='device_id mismatch'), 400
    records = data.get('records')
    if not isinstance(records, list) or len(records) != 1:
        return jsonify(ok=False, error='Send exactly one record per request'), 400
    record = records[0]
    if (not isinstance(record, dict)
            or not isinstance(record.get('record_id'), str)
            or not re.fullmatch(r'[A-Za-z0-9_-]{1,192}', record['record_id'])
            or record.get('source_file') not in SOURCES
            or not isinstance(record.get('record_type'), str)
            or type(record.get('source_line')) is not int or record['source_line'] < 1
            or not isinstance(record.get('fields'), list)
            or not all(isinstance(v, str) for v in record['fields'])):
        return jsonify(ok=False, error='Invalid record'), 400
    encoded = json.dumps(record, sort_keys=True, separators=(',', ':'), ensure_ascii=False)
    connection = None
    try:
        connection = sqlite3.connect(app.config['SYNC_DB'], timeout=10)
        connection.execute('PRAGMA synchronous=FULL')
        connection.execute('''CREATE TABLE IF NOT EXISTS records (
            serial_number TEXT NOT NULL, record_id TEXT NOT NULL,
            firmware TEXT, payload_json TEXT NOT NULL,
            received_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (serial_number, record_id))''')
        connection.execute('BEGIN IMMEDIATE')
        previous = connection.execute(
            'SELECT payload_json FROM records WHERE serial_number=? AND record_id=?',
            (serial_number, record['record_id'])).fetchone()
        if previous and previous[0] != encoded:
            connection.rollback()
            return jsonify(ok=False, error='Record ID already has different data'), 409
        if not previous:
            connection.execute('INSERT INTO records (serial_number,record_id,firmware,payload_json) VALUES (?,?,?,?)',
                               (serial_number, record['record_id'], str(data.get('firmware', '')), encoded))
        connection.commit()
    except sqlite3.Error:
        app.logger.exception('Record storage failed')
        return jsonify(ok=False, error='Storage failed; retry later'), 503
    finally:
        if connection is not None:
            connection.close()
    response = jsonify(ok=True, serial_number=serial_number,
                       received_ids=[] if previous else [record['record_id']],
                       duplicate_ids=[record['record_id']] if previous else [])
    # Sent only after the transaction is committed, including duplicate retries.
    response.headers['X-Ack-Record-Id'] = record['record_id']
    return response, 200


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=int(os.environ.get('AIR_SAMPLER_PORT', '5002')),
            debug=False, threaded=True, use_reloader=False)
