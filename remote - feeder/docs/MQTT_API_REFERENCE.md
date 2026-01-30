# 📡 API MQTT - Referência Completa

## 📋 Índice

1. [Visão Geral](#-visão-geral)
2. [Configuração da Conexão](#-configuração-da-conexão)
3. [Tópicos MQTT](#-tópicos-mqtt)
4. [Comandos](#-comandos)
5. [Respostas](#-respostas)
6. [Exemplos Práticos](#-exemplos-práticos)
7. [Códigos de Status](#-códigos-de-status)
8. [Troubleshooting](#-troubleshooting)

## 🌐 Visão Geral

A comunicação entre ESP32 CENTRAL e ESP32 REMOTA utiliza protocolo MQTT com SSL/TLS para garantir segurança e confiabilidade.

### Características

- **Protocolo**: MQTT 3.1.1 com SSL/TLS
- **Broker**: HiveMQ Cloud
- **Porta**: 8883 (SSL)
- **QoS**: 0 (At most once)
- **Retain**: false
- **Clean Session**: true

## 🔐 Configuração da Conexão

### Broker MQTT

```
Servidor: 9aa85a8cfb4a4ba896f2289aa408ba5a.s1.eu.hivemq.cloud
Porta: 8883
Protocolo: MQTT sobre SSL/TLS
```

### Credenciais

```cpp
// Configuração da REMOTA
const char* MQTT_CLIENT_ID = "ESP32_Remota_001";
const char* MQTT_USERNAME = "SEU_USUARIO";
const char* MQTT_PASSWORD = "SUA_SENHA";
```

### Certificados SSL

```cpp
// Configuração SSL/TLS
wifiClient.setCACert(CA_CERT);
wifiClient.setCertificate(CLIENT_CERT);  // Opcional
wifiClient.setPrivateKey(CLIENT_KEY);    // Opcional
```

## 📨 Tópicos MQTT

### Estrutura de Tópicos

```
alimentador/
└── remota/
    ├── comando     (Central → Remota)
    ├── status      (Remota → Central)
    ├── resposta    (Remota → Central)
    └── heartbeat   (Remota → Central)
```

### Detalhamento dos Tópicos

| Tópico | Direção | QoS | Retain | Descrição |
|--------|---------|-----|--------|-----------|
| `alimentador/remota/comando` | Central → Remota | 0 | false | Comandos de alimentação |
| `alimentador/remota/status` | Remota → Central | 0 | false | Status operacional |
| `alimentador/remota/resposta` | Remota → Central | 0 | false | Confirmação de execução |
| `alimentador/remota/heartbeat` | Remota → Central | 0 | false | Sinal de vida |

## 📤 Comandos

### 1. Comando de Alimentação (JSON)

**Tópico**: `alimentador/remota/comando`

```json
{
  "acao": "alimentar",
  "tempo": 5,
  "remota_id": 1
}
```

**Parâmetros**:
- `acao`: Sempre "alimentar"
- `tempo`: Duração em segundos (1-60)
- `remota_id`: ID da remota (1-99)

**Exemplo**:
```json
{
  "acao": "alimentar",
  "tempo": 10,
  "remota_id": 1
}
```

### 2. Comandos Simples (String)

**Tópico**: `alimentador/remota/comando`

| Comando | Descrição | Resposta Esperada |
|---------|-----------|-------------------|
| `PING` | Teste de comunicação | `PONG` |
| `STATUS` | Solicitar status atual | Status detalhado |
| `STOP` | Parar alimentação | Confirmação |

**Exemplos**:
```
PING
STATUS
STOP
```

### 3. Comandos Legados (String)

**Tópico**: `alimentador/remota/comando`

**Formato**: `a<TEMPO>`

| Comando | Descrição |
|---------|-----------|
| `a3` | Alimentar por 3 segundos |
| `a5` | Alimentar por 5 segundos |
| `a10` | Alimentar por 10 segundos |

**Limitações**:
- Tempo: 1-60 segundos
- Apenas números inteiros

## 📥 Respostas

### 1. Status Operacional

**Tópico**: `alimentador/remota/status`

```json
{
  "status": "DISPONIVEL",
  "timestamp": 1234567890
}
```

**Possíveis Status**:
- `DISPONIVEL`: Pronto para comandos
- `ATIVO`: Alimentação em andamento  
- `INATIVO`: Sistema inativo
- `PAUSADO_BOTAO`: Pausado por botão
- `RETOMADO_BOTAO`: Retomado após pausa
- `SERVO_TRAVADO_90`: Servo travado manualmente
- `SISTEMA_ATIVO_0`: Sistema ativo na posição 0°
- `FINALIZADO`: Alimentação concluída
- `ERRO_*`: Diversos tipos de erro

### 2. Confirmação de Execução

**Tópico**: `alimentador/remota/resposta`

```json
{
  "concluido": true,
  "tempo_segundos": 5,
  "comando_id": "ALIMENTAR_1234567890",
  "timestamp": 1234567890
}
```

**Campos**:
- `concluido`: Sempre true para sucesso
- `tempo_segundos`: Duração real da alimentação
- `comando_id`: ID único do comando executado
- `timestamp`: Momento da conclusão

### 3. Heartbeat (Sinal de Vida)

**Tópico**: `alimentador/remota/heartbeat`

```json
{
  "status": "ALIVE",
  "remota_id": 1,
  "uptime": 1234567890,
  "wifi_rssi": -45,
  "free_heap": 200000,
  "alimentacao_ativa": false,
  "servo_travado": false
}
```

**Campos**:
- `status`: Sempre "ALIVE"
- `remota_id`: ID da remota
- `uptime`: Tempo desde inicialização (ms)
- `wifi_rssi`: Força do sinal WiFi (dBm)
- `free_heap`: Memória RAM livre (bytes)
- `alimentacao_ativa`: Se está alimentando
- `servo_travado`: Se servo está travado manualmente

**Frequência**: A cada 30 segundos

## 💡 Exemplos Práticos

### Exemplo 1: Alimentação de 5 segundos

**1. Comando enviado** (Central → Remota):
```json
{
  "acao": "alimentar",
  "tempo": 5,
  "remota_id": 1
}
```

**2. Status inicial** (Remota → Central):
```json
{
  "status": "INICIANDO_ALIMENTAR_1234567890",
  "timestamp": 1234567890
}
```

**3. Confirmação final** (Remota → Central):
```json
{
  "concluido": true,
  "tempo_segundos": 5,
  "comando_id": "ALIMENTAR_1234567890",
  "timestamp": 1234567895
}
```

### Exemplo 2: Teste de comunicação

**1. Comando PING** (Central → Remota):
```
PING
```

**2. Resposta PONG** (Remota → Central):
```json
{
  "status": "PONG",
  "timestamp": 1234567890
}
```

### Exemplo 3: Parar alimentação

**1. Comando STOP** (Central → Remota):
```
STOP
```

**2. Confirmação** (Remota → Central):
```json
{
  "status": "PARADO_CENTRAL",
  "timestamp": 1234567890
}
```

## 📊 Códigos de Status

### Status de Sistema

| Código | Descrição | Ação Recomendada |
|--------|-----------|------------------|
| `DISPONIVEL` | Sistema pronto | Pode enviar comandos |
| `ATIVO` | Alimentação ativa | Aguardar conclusão |
| `INATIVO` | Sistema inativo | Verificar hardware |
| `FINALIZADO` | Operação concluída | Sistema pronto novamente |

### Status de Controle Manual

| Código | Descrição | Ação Recomendada |
|--------|-----------|------------------|
| `PAUSADO_BOTAO` | Pausado por botão | Aguardar liberação |
| `RETOMADO_BOTAO` | Retomado após pausa | Operação continua |
| `SERVO_TRAVADO_90` | Servo travado em 90° | Alimentação contínua |
| `SISTEMA_ATIVO_0` | Volta à posição 0° | Sistema operacional |

### Status de Erro

| Código | Descrição | Ação Recomendada |
|--------|-----------|------------------|
| `COMANDO_INVALIDO` | Comando não reconhecido | Verificar formato |
| `ERRO_PARAMETRO_LEGACY` | Parâmetro inválido | Ajustar valores (1-60) |
| `JA_PARADO` | Já estava parado | Sistema já inativo |

### Status de Comunicação

| Código | Descrição | Ação Recomendada |
|--------|-----------|------------------|
| `PONG` | Resposta ao PING | Comunicação OK |
| `ONLINE` | Sistema inicializado | Pronto para uso |

## 🛠️ Troubleshooting

### Problemas Comuns

#### 1. Comando não executado

**Sintomas**:
- Comando enviado mas sem resposta
- Status não muda

**Possíveis Causas**:
- Formato JSON inválido
- Tópico incorreto
- Remota offline

**Soluções**:
```bash
# 1. Testar comunicação
mosquitto_pub -h broker.hivemq.com -p 8883 -t "alimentador/remota/comando" -m "PING"

# 2. Verificar formato JSON
{
  "acao": "alimentar",
  "tempo": 5,
  "remota_id": 1
}

# 3. Monitorar heartbeat
mosquitto_sub -h broker.hivemq.com -p 8883 -t "alimentador/remota/heartbeat"
```

#### 2. Conexão SSL falha

**Sintomas**:
- Erro de conexão MQTT
- Timeout de handshake SSL

**Soluções**:
```cpp
// Verificar certificados
wifiClient.setInsecure(); // Temporário para teste

// Verificar servidor e porta
const char* MQTT_SERVER = "9aa85a8cfb4a4ba896f2289aa408ba5a.s1.eu.hivemq.cloud";
const int MQTT_PORT = 8883;
```

#### 3. Heartbeat ausente

**Sintomas**:
- Não recebe heartbeat
- Sistema parece offline

**Verificações**:
```cpp
// Intervalo configurado (30s)
const unsigned long INTERVALO_HEARTBEAT = 30000;

// Status de conexão
Serial.printf("WiFi: %s, MQTT: %s\n", 
  wifiManager.estaConectado() ? "OK" : "ERRO",
  mqttManager.estaConectado() ? "OK" : "ERRO");
```

### Ferramentas de Teste

#### 1. Mosquitto CLI

```bash
# Subscriber (escutar mensagens)
mosquitto_sub -h 9aa85a8cfb4a4ba896f2289aa408ba5a.s1.eu.hivemq.cloud -p 8883 -t "alimentador/remota/+" -u "SEU_USUARIO" -P "SUA_SENHA"

# Publisher (enviar comandos)
mosquitto_pub -h 9aa85a8cfb4a4ba896f2289aa408ba5a.s1.eu.hivemq.cloud -p 8883 -t "alimentador/remota/comando" -m "PING" -u "SEU_USUARIO" -P "SUA_SENHA"
```

#### 2. MQTT Explorer

Interface gráfica para monitorar e enviar mensagens MQTT.

#### 3. HiveMQ Websocket Client

Cliente web para teste direto no navegador.

### Logs de Debug

```cpp
// Habilitar logs detalhados
void callbackMQTT(String topic, String payload) {
    Serial.printf("📥 MQTT [%s]: %s\n", topic.c_str(), payload.c_str());
    Serial.printf("📊 Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("📡 WiFi RSSI: %d dBm\n", WiFi.RSSI());
    
    processarComandoCentral(payload);
}
```

## 📈 Monitoramento de Performance

### Métricas de Latência

```cpp
void medirLatencia() {
    unsigned long inicio = millis();
    
    // Executar comando
    processarComandoCentral(payload);
    
    unsigned long latencia = millis() - inicio;
    Serial.printf("⏱️ Latência: %lu ms\n", latencia);
}
```

### Monitoramento de Conexão

```cpp
void monitorarConexao() {
    static unsigned long ultimaVerificacao = 0;
    
    if (millis() - ultimaVerificacao >= 10000) { // 10s
        Serial.printf("📊 Status: WiFi %s | MQTT %s | Uptime %lus\n",
            WiFi.isConnected() ? "✅" : "❌",
            mqttClient.connected() ? "✅" : "❌",
            millis() / 1000);
        ultimaVerificacao = millis();
    }
}
```

---

**Esta documentação fornece uma referência completa da API MQTT para desenvolvimento, integração e troubleshooting do sistema.**
