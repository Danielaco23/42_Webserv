# Guía Práctica de Pruebas: Tu Parte HTTP + CGI + Uploads

## Paso 0: Compilar

```bash
make clean && make -j2
```

## Paso 1: Arrancar servidor

```bash
./webserv
```

Debería decir "Server listening on port 8080".

## Paso 2: Pruebas Individuales (Copia y pega en otra terminal)

### GET estáticos
```bash
curl -i http://127.0.0.1:8080/
# Esperado: HTTP/1.1 200 OK
```

### GET no existe
```bash
curl -i http://127.0.0.1:8080/no-existe
# Esperado: HTTP/1.1 404 Not Found
```

### POST uploads
```bash
echo "test content" > /tmp/test.txt
curl -i -F "files=@/tmp/test.txt" http://127.0.0.1:8080/upload
# Esperado: HTTP/1.1 200 OK
```

### CGI GET con query
```bash
curl -i "http://127.0.0.1:8080/cgi-bin/echo.py?name=paula&age=25"
# Esperado: HTTP/1.1 200 OK
# Body contendrá: CGI GET OK y query=name=paula&age=25
```

### CGI POST
```bash
curl -i -X POST -d "usuario=paula&pass=secret" http://127.0.0.1:8080/cgi-bin/echo.py
# Esperado: HTTP/1.1 200 OK
# Body contendrá: CGI POST OK y el body recibido
```

### CGI DELETE
```bash
curl -i -X DELETE http://127.0.0.1:8080/cgi-bin/echo.py
# Esperado: HTTP/1.1 200 OK
# Body contendrá: CGI DELETE OK
```

### CGI Counter (GET actual)
```bash
curl -i http://127.0.0.1:8080/cgi-bin/counter.py
# Esperado: HTTP/1.1 200 OK
# Body: JSON con contador actual
```

### CGI Counter (POST incrementa)
```bash
curl -i -X POST http://127.0.0.1:8080/cgi-bin/counter.py
# Esperado: HTTP/1.1 200 OK
# Body: JSON con contador incrementado
```

### CGI Counter (DELETE reinicia)
```bash
curl -i -X DELETE http://127.0.0.1:8080/cgi-bin/counter.py
# Esperado: HTTP/1.1 200 OK
# Body: JSON con contador en 0
```

### DELETE archivo
```bash
curl -i -X DELETE http://127.0.0.1:8080/uploads/test.txt
# Esperado: HTTP/1.1 204 No Content (si existía) o 404 (si no existe)
```

### Error: POST sin Content-Length (nuevo)
```bash
curl -i -X POST --data "x=1" -H "Transfer-Encoding:" -H "Content-Length:" http://127.0.0.1:8080/upload
# Esperado: HTTP/1.1 411 Length Required
```

### Error: Método no permitido en CGI (nuevo)
```bash
curl -i -X PATCH http://127.0.0.1:8080/cgi-bin/echo.py
# Esperado: HTTP/1.1 405 Method Not Allowed + header "Allow: GET, POST, DELETE"
```

### CGI Timeout (nuevo - espera 6 segundos)
```bash
# Primero crea un script lento:
echo '#!/bin/bash
sleep 10
echo "Status: 200 OK"
echo "Content-Type: text/plain"
echo ""
echo "OK"' > cgi-bin/slow.sh
chmod +x cgi-bin/slow.sh

# Luego prueba:
curl -i --max-time 10 http://127.0.0.1:8080/cgi-bin/slow.sh
# Esperado: HTTP/1.1 504 Gateway Timeout (porque timeout es 5s)
```

## Paso 3: Resumen de Qué Prueba Cada Cosa

| Comando | Prueba |
|---------|--------|
| GET / | Estáticos, GET normal |
| GET /no-existe | 404, manejo errores |
| POST /upload | Uploads, multipart |
| GET /cgi-bin/echo.py?... | CGI GET, QUERY_STRING |
| POST /cgi-bin/echo.py | CGI POST, body, stdin |
| DELETE /cgi-bin/echo.py | CGI DELETE, REQUEST_METHOD |
| GET /cgi-bin/counter.py | CGI GET, estado persistente |
| POST /cgi-bin/counter.py | CGI POST, modifica estado |
| DELETE /cgi-bin/counter.py | CGI DELETE, reinicia estado |
| DELETE /uploads/test | DELETE, filesystem |
| POST sin Content-Length | 411 validación HTTP |
| PATCH /cgi-bin/... | 405 método no permitido |
| GET /cgi-bin/slow.sh | 504 timeout CGI |

## Paso 4: Checklist Para Defensa

Cuando valores te pidan demo:

1. ✅ Servidor arranca
2. ✅ GET estático devuelve 200
3. ✅ GET inexistente devuelve 404
4. ✅ POST upload funciona (multipart)
5. ✅ DELETE archivo funciona (204 o 404)
6. ✅ CGI ejecuta script en /cgi-bin/
7. ✅ CGI recibe REQUEST_METHOD
8. ✅ CGI recibe QUERY_STRING (GET)
9. ✅ CGI recibe body (POST)
10. ✅ Error pages personalizadas
11. ✅ Validaciones HTTP (Host, HTTP/1.1, Content-Length)
12. ✅ Timeout CGI (504 después de 5s)

## Notas Finales

- El timeout está en [src/webManager/handleCgi.cpp](src/webManager/handleCgi.cpp) (5 segundos).
- El validación de Content-Length está en [src/webManager/handlePost.cpp](src/webManager/handlePost.cpp).
- El método 405 está en [src/webManager/handleCgi.cpp](src/webManager/handleCgi.cpp).
- Si algo no funciona, mira logs con `make clean && make` para ver errores de compilación.

---

**Si todo esto te funciona, has cubierto tu parte del subject completa.**
