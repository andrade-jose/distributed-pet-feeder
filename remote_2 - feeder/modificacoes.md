# 📋 Guia de Modificações da Remota para Broker MQTT Local

## 🎯 Objetivo
Migrar a remota de **HiveMQ Cloud (SSL/TLS porta 8883)** para **Broker MQTT Local na Central (porta 1883)** sem SSL.

---

## ⚠️ MUDANÇAS PRINCIPAIS

### 1️⃣ **Eliminar SSL/TLS**
- **ANTES**: Conexão segura SSL/TLS obrigatória (HiveMQ Cloud porta 8883)
- **DEPOIS**: Conexão local sem SSL (porta 1883)

### 2️⃣ **Mudar Servidor MQTT**
- **ANTES**: `9aa85a8cfb4a4ba896f2289aa408ba5a.s1.eu.hivemq.cloud`
- **DEPOIS**: IP local da central (exemplo: `192.168.1.100`)

### 3️⃣ **Remover Autenticação**
- **ANTES**: Usuário + senha obrigatórios
- **DEPOIS**: Sem autenticação (rede local confiável)

---

## 📝 MODIFICAÇÕES NO CÓDIGO

### **Arquivo 1: `remote-feeder/src/config.cpp`**

#### ❌ **REMOVER (código antigo HiveMQ)**
```cpp
// ===== CONFIGURAÇÃO MQTT =====
const char* MQTT_SERVER = "9aa85a8cfb4a4ba896f2289aa408ba5a.s1.eu.hivemq.cloud";
const int MQTT_PORT = 8883;
const char* MQTT_CLIENT_ID = "ESP32_Remota_001";
const char* MQTT_USERNAME = "Romota1";
const char* MQTT_PASSWORD = "Senha1234";
```

#### ✅ **ADICIONAR (código novo Broker Local)**
```cpp
// ===== CONFIGURAÇÃO MQTT - BROKER LOCAL =====
// IMPORTANTE: Substitua pelo IP da sua Central ESP32
// O IP será mostrado no LCD da Central após conectar ao WiFi
const char* MQTT_SERVER = "192.168.1.100";  // ⚠️ ALTERE PARA O IP DA SUA CENTRAL!
const int MQTT_PORT = 1883;                  // Porta padrão MQTT (sem SSL)
const char* MQTT_CLIENT_ID = "ESP32_Remota_001"; // Pode manter o mesmo
// NÃO É NECESSÁRIO USUÁRIO E SENHA NO BROKER LOCAL
```

#### 🔧 **Tópicos permanecem iguais** (compatibilidade mantida)
```cpp
// ===== TÓPICOS MQTT =====
const char* TOPIC_COMANDO = "alimentador/remota/comando";
const char* TOPIC_STATUS = "alimentador/remota/status";
const char* TOPIC_RESPOSTA = "alimentador/remota/resposta";
const char* TOPIC_HEARTBEAT = "alimentador/remota/heartbeat";
const char* TOPIC_ALERTA_RACAO = "alimentador/remota/alerta_racao";
```

---

### **Arquivo 2: `remote-feeder/include/config.h`**

#### ❌ **REMOVER declarações de usuário/senha**
```cpp
extern const char* MQTT_USERNAME;
extern const char* MQTT_PASSWORD;
```

#### ✅ **Manter apenas as necessárias**
```cpp
// ===== CONFIGURAÇÃO MQTT =====
extern const char* MQTT_SERVER;
extern const int MQTT_PORT;
extern const char* MQTT_CLIENT_ID;

// ===== TÓPICOS MQTT =====
extern const char* TOPIC_COMANDO;
extern const char* TOPIC_STATUS;
extern const char* TOPIC_RESPOSTA;
extern const char* TOPIC_HEARTBEAT;
extern const char* TOPIC_ALERTA_RACAO;
```

---

### **Arquivo 3: `remote-feeder/src/gerenciador_mqtt.cpp`**

