# 🔄 Compatibilidade com Central Pet Feeder

Este documento descreve as mudanças implementadas para tornar o Remote Pet Feeder **100% compatível** com o protocolo da Central Pet Feeder.

---

## ✅ Status: TOTALMENTE COMPATÍVEL

O remote-feeder agora está **100% compatível** com a Central Pet Feeder após as seguintes correções:

---

## 🔧 Mudanças Implementadas

### 1. ✅ Tópicos MQTT Corrigidos

**Antes:**
```cpp
#define DEVICE_ID "petfeeder_s3_001"
#define TOPIC_CMD "petfeeder/petfeeder_s3_001/cmd"
```

**Depois:**
```cpp
#define REMOTE_ID 1
#define DEVICE_ID "remote1"
#define TOPIC_CMD "petfeeder/remote/1/cmd"
#define TOPIC_STATUS "petfeeder/remote/1/status"
#define TOPIC_DATA "petfeeder/remote/1/data"
#define TOPIC_LOGS "petfeeder/logs"
```

**Impacto:** Tópicos agora seguem o padrão esperado pela Central.

---

### 2. ✅ Parsing JSON com ArduinoJson

**Antes:**
```cpp
if (cmdStr.indexOf("\"feed\"") >= 0) {
    int qty = 100;
    int qtyIndex = cmdStr.indexOf("\"feed\":");
    // parsing manual frágil...
}
```

**Depois:**
```cpp
JsonDocument doc;
DeserializationError error = deserializeJson(doc, payload);

String cmd = doc["cmd"] | "";
if (cmd == "FEED") {
    int quantity = doc["quantity"] | 100;
    // processamento robusto...
}
```

**Impacto:** Parsing robusto e confiável de todos os comandos JSON.

---

### 3. ✅ Comandos Implementados

#### FEED - Alimentação Manual
```cpp
if (cmd == "FEED") {
    int quantity = doc["quantity"] | 100;

    if (!validateFeedQuantity(quantity)) {
        LOG("❌ Quantidade inválida: " + String(quantity) + "g");
        return;
    }

    feederService.dispense(quantity);
    publishStatus(true);
}
```

#### CONFIG_MEAL - Configurar Refeição
```cpp
else if (cmd == "CONFIG_MEAL") {
    int meal = doc["meal"] | 0;
    int hour = doc["hour"] | 0;
    int minute = doc["minute"] | 0;
    int quantity = doc["quantity"] | 0;

    // Validações
    if (!validateMealIndex(meal)) return;
    if (!validateTime(hour, minute)) return;
    if (!validateFeedQuantity(quantity)) return;

    // Configurar no ScheduleService
    scheduleService.setMeal(meal, hour, minute, quantity);
    publishStatus(true);
}
```

#### SYNC - Sincronizar Logs
```cpp
else if (cmd == "SYNC") {
    publishStatus(true);
    logService.sendPendingLogsMQTT();
}
```

#### STATUS - Solicitar Status
```cpp
else if (cmd == "STATUS") {
    publishStatus(true);
    publishData("OK");
}
```

---

### 4. ✅ Formato de Status Corrigido

**Antes:**
```json
{
  "device": "petfeeder_s3_001",
  "status": "feeding",
  "timestamp": 12345,
  "logs_pending": 0
}
```

**Depois:**
```json
{
  "online": true,
  "timestamp": 12345
}
```

**Impacto:** Central agora reconhece corretamente o status da remota.

---

### 5. ✅ Telemetria (feed_level) Implementada

```cpp
void MQTTService::publishData(const char* feedLevel) {
    if (!mqttClient.connected()) return;

    JsonDocument doc;
    doc["feed_level"] = feedLevel;
    doc["timestamp"] = clockService.getUnixTime();

    String payload;
    serializeJson(doc, payload);

    mqttClient.publish(TOPIC_DATA, payload.c_str());
}
```

**Tópico:** `petfeeder/remote/1/data`
**Formato:** `{"feed_level":"OK","timestamp":12345}`
**Valores:** "OK", "LOW", "EMPTY"

---

### 6. ✅ Logs Offline com MQTT Real

**Antes:**
```cpp
// TODO: Implementar envio MQTT real
// Por enquanto só simula o envio
for (int i = 0; i < logCount; i++) {
    LOG("📨 [SIM] Enviando log...");
}
```

**Depois:**
```cpp
void LogService::sendPendingLogsMQTT() {
    if (!mqttService.isConnected()) return;

    for (int i = 0; i < logCount; i++) {
        JsonDocument doc;
        doc["deviceId"] = DEVICE_ID;
        doc["timestamp"] = logs[i].timestamp;
        doc["qty"] = logs[i].qty;
        doc["delivered"] = logs[i].delivered;
        doc["source"] = logs[i].source;

        String payload;
        serializeJson(doc, payload);

        mqttService.publishLog(payload.c_str());
    }

    clearLogs();
}
```

**Tópico:** `petfeeder/logs`
**Formato:** `{"deviceId":"remote1","timestamp":1735689600,"qty":50,"delivered":true,"source":"rtc_auto"}`

---

### 7. ✅ Validações de Segurança

```cpp
bool MQTTService::validateFeedQuantity(int qty) {
    return (qty >= MIN_FEED_QUANTITY && qty <= MAX_FEED_QUANTITY);
}

bool MQTTService::validateMealIndex(int meal) {
    return (meal >= 0 && meal < MAX_MEALS);
}

bool MQTTService::validateTime(int hour, int minute) {
    return (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59);
}
```

