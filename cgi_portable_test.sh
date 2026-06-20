#!/usr/bin/env bash

set -u

BASE_URL="${BASE_URL:-http://127.0.0.1:8080}"
CGI_PATH="${CGI_PATH:-/cgi-bin/cgi_tester_portable.py}"
AUTO_START="${AUTO_START:-1}"

PASS=0
FAIL=0
SERVER_STARTED_BY_SCRIPT=0
SERVER_PID=""

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

cleanup() {
  if [[ "$SERVER_STARTED_BY_SCRIPT" -eq 1 && -n "$SERVER_PID" ]]; then
    kill "$SERVER_PID" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

extract_status() {
  printf '%s\n' "$1" | awk 'NR==1{gsub("\r", "", $0); print $2}'
}

split_body() {
  awk 'BEGIN{found=0} /^\r?$/{found=1; next} {if(found) print}'
}

assert_contains() {
  local haystack="$1"
  local needle="$2"
  local msg="$3"

  if printf '%s' "$haystack" | grep -Fq "$needle"; then
    echo -e "${GREEN}PASS${NC} - $msg"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}FAIL${NC} - $msg (missing: $needle)"
    FAIL=$((FAIL + 1))
  fi
}

assert_status() {
  local response="$1"
  local expected="$2"
  local name="$3"
  local status
  status="$(extract_status "$response")"

  if [[ "$status" == "$expected" ]]; then
    echo -e "${GREEN}PASS${NC} - $name (status=$status)"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}FAIL${NC} - $name (expected=$expected got=${status:-none})"
    FAIL=$((FAIL + 1))
  fi
}

ensure_server() {
  if pgrep -x webserv >/dev/null 2>&1; then
    return
  fi

  if [[ "$AUTO_START" != "1" ]]; then
    echo -e "${RED}webserv no está corriendo y AUTO_START=0${NC}"
    exit 1
  fi

  if [[ ! -x ./webserv ]]; then
    echo -e "${YELLOW}Compilando webserv...${NC}"
    make >/tmp/webserv_make.log 2>&1 || {
      echo -e "${RED}No se pudo compilar webserv (ver /tmp/webserv_make.log)${NC}"
      exit 1
    }
  fi

  echo -e "${YELLOW}Levantando webserv automáticamente...${NC}"
  ./webserv >/tmp/webserv_cgi_tests.log 2>&1 &
  SERVER_PID=$!
  SERVER_STARTED_BY_SCRIPT=1
  sleep 1
}

run_tests() {
  local endpoint="${BASE_URL}${CGI_PATH}"

  echo -e "${YELLOW}Running CGI tests on ${endpoint}${NC}"

  local response body start_ts end_ts elapsed

  echo ""
  echo "=== GET ==="
  response="$(curl -i -s "${endpoint}?foo=bar&n=1")"
  assert_status "$response" "200" "GET status"
  body="$(printf '%s\n' "$response" | split_body)"
  assert_contains "$body" '"method": "GET"' "GET method echo"
  assert_contains "$body" '"foo": "bar"' "GET query echo"

  echo ""
  echo "=== POST ==="
  response="$(curl -i -s -X POST -H 'Content-Type: application/json' -d '{"hola":1}' "${endpoint}?mode=post")"
  assert_status "$response" "200" "POST status"
  body="$(printf '%s\n' "$response" | split_body)"
  assert_contains "$body" '"method": "POST"' "POST method echo"
  assert_contains "$body" '"content_length": 10' "POST content length echo"
  assert_contains "$body" '"body_size": 10' "POST body size echo"

  echo ""
  echo "=== DELETE ==="
  response="$(curl -i -s -X DELETE "${endpoint}?mode=delete")"
  assert_status "$response" "200" "DELETE status"
  body="$(printf '%s\n' "$response" | split_body)"
  assert_contains "$body" '"method": "DELETE"' "DELETE method echo"

  echo ""
  echo "=== HEAD ==="
  response="$(curl -i -s -X HEAD "${endpoint}")"
  assert_status "$response" "405" "HEAD status"

  echo ""
  echo "=== METHOD NOT ALLOWED ==="
  response="$(curl -i -s -X PATCH "${endpoint}")"
  assert_status "$response" "405" "PATCH status"

  echo ""
  echo "=== SLEEP (latency) ==="
  start_ts="$(date +%s)"
  response="$(curl -i -s "${endpoint}?sleep=2")"
  end_ts="$(date +%s)"
  elapsed=$((end_ts - start_ts))
  assert_status "$response" "200" "SLEEP status"
  if [[ "$elapsed" -ge 2 ]]; then
    echo -e "${GREEN}PASS${NC} - sleep delay (elapsed=${elapsed}s)"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}FAIL${NC} - sleep delay too short (elapsed=${elapsed}s)"
    FAIL=$((FAIL + 1))
  fi
}

ensure_server
run_tests

echo ""
echo "=== RESUMEN CGI ==="
echo -e "${GREEN}PASS: $PASS${NC}"
echo -e "${RED}FAIL: $FAIL${NC}"

if [[ "$FAIL" -eq 0 ]]; then
  echo -e "${GREEN}✓ Todas las pruebas CGI pasaron${NC}"
  exit 0
fi

echo -e "${RED}✗ Hay pruebas CGI fallidas${NC}"
exit 1
