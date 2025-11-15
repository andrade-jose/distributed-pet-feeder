# 🚀 CONFIGURAÇÃO MQTT CENTRAL - HIVEMQ CLOUD

## 📋 **1. CONFIGURAÇÃO ESPECÍFICA DA CENTRAL**

A **Central** atua como o **controlador principal** que:
- ✅ **PUBLICA** comandos para as remotas
- ✅ **INSCREVE** nos status das remotas  
- ✅ **Gerencia** os horários de alimentação
- ✅ **Monitora** o sinal de vida das remotas

## 🔧 **2. CONFIGURAR CREDENCIAIS DA CENTRAL**

### 2.1 Criar Usuário da Central no HiveMQ:
1. No cluster HiveMQ, vá em **"Access Management"**
2. Clique na aba **"Credentials"**
3. Clique em **"Add Credentials"**
4. Configure:
   ```
   Username: Central1
   Password: SenhaC123
   ```
5. Clique em **"Add"**

### 2.2 Configurar Permissões da Central:
1. Vá na aba **"Access Control"**
2. Clique em **"Add Permission"**
3. Configure:
   ```
   Topic Filter: alimentador/#
   Permission: Publish & Subscribe
   Client Identifier: ESP32_Central_001
   ```
4. Clique em **"Add"**

## ⚙️ **3. CONFIGURAÇÃO NO ESP32 CENTRAL**

Edite o arquivo `include/config.h`:

```cpp
// ===== CONFIGURAÇÃO MQTT HIVEMQ CLOUD - CENTRAL =====

// Configurações do servidor HiveMQ Cloud
#define MQTT_SERVER "SUA_CLUSTER_URL.hivemq.cloud"  // Cole a URL do seu cluster
#define MQTT_PORT 8884                               // Porta WebSocket SSL
#define MQTT_USE_SSL true                            // WebSocket SSL habilitado

// Credenciais específicas da CENTRAL
#define MQTT_CLIENT_ID "ESP32_Central_001"          // ID único da central
#define MQTT_USERNAME "Central1"                    // Usuário específico da central
#define MQTT_PASSWORD "SenhaC123"                   // Senha específica da central
```

## 📡 **4. TÓPICOS MQTT DA CENTRAL**

### 4.1 Tópicos que a CENTRAL PUBLICA (Comandos para Remotas):

| Tópico | Propósito | Exemplo de Payload |
|--------|-----------|-------------------|
| `alimentador/remota/1/comando` | Comando geral | `{"acao": "INICIAR"}` |
| `alimentador/remota/1/horario` | Configurar horário | `{"hora": 8, "minuto": 30, "quantidade": 40}` |
| `alimentador/remota/1/tempo` | Tempo de movimento | `{"tempo_movimento": 4}` |

### 4.2 Tópicos que a CENTRAL INSCREVE (Status das Remotas):

| Tópico | Propósito | Exemplo de Payload |
|--------|-----------|-------------------|
| `alimentador/remota/1/status` | Status da remota | `{"status": "ONLINE", "ultimo_movimento": "08:30"}` |
| `alimentador/remota/1/vida` | Sinal de vida (heartbeat) | `{"timestamp": 1234567890, "status": "ALIVE"}` |
| `alimentador/remota/1/resposta` | Resposta a comandos | `{"resultado": "MOVIMENTO_CONCLUIDO"}` |

## 🎯 **5. LÓGICA DE FUNCIONAMENTO**

### 5.1 Quando o horário de uma refeição chegar:
1. **Central** verifica se é hora da alimentação
2. **Central** publica no tópico `alimentador/remota/X/comando`:
   ```json
   {
     "acao": "INICIAR",
     "tempo": 4,
     "timestamp": 1654123456
   }
   ```
3. **Remota** recebe o comando e inicia o movimento
4. **Remota** responde em `alimentador/remota/X/resposta`:
   ```json
   {
     "resultado": "MOVIMENTO_INICIADO",
     "timestamp": 1654123456
   }
   ```

### 5.2 Configuração de Tempo de Movimento:
- **Regra**: Cada 10g = 1 segundo de movimento
- **Exemplo**: 40g = 4 segundos
- **Central** publica em `alimentador/remota/X/tempo`:
   ```json
   {
     "tempo_movimento": 4
   }
   ```

