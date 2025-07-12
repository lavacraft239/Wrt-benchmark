# wrt - Herramienta de benchmark HTTP

wrt es una herramienta sencilla para realizar pruebas de carga y benchmark sobre URLs HTTP/HTTPS (y RTMP), con múltiples threads, timeout configurable, y generación de resultados en CSV.

---

## Instalación

Instalá globalmente con npm:

```bash
npm install -g wrt


---

Uso

Ejecutá desde la terminal:

wrtJ https://example.com -r 10 -t 30 -w 5000 [--insecure] [-x]

Parámetros

Opción	Descripción	Ejemplo

URL	URL a testear (HTTP, HTTPS o RTMP)	https://google.com
-r	Cantidad de threads (concurrentes)	-r 5
-t	Duración total en segundos	-t 15
-w	Timeout en milisegundos por request	-w 3000
--insecure	Ignorar validación SSL	
-x	Modo simulación sin hacer peticiones reales	
-f	Archivo con lista de URLs (una por línea)	-f urls.txt



---

Ejemplos

# Prueba simple
wrt https://example.com -r 5 -t 10 -w 2000

# Ignorar SSL
wrt https://example.com -r 5 -t 10 -w 2000 --insecure

# Simulación sin requests reales
wrt https://example.com -r 5 -t 10 -w 2000 -x

# Múltiples URLs desde archivo
wrt -f urls.txt -r 3 -t 20 -w 4000


---

Resultados

Los resultados se guardan en wrt_results.csv en el directorio donde se ejecuta el comando. El archivo incluye:

Timestamp

URL

Threads

Duración

Requests realizadas

Errores

Timeouts

Latencia promedio, mínima y máxima (en segundos)



---

Desarrollo

Si querés colaborar o mejorar wrt, podés clonar este repositorio y usar:

npm link

para instalar localmente en modo desarrollo.


---

Licencia

MIT © Santiago