#### ❌ **REMOVER código de conexão SSL (WiFiClientSecure)**
```cpp
// CÓDIGO ANTIGO - DELETAR TUDO ISSO:
#include <WiFiClientSecure.h>

WiFiClientSecure wifiClient;  // Cliente SSL

void configurarSSL() {
    wifiClient.setInsecure();  // Aceitar qualquer certificado
}

bool conectarMQTT() {
    // ... código de conexão SSL com usuário/senha
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
        // ...
    }
}
```

#### ✅ **ADICIONAR código de conexão simples (WiFiClient)**
```cpp
// CÓDIGO NOVO - BROKER LOCAL SEM SSL:
#include <WiFiClient.h>

WiFiClient wifiClient;  // Cliente TCP simples (sem SSL)

bool conectarMQTT() {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_MQTT_PRINTLN("WiFi não conectado");
        return false;
    }

    DEBUG_MQTT_PRINT("Conectando ao broker MQTT local...");

    // Conectar SEM usuário/senha (broker local)
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
        DEBUG_MQTT_PRINTLN(" Conectado!");

        // Inscrever nos tópicos
        mqttClient.subscribe(TOPIC_COMANDO);

        // Publicar status inicial
        publicarStatus("ONLINE");

        return true;
    } else {
        int estado = mqttClient.state();
        DEBUG_MQTT_PRINTF(" Falhou (código %d)\n", estado);
        return false;
    }
}
```

---

### **Arquivo 4: `remote-feeder/platformio.ini`**

#### ❌ **REMOVER bibliotecas SSL** (se existirem)
```ini
# NÃO é mais necessário:
# SSLClient ou outras bibliotecas de criptografia
```

#### ✅ **Manter apenas as essenciais**
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps =
    knolleary/PubSubClient@^2.8
    bblanchon/ArduinoJson@^6.21.3
    # Outras bibliotecas para sensores/servos
```

---

## 🔧 CHECKLIST DE MODIFICAÇÕES

- [ ] **1. Atualizar `config.cpp`**
  - [ ] Mudar `MQTT_SERVER` para IP da central
  - [ ] Mudar `MQTT_PORT` para 1883
  - [ ] Remover `MQTT_USERNAME` e `MQTT_PASSWORD`

- [ ] **2. Atualizar `config.h`**
  - [ ] Remover declarações `extern` de usuário/senha

- [ ] **3. Atualizar `gerenciador_mqtt.cpp`**
  - [ ] Trocar `#include <WiFiClientSecure.h>` por `#include <WiFiClient.h>`
  - [ ] Trocar `WiFiClientSecure wifiClient` por `WiFiClient wifiClient`
  - [ ] Remover função `configurarSSL()`
  - [ ] Alterar `mqttClient.connect()` para NÃO usar usuário/senha

- [ ] **4. Atualizar `platformio.ini`**
  - [ ] Remover bibliotecas SSL desnecessárias

- [ ] **5. Testar conexão**
  - [ ] Compilar código
  - [ ] Carregar na remota
  - [ ] Verificar se conecta ao broker local
  - [ ] Testar envio de comandos da central

---

## 🌐 COMO DESCOBRIR O IP DA CENTRAL

### Método 1: **LCD da Central**
- A central mostra o IP no display LCD após conectar ao WiFi
- Navegue até: Menu Principal → WiFi → Rede
- O IP aparecerá como: `IP: 192.168.x.xxx`

### Método 2: **Serial Monitor**
- Conecte a central via USB
- Abra o Serial Monitor (115200 baud)
- Após conectar ao WiFi, verá:
```
[WiFi] WiFi conectado!
[WiFi] IP: 192.168.1.100
[MQTT] Broker MQTT iniciado em: 192.168.1.100:1883
```

### Método 3: **Roteador**
- Acesse a interface do seu roteador
- Procure por "Dispositivos Conectados" ou "DHCP Clients"
- Encontre o dispositivo "ESP32_Central" ou "Alimentador"

---

## 📊 FORMATO DAS MENSAGENS MQTT (sem alteração)

### **Comando de Alimentação (Central → Remota)**
```json
{
  "acao": "alimentar",
  "tempo": 4,
  "remota_id": 1,
  "timestamp": 1234567890
}
```

