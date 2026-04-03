# Guia Rapida: HTTP + CGI + Uploads

Esta guia resume el bloque de trabajo de HTTP + CGI + uploads y como se conecta con el resto del proyecto.

## 1) Mapa rapido de archivos

- `src/main_server.cpp`
  - Arranca el servidor y entra en el loop principal.
- `includes/Server.hpp`
  - Define la clase `Server` y helpers de upload.
- `src/Server.cpp`
  - Recibe conexiones, parsea request basica y decide como responder (GET/POST).
- `src/sendWebPage.cpp`
  - Construye responses HTTP, sirve ficheros estaticos y error pages.
- `src/Upload.cpp`
  - Lee el body de POST y procesa `multipart/form-data` para guardar ficheros.
- `www/index.html`
  - Frontend de subida (form + JavaScript).
- `www/errors/template.html`
  - Plantilla por defecto de error pages.

## 2) Flujo de una request (vision general)

1. El cliente se conecta al socket del servidor.
2. `Server::acceptConnection()` lee la request inicial.
3. Se parsea request line: metodo, ruta, version.
4. Si es `POST`, entra en logica de upload:
   - extrae `Content-Length`
   - lee body
   - extrae boundary
   - guarda archivos
5. Si es `GET`, resuelve ruta fisica y llama a `send_file()`.
6. Si hay error, responde con `send_error_page()`.

## 3) Flujo de upload (paso a paso)

1. En `www/index.html`, el usuario elige archivos.
2. El JS hace `POST /upload` (uno o varios archivos, segun la accion).
3. `src/Server.cpp` detecta `method == "POST"`.
4. `read_request_body()` en `src/Upload.cpp` intenta leer el body completo.
5. `extract_multipart_file()` separa nombre y contenido de cada parte.
6. `save_uploaded_file()` escribe en `www/uploads`.
7. El servidor responde `Files uploaded.`.

## 4) Responsabilidades de tu bloque

### HTTP

- Parseo correcto de request line, headers y body.
- Validar entradas invalidas y responder con status code correcto.
- Soportar al menos `GET`, `POST`, `DELETE`.

### Error handling

- Priorizar error page personalizada si existe.
- Si no existe, usar plantilla por defecto.
- Fallback a texto plano si no se puede abrir la plantilla.

### CGI

- Ejecutar scripts CGI con `fork/execve`.
- Montar entorno (`REQUEST_METHOD`, `QUERY_STRING`, etc.).
- Capturar salida del script y traducir errores a `5xx`.

### Uploads

- Parseo de `multipart/form-data`.
- Guardado en ruta permitida.
- Limites de tamano y validacion de nombre de archivo.

## 5) Problemas frecuentes (y como detectarlos)

- "Me redirige a otra pagina al subir"
  - Causa: submit normal del formulario.
  - Solucion: interceptar `submit` y usar JS (XHR/fetch).

- "Se queda colgado en POST"
  - Causa habitual: body incompleto o lectura bloqueante.
  - Revisar: `Content-Length`, bytes ya leidos en el primer `recv`, timeouts.

- "sale 404 de /favicon.ico"
  - No rompe uploads; es una peticion automatica del navegador.

## 6) Checklist minimo para cerrar tu parte

- [ ] GET/POST/DELETE funcionando.
- [ ] No hay requests colgadas indefinidamente.
- [ ] Uploads uno a uno y multiples funcionando.
- [ ] Error pages por defecto funcionando.
- [ ] CGI ejecuta script simple y maneja error interno.
- [ ] Status codes coherentes en casos felices y errores.

## 7) Casos de prueba rapidos

- GET home:
  - `curl -i http://127.0.0.1:8080/`
- GET inexistente:
  - `curl -i http://127.0.0.1:8080/no-existe`
- POST upload:
  - `curl -i -F "files=@README.md" http://127.0.0.1:8080/upload`
- DELETE (cuando este implementado):
  - `curl -i -X DELETE http://127.0.0.1:8080/uploads/test.txt`

---

Si te atascas, vuelve a esta secuencia: request -> parseo -> routing -> handler -> response.
