import os
import sys

method = os.environ.get("REQUEST_METHOD", "GET")
query = os.environ.get("QUERY_STRING", "")
content_length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
body = ""
if content_length > 0:
    body = sys.stdin.read(content_length)

if method == "GET":
    payload = "CGI GET OK\nquery=" + query + "\n"
elif method == "POST":
    payload = "CGI POST OK\nbody=" + body + "\n"
elif method == "DELETE":
    payload = "CGI DELETE OK\n"
else:
    payload = "Method not allowed in CGI: " + method + "\n"

print("Status: 200 OK")
print("Content-Type: text/plain; charset=UTF-8")
print("")
print(payload)
