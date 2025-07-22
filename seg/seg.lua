local https = require("ssl.https")
local http = require("socket.http")
local ltn12 = require("ltn12")

local target_url = arg[1]
local notify_url = arg[2] -- URL para avisar si detecta problema

if not target_url or not notify_url then
  print("Uso: lua security_check.lua <target_url> <notify_url>")
  os.exit(1)
end

local function detectar_problema(contenido)
  -- Aquí chequeas patrones sospechosos (ejemplo)
  if contenido:find("virus_signature") or contenido:find("malware") then
    return true
  end
  return false
end

local function notificar_aviso(msg)
  local body = "alert=" .. msg
  local response_body = {}
  local res, code = https.request{
    url = notify_url,
    method = "POST",
    headers = {
      ["Content-Type"] = "application/x-www-form-urlencoded",
      ["Content-Length"] = tostring(#body)
    },
    source = ltn12.source.string(body),
    sink = ltn12.sink.table(response_body),
    protocol = "tlsv1_2"
  }
  if code == 200 then
    print("Servidor notificado OK")
  else
    print("Error notificando servidor: ".. tostring(code))
  end
end

local function fetch_url(url)
  local body = {}
  local res, code, headers, status

  if url:find("^https") then
    res, code, headers, status = https.request{
      url = url,
      sink = ltn12.sink.table(body),
      protocol = "tlsv1_2"
    }
  elseif url:find("^http") then
    res, code, headers, status = http.request{
      url = url,
      sink = ltn12.sink.table(body)
    }
  elseif url:find("^rtmp") then
    -- No se puede procesar en este script, solo avisamos
    notificar_aviso("Intento de conexión RTMP detectado: ".. url)
    return nil, "RTMP no soportado"
  else
    return nil, "Protocolo no soportado"
  end

  if not res then
    notificar_aviso("Error conexión: ".. tostring(code) .. " a ".. url)
    return nil, "Error al conectar"
  end

  local contenido = table.concat(body)
  if detectar_problema(contenido) then
    notificar_aviso("Respuesta sospechosa detectada en: ".. url)
    return nil, "Virus detectado"
  end

  return code, nil
end

local code, err = fetch_url(target_url)
if err then
  print("Chequeo falló: ".. err)
  os.exit(1)
else
  print("Chequeo OK, código HTTP: ".. tostring(code))
end
