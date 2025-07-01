from flask import Flask, request, jsonify, abort
import secrets

app = Flask(__name__)

# Diccionario para guardar los tokens activos por IP
active_tokens = {}

# Ruta para obtener un token aleatorio
@app.route("/get-token", methods=["GET"])
def get_token():
    ip = request.remote_addr
    token = secrets.token_hex(16)  # Token aleatorio de 32 caracteres
    active_tokens[ip] = token
    return jsonify({"token": token})

# Ruta protegida con validación de token
@app.route("/protegido", methods=["GET"])
def zona_segura():
    ip = request.remote_addr
    token = request.headers.get("Authorization")
    
    if not token or active_tokens.get(ip) != token:
        abort(403)  # Acceso denegado

    return jsonify({"mensaje": "Acceso permitido desde " + ip})

# Ruta para eliminar token (logout)
@app.route("/logout", methods=["POST"])
def logout():
    ip = request.remote_addr
    active_tokens.pop(ip, None)
    return jsonify({"mensaje": "Sesión cerrada"})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
