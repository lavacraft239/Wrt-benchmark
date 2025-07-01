import re
import sys

def validar_url(url):
    # Patrón simple para validar URLs http, https y rtmp
    patron = re.compile(r'^(http|https|rtmp)://[^\s]+$')
    return bool(patron.match(url))

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Uso: python validar_url.py <URL>")
        sys.exit(1)

    url = sys.argv[1]
    if validar_url(url):
        print("URL válida")
        sys.exit(0)
    else:
        print("URL inválida")
        sys.exit(1)