**Limites:**
- Quantidade: 10g - 500g
- Índice de refeição: 0 - 2
- Horário: 00:00 - 23:59

---

### 8. ✅ Estrutura de Pastas Organizada

```
src/
├── comm/
│   └── mqtt_service.cpp        # Comunicação MQTT
├── services/
│   ├── log_service.cpp         # Logs offline
│   └── schedule_service.cpp    # Agendamentos
├── hardware/
│   └── feeder_service.cpp      # Controle do servo
└── core/
    └── rtc_service_simple.cpp  # RTC (simulado)

include/
├── comm/
│   └── mqtt_service.h
├── services/
│   ├── log_service.h
│   └── schedule_service.h
├── hardware/
│   └── feeder_service.h
└── core/
    └── rtc_service_simple.h
```

---

## 📡 Protocolo MQTT - Resumo

### Tópicos

| Tópico | Direção | Descrição |
|--------|---------|-----------|
| `petfeeder/remote/1/cmd` | Central → Remota | Comandos |
| `petfeeder/remote/1/status` | Remota → Central | Status online/offline |
| `petfeeder/remote/1/data` | Remota → Central | Telemetria (feed_level) |
| `petfeeder/logs` | Remota → Central | Logs offline |

### Comandos Suportados

| Comando | Descrição | Status |
|---------|-----------|--------|
| `FEED` | Alimentação manual | ✅ Implementado |
| `CONFIG_MEAL` | Configurar refeição | ✅ Implementado |
| `SYNC` | Sincronizar logs | ✅ Implementado |
| `STATUS` | Solicitar status | ✅ Implementado |

---

## 🧪 Teste de Compatibilidade

### 1. Testar Comando FEED
```bash
mosquitto_pub -h 192.168.1.100 -p 8883 \
  --cafile ca.crt -u usuario -P senha \
  -t "petfeeder/remote/1/cmd" \
  -m '{"cmd":"FEED","quantity":50,"timestamp":12345}'
```

**Saída esperada no Serial Monitor:**
```
📨 MQTT Recebido: petfeeder/remote/1/cmd = {"cmd":"FEED",...}
🔄 Processando comando...
🍖 Comando FEED recebido: 50g
📤 Status publicado: ONLINE
✅ Alimentação executada com sucesso
```

---

### 2. Testar Comando CONFIG_MEAL
```bash
mosquitto_pub -h 192.168.1.100 -p 8883 \
  --cafile ca.crt -u usuario -P senha \
  -t "petfeeder/remote/1/cmd" \
  -m '{"cmd":"CONFIG_MEAL","meal":0,"hour":8,"minute":30,"quantity":100,"timestamp":12345}'
```

**Saída esperada:**
```
📨 MQTT Recebido: petfeeder/remote/1/cmd = {"cmd":"CONFIG_MEAL",...}
📅 Comando CONFIG_MEAL recebido:
   Refeição: 0
   Horário: 8:30
   Quantidade: 100g
✅ Refeição configurada com sucesso
📤 Status publicado: ONLINE
```

---

### 3. Ouvir Status da Remota
```bash
mosquitto_sub -h 192.168.1.100 -p 8883 \
  --cafile ca.crt -u usuario -P senha \
  -t "petfeeder/remote/1/status" -v
```

**Saída esperada:**
```
petfeeder/remote/1/status {"online":true,"timestamp":12345}
```

---

### 4. Ouvir Logs Offline
```bash
mosquitto_sub -h 192.168.1.100 -p 8883 \
  --cafile ca.crt -u usuario -P senha \
  -t "petfeeder/logs" -v
```

**Saída esperada:**
```
petfeeder/logs {"deviceId":"remote1","timestamp":1735689600,"qty":50,"delivered":true,"source":"rtc_auto"}
```

---

## 📊 Checklist de Compatibilidade

- [x] Tópicos MQTT corretos
- [x] Formato JSON compatível
- [x] Comando FEED implementado
- [x] Comando CONFIG_MEAL implementado
- [x] Comando SYNC implementado
- [x] Comando STATUS implementado
- [x] Status publicado corretamente
- [x] Telemetria (feed_level) implementada
- [x] Logs offline com JSON correto
- [x] Validações de segurança
- [x] Parsing JSON robusto
- [x] Estrutura de pastas organizada

---

## 🎯 Resultado Final

**STATUS: ✅ 100% COMPATÍVEL**

O remote-feeder agora:
- ✅ Usa os mesmos tópicos MQTT da Central
- ✅ Reconhece todos os comandos enviados pela Central
- ✅ Publica status no formato esperado
- ✅ Envia logs offline corretamente
- ✅ Valida todos os parâmetros recebidos
- ✅ Integra perfeitamente com o ecossistema da Central Pet Feeder

---

## 📚 Documentação Relacionada

- [README.md](README.md) - Documentação completa do projeto
- [Central - PROTOCOLO_MQTT.md](../Central_Pet_Feeder/PROTOCOLO_MQTT.md)
- [Central - PROTOCOLO_LOGS_OFFLINE.md](../Central_Pet_Feeder/PROTOCOLO_LOGS_OFFLINE.md)

---

**Última atualização:** 2025-11-15
**Versão:** 2.0.0 (Compatível com Central Pet Feeder)