#!/usr/bin/env python3
import hashlib
import json
import os
import sys
import time
import urllib.parse


def parse_content_length():
    raw = os.environ.get("CONTENT_LENGTH", "0")
    try:
        value = int(raw)
    except (TypeError, ValueError):
        return 0
    return value if value >= 0 else 0


def read_body():
    content_length = parse_content_length()
    if content_length == 0:
        return ""
    return sys.stdin.read(content_length)


def parse_query_string(raw_query):
    parsed = urllib.parse.parse_qs(raw_query, keep_blank_values=True)
    flat = {}
    for key, values in parsed.items():
        if len(values) == 1:
            flat[key] = values[0]
        else:
            flat[key] = values
    return flat


def collect_http_headers():
    headers = {}
    for key, value in os.environ.items():
        if not key.startswith("HTTP_"):
            continue
        name = key[5:].replace("_", "-")
        headers[name] = value
    if "CONTENT_TYPE" in os.environ:
        headers["CONTENT-TYPE"] = os.environ["CONTENT_TYPE"]
    if "CONTENT_LENGTH" in os.environ:
        headers["CONTENT-LENGTH"] = os.environ["CONTENT_LENGTH"]
    return dict(sorted(headers.items()))


def build_status(method):
    allowed = {"GET", "POST", "DELETE", "HEAD"}
    if method not in allowed:
        return 405, "Method Not Allowed"
    return 200, "OK"


def main():
    started_at = time.time()
    method = os.environ.get("REQUEST_METHOD", "GET")
    query_string = os.environ.get("QUERY_STRING", "")
    query = parse_query_string(query_string)

    sleep_secs = 0
    if "sleep" in query:
        try:
            sleep_secs = max(0, min(int(query["sleep"]), 8))
        except (TypeError, ValueError):
            sleep_secs = 0
    if sleep_secs:
        time.sleep(sleep_secs)

    body = read_body()
    status_code, status_text = build_status(method)
    ok = status_code == 200

    payload = {
        "ok": ok,
        "tester": "cgi_tester_portable",
        "status": {
            "code": status_code,
            "text": status_text,
        },
        "request": {
            "method": method,
            "script_name": os.environ.get("SCRIPT_NAME", ""),
            "path_info": os.environ.get("PATH_INFO", ""),
            "query_string": query_string,
            "query": query,
            "headers": collect_http_headers(),
            "content_type": os.environ.get("CONTENT_TYPE", ""),
            "content_length": parse_content_length(),
            "body": body,
            "body_size": len(body),
            "body_sha256": hashlib.sha256(body.encode("utf-8", "replace")).hexdigest(),
        },
        "cgi_env": {
            "gateway_interface": os.environ.get("GATEWAY_INTERFACE", ""),
            "server_protocol": os.environ.get("SERVER_PROTOCOL", ""),
            "server_software": os.environ.get("SERVER_SOFTWARE", ""),
        },
        "timing_ms": int((time.time() - started_at) * 1000),
    }

    serialized = json.dumps(payload, ensure_ascii=False)
    print(f"Status: {status_code} {status_text}")
    print("Content-Type: application/json; charset=UTF-8")
    print(f"Content-Length: {len(serialized.encode('utf-8'))}")
    print("")
    if method != "HEAD":
        print(serialized)


if __name__ == "__main__":
    main()