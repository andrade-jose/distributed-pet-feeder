# 🎯 Sistema de Logs Offline com RTC

## 📌 Visão Geral

Este documento descreve as **mudanças necessárias na Central** para suportar o sistema de RTC + Logs Offline das Remotas.

### Contexto

As **Remotas** possuem:
- ✅ RTC (DS3231) para operação autônoma
- ✅ Sistema de agendamento (3 refeições programáveis)
- ✅ Armazenamento local de logs (NVS)
- ✅ Sincronização de logs quando voltam online

A **Central** precisa:
- ✅ Receber logs offline das remotas
- ✅ Processar logs com timestamps Unix
- ✅ Repassar histórico para o Dashboard

---

## 🔄 Mudanças Necessárias na Central

### ✅ Complexidade: **Muito Baixa** (~25 linhas de código)

As mudanças são **pequenas e não invasivas**:

| Item | Status | Descrição |
|------|--------|-----------|
| 1. Novo tópico MQTT | ✅ | Subscribe em `petfeeder/logs` |
| 2. Parser de logs | ✅ | Processar JSON com timestamp |
| 3. Repasse ao Dashboard | ✅ | Publicar em `petfeeder/dashboard/history` |

### ❌ O que NÃO muda:
- Lógica de envio de comandos (continua igual)
- Estrutura MQTT existente
- Dashboard atual (só recebe dados extras)

---

## 📡 Novo Tópico MQTT

### Tópico de Logs Offline

**Tópico:** `petfeeder/logs`
**Direção:** Remotas → Central → Dashboard
**Formato:** JSON

```json
{
  "deviceId": "remote1",
  "timestamp": 1735689600,
  "qty": 50,
  "delivered": true,
  "source": "rtc_auto"
}
```

**Campos:**
- `deviceId` (string): ID da remota que gerou o log
- `timestamp` (long): Unix timestamp (segundos desde 1970-01-01)
- `qty` (int): Quantidade alimentada (gramas)
- `delivered` (bool): Se a alimentação foi bem-sucedida
- `source` (string): Origem da alimentação
  - `"rtc_auto"` - Disparada automaticamente pelo RTC
  - `"manual"` - Disparada manualmente (botão/LCD)
  - `"mqtt"` - Disparada por comando MQTT

---

## 🔧 Implementação na Central

### 1. Definir Constante do Tópico

**Arquivo:** `include/config.h`

```cpp
// Tópico de logs offline (remotas enviam histórico quando voltam online)
#define MQTT_TOPIC_LOGS MQTT_TOPIC_PREFIX "/logs"
```

---

### 2. Subscribe no Tópico de Logs

**Arquivo:** `src/main.cpp` → Função `initMQTT()`

```cpp
void initMQTT() {
    // ... código existente ...

    if (mqttClient.connect()) {
        Serial.println("✅ MQTT conectado!");

        // Inscrições existentes
        mqttClient.subscribe(MQTT_TOPIC_CENTRAL_CMD);

        // ← ADICIONAR ESTA LINHA
        mqttClient.subscribe(MQTT_TOPIC_LOGS);

        publishCentralStateToDA();
    }
}
```

---

### 3. Processar Logs Recebidos

**Arquivo:** `src/main.cpp` → Função `onMQTTMessage()`

Adicionar **antes** do bloco `if (topic == MQTT_TOPIC_CENTRAL_CMD)`:

```cpp
void onMQTTMessage(const String& topic, const String& payload) {
    Serial.printf("[MQTT] Mensagem recebida: %s -> %s\n", topic.c_str(), payload.c_str());

    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.printf("[MQTT] Erro ao parsear JSON: %s\n", error.c_str());
        return;
    }

    // ========== LOGS OFFLINE DAS REMOTAS ========== ← ADICIONAR ESTE BLOCO
    if (topic == MQTT_TOPIC_LOGS) {
        Serial.println("📥 Log offline recebido da remota:");

        // Extrair informações do log
        String deviceId = doc["deviceId"] | "";
        long timestamp = doc["timestamp"] | 0;
        int quantity = doc["qty"] | 0;
        bool delivered = doc["delivered"] | false;
        String source = doc["source"] | "";

        // Log detalhado no Serial
        Serial.printf("   Device: %s\n", deviceId.c_str());
        Serial.printf("   Timestamp: %ld\n", timestamp);
        Serial.printf("   Quantidade: %dg\n", quantity);
        Serial.printf("   Status: %s\n", delivered ? "✅ Sucesso" : "❌ Falha");
        Serial.printf("   Origem: %s\n", source.c_str());

        // Repassa para Dashboard (tópico separado para histórico)
        mqttClient.publish("petfeeder/dashboard/history", payload, false);

        Serial.println("   ↳ Log repassado para Dashboard");
        return;
    }

    // ========== COMANDOS DO DASHBOARD ==========
    if (topic == MQTT_TOPIC_CENTRAL_CMD) {
        // ... código existente ...
    }
}
```

