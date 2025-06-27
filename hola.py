import subprocess

programas = {
    "curl": True,
    "git": True,
    "nado": True,   # en vez de nano
    "htop": True,
    "wget": True,
    "python": True,
    "node": True,
    "ls": True,
    "cat": True,
    "wrt": True,
}

def esta_instalado(prog):
    resultado = subprocess.run(f"command -v {prog}", shell=True, stdout=subprocess.DEVNULL)
    return resultado.returncode == 0

def nado_editor(nombre_archivo):
    print(f"--- Editor NADO - Archivo: {nombre_archivo} ---")
    print("Escribí texto. CTRL+D para guardar y salir.\n")
    lineas = []
    try:
        while True:
            linea = input()
            lineas.append(linea)
    except EOFError:
        # Guardar archivo
        try:
            with open(nombre_archivo, "w", encoding="utf-8") as f:
                f.write("\n".join(lineas) + "\n")
            print(f"[✓] Archivo guardado: {nombre_archivo}")
        except Exception as e:
            print(f"[✗] Error guardando archivo: {e}")

def terminal():
    print("Terminal Termux real en Python con nado editor")
    print("Escribí 'salir' para cerrar.\n")

    while True:
        try:
            entrada = input("@$ ").strip()
            if entrada.lower() in ("salir", "exit", "quit"):
                print("Chau!")
                break
            if not entrada:
                continue

            partes = entrada.split()
            comando = partes[0]

            if comando == "nado":
                if len(partes) < 2:
                    print("Uso: nado [archivo]")
                else:
                    nado_editor(partes[1])
                continue

            if comando in programas:
                if not esta_instalado(comando):
                    print(f"[!] '{comando}' no está instalado. Instalalo con: pkg install {comando}")
                    continue
                proceso = subprocess.run(entrada, shell=True)
            else:
                proceso = subprocess.run(entrada, shell=True)

        except KeyboardInterrupt:
            print("\n[!] Cancelado.")
        except Exception as e:
            print(f"[✗] Error: {e}")

if __name__ == "__main__":
    terminal()
