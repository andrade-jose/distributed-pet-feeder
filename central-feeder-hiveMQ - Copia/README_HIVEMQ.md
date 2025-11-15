# ESP32 Cliente MQTT - HiveMQ Cloud

Este projeto foi modificado para funcionar como um **cliente MQTT seguro** conectado ao **HiveMQ Cloud** via porta 8883 (TLS/SSL).

## Mudanças Principais

### ✅ O que foi REMOVIDO:
- ❌ Broker MQTT local (PicoMQTT)
- ❌ Display LCD e gerenciamento de telas
- ❌ Botões físicos
- ❌ Interface de menu
- ❌ Portal captive WiFi
- ❌ Web server local
- ❌ Dependências: `LiquidCrystal_I2C`, `PicoMQTT`, `ESPAsyncWebServer`, `DNSServer`

### ✅ O que foi MANTIDO:
- ✅ Cliente MQTT seguro (TLS/SSL)
- ✅ Gerenciador WiFi
- ✅ Gerenciador de tempo (NTP + RTC)
- ✅ Sistema de comunicação com remotas
- ✅ Tópicos MQTT
- ✅ Callbacks para processamento de mensagens

## Configuração

### 1. Configurar credenciais HiveMQ Cloud

Edite o arquivo `include/config.h` e atualize as seguintes constantes:

```cpp
// Linha 45-49
#define MQTT_BROKER_HOST "seu-cluster.hivemq.cloud"  // Seu cluster HiveMQ
#define MQTT_BROKER_PORT 8883                        // Porta TLS
#define MQTT_USERNAME "seu_usuario"                  // Usuário HiveMQ
#define MQTT_PASSWORD "sua_senha"                    // Senha HiveMQ
#define MQTT_CLIENT_ID "ESP32_Central"               // ID único do cliente
```

### 2. Configurar WiFi

```cpp
// Linha 39-40
#define DEFAULT_WIFI_SSID "SuaRedeWiFi"
#define DEFAULT_WIFI_PASSWORD "SuaSenhaWiFi"
```

### 3. Certificado TLS

O certificado raiz do HiveMQ Cloud (Let's Encrypt ISRG Root X1) já está incluído no arquivo `config.h` (linhas 57-89).

## Como Usar

### Compilar e fazer upload:

```bash
pio run --target upload
```

### Monitor Serial:

```bash
pio device monitor
```

## Arquitetura

```
ESP32 (Cliente MQTT)
    ↓
  WiFi
    ↓
HiveMQ Cloud (Broker MQTT - TLS/SSL - Porta 8883)
    ↓
Remotas (Outros ESP32s)
```

## Tópicos MQTT

### Tópicos que o ESP32 Central PUBLICA (envia comandos):
- `alimentador/remota/{ID}/comando` - Comandos para remotas específicas
- `alimentador/remota/{ID}/horario` - Configuração de horários
- `alimentador/remota/{ID}/tempo` - Configuração de tempo de movimento
- `alimentador/remota/comando` - Comandos gerais
- `alimentador/central/status` - Status da central

### Tópicos que o ESP32 Central INSCREVE (recebe mensagens):
- `alimentador/remota/+/status` - Status das remotas
- `alimentador/remota/+/vida` - Heartbeat das remotas
- `alimentador/remota/+/resposta` - Respostas das remotas
- `alimentador/remota/heartbeat` - Heartbeat geral
- `alimentador/remota/concluido` - Conclusão de tarefas
- `alimentador/remota/alerta_racao` - Alertas de ração baixa

## Debugging

O sistema possui logs detalhados via Serial Monitor (115200 baud):

- `[MQTT]` - Mensagens do cliente MQTT
- `[WiFi]` - Mensagens de WiFi
- `DEBUG_ENABLED` - Controle geral de debug

## Estrutura de Arquivos

```
central-feeder-hiveMQ/
├── include/
│   ├── config.h              (Configurações + Certificado TLS)
│   ├── gerenciador_mqtt.h    (Cliente MQTT)
│   ├── gerenciador_wifi.h
│   └── gerenciador_tempo.h
├── src/
│   ├── principal.cpp         (Loop principal simplificado)
│   ├── gerenciador_mqtt.cpp  (Implementação cliente MQTT)
│   ├── gerenciador_wifi.cpp
│   └── gerenciador_tempo.cpp
└── platformio.ini            (Dependências mínimas)
```

## Dependências PlatformIO

```ini
lib_deps =
    adafruit/RTClib@^2.1.4
    knolleary/PubSubClient@^2.8
    bblanchon/ArduinoJson@^6.21.3
```

## Segurança

- ✅ Conexão TLS/SSL (porta 8883)
- ✅ Autenticação por usuário e senha
- ✅ Certificado raiz verificado
- ✅ Sem portas abertas no ESP32

## Logs Esperados

```
=== Inicializando Sistema Cliente MQTT ===
[WiFi] Conectando...
[WiFi] Conectado! IP: 192.168.1.100
=== Inicializando Cliente MQTT HiveMQ ===
Servidor: seu-cluster.hivemq.cloud:8883
TLS/SSL: Habilitado
Conectando ao HiveMQ Cloud...
✓ CONECTADO AO HIVEMQ CLOUD!
=== INSCREVENDO EM TÓPICOS ===
✓ Inscrito: alimentador/remota/+/status
✓ Inscrito: alimentador/remota/+/vida
...
Sistema pronto!
```

## Solução de Problemas

### Erro: "MQTT_CONNECT_BAD_CREDENTIALS"
- Verifique usuário e senha no HiveMQ Cloud

### Erro: "MQTT_CONNECTION_TIMEOUT"
- Verifique o hostname do cluster HiveMQ
- Verifique se a porta 8883 não está bloqueada

### Erro: "WiFi não conectado"
- Verifique SSID e senha do WiFi
- Verifique sinal WiFi

## Próximos Passos

1. Configure suas credenciais HiveMQ Cloud
2. Configure suas credenciais WiFi
3. Compile e faça upload
4. Monitore o Serial para verificar conexão
5. Configure as remotas para se conectarem ao mesmo broker HiveMQ

---

**Versão:** 1.0.0
**Data:** 2025
**Autor:** Sistema simplificado para cliente MQTT apenas