### 5.3 Monitoramento de Sinal de Vida:
- **Remotas** enviam heartbeat a cada 30 segundos
- **Central** monitora em `alimentador/remota/X/vida`
- Se não receber por 2 minutos → marcar como OFF

## 🧪 **6. TESTAR COMUNICAÇÃO**

### 6.1 Teste Manual via HiveMQ Websocket Client:

1. **Conectar como Central**:
   ```
   Host: SUA_CLUSTER_URL.hivemq.cloud
   Port: 8884
   Username: Central1
   Password: SenhaC123
   SSL: ✅ (ativado)
   ```

2. **Inscrever nos tópicos de status**:
   - `alimentador/remota/1/status`
   - `alimentador/remota/1/vida`
   - `alimentador/remota/1/resposta`

3. **Publicar comando de teste**:
   - **Tópico**: `alimentador/remota/1/comando`
   - **Payload**: `{"acao": "STATUS"}`

### 6.2 Resultado Esperado no Serial Monitor da Central:
```
🌐 WebSocket SSL: Ativado
📍 Protocolo: WSS (WebSocket Secure)
✅ MQTT conectado como Central!
📥 Inscrito em: alimentador/remota/+/status
📥 Inscrito em: alimentador/remota/+/vida
📥 Inscrito em: alimentador/remota/+/resposta
⏰ Horário detectado: 08:30 - Enviando comando para Remota 1
📤 MQTT enviado [alimentador/remota/1/comando]: {"acao":"INICIAR","tempo":4}
📥 MQTT recebido [alimentador/remota/1/resposta]: {"resultado":"MOVIMENTO_INICIADO"}
```

## 📊 **7. ESTRUTURA DE DADOS**

### 7.1 Comando de Alimentação:
```json
{
  "acao": "INICIAR",
  "tempo": 4,
  "timestamp": 1654123456,
  "refeicao_id": 1
}
```

### 7.2 Configuração de Horário:
```json
{
  "refeicao_id": 1,
  "hora": 8,
  "minuto": 30,
  "quantidade": 40,
  "ativo": true
}
```

### 7.3 Status da Remota:
```json
{
  "status": "ONLINE",
  "ultimo_movimento": "2025-08-02T08:30:00Z",
  "proxima_refeicao": "2025-08-02T14:30:00Z",
  "timestamp": 1654123456
}
```

### 7.4 Sinal de Vida (Heartbeat):
```json
{
  "timestamp": 1654123456,
  "status": "ALIVE",
  "uptime": 3600
}
```

## 🚨 **8. TRATAMENTO DE ERROS**

### 8.1 Remota Não Responde:
- **Central** envia comando
- Aguarda resposta por 10 segundos
- Se não receber → marca remota como OFF
- Exibe mensagem no LCD: "Remota X: SEM RESPOSTA"

### 8.2 Falha na Conexão MQTT:
- **Central** tenta reconectar automaticamente
- Exibe no LCD: "MQTT: Reconectando..."
- Se falhar após 5 tentativas → usar modo offline

### 8.3 Comando Rejeitado:
- **Remota** responde com erro
- **Central** exibe no LCD: "Remota X: ERRO"
- Log detalhado no Serial Monitor

## ⚡ **9. VERIFICAÇÃO FINAL**

**Fluxo Completo Funcionando:**

1. **Central** conecta ao MQTT ✅
2. **Central** inscreve nos tópicos das remotas ✅  
3. **Central** recebe heartbeat das remotas ✅
4. **Central** envia comando no horário ✅
5. **Remota** confirma recebimento ✅
6. **Remota** confirma conclusão ✅
7. **Central** atualiza status no LCD ✅

**Mensagens no Serial Monitor:**
```
✅ Central MQTT conectada!
📥 Heartbeat Remota 1: ALIVE
📥 Heartbeat Remota 2: ALIVE  
⏰ 08:30 - Iniciando alimentação Remota 1
📤 Comando enviado para Remota 1
📥 Remota 1: MOVIMENTO_INICIADO
📥 Remota 1: MOVIMENTO_CONCLUIDO
✅ Alimentação Remota 1 concluída!
```

---

**✅ PRONTO!** A Central está configurada para gerenciar todas as remotas via MQTT HiveMQ Cloud.
