# 📡 Protocolo MQTT - Dashboard ↔ Central ↔ Remotas

## 🔄 Arquitetura de Comunicação

```
Dashboard (Web/Mobile)
       ↕️ MQTT
Central Gateway (ESP32)
       ↕️ MQTT
Remotas (ESP32s)
```

**IMPORTANTE:**
- O Dashboard **NUNCA** conversa diretamente com as Remotas
- Toda comunicação passa pela Central
- A Central age como **proxy inteligente**

---

## 📬 Tópicos MQTT

### 1️⃣ Dashboard → Central (Comandos)

**Tópico:** `petfeeder/central/cmd`

#### Configurar Refeição
```json
{
  "cmd": "CONFIG_MEAL",
  "remote_id": 1,
  "meal": 0,
  "hour": 8,
  "minute": 30,
  "quantity": 100,
  "timestamp": 12345
}
```

#### Alimentação Manual
```json
{
  "cmd": "FEED_NOW",
  "remote_id": 1,
  "quantity": 50,
  "timestamp": 12345
}
```

#### Solicitar Estado Completo
```json
{
  "cmd": "GET_STATE",
  "timestamp": 12345
}
```

---

### 2️⃣ Central → Dashboard (Estado)

**Tópico:** `petfeeder/central/status`
**Retain:** `true` (mantém último estado)

```json
{
  "timestamp": 12345,
  "status": "ONLINE",
  "uptime": 12345,
  "remotes_count": 4,
  "remotes_online": 2,
  "remotes": [
    {
      "id": 1,
      "name": "Remota 1",
      "online": true,
      "feed_level": "OK",
      "last_seen": 12340,
      "meals": [
        {
          "hour": 8,
          "minute": 0,
          "quantity": 100,
          "enabled": true
        },
        {
          "hour": 12,
          "minute": 0,
          "quantity": 150,
          "enabled": true
        },
        {
          "hour": 18,
          "minute": 0,
          "quantity": 100,
          "enabled": true
        }
      ]
    },
    {
      "id": 2,
      "name": "Remota 2",
      "online": false,
      "feed_level": "LOW",
      "last_seen": 10000,
      "meals": [...]
    }
  ]
}
```

**Quando é publicado:**
- ✅ Ao conectar no broker (inicial)
- ✅ A cada 30 segundos (heartbeat)
- ✅ Quando uma remota muda de status (online/offline)
- ✅ Quando o nível de ração muda
- ✅ Quando uma refeição é configurada (Dashboard ou LCD)
- ✅ Quando o Dashboard solicita (`GET_STATE`)

---

### 3️⃣ Central → Remotas (Comandos)

**Tópico:** `petfeeder/remote/{ID}/cmd`

#### Configurar Refeição
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

#### Alimentar Agora
```json
{
  "cmd": "FEED",
  "quantity": 50,
  "timestamp": 12345
}
```

---

### 4️⃣ Remotas → Central (Status)

**Tópico:** `petfeeder/remote/{ID}/status`

```json
{
  "online": true,
  "timestamp": 12345
}
```

---

### 5️⃣ Remotas → Central (Dados/Telemetria)

**Tópico:** `petfeeder/remote/{ID}/data`

```json
{
  "feed_level": "OK",
  "timestamp": 12345
}
```

**Possíveis valores de `feed_level`:**
- `"OK"` - Nível normal
- `"LOW"` - Nível baixo (< 30%)
- `"EMPTY"` - Vazio (< 10%)

---

## 🔄 Fluxos de Dados

### Fluxo 1: Dashboard Configura Refeição

```
1. Dashboard → petfeeder/central/cmd
   { "cmd": "CONFIG_MEAL", "remote_id": 1, "meal": 0, ... }

2. Central recebe e processa:
   - Atualiza RemoteManager
   - Salva em ConfigManager
   - Publica em petfeeder/remote/1/cmd

3. Central → petfeeder/central/status
   { estado completo atualizado com retain }

4. Remota recebe e aplica configuração

5. Dashboard recebe estado atualizado automaticamente
```

### Fluxo 2: Usuário Configura pelo LCD

```
1. Usuário altera refeição no LCD da Central

2. MenuController chama onMealConfigChanged()

3. Central:
   - Atualiza RemoteManager
   - Salva em ConfigManager
   - Publica em petfeeder/remote/1/cmd
   - Publica em petfeeder/central/status

4. Dashboard recebe estado atualizado automaticamente
```

### Fluxo 3: Remota Muda Status

```
1. Remota → petfeeder/remote/1/status
   { "online": true }

2. Central recebe e processa:
   - Atualiza RemoteManager
   - Publica estado completo em petfeeder/central/status

3. Dashboard recebe estado atualizado automaticamente
```

### Fluxo 4: Nível de Ração Baixo

