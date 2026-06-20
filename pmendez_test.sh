#!/bin/bash

# Tester para HTTP + CGI + Uploads (pmendez-)
# Uso:
#   bash test_pmendez.sh
# Requiere servidor corriendo en http://127.0.0.1:8080

set -u

BASE_URL="${BASE_URL:-http://127.0.0.1:8080}"
PASS=0
FAIL=0

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

tmp_dir=$(mktemp -d /tmp/webserv-tests.XXXXXX)
trap 'rm -rf "$tmp_dir"' EXIT

print_section() {
	echo ""
	echo "=== $1 ==="
}

extract_status() {
	awk 'NR==1 {print $2}'
}

run_cmd() {
	local command="$1"
	eval "$command" 2>/dev/null
}

test_case() {
	local name="$1"
	local command="$2"
	local expected_status="$3"

	echo -n "[TEST] $name ... "
	response=$(run_cmd "$command")
	status=$(printf '%s\n' "$response" | extract_status)

	if [ "$status" = "$expected_status" ]; then
		echo -e "${GREEN}PASS${NC} (status: $status)"
		PASS=$((PASS + 1))
	else
		echo -e "${RED}FAIL${NC} (expected: $expected_status, got: ${status:-no-status})"
		printf '%s\n' "$response" | head -n 5
		FAIL=$((FAIL + 1))
	fi
}

test_raw_case() {
	local name="$1"
	local raw_request="$2"
	local expected_status="$3"
	local decoded_request
	decoded_request=$(printf '%b' "$raw_request")

	echo -n "[TEST] $name ... "
	response=$(RAW_REQUEST="$decoded_request" python3 - <<'PY'
import os
import socket

data = os.environ.get("RAW_REQUEST", "").encode("latin1")
s = socket.create_connection(("127.0.0.1", 8080))
s.sendall(data)
s.shutdown(socket.SHUT_WR)
out = b""
while True:
    chunk = s.recv(4096)
    if not chunk:
        break
    out += chunk
print(out.decode("utf-8", "replace"), end="")
PY
)
	status=$(printf '%s\n' "$response" | extract_status)

	if [ "$status" = "$expected_status" ]; then
		echo -e "${GREEN}PASS${NC} (status: $status)"
		PASS=$((PASS + 1))
	else
		echo -e "${RED}FAIL${NC} (expected: $expected_status, got: ${status:-no-status})"
		printf '%s\n' "$response" | head -n 5
		FAIL=$((FAIL + 1))
	fi
}

echo -e "${YELLOW}Running tests against ${BASE_URL}${NC}"

print_section "GET ESTÁTICOS"
test_case "GET /" "curl -i -s '${BASE_URL}/'" "200"
test_case "GET /index.html" "curl -i -s '${BASE_URL}/index.html'" "200"
test_case "GET inexistente" "curl -i -s '${BASE_URL}/no-existe'" "404"

print_section "CGI BÁSICO"
test_case "GET /cgi-bin/echo.py" "curl -i -s '${BASE_URL}/cgi-bin/echo.py'" "200"
test_case "GET /cgi-bin/echo.py?test=1" "curl -i -s '${BASE_URL}/cgi-bin/echo.py?test=1'" "200"
test_case "POST /cgi-bin/echo.py" "curl -i -s -X POST -d 'hola=1' '${BASE_URL}/cgi-bin/echo.py'" "200"
test_case "DELETE /cgi-bin/echo.py" "curl -i -s -X DELETE '${BASE_URL}/cgi-bin/echo.py'" "200"

print_section "CGI COUNTER"
test_case "GET /cgi-bin/counter.py" "curl -i -s '${BASE_URL}/cgi-bin/counter.py'" "200"
test_case "POST /cgi-bin/counter.py" "curl -i -s -X POST '${BASE_URL}/cgi-bin/counter.py'" "200"
test_case "DELETE /cgi-bin/counter.py" "curl -i -s -X DELETE '${BASE_URL}/cgi-bin/counter.py'" "200"

print_section "UPLOADS"
echo 'test content' > "$tmp_dir/test_file.txt"
test_case "POST /upload" "curl -i -s -F 'files=@$tmp_dir/test_file.txt' '${BASE_URL}/upload'" "200"
python3 - <<'PY' > "$tmp_dir/too_big.bin"
print("A" * 1000001, end="")
PY
test_case "POST /upload body > limit" "curl -i -s -F 'files=@$tmp_dir/too_big.bin' '${BASE_URL}/upload'" "413"
test_case "POST /upload filename inseguro" "curl -i -s -F 'files=@$tmp_dir/test_file.txt;filename=../../evil.txt' '${BASE_URL}/upload'" "400"

print_section "ERRORES ESPERADOS"
test_case "DELETE no existe" "curl -i -s -X DELETE '${BASE_URL}/uploads/no-existe'" "404"
test_case "CGI root /cgi-bin" "curl -i -s '${BASE_URL}/cgi-bin'" "200"
test_case "Método no permitido en CGI" "curl -i -s -X PATCH '${BASE_URL}/cgi-bin/echo.py'" "405"

print_section "VALIDACIONES HTTP"
test_raw_case "POST sin Content-Length" "POST /upload HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\nx=1" "411"
test_raw_case "HTTP/1.0 inválido" "GET / HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n" "505"

print_section "CGI TIMEOUT"
if [ ! -f cgi-bin/slow.sh ]; then
	cat > cgi-bin/slow.sh <<'EOF'
#!/bin/bash
sleep 10
echo "Status: 200 OK"
echo "Content-Type: text/plain"
echo ""
echo "OK"
EOF
	chmod +x cgi-bin/slow.sh
fi
test_case "GET /cgi-bin/slow.sh" "curl -i -s --max-time 10 '${BASE_URL}/cgi-bin/slow.sh'" "504"

echo ""
echo "=== RESUMEN ==="
echo -e "${GREEN}PASS: $PASS${NC}"
echo -e "${RED}FAIL: $FAIL${NC}"
echo ""

if [ "$FAIL" -eq 0 ]; then
	echo -e "${GREEN}✓ Todos los tests pasaron${NC}"
	exit 0
fi

echo -e "${RED}✗ Algunos tests fallaron${NC}"
exit 1