---

## 🔄 Fluxo Completo de Logs Offline

```
┌─────────────────────────────────────────────────────────┐
│                    1. REMOTA OFFLINE                    │
│   - RTC dispara alimentação às 08:00                    │
│   - Log salvo localmente na NVS:                        │
│     {"timestamp": 1735689600, "qty": 50, ...}           │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                 2. REMOTA VOLTA ONLINE                  │
│   - Conecta ao Wi-Fi e MQTT                             │
│   - Detecta logs pendentes na NVS                       │
│   - Publica em "petfeeder/logs"                         │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                    3. CENTRAL RECEBE                    │
│   - Callback onMQTTMessage() é disparado                │
│   - Processa JSON do log                                │
│   - Exibe informações no Serial Monitor                 │
│   - Republica em "petfeeder/dashboard/history"          │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                   4. DASHBOARD RECEBE                   │
│   - Assina "petfeeder/dashboard/history"                │
│   - Recebe logs com timestamps                          │
│   - Exibe histórico de alimentações offline             │
│   - Permite análise temporal dos dados                  │
└─────────────────────────────────────────────────────────┘
```

---

## 🎯 Exemplo de Saída no Serial Monitor

Quando uma remota envia um log offline, a Central exibe:

```
[MQTT←] petfeeder/logs: {"deviceId":"remote1","timestamp":1735689600,"qty":50,"delivered":true,"source":"rtc_auto"}
📥 Log offline recebido da remota:
   Device: remote1
   Timestamp: 1735689600
   Quantidade: 50g
   Status: ✅ Sucesso
   Origem: rtc_auto
   ↳ Log repassado para Dashboard
[MQTT→] petfeeder/dashboard/history: {"deviceId":"remote1","timestamp":1735689600,"qty":50,"delivered":true,"source":"rtc_auto"}
```

---

## 📊 Resumo das Mudanças

### Arquivos Modificados

| Arquivo | Mudança | Linhas |
|---------|---------|--------|
| `include/config.h` | Adicionar `MQTT_TOPIC_LOGS` | +2 |
| `src/main.cpp` (initMQTT) | Subscribe no tópico de logs | +2 |
| `src/main.cpp` (onMQTTMessage) | Processar logs offline | +21 |
| **TOTAL** | | **~25 linhas** |

### Componentes Afetados

| Componente | **Antes** | **Depois** | Motivo |
|------------|-----------|------------|--------|
| **Tópicos MQTT** | Só publica comandos | Publica comandos + assina logs | Receber histórico das remotas |
| **Callback MQTT** | Processa comandos + status | Processa comandos + status + logs | Novo tipo de mensagem |
| **Processamento** | Envia + recebe status | Envia + recebe status + repassa logs | Central vira "ponte" de dados |
| **Dashboard** | Tempo real apenas | Tempo real + histórico offline | Melhor visibilidade |

---

## 🧪 Como Testar

### 1. Simular Remota Enviando Log Offline

```bash
# Publicar log simulado
mosquitto_pub -h IP_MOSQUITTO -p 8883 \
  --cafile ca.crt -u petfeeder -P senha \
  -t "petfeeder/logs" \
  -m '{"deviceId":"remote1","timestamp":1735689600,"qty":50,"delivered":true,"source":"rtc_auto"}'
```

**Resultado esperado na Central:**
```
📥 Log offline recebido da remota:
   Device: remote1
   Timestamp: 1735689600
   Quantidade: 50g
   Status: ✅ Sucesso
   Origem: rtc_auto
   ↳ Log repassado para Dashboard
```

---

### 2. Ouvir Logs Repassados ao Dashboard

```bash
# Dashboard deve assinar este tópico
mosquitto_sub -h IP_MOSQUITTO -p 8883 \
  --cafile ca.crt -u petfeeder -P senha \
  -t "petfeeder/dashboard/history" -v
```

**Saída esperada:**
```
petfeeder/dashboard/history {"deviceId":"remote1","timestamp":1735689600,"qty":50,"delivered":true,"source":"rtc_auto"}
```

---

### 3. Monitorar Toda Comunicação

```bash
# Ver todos os tópicos
mosquitto_sub -h IP_MOSQUITTO -p 8883 \
  --cafile ca.crt -u petfeeder -P senha \
  -t "petfeeder/#" -v
```

---

## 🔐 Formato dos Timestamps

### Unix Timestamp (Segundos desde 1970-01-01)

**Exemplo:** `1735689600` = **2025-01-01 00:00:00 UTC**

### Conversão no Dashboard (JavaScript)

```javascript
// Converter timestamp para data legível
const timestamp = 1735689600;
const date = new Date(timestamp * 1000);  // Multiplicar por 1000 (JS usa milissegundos)

console.log(date.toLocaleString('pt-BR', {
  year: 'numeric',
  month: '2-digit',
  day: '2-digit',
  hour: '2-digit',
  minute: '2-digit',
  second: '2-digit'
}));
// Saída: "01/01/2025 00:00:00"
```

