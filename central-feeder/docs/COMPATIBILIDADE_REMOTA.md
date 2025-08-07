# 🔧 Compatibilidade Central ↔ Remota

## 📋 Resumo das Mudanças Necessárias

Este documento detalha as mudanças necessárias no código da **REMOTA** para garantir compatibilidade total com a **CENTRAL**.

---

## 🎯 Mudanças Obrigatórias

### 1. 📡 **Correção dos Tópicos MQTT**

**PROBLEMA:** O tópico de conclusão está incorreto

**LOCALIZAÇÃO:** Constantes no início do arquivo principal da remota

**MUDANÇA NECESSÁRIA:**
```cpp
// ❌ ATUAL (INCORRETO):
const char* TOPIC_CONCLUIDO = "alimentador/remota/concluido";

// ✅ CORRETO:
const char* TOPIC_CONCLUIDO = "alimentador/remota/resposta";
```

### 2. 🎯 **Adicionar Processamento do Comando "alimentar"**

**PROBLEMA:** A remota não processa o comando `"alimentar"` enviado pela central

**LOCALIZAÇÃO:** Função `processarComandoCentral(String payload)`

**CÓDIGO A ADICIONAR:** (Inserir **ANTES** da verificação de JSON existente)

```cpp
// ===== PROCESSAR COMANDO "alimentar" DA CENTRAL =====
if (payload.indexOf("\"acao\":\"alimentar\"") > 0) {
    Serial.println("📥 Comando 'alimentar' recebido da central");
    
    // Extrair tempo (se fornecido)
    int startTempo = payload.indexOf("\"tempo\":") + 8;
    int endTempo = payload.indexOf(",", startTempo);
    if (endTempo == -1) endTempo = payload.indexOf("}", startTempo);
    
    int tempoSegundos = 5; // padrão
    if (startTempo > 8) {
        String tempoStr = payload.substring(startTempo, endTempo);
        tempoSegundos = tempoStr.toInt();
    }
    
    // Extrair remota_id se fornecido
    int startId = payload.indexOf("\"remota_id\":") + 12;
    int endId = payload.indexOf(",", startId);
    if (endId == -1) endId = payload.indexOf("}", startId);
    
    int remotaId = 1; // padrão
    if (startId > 12) {
        String idStr = payload.substring(startId, endId);
        remotaId = idStr.toInt();
    }
    
    Serial.printf("🎯 Comando alimentar: %d segundos para remota %d\n", tempoSegundos, remotaId);
    
    // Converter tempo em movimentos (1 movimento por segundo)
    int movimentos = tempoSegundos;
    if (movimentos < 1) movimentos = 1;
    if (movimentos > 20) movimentos = 20;
    
    idComandoAtual = "ALIMENTAR_" + String(millis());
    enviarStatusMQTT("INICIANDO_" + idComandoAtual);
    iniciarAlimentacao(movimentos);
    return;
}
```

### 3. 💓 **Corrigir Formato do Heartbeat**

**PROBLEMA:** A central espera status "ALIVE" e campo "remota_id"

**LOCALIZAÇÃO:** Função `enviarHeartbeat()`

**MUDANÇA NECESSÁRIA:**
```cpp
// ❌ ATUAL (INCORRETO):
String statusSistema;
if (alimentacaoAtiva) {
    statusSistema = "ATIVO";
} else if (servoTravado) {
    statusSistema = "INATIVO";
} else {
    statusSistema = "DISPONIVEL";
}

String statusTravamento = servoTravado ? "_TRAVADO" : "";
String payload = "{\"status\":\"" + statusSistema + statusTravamento +
                 "\",\"uptime\":" + String(millis()) +
                 ",\"wifi_rssi\":" + String(WiFi.RSSI()) +
                 ",\"free_heap\":" + String(ESP.getFreeHeap()) + "}";

// ✅ CORRETO:
String payload = "{\"status\":\"ALIVE\"" +
                 ",\"remota_id\":1" +
                 ",\"uptime\":" + String(millis()) +
                 ",\"wifi_rssi\":" + String(WiFi.RSSI()) +
                 ",\"free_heap\":" + String(ESP.getFreeHeap()) +
                 ",\"alimentacao_ativa\":" + String(alimentacaoAtiva ? "true" : "false") +
                 ",\"servo_travado\":" + String(servoTravado ? "true" : "false") + "}";
```

