# 🚀 Guia de Instalação e Configuração

## 📋 Índice

1. [Pré-requisitos](#-pré-requisitos)
2. [Preparação do Hardware](#-preparação-do-hardware)
3. [Instalação do Software](#-instalação-do-software)
4. [Configuração do Projeto](#-configuração-do-projeto)
5. [Upload e Teste](#-upload-e-teste)
6. [Configuração do Broker MQTT](#-configuração-do-broker-mqtt)
7. [Troubleshooting](#-troubleshooting)
8. [Verificação Final](#-verificação-final)

## 🛠️ Pré-requisitos

### Software Necessário

- **PlatformIO IDE** ou **VS Code + PlatformIO Extension**
- **Git** (para clonar o repositório)
- **Mosquitto Client** (opcional, para testes)

### Hardware Necessário

| Item | Especificação | Quantidade |
|------|---------------|------------|
| ESP32 | ESP32-D0WD-V3 ou compatível | 1 |
| Servo Motor | PDI 6221MG (180°) | 1 |
| Sensor Hall | A3144 | 1 |
| Botão Push | Momentâneo | 1 |
| LED | 5mm comum | 1 |
| Resistores | 220Ω (LED), 10kΩ (pull-up) | 2 |
| Breadboard | Meio ou completa | 1 |
| Jumpers | Macho-macho e macho-fêmea | ~20 |
| Fonte | 5V/2A para servo | 1 |

### Ferramentas

- Ferro de solda (opcional)
- Multímetro
- Alicate desencapador
- Ímã pequeno (para teste do sensor Hall)

## 🔌 Preparação do Hardware

### Esquema de Conexões

```
ESP32 (GPIO)    Componente
────────────    ──────────
GPIO 5     ──►  Servo (PWM - Fio Laranja)
GPIO 4     ──►  Sensor Hall (OUT)
GPIO 18    ──►  Botão (Pull-up interno)
GPIO 13    ──►  LED (através de 220Ω)
GND        ──►  GND comum
3.3V       ──►  VCC sensores
5V/VIN     ──►  VCC servo (fio vermelho)
```

### Montagem Detalhada

#### 1. ESP32
```
┌─────────────────┐
│      ESP32      │
│                 │
│ 3V3 ────────────┼──► VCC Sensor Hall
│ GND ────────────┼──► GND Comum
│ 5V  ────────────┼──► VCC Servo (vermelho)
│ G5  ────────────┼──► PWM Servo (laranja)
│ G4  ────────────┼──► OUT Sensor Hall
│ G18 ────────────┼──► Botão
│ G13 ────────────┼──► LED (+)
└─────────────────┘
```

#### 2. Servo PDI 6221MG
```
Cabo do Servo:
├── Vermelho ──► 5V (fonte externa recomendada)
├── Marrom   ──► GND
└── Laranja  ──► GPIO 5 (PWM)
```

#### 3. Sensor Hall A3144
```
Sensor Hall (frente para você):
├── Pino 1 (VCC) ──► 3.3V
├── Pino 2 (GND) ──► GND
└── Pino 3 (OUT) ──► GPIO 4
```

#### 4. Botão e LED
```
Botão:
├── Terminal 1 ──► GPIO 18
└── Terminal 2 ──► GND

LED:
├── Ânodo (+) ──► GPIO 13 (através de 220Ω)
└── Cátodo (-) ──► GND
```

### Verificação das Conexões

```cpp
// Código de teste para verificar conexões
void testarConexoes() {
    // Teste do LED
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);
    
    // Teste do botão
    Serial.printf("Botão: %s\n", digitalRead(18) ? "Solto" : "Pressionado");
    
    // Teste do sensor Hall
    Serial.printf("Hall: %s\n", digitalRead(4) ? "Sem ímã" : "Com ímã");
    
    // Teste do servo
    servo.write(90);
    delay(1000);
    servo.write(0);
}
```

## 💻 Instalação do Software

### 1. Instalar VS Code + PlatformIO

#### Windows
```powershell
# Via Chocolatey
choco install vscode
# Ou baixar de: https://code.visualstudio.com/

# Instalar extensão PlatformIO
code --install-extension platformio.platformio-ide
```

#### Linux
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install code

# Arch Linux
sudo pacman -S code

# Instalar extensão
code --install-extension platformio.platformio-ide
```

#### macOS
```bash
# Via Homebrew
brew install --cask visual-studio-code

# Instalar extensão
code --install-extension platformio.platformio-ide
```

### 2. Clonar o Repositório

```bash
git clone <URL_DO_REPOSITORIO>
cd Alimentador_remota
```

### 3. Abrir no VS Code

```bash
code .
```

### 4. Verificar Dependências

O arquivo `platformio.ini` deve conter:

```ini
[env:esp32c3]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    ESP32Servo@^0.13.0
    PubSubClient@^2.8.0
    WiFi@^2.0.0
    WiFiClientSecure@^2.0.0

monitor_speed = 115200
upload_speed = 921600
```

## ⚙️ Configuração do Projeto

### 1. Configurar WiFi

Edite o arquivo `src/principal.cpp`:

```cpp
// ===== CONFIGURAÇÃO WIFI =====
const char* WIFI_SSID = "SUA_REDE_WIFI";        
const char* WIFI_PASSWORD = "SUA_SENHA_WIFI";
```

### 2. Configurar MQTT

```cpp
// ===== CONFIGURAÇÃO MQTT =====
const char* MQTT_SERVER = "SEU_BROKER.hivemq.cloud";  
const int MQTT_PORT = 8883;                     
const char* MQTT_CLIENT_ID = "ESP32_Remota_001"; 
const char* MQTT_USERNAME = "SEU_USUARIO";          
const char* MQTT_PASSWORD = "SUA_SENHA";
```

### 3. Ajustar Pinos (se necessário)

```cpp
// ===== CONFIGURAÇÃO DO HARDWARE =====
const int PINO_SERVO = 5;      
const int PINO_HALL = 4;       
const int PINO_BOTAO = 18;      
const int PINO_LED_STATUS = 13;
```

### 4. Criar Classes de Componentes

Se não existirem, crie os arquivos:

#### `include/ServoControl.h`
```cpp
#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <ESP32Servo.h>

class ServoControl {
private:
    Servo servo;
    int pino;
    int anguloAtual;
    
public:
    void iniciar(int pin);
    void ativar();
    void moverParaAngulo(int angulo);
    int obterAngulo();
};

#endif
```

#### `src/ServoControl.cpp`
```cpp
#include "ServoControl.h"

void ServoControl::iniciar(int pin) {
    pino = pin;
    servo.attach(pino);
    anguloAtual = 0;
}

void ServoControl::ativar() {
    servo.attach(pino);
}

void ServoControl::moverParaAngulo(int angulo) {
    servo.write(angulo);
    anguloAtual = angulo;
    Serial.printf("Servo movido para %d°\n", angulo);
}

int ServoControl::obterAngulo() {
    return anguloAtual;
}
```

#### `include/SensorHall.h`
```cpp
#ifndef SENSOR_HALL_H
#define SENSOR_HALL_H

#include <Arduino.h>

class SensorHall {
private:
    int pino;
    bool estadoAtual;
    bool estadoAnterior;
    
public:
    void iniciar(int pin);
    void verificar();
    bool estaDetectando();
    bool mudouEstado();
};

#endif
```

#### `src/SensorHall.cpp`
```cpp
#include "SensorHall.h"

void SensorHall::iniciar(int pin) {
    pino = pin;
    pinMode(pino, INPUT);
    estadoAtual = false;
    estadoAnterior = false;
}

void SensorHall::verificar() {
    estadoAnterior = estadoAtual;
    estadoAtual = !digitalRead(pino); // A3144 é LOW quando detecta
}

bool SensorHall::estaDetectando() {
    return estadoAtual;
}

bool SensorHall::mudouEstado() {
    return estadoAtual != estadoAnterior;
}
```

### 5. Criar Classes de Gerenciamento

#### `include/WiFiManager.h`
```cpp
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

class WiFiManager {
private:
    const char* ssid;
    const char* password;
    bool conectado;
    
public:
    WiFiManager(const char* ssid, const char* password);
    void iniciar();
    bool conectar();
    void verificarConexao();
    bool estaConectado();
};

#endif
```

#### `src/WiFiManager.cpp`
```cpp
#include "WiFiManager.h"

WiFiManager::WiFiManager(const char* s, const char* p) {
    ssid = s;
    password = p;
    conectado = false;
}

void WiFiManager::iniciar() {
    WiFi.mode(WIFI_STA);
}

bool WiFiManager::conectar() {
    Serial.printf("Conectando ao WiFi: %s\n", ssid);
    WiFi.begin(ssid, password);
    
    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
        delay(500);
        Serial.print(".");
        tentativas++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi conectado! IP: %s\n", WiFi.localIP().toString().c_str());
        conectado = true;
        return true;
    } else {
        Serial.println("\nFalha na conexão WiFi!");
        conectado = false;
        return false;
    }
}

void WiFiManager::verificarConexao() {
    if (WiFi.status() != WL_CONNECTED) {
        conectado = false;
        Serial.println("WiFi desconectado, tentando reconectar...");
        conectar();
    } else {
        conectado = true;
    }
}

bool WiFiManager::estaConectado() {
    return conectado && (WiFi.status() == WL_CONNECTED);
}
```

#### `include/MQTTManager.h`
```cpp
#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "WiFiManager.h"

typedef std::function<void(String topic, String payload)> CallbackFunction;

class MQTTManager {
private:
    WiFiClientSecure wifiClient;
    PubSubClient mqttClient;
    WiFiManager* wifiManager;
    const char* server;
    int port;
    const char* clientId;
    const char* username;
    const char* password;
    CallbackFunction callback;
    
public:
    MQTTManager(WiFiManager* wm, const char* server, int port, 
                const char* clientId, const char* username, const char* password);
    void iniciar();
    bool conectar();
    void loop();
    void verificarConexao();
    bool publicar(const char* topic, String payload);
    void subscrever(const char* topic);
    void definirCallback(CallbackFunction cb);
    bool estaConectado();
    
    static void callbackMQTT(char* topic, byte* payload, unsigned int length);
    static MQTTManager* instance;
};

#endif
```

#### `src/MQTTManager.cpp`
```cpp
#include "MQTTManager.h"

MQTTManager* MQTTManager::instance = nullptr;

MQTTManager::MQTTManager(WiFiManager* wm, const char* srv, int p, 
                         const char* id, const char* usr, const char* pwd) {
    wifiManager = wm;
    server = srv;
    port = p;
    clientId = id;
    username = usr;
    password = pwd;
    instance = this;
    mqttClient.setClient(wifiClient);
}

void MQTTManager::iniciar() {
    wifiClient.setInsecure(); // Para desenvolvimento
    mqttClient.setServer(server, port);
    mqttClient.setCallback(callbackMQTT);
}

bool MQTTManager::conectar() {
    if (!wifiManager->estaConectado()) {
        return false;
    }
    
    Serial.printf("Conectando ao MQTT: %s:%d\n", server, port);
    
    if (mqttClient.connect(clientId, username, password)) {
        Serial.println("MQTT conectado!");
        return true;
    } else {
        Serial.printf("Falha MQTT: %d\n", mqttClient.state());
        return false;
    }
}

void MQTTManager::loop() {
    mqttClient.loop();
}

void MQTTManager::verificarConexao() {
    if (!mqttClient.connected()) {
        conectar();
    }
}

bool MQTTManager::publicar(const char* topic, String payload) {
    if (mqttClient.connected()) {
        return mqttClient.publish(topic, payload.c_str());
    }
    return false;
}

void MQTTManager::subscrever(const char* topic) {
    if (mqttClient.connected()) {
        mqttClient.subscribe(topic);
    }
}

void MQTTManager::definirCallback(CallbackFunction cb) {
    callback = cb;
}

bool MQTTManager::estaConectado() {
    return mqttClient.connected();
}

void MQTTManager::callbackMQTT(char* topic, byte* payload, unsigned int length) {
    if (instance && instance->callback) {
        String topicStr = String(topic);
        String payloadStr = "";
        for (unsigned int i = 0; i < length; i++) {
            payloadStr += (char)payload[i];
        }
        instance->callback(topicStr, payloadStr);
    }
}
```

## 📤 Upload e Teste

### 1. Compilar o Projeto

```bash
pio run
```

**Saída esperada:**
```
Processing esp32c3 (platform: espressif32; board: esp32dev; framework: arduino)
...
RAM:   [=         ]  14.1% (used 46320 bytes from 327680 bytes)
Flash: [=======   ]  69.9% (used 916489 bytes from 1310720 bytes)
[SUCCESS] Took 7.73 seconds
```

### 2. Conectar ESP32

- Conecte o ESP32 via cabo USB
- Identifique a porta COM (Windows) ou /dev/ttyUSB (Linux)

### 3. Upload do Firmware

```bash
pio run --target upload
```

### 4. Monitor Serial

```bash
pio device monitor
```

**Saída esperada:**
```
🍽️ ALIMENTADOR AUTOMÁTICO REMOTA - MQTT
==========================================
🏗️ Arquitetura:
   • ESP32 CENTRAL → Envia comandos
   • ESP32 REMOTA → Executa movimentos (ESTE)
...
✅ Hardware inicializado!
📡 Inicializando comunicação...
WiFi conectado! IP: 192.168.1.100
MQTT conectado!
🧪 TESTE INICIAL DOS COMPONENTES
...
🚀 ESP32 REMOTA pronto para receber comandos do CENTRAL!
```

## 🌐 Configuração do Broker MQTT

### 1. HiveMQ Cloud (Recomendado)

1. Acesse: https://www.hivemq.com/mqtt-cloud-broker/
2. Crie uma conta gratuita
3. Crie um cluster
4. Configure credenciais:
   ```
   Cluster URL: xxxxxxxx.s1.eu.hivemq.cloud
   Username: SeuUsuario
   Password: SuaSenha
   Port: 8883 (SSL/TLS)
   ```

### 2. Mosquitto Local (Desenvolvimento)

```bash
# Ubuntu/Debian
sudo apt install mosquitto mosquitto-clients

# Iniciar broker
sudo systemctl start mosquitto

# Testar
mosquitto_pub -h localhost -t test -m "hello"
mosquitto_sub -h localhost -t test
```

### 3. Configurar SSL/TLS

Para produção, configure certificados SSL:

```cpp
// Certificado CA do HiveMQ
const char* CA_CERT = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n" \
"...\n" \
"-----END CERTIFICATE-----\n";

void configurarSSL() {
    wifiClient.setCACert(CA_CERT);
}
```

## 🧪 Testes de Validação

### 1. Teste de Hardware

```cpp
void testarComponentes() {
    Serial.println("=== TESTE DE HARDWARE ===");
    
    // Teste do LED
    Serial.println("Testando LED...");
    digitalWrite(PINO_LED_STATUS, HIGH);
    delay(1000);
    digitalWrite(PINO_LED_STATUS, LOW);
    
    // Teste do servo
    Serial.println("Testando servo...");
    servo.moverParaAngulo(90);
    delay(2000);
    servo.moverParaAngulo(0);
    delay(2000);
    
    // Teste do sensor Hall
    Serial.println("Testando sensor Hall (aproxime um ímã)...");
    for (int i = 0; i < 10; i++) {
        sensorHall.verificar();
        Serial.printf("Hall: %s\n", sensorHall.estaDetectando() ? "DETECTADO" : "Normal");
        delay(500);
    }
    
    // Teste do botão
    Serial.println("Testando botão (pressione o botão)...");
    for (int i = 0; i < 10; i++) {
        bool botao = !digitalRead(PINO_BOTAO);
        Serial.printf("Botão: %s\n", botao ? "PRESSIONADO" : "Solto");
        delay(500);
    }
    
    Serial.println("=== TESTE CONCLUÍDO ===");
}
```

### 2. Teste de Comunicação

```bash
# Terminal 1: Monitorar mensagens
mosquitto_sub -h SEU_BROKER.hivemq.cloud -p 8883 \
  -t "alimentador/remota/+" \
  -u "SeuUsuario" -P "SuaSenha"

# Terminal 2: Enviar comandos
mosquitto_pub -h SEU_BROKER.hivemq.cloud -p 8883 \
  -t "alimentador/remota/comando" \
  -m "PING" \
  -u "SeuUsuario" -P "SuaSenha"
```

### 3. Teste de Alimentação

```bash
# Comando de alimentação por 5 segundos
mosquitto_pub -h SEU_BROKER.hivemq.cloud -p 8883 \
  -t "alimentador/remota/comando" \
  -m '{"acao":"alimentar","tempo":5,"remota_id":1}' \
  -u "SeuUsuario" -P "SuaSenha"
```

## 🔧 Troubleshooting

### Problema: ESP32 não conecta ao WiFi

**Soluções:**
```cpp
// 1. Verificar credenciais
Serial.printf("SSID: %s\n", WIFI_SSID);
Serial.printf("Password: %s\n", WIFI_PASSWORD);

// 2. Verificar status WiFi
Serial.printf("WiFi Status: %d\n", WiFi.status());
// 3 = WL_CONNECTED
// 4 = WL_CONNECT_FAILED
// 6 = WL_DISCONNECTED

// 3. Aumentar timeout
int tentativas = 0;
while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(1000);
    Serial.print(".");
    tentativas++;
}
```

### Problema: MQTT não conecta

**Soluções:**
```cpp
// 1. Testar sem SSL
wifiClient.setInsecure();

// 2. Verificar estado MQTT
Serial.printf("MQTT State: %d\n", mqttClient.state());
// -4 = MQTT_CONNECTION_TIMEOUT
// -3 = MQTT_CONNECTION_LOST  
// -2 = MQTT_CONNECT_FAILED
// -1 = MQTT_DISCONNECTED
//  0 = MQTT_CONNECTED

// 3. Testar com cliente externo
mosquitto_pub -h SEU_BROKER.hivemq.cloud -p 8883 \
  -t "test" -m "hello" \
  -u "SeuUsuario" -P "SuaSenha"
```

### Problema: Servo não move

**Soluções:**
```cpp
// 1. Verificar alimentação
// Servo PDI 6221MG precisa de 5V/2A

// 2. Testar PWM
servo.attach(PINO_SERVO);
servo.write(90);
delay(2000);

// 3. Verificar conexão
digitalWrite(PINO_SERVO, HIGH);
delay(20);
digitalWrite(PINO_SERVO, LOW);
```

### Problema: Sensor Hall não detecta

**Soluções:**
```cpp
// 1. Testar com multímetro
// A3144: LOW quando detecta campo magnético

// 2. Verificar orientação do ímã
// Testar com pólo Norte e Sul

// 3. Verificar distância
// Máximo ~5mm do sensor
```

## ✅ Verificação Final

### Checklist de Instalação

- [ ] Hardware montado corretamente
- [ ] ESP32 conectado e reconhecido
- [ ] Firmware compilado sem erros
- [ ] Upload realizado com sucesso
- [ ] WiFi conectado
- [ ] MQTT conectado
- [ ] Teste de componentes aprovado
- [ ] Comunicação MQTT funcional
- [ ] LED de status funcionando
- [ ] Servo movendo corretamente
- [ ] Sensor Hall detectando
- [ ] Botão respondendo
- [ ] Heartbeat sendo enviado

### Comandos de Teste Final

```bash
# 1. Monitor em tempo real
pio device monitor

# 2. Teste PING
mosquitto_pub -h SEU_BROKER -p 8883 -t "alimentador/remota/comando" -m "PING"

# 3. Teste de alimentação
mosquitto_pub -h SEU_BROKER -p 8883 -t "alimentador/remota/comando" -m "a3"

# 4. Verificar heartbeat
mosquitto_sub -h SEU_BROKER -p 8883 -t "alimentador/remota/heartbeat"
```

### Logs de Sucesso

```
💓 Heartbeat enviado: ALIVE (remota_id: 1)
📥 MQTT recebido [alimentador/remota/comando]: PING
📤 Status enviado: PONG
📥 MQTT recebido [alimentador/remota/comando]: a3
🎯 Comando legado: 3 segundos [ID: LEGACY_1234567890]
🍽️ INICIANDO ALIMENTAÇÃO
...
✅ ALIMENTAÇÃO CONCLUÍDA! Duração: 3 segundos
```

---

**Parabéns! 🎉 Seu alimentador automático está instalado e funcionando!**

Para suporte adicional, consulte:
- [README.md](../README.md) - Documentação principal
- [TECHNICAL_DOCUMENTATION.md](TECHNICAL_DOCUMENTATION.md) - Detalhes técnicos
- [MQTT_API_REFERENCE.md](MQTT_API_REFERENCE.md) - Referência da API