### Conversão em Python (Backend do Dashboard)

```python
from datetime import datetime

timestamp = 1735689600
date = datetime.fromtimestamp(timestamp)

print(date.strftime('%Y-%m-%d %H:%M:%S'))
# Saída: "2025-01-01 00:00:00"
```

---

## 🚀 Implementação no Dashboard

### Node-RED: Receber Histórico de Logs

```javascript
// Flow Node-RED
[
    {
        "id": "mqtt-in-history",
        "type": "mqtt in",
        "topic": "petfeeder/dashboard/history",
        "broker": "mosquitto-broker",
        "outputs": 1
    },
    {
        "id": "process-log",
        "type": "function",
        "func": `
            const log = msg.payload;

            // Converter timestamp
            const date = new Date(log.timestamp * 1000);

            // Adicionar ao banco de dados
            msg.topic = "INSERT INTO feed_history (device_id, timestamp, quantity, delivered, source) VALUES (?, ?, ?, ?, ?)";
            msg.payload = [
                log.deviceId,
                date.toISOString(),
                log.qty,
                log.delivered,
                log.source
            ];

            return msg;
        `,
        "outputs": 1
    },
    {
        "id": "db-insert",
        "type": "sqlite",
        "db": "/data/petfeeder.db",
        "outputs": 1
    }
]
```

### React/Vue/Angular: Exibir Histórico

```javascript
// Componente React exemplo
import { useState, useEffect } from 'react';
import mqtt from 'mqtt';

function FeedHistory() {
    const [history, setHistory] = useState([]);

    useEffect(() => {
        const client = mqtt.connect('mqtts://IP_MOSQUITTO:8883', {
            username: 'petfeeder',
            password: 'senha'
        });

        client.subscribe('petfeeder/dashboard/history');

        client.on('message', (topic, message) => {
            const log = JSON.parse(message.toString());

            // Adicionar ao histórico
            setHistory(prev => [{
                device: log.deviceId,
                date: new Date(log.timestamp * 1000),
                quantity: log.qty,
                success: log.delivered,
                source: log.source === 'rtc_auto' ? 'Automático (RTC)' :
                        log.source === 'mqtt' ? 'Remoto (MQTT)' : 'Manual'
            }, ...prev]);
        });

        return () => client.end();
    }, []);

    return (
        <div>
            <h2>Histórico de Alimentações</h2>
            <table>
                <thead>
                    <tr>
                        <th>Dispositivo</th>
                        <th>Data/Hora</th>
                        <th>Quantidade</th>
                        <th>Status</th>
                        <th>Origem</th>
                    </tr>
                </thead>
                <tbody>
                    {history.map((log, i) => (
                        <tr key={i}>
                            <td>{log.device}</td>
                            <td>{log.date.toLocaleString('pt-BR')}</td>
                            <td>{log.quantity}g</td>
                            <td>{log.success ? '✅' : '❌'}</td>
                            <td>{log.source}</td>
                        </tr>
                    ))}
                </tbody>
            </table>
        </div>
    );
}
```

---

## 🎓 Diferenças de Origem (`source`)

| Valor | Descrição | Quando Ocorre |
|-------|-----------|---------------|
| `rtc_auto` | Alimentação automática disparada pelo RTC | Horário programado atingido, remota offline |
| `mqtt` | Comando recebido via MQTT | Dashboard ou Central enviou comando |
| `manual` | Alimentação manual local | Usuário apertou botão na remota ou configurou pelo LCD |

Essa flag permite ao Dashboard diferenciar alimentações **offline automáticas** de **comandos manuais**.

---

## ✅ Checklist de Implementação

- [x] **Central**
  - [x] Adicionar `MQTT_TOPIC_LOGS` em `config.h`
  - [x] Subscribe em `petfeeder/logs` no `initMQTT()`
  - [x] Processar logs em `onMQTTMessage()`
  - [x] Repassar logs para `petfeeder/dashboard/history`

- [ ] **Dashboard**
  - [ ] Subscribe em `petfeeder/dashboard/history`
  - [ ] Converter timestamps Unix para data/hora local
  - [ ] Armazenar histórico (banco de dados ou localStorage)
  - [ ] Exibir tabela/gráfico de alimentações

- [ ] **Remota** (já implementado em outra branch)
  - [ ] RTC configurado e sincronizado
  - [ ] Logs salvos localmente na NVS
  - [ ] Envio de logs pendentes ao conectar MQTT

---

## 📚 Referências

- [PROTOCOLO_MQTT.md](PROTOCOLO_MQTT.md) - Protocolo completo Dashboard ↔ Central ↔ Remotas
- Documentação PubSubClient: https://pubsubclient.knolleary.net/
- Documentação ArduinoJson: https://arduinojson.org/
- DS3231 RTC Datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf

---

**Pronto para integração! 🎯**