### 4. 🆔 **Corrigir Username MQTT**

**PROBLEMA:** Username incompatível com a central

**LOCALIZAÇÃO:** Constantes no início do arquivo principal da remota

**MUDANÇA NECESSÁRIA:**
```cpp
// ❌ ATUAL (INCORRETO):
const char* MQTT_USERNAME = "Romota1";

// ✅ CORRETO:
const char* MQTT_USERNAME = "Central";
```

---

## 📊 Comparação de Comunicação

### Antes das Mudanças (❌ Incompatível)
```
CENTRAL envia:  alimentador/remota/comando → {"acao":"alimentar","tempo":5,"remota_id":1}
REMOTA processa: ❌ Comando ignorado (não reconhece "acao":"alimentar")

REMOTA envia:   alimentador/remota/heartbeat → {"status":"ATIVO","uptime":12345}
CENTRAL espera: ❌ Campo "remota_id" ausente

REMOTA envia:   alimentador/remota/concluido → {"concluido":true}
CENTRAL espera: ❌ Tópico incorreto (deveria ser "resposta")
```

### Depois das Mudanças (✅ Compatível)
```
CENTRAL envia:  alimentador/remota/comando → {"acao":"alimentar","tempo":5,"remota_id":1}
REMOTA processa: ✅ Inicia alimentação por 5 movimentos

REMOTA envia:   alimentador/remota/heartbeat → {"status":"ALIVE","remota_id":1,"uptime":12345}
CENTRAL recebe: ✅ Remota aparece como "ON" na interface

REMOTA envia:   alimentador/remota/resposta → {"concluido":true,"movimentos":5}
CENTRAL recebe: ✅ Confirmação de conclusão processada
```

---

## 🔍 Como Verificar se Funcionou

### 1. **Monitor Serial da Central**
Procure por estas mensagens:
```
📥 === MENSAGEM MQTT RECEBIDA ===
   Tópico: alimentador/remota/heartbeat
   Payload: {"status":"ALIVE","remota_id":1,...}
   Processando HEARTBEAT GERAL
💗 Remota 1 está VIVA
[TELAS] Remota 1 heartbeat atualizado: ON
```

### 2. **Interface LCD da Central**
- **Antes:** `Remota 1: OFF`
- **Depois:** `Remota 1: OK`

### 3. **Teste do Comando**
O teste automático da central deve resultar em:
```
✅ Comando de teste enviado com sucesso!
```

E na remota:
```
📥 MQTT recebido [alimentador/remota/comando]: {"acao":"alimentar","remota_id":1,"timestamp":7037,"tempo":5}
📥 Comando 'alimentar' recebido da central
🎯 Comando alimentar: 5 segundos para remota 1
🍽️ INICIANDO ALIMENTAÇÃO
```

---

## 📁 Arquivos Afetados na Remota

- **Principal:** `src/main.cpp` ou arquivo principal
  - Constantes MQTT (tópicos e username)
  - Função `processarComandoCentral()`
  - Função `enviarHeartbeat()`

---

## ⚠️ Notas Importantes

1. **Backup:** Faça backup do código da remota antes das mudanças
2. **Teste:** Teste cada mudança individualmente
3. **Debug:** Mantenha o Serial Monitor ativo para acompanhar
4. **ID da Remota:** Este exemplo usa `remota_id: 1`. Para múltiplas remotas, cada uma deve ter ID único

---

## 🚀 Próximos Passos

1. ✅ Implementar mudanças na remota
2. ✅ Upload do código modificado
3. ✅ Verificar heartbeat na central  
4. ✅ Testar comando "alimentar"
5. ✅ Verificar interface LCD (ON/OFF)
6. ✅ Testar comunicação bidirecional completa

---

**Data:** 02/08/2025  
**Versão:** 1.0  
**Status:** Pronto para implementação