### **Status da Remota (Remota → Central)**
```json
{
  "status": "ONLINE",
  "timestamp": 1234567890
}
```

### **Heartbeat (Remota → Central)**
```json
{
  "status": "ALIVE",
  "remota_id": 1,
  "timestamp": 1234567890
}
```

### **Alerta de Ração Baixa (Remota → Central)**
```json
{
  "remota_id": 1,
  "nivel": "BAIXO",
  "distancia": 4.5
}
```

---

## 🧪 TESTE DE CONEXÃO

### **Passos para testar**
1. Compile e carregue o código na remota
2. Abra o Serial Monitor (115200 baud)
3. Aguarde conectar ao WiFi
4. Verifique as mensagens de conexão MQTT:

```
[WiFi] WiFi conectado!
[WiFi] IP: 192.168.1.101
[MQTT] Conectando ao broker MQTT local... Conectado!
[MQTT] Inscrito em: alimentador/remota/comando - OK
[MQTT] Status publicado: ONLINE
```

### **Teste de comando manual**
- Use a interface web da central ou o LCD
- Envie comando "Alimentar" para a remota
- Verifique no Serial Monitor se recebeu o comando

---

## ⚠️ PROBLEMAS COMUNS

### **1. Remota não conecta ao broker**
**Sintoma**: `[MQTT] Conectando ao broker MQTT local... Falhou (código -2)`

**Solução**:
- Verifique se o IP da central está correto
- Verifique se a central e a remota estão na mesma rede WiFi
- Verifique se o broker da central está rodando (veja o LCD ou Serial Monitor)

### **2. Erro de compilação: WiFiClientSecure**
**Sintoma**: `'WiFiClientSecure' does not name a type`

**Solução**:
- Certifique-se de ter trocado `WiFiClientSecure` por `WiFiClient`
- Remova todos os `#include <WiFiClientSecure.h>`

### **3. Remota conecta mas não recebe comandos**
**Sintoma**: Conexão OK mas comandos não chegam

**Solução**:
- Verifique se os tópicos MQTT estão corretos
- Confirme que a remota se inscreveu em `alimentador/remota/comando`
- Use o Serial Monitor para ver se as mensagens estão chegando

### **4. IP da central muda após reiniciar**
**Sintoma**: Remota para de conectar após reiniciar o roteador

**Solução**:
- Configure IP estático no roteador para a central
- Ou use mDNS/DNS local (não implementado ainda)

---

## 🎯 RESUMO RÁPIDO

| Item | ANTES (HiveMQ Cloud) | DEPOIS (Broker Local) |
|------|----------------------|------------------------|
| **Servidor** | `9aa85a8cfb4a4ba896f2289aa408ba5a.s1.eu.hivemq.cloud` | IP da Central (ex: `192.168.1.100`) |
| **Porta** | `8883` (SSL) | `1883` (TCP) |
| **SSL/TLS** | ✅ Obrigatório | ❌ Desabilitado |
| **Usuário/Senha** | ✅ Obrigatório | ❌ Não necessário |
| **Biblioteca WiFi** | `WiFiClientSecure` | `WiFiClient` |
| **Tópicos MQTT** | ✅ Mantidos | ✅ Mantidos |
| **Formato JSON** | ✅ Mantido | ✅ Mantido |

---

## 📚 REFERÊNCIAS

- Documentação PubSubClient: https://pubsubclient.knolleary.net/
- Documentação ESP32 WiFi: https://docs.espressif.com/projects/arduino-esp32/
- Documentação PicoMQTT (Broker): https://github.com/mlesniew/PicoMQTT

---

## ✅ CONCLUSÃO

Após implementar todas essas modificações, sua remota estará pronta para se conectar ao broker MQTT local rodando na central, eliminando a dependência do HiveMQ Cloud e tornando o sistema 100% local e independente da internet!

**Tempo estimado**: 30-60 minutos (incluindo testes)

---

**Autor**: Claude Code
**Data**: 2025-01-11
**Versão**: 1.0
