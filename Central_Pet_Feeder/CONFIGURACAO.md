# 🚀 Guia de Configuração - Central Pet Feeder Refatorado

## 📋 Pré-requisitos

- ESP32 conectado via USB
- Servidor Mosquitto rodando (com TLS na porta 8883)
- PlatformIO instalado

---

## 1️⃣ Configurar credenciais Wi-Fi e MQTT

Edite o arquivo `include/config.h`:

```cpp
// ========== CONFIGURAÇÕES DE WIFI ==========
#define WIFI_SSID "SEU_WIFI_AQUI"
#define WIFI_PASSWORD "SUA_SENHA_AQUI"

// ========== CONFIGURAÇÕES DE MQTT ==========
#define MQTT_BROKER_HOST "IP_DO_MOSQUITTO"  // Ex: "192.168.1.100"
#define MQTT_BROKER_PORT 8883                // 8883 = TLS, 1883 = sem TLS
#define MQTT_USERNAME "usuario_mqtt"
#define MQTT_PASSWORD "senha_mqtt"
#define MQTT_CLIENT_ID "central_gateway"
```

---

## 2️⃣ Certificado TLS já está configurado! ✅

O certificado CA já foi adicionado em `include/mqtt_cert.h` e está sendo usado automaticamente.

Se você quiser **desabilitar TLS temporariamente** (para testar):

1. Abra `src/main.cpp`
2. Encontre a linha:
   ```cpp
   mqttClient.setTLSCertificate(MQTT_ROOT_CA);
   ```
3. Troque por:
   ```cpp
   mqttClient.setTLSCertificate("");  // Modo inseguro (aceita qualquer certificado)
   ```
4. E altere a porta no `config.h` para `1883`

---

## 3️⃣ Configurar porta serial

No `platformio.ini`, ajuste a porta COM se necessário:

```ini
upload_port = COM3      # Sua porta USB
monitor_port = COM3
```

Para descobrir a porta, use:
```bash
platformio device list
```

---

## 4️⃣ Compilar e fazer upload

### Compilar apenas:
```bash
cd "C:\Users\ALEXANDRE\OneDrive\Área de Trabalho\Projeto Alimentador\distributed-pet-feeder\Central_Pet_Feeder"
platformio run
```

### Upload para ESP32:
```bash
platformio run --target upload
```

### Monitorar Serial:
```bash
platformio device monitor --baud 115200
```

**OU tudo de uma vez:**
```bash
platformio run --target upload && platformio device monitor
```

---

## 5️⃣ Configuração do Mosquitto (servidor)

Seu servidor Mosquitto deve estar rodando com TLS. Exemplo de configuração mínima:

### `/etc/mosquitto/mosquitto.conf`:
```conf
listener 8883
cafile /etc/mosquitto/certs/ca.crt
certfile /etc/mosquitto/certs/server.crt
keyfile /etc/mosquitto/certs/server.key

allow_anonymous false
password_file /etc/mosquitto/passwd

log_dest stdout
log_type all
```

### Criar usuário MQTT:
```bash
sudo mosquitto_passwd -c /etc/mosquitto/passwd petfeeder
# Digite a senha quando solicitado
```

### Reiniciar Mosquitto:
```bash
sudo systemctl restart mosquitto
sudo systemctl status mosquitto
```

---

## 6️⃣ Tópicos MQTT usados

### Publicados pela Central:
- `petfeeder/central/status` - Status da central (online, remotas conectadas)

### Publicados pelas Remotas:
- `petfeeder/remote/1/status` - Status da remota 1 (online/offline)
- `petfeeder/remote/1/data` - Dados da remota 1 (nível de ração, etc)
- `petfeeder/remote/2/status` - Status da remota 2
- `petfeeder/remote/2/data` - Dados da remota 2
- (etc para remotas 3 e 4)

### Comandos enviados para Remotas:
- `petfeeder/remote/1/cmd` - Comandos para remota 1 (configurar refeições, etc)
- `petfeeder/remote/2/cmd` - Comandos para remota 2
- (etc)

### Formato dos Payloads:

**Status:**
```json
{
  "online": true,
  "timestamp": 12345
}
```

**Data (telemetria):**
```json
{
  "feed_level": "OK",
  "timestamp": 12345
}
```

**Comando de configuração de refeição:**
```json
{
  "cmd": "CONFIG_MEAL",
  "meal": 0,
  "hour": 8,
  "minute": 30,
  "quantity": 100,
  "timestamp": 12345
}
```

---

## 7️⃣ Testar conexão MQTT