```
1. Remota → petfeeder/remote/1/data
   { "feed_level": "LOW" }

2. Central recebe e processa:
   - Atualiza RemoteManager
   - Publica estado completo em petfeeder/central/status
   - LCD exibe alerta "RACAO BAIXA"

3. Dashboard recebe estado atualizado e exibe alerta
```

---

## 🎯 Sincronização de Estado

### Garantia de Consistência

1. **Fonte Única de Verdade:** A Central mantém o estado autoritativo
2. **Retain no Status:** O último estado fica no broker (novos clientes recebem imediatamente)
3. **Publicação Automática:** Qualquer mudança gatilha publicação
4. **Heartbeat:** Estado completo a cada 30s garante sincronização

### Exemplo de Timeline

```
T+0s   : Central conecta, publica estado inicial (retain)
T+5s   : Dashboard conecta, recebe estado (do retain)
T+10s  : Remota 1 conecta, envia status
T+10.1s: Central publica estado atualizado
T+10.2s: Dashboard recebe atualização
T+15s  : Dashboard configura refeição
T+15.1s: Central atualiza e publica
T+15.2s: Dashboard recebe confirmação
T+15.3s: Remota 1 recebe comando
T+30s  : Heartbeat - Central publica estado
```

---

## 🔐 Segurança

- ✅ **TLS:** Todas as conexões usam porta 8883 com certificado
- ✅ **Autenticação:** Username/password obrigatório
- ✅ **Validação:** Central valida todos os comandos antes de repassar
- ✅ **Isolamento:** Remotas não podem publicar em tópicos da Central

---

## 🧪 Como Testar

### 1. Testar Dashboard → Central

```bash
# Simular Dashboard configurando refeição
mosquitto_pub -h IP_MOSQUITTO -p 8883 \
  --cafile ca.crt -u petfeeder -P senha \
  -t "petfeeder/central/cmd" \
  -m '{"cmd":"CONFIG_MEAL","remote_id":1,"meal":0,"hour":8,"minute":30,"quantity":100,"timestamp":12345}'

# Ouvir resposta da Central
mosquitto_sub -h IP_MOSQUITTO -p 8883 \
  --cafile ca.crt -u petfeeder -P senha \
  -t "petfeeder/central/status" -v
```

### 2. Testar Remota → Central

```bash
# Simular Remota enviando status
mosquitto_pub -h IP_MOSQUITTO -p 8883 \
  --cafile ca.crt -u petfeeder -P senha \
  -t "petfeeder/remote/1/status" \
  -m '{"online":true,"timestamp":12345}'

# Simular Remota enviando dados
mosquitto_pub -h IP_MOSQUITTO -p 8883 \
  --cafile ca.crt -u petfeeder -P senha \
  -t "petfeeder/remote/1/data" \
  -m '{"feed_level":"LOW","timestamp":12345}'
```

### 3. Monitorar Toda Comunicação

```bash
# Ouvir TODOS os tópicos
mosquitto_sub -h IP_MOSQUITTO -p 8883 \
  --cafile ca.crt -u petfeeder -P senha \
  -t "petfeeder/#" -v
```

---

## 📊 Diagrama de Sequência

```
Dashboard          Central          Remota
   |                 |                |
   |--CONFIG_MEAL--->|                |
   |                 |--UPDATE------->| (internal)
   |                 |--CONFIG_MEAL-->|
   |<--STATUS--------|                |
   |   (retain)      |                |
   |                 |<--STATUS-------|
   |<--STATUS--------|                |
   |   (updated)     |                |
```

---

## 🚀 Implementação no Dashboard

### Conectar e Ouvir Estado

```javascript
const mqtt = require('mqtt');

const client = mqtt.connect('mqtts://IP_MOSQUITTO:8883', {
  username: 'petfeeder',
  password: 'senha',
  ca: fs.readFileSync('ca.crt')
});

client.on('connect', () => {
  // Inscrever no status da central
  client.subscribe('petfeeder/central/status');

  // Solicitar estado atual
  client.publish('petfeeder/central/cmd', JSON.stringify({
    cmd: 'GET_STATE',
    timestamp: Date.now()
  }));
});

client.on('message', (topic, message) => {
  if (topic === 'petfeeder/central/status') {
    const state = JSON.parse(message.toString());
    console.log('Estado da central:', state);
    // Atualizar UI do Dashboard
    updateDashboard(state);
  }
});
```

### Configurar Refeição

```javascript
function configurarRefeicao(remoteId, mealIndex, hour, minute, quantity) {
  client.publish('petfeeder/central/cmd', JSON.stringify({
    cmd: 'CONFIG_MEAL',
    remote_id: remoteId,
    meal: mealIndex,
    hour: hour,
    minute: minute,
    quantity: quantity,
    timestamp: Date.now()
  }));

  // Estado atualizado virá automaticamente via petfeeder/central/status
}
```

---

**Pronto para integração com o Dashboard! 🎯**