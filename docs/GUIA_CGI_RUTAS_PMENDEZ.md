# Guia corta: cuando usar CGI y cuando no

## Regla simple

- Usa handlers nativos del servidor para estaticos y uploads.
- Usa CGI solo para rutas dinamicas bajo /cgi-bin/.

## Tu enrutado recomendado

- GET /, /showUploads, ficheros estaticos -> handlers nativos C++.
- POST /upload -> handler nativo C++ de uploads.
- DELETE /uploads/<file> -> handler nativo C++.
- GET/POST/DELETE /cgi-bin/<script>.py -> handler CGI.

## Importante

No tienes que mover tus funciones GET/POST/DELETE actuales.
Solo defines una zona dinamica (cgi-bin) que delega en scripts.

## Pruebas rapidas

- GET CGI:
  curl -i "http://127.0.0.1:8080/cgi-bin/echo.py?name=paula"

- POST CGI:
  curl -i -X POST -d "hola=1" http://127.0.0.1:8080/cgi-bin/echo.py

- DELETE CGI:
  curl -i -X DELETE http://127.0.0.1:8080/cgi-bin/echo.py

## Que hace el servidor en CGI

1. Detecta /cgi-bin/.
2. Prepara variables de entorno (REQUEST_METHOD, QUERY_STRING, CONTENT_LENGTH...).
3. Ejecuta el script con fork/execve.
4. Devuelve la salida del script al cliente.