### Teste 1: Ouvir mensagens da central
```bash
mosquitto_sub -h IP_DO_MOSQUITTO -p 8883 \
  --cafile ca.crt \
  -u petfeeder -P senha_mqtt \
  -t "petfeeder/central/#" -v
```

### Teste 2: Simular uma remota enviando status
```bash
mosquitto_pub -h IP_DO_MOSQUITTO -p 8883 \
  --cafile ca.crt \
  -u petfeeder -P senha_mqtt \
  -t "petfeeder/remote/1/status" \
  -m '{"online":true,"timestamp":12345}'
```

---

## 8️⃣ Estrutura de Menus no LCD

### Tela inicial (STATUS_GATEWAY)
```
    GATEWAY CENTRAL
[V] ONLINE
Remotas: 2/4
> Configurar
```

### Lista de remotas (REMOTE_LIST)
```
      Remotas
> Remota 1: OK
  Remota 2: OFF
  Voltar
```

### Configurar refeições (MEAL_CONFIG)
```
      Remota 1
> R1 08:00 100g
  R2 12:00 150g
  R3 18:00 100g
```

### Editar horário (EDIT_TIME)
```
  Editar Horario

   [08]:[00]
Enter para editar
```

### Editar quantidade (EDIT_QUANTITY)
```
 Quantidade (g)

     [100]g
Enter para editar
```

---

## 🔧 Troubleshooting

### Problema: WiFi não conecta
- Verifique SSID e senha no `config.h`
- Certifique-se que o roteador está alcançável

### Problema: MQTT não conecta
- Verifique se o Mosquitto está rodando: `sudo systemctl status mosquitto`
- Teste a porta: `telnet IP_MOSQUITTO 8883`
- Verifique usuário/senha no arquivo passwd
- Veja os logs do Mosquitto: `sudo journalctl -u mosquitto -f`

### Problema: Certificado TLS inválido
- Certifique-se que o certificado em `mqtt_cert.h` está completo
- Verifique se o servidor Mosquitto está usando o mesmo CA
- Para teste rápido, use porta 1883 sem TLS

### Problema: Display não aparece nada
- Verifique conexões I2C (SDA/SCL)
- Confirme o endereço I2C: normalmente é 0x27 ou 0x3F
- Se necessário, altere `LCD_ADDRESS` no `config.h`

### Problema: Botões não respondem
- Verifique os pinos no `config.h`:
  - `BTN_UP_PIN = 34`
  - `BTN_DOWN_PIN = 35`
  - `BTN_OK_PIN = 32`
  - `BTN_BACK_PIN = 33`

---

## 📊 Logs esperados no Serial Monitor

```
╔══════════════════════════════════════╗
║   CENTRAL PET FEEDER - REFACTORED   ║
║          Version 2.0.0-refactored   ║
╚══════════════════════════════════════╝

[CORE] Inicializando RemoteManager...
[RemoteManager] Remota 1 adicionada (1/4)
[RemoteManager] Remota 2 adicionada (2/4)
[RemoteManager] Remota 3 adicionada (3/4)
[RemoteManager] Remota 4 adicionada (4/4)

[CORE] Inicializando ClockService...
[ClockService] RTC inicializado com sucesso!

[CORE] Inicializando ConfigManager...
[ConfigManager] Preferences inicializado

[HAL] Inicializando Buttons...
[Buttons] Botões inicializados

[HAL] Inicializando LCD...
[LCDRenderer] Display inicializado (20x4)

[UI] Inicializando MenuController...
[MenuController] Inicializado
[MenuController] Estado mudou para: 0

========== INICIALIZANDO WIFI ==========
Conectando a SEU_WIFI...........
✅ WiFi conectado!
IP: 192.168.1.150
RSSI: -45 dBm
========================================

========== INICIALIZANDO MQTT ==========
[MQTTClient] Configurado:
  Host: 192.168.1.100:8883
  User: petfeeder
  ClientID: central_gateway
[MQTTClient] Certificado TLS configurado
[MQTTClient] Conectando ao broker MQTT...
✅ MQTT conectado!
[MQTT] Inscrito em: petfeeder/remote/1/status
[MQTT] Inscrito em: petfeeder/remote/1/data
...
========================================

✅ SISTEMA INICIADO COM SUCESSO!
```

---

## ✅ Próximos passos

1. Configurar as remotas para publicar nos tópicos corretos
2. Testar navegação nos menus pelo LCD
3. Configurar horários de refeições
4. Monitorar comunicação MQTT

**Pronto para usar! 🚀**