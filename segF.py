from flask import Flask, request, jsonify, abort
import random
import string
import smtplib

app = Flask(__name__)

# Guardamos tokens aquí (en memoria, para demo)
tokens_usuarios = {}

def generar_token(length=6):
    return ''.join(random.choices(string.ascii_uppercase + string.digits, k=length))

def enviar_email(email, token):
    # Configurá tu SMTP aquí
    SMTP_SERVER = "smtp.tu-email.com"
    SMTP_PORT = 587
    SMTP_USER = "tu-email@ejemplo.com"
    SMTP_PASS = "tu-contraseña"

    mensaje = f"Subject: Tu código de acceso\n\nTu token es: {token}"

    try:
        server = smtplib.SMTP(SMTP_SERVER, SMTP_PORT)
        server.starttls()
        server.login(SMTP_USER, SMTP_PASS)
        server.sendmail(SMTP_USER, email, mensaje)
        server.quit()
        print(f"Email enviado a {email}")
    except Exception as e:
        print(f"Error enviando email: {e}")

@app.route("/request_token", methods=["POST"])
def request_token():
    data = request.json
    email = data.get("email")
    if not email:
        abort(400, "Falta email")

    token = generar_token()
    tokens_usuarios[email] = token
    enviar_email(email, token)
    return jsonify({"message": "Token enviado a tu email"}), 200

@app.route("/protegido", methods=["GET"])
def protegido():
    email = request.headers.get("X-User-Email")
    token = request.headers.get("X-User-Token")

    if not email or not token:
        abort(401, "Faltan credenciales")

    if tokens_usuarios.get(email) == token:
        return jsonify({"message": "Acceso concedido al recurso protegido"})
    else:
        abort(403, "Token inválido")

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=6565, debug=True)
