#!/usr/bin/env python3
"""
Contador CGI simple.
GET: devuelve el contador actual.
POST: incrementa un contador en archivo temporal.
"""

import os
import sys
import json

method = os.environ.get("REQUEST_METHOD", "GET")
counter_file = "/tmp/webserv_counter.txt"

# Inicializar contador si no existe
if not os.path.exists(counter_file):
    with open(counter_file, "w") as f:
        f.write("0")

if method == "GET":
    with open(counter_file, "r") as f:
        count = int(f.read() or "0")
    
    response = {
        "status": "ok",
        "counter": count,
        "message": "Current counter value"
    }
    
    print("Status: 200 OK")
    print("Content-Type: application/json; charset=UTF-8")
    print("")
    print(json.dumps(response))

elif method == "POST":
    with open(counter_file, "r") as f:
        count = int(f.read() or "0")
    
    count += 1
    
    with open(counter_file, "w") as f:
        f.write(str(count))
    
    response = {
        "status": "ok",
        "counter": count,
        "message": "Counter incremented"
    }
    
    print("Status: 200 OK")
    print("Content-Type: application/json; charset=UTF-8")
    print("")
    print(json.dumps(response))

elif method == "DELETE":
    with open(counter_file, "w") as f:
        f.write("0")
    
    response = {
        "status": "ok",
        "counter": 0,
        "message": "Counter reset"
    }
    
    print("Status: 200 OK")
    print("Content-Type: application/json; charset=UTF-8")
    print("")
    print(json.dumps(response))

else:
    print("Status: 405 Method Not Allowed")
    print("Content-Type: text/plain")
    print("")
    print("Method not supported")
