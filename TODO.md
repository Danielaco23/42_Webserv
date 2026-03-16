
# 42_Webserv
ServWeb

1. Parsing de config [👤 Dgomez-l]
	
	Responsable de leer el .conf y convertirlo en una estructura C++ usable.

	Incluye:

	- Lexer/parser del archivo (bloques server {}, location {}, etc.).
	- Validación de parámetros (puertos, rutas, error pages, client_max_body_size…).
	- Estructuras ServerConfig, LocationConfig, etc. que luego usará el core del servidor.
	- Ideal para 1 persona que disfrute de parsing y diseño de clases.

2. Core del servidor (sockets + poll/select) [👤 owmarqui]
	Es el corazón: aceptar conexiones, gestionar el loop de eventos y enviar/recibir datos.

	Incluye:

	- Creación de sockets de escucha para cada server/puerto de la config.
	- Uso de poll() o select() no bloqueante para todos los sockets.
	- Gestión del estado de cada cliente (recibiendo request, procesando, enviando response).
	- Enrutado de la request a la LocationConfig correcta y construcción de la respuesta (estáticos, 	errores, redirects…).

3. HTTP + CGI + uploads [👤 pmendez-]
	Aquí metes toda la lógica de protocolo HTTP y la parte dinámica (CGI).

	Incluye:

	- Parser de request HTTP (request line, headers, body) y generación de responses con status codes correctos.
	- Soporte de métodos GET/POST/DELETE (y los que pidan en vuestro subject).
	- Manejo de error pages personalizadas y por defecto.
	- Implementación de CGI (montar entorno, execve del script, leer su salida, mapear errores a 5xx).
	- Lógica de subida de archivos (ruta de upload, límites de tamaño, etc.).

	Perfecto para alguien que quiera pelearse con HTTP y procesos.

