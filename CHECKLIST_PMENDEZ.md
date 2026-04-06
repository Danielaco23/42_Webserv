# Checklist: HTTP + CGI + Uploads (pmendez-)

## ✅ YA FUNCIONA

- [x] GET de archivos estáticos
- [x] POST de uploads (multipart/form-data)
- [x] DELETE de archivos
- [x] Error pages personalizadas con template
- [x] CGI: detecta ruta /cgi-bin y ejecuta scripts
- [x] CGI: pasa REQUEST_METHOD al script
- [x] CGI: pasa QUERY_STRING (GET)
- [x] CGI: pasa CONTENT_LENGTH y body (POST)
- [x] Parser request line (método, ruta, versión)
- [x] Lectura completa de headers + body

## ❌ FALTA O NO FUNCIONA BIEN

- [x] HTTP/1.1 obligatorio, rechazar otras versiones (devolver 505)
- [x] Content-Length obligatorio en POST, si no → 411
- [x] Body > límite → 413 Payload Too Large
- [x] Métodos no permitidos → 405 Method Not Allowed + header Allow
- [x] CGI timeout (script colgado → 504)
- [x] CGI error interno (script falla) → 502 Bad Gateway
- [x] Upload: sanitizar nombres (bloquear ../, chars raros)
- [x] Host header obligatorio en HTTP/1.1 → 400 si falta
- [ ] Status codes coherentes en todos los casos

## CÓMO DEMOSTRAR EN DEFENSA

Necesitas 3+1 comandos curl que pruebes en directo:
1. GET estático OK → 200
2. POST upload OK → 200
3. GET /cgi-bin/echo.py?test=1 → 200 con salida CGI
4. POST /cgi-bin/echo.py con body → 200 con body echo
5. DELETE archivo → 200 o 404
6. ERROR: método no permitido → 405
7. ERROR: archivo no existe → 404
8. ERROR: versión HTTP mala → 505 (si lo implementas)
9. ERROR: CGI timeout (si lo implementas)

## PRÓXIMOS PASOS

### FASE 1: VALIDACIONES BÁSICAS (CRÍTICO)
Añadir en [src/webManager/webChecker.cpp](src/webManager/webChecker.cpp):
- [x] Host header obligatorio (ya hecho)
- [x] HTTP/1.1 obligatorio (ya hecho)
- [x] Headers terminados correctamente (ya hecho)
- [x] Content-Length en POST (ya hecho)

### FASE 2: CGI ROBUSTO (IMPORTANTE)
Mejorar en [src/webManager/handleCgi.cpp](src/webManager/handleCgi.cpp):
- [x] Timeout: kill script si tarda >5s
- [ ] Capturar stderr del script
- [x] Mapear exit code a 5xx claros

### FASE 3: UPLOAD SEGURO (IMPORTANTE)
Mejorar en [src/webManager/Upload.cpp](src/webManager/Upload.cpp):
- [x] Sanitizar filename (bloquear ..)
- [ ] Validar extensión si lo pide
- [x] Límite de tamaño real

### FASE 4: TESTS + DOCUMENTACIÓN (MEDIO)
- [x] Script de pruebas curl documentado
- [x] Guía de casos de error esperados
- [ ] README actualizado

---

**Estado actual**: Infraestructura base funciona. Faltan validaciones y edges.
**Tiempo estimado**: 2-3 horas más para tenerlo sólido.
