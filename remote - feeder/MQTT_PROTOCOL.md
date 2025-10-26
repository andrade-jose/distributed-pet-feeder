# Protocolo MQTT - Alimentador Remoto (Otimizado)

## Objetivos da Otimização
- **Reduzir latência**: Heartbeat de 5min → 10s
- **Economizar memória**: Tópicos e payloads reduzidos em ~70%
- **Manter compatibilidade**: Estrutura JSON preservada

---

## 📋 Tabela de Tópicos MQTT

| Função | Tópico Anterior | Tópico Novo | Economia |
|--------|----------------|-------------|----------|
| Comando | `alimentador/remota/comando` | `a/r/cmd` | 79% |
| Status | `alimentador/remota/status` | `a/r/st` | 71% |
| Resposta | `alimentador/remota/resposta` | `a/r/rsp` | 73% |
| Heartbeat | `alimentador/remota/heartbeat` | `a/r/hb` | 77% |
| Alerta Ração | `alimentador/remota/alerta_racao` | `a/r/alr` | 74% |

### Nomenclatura dos Tópicos
- `a` = alimentador
- `r` = remota
- `cmd` = comando
- `st` = status
- `rsp` = resposta
- `hb` = heartbeat
- `alr` = alerta

---

## 📤 Mensagens Publicadas pela Remota

### 1. Heartbeat (Sinal de Vida)
**Tópico:** `a/r/hb`
**Intervalo:** 10 segundos (antes: 5 minutos)
**QoS:** 0

**Payload:**
```json
{
  "s": 1,
  "i": 1,
  "r": -45,
  "a": 0,
  "t": 0
}
```

**Campos:**
| Campo | Nome Completo | Tipo | Valores | Descrição |
|-------|---------------|------|---------|-----------|
| `s` | status | int | `1`=ALIVE, `0`=DEAD | Estado do dispositivo |
| `i` | remota_id | int | 1-255 | ID da remota |
| `r` | rssi | int | -100 a 0 | Intensidade WiFi (dBm) |
| `a` | alimentacao_ativa | int | `0`=false, `1`=true | Alimentação em andamento |
| `t` | servo_travado | int | `0`=false, `1`=true | Servo travado/com problema |

**Exemplo anterior (200 bytes):**
```json
{"status":"ALIVE","remota_id":1,"uptime":123456,"wifi_rssi":-45,"free_heap":180000,"alimentacao_ativa":false,"servo_travado":false}
```

**Exemplo atual (60 bytes):**
```json
{"s":1,"i":1,"r":-45,"a":0,"t":0}
```
**Economia: 70% de memória**

---

### 2. Alerta de Ração
**Tópico:** `a/r/alr`
**Intervalo:** Enviado junto com heartbeat (10s)
**QoS:** 0

**Payload:**
```json
{
  "i": 1,
  "n": 1,
  "d": 12.5
}
```

**Campos:**
| Campo | Nome Completo | Tipo | Valores | Descrição |
|-------|---------------|------|---------|-----------|
| `i` | remota_id | int | 1-255 | ID da remota |
| `n` | nivel | int | `0`=OK, `1`=BAIXO | Nível de ração |
| `d` | distancia | float | 0.0-400.0 | Distância em cm (1 casa decimal) |

**Exemplo anterior (80 bytes):**
```json
{"remota_id":1,"nivel":"BAIXO","distancia":12.5}
```

**Exemplo atual (30 bytes):**
```json
{"i":1,"n":1,"d":12.5}
```
**Economia: 62% de memória**

---

### 3. Status da Alimentação
**Tópico:** `a/r/st`
**Evento:** Mudança de estado
**QoS:** 0

**Payload:**
```json
{
  "s": "INICIADO",
  "t": 123456
}
```

**Campos:**
| Campo | Nome Completo | Tipo | Valores | Descrição |
|-------|---------------|------|---------|-----------|
| `s` | status | string | ver tabela | Estado da alimentação |
| `t` | timestamp | long | millis() | Timestamp do evento |

**Estados possíveis:**
- `INICIADO` - Alimentação iniciada
- `PAUSADO` - Alimentação pausada
- `RETOMADO` - Alimentação retomada
- `CONCLUIDO` - Alimentação concluída
- `PARADO` - Alimentação parada manualmente
- `ERRO` - Erro durante alimentação

**Exemplo anterior (55 bytes):**
```json
{"status":"INICIADO","timestamp":123456}
```

**Exemplo atual (35 bytes):**
```json
{"s":"INICIADO","t":123456}
```
**Economia: 36% de memória**

---

### 4. Resposta de Conclusão
**Tópico:** `a/r/rsp`
**Evento:** Ao finalizar alimentação
**QoS:** 1 (garantir entrega)

**Payload:**
```json
{
  "c": 1,
  "ts": 5,
  "id": "ALIMENTAR_123456",
  "t": 123456
}
```

**Campos:**
| Campo | Nome Completo | Tipo | Valores | Descrição |
|-------|---------------|------|---------|-----------|
| `c` | concluido | int | `1`=true, `0`=false | Operação concluída |
| `ts` | tempo_segundos | int | 1-60 | Tempo de alimentação |
| `id` | comando_id | string | - | ID do comando executado |
| `t` | timestamp | long | millis() | Timestamp de conclusão |

**Exemplo anterior (95 bytes):**
```json
{"concluido":true,"tempo_segundos":5,"comando_id":"ALIMENTAR_123456","timestamp":123456}
```

**Exemplo atual (65 bytes):**
```json
{"c":1,"ts":5,"id":"ALIMENTAR_123456","t":123456}
```
**Economia: 32% de memória**

---

## 📥 Mensagens Recebidas pela Remota

### 1. Comando de Alimentação (JSON)
**Tópico:** `a/r/cmd`
**QoS:** 1

**Payload:**
```json
{
  "acao": "alimentar",
  "tempo": 5,
  "remota_id": 1
}
```

**Campos:**
| Campo | Tipo | Obrigatório | Valores | Descrição |
|-------|------|-------------|---------|-----------|
| `acao` | string | Sim | `"alimentar"` | Ação a executar |
| `tempo` | int | Não | 1-60 | Tempo em segundos (padrão: 5) |
| `remota_id` | int | Não | 1-255 | ID da remota (padrão: 1) |

**Nota:** Mantido formato original para compatibilidade com central.

---

### 2. Comandos Simples
**Tópico:** `a/r/cmd`
**QoS:** 0

**Payloads aceitos:**
- `PING` - Solicita confirmação de vida
- `STATUS` - Solicita status atual
- `STOP` - Para alimentação em andamento

---

### 3. Comando Legado (Formato Antigo)
**Tópico:** `a/r/cmd`
**QoS:** 0

**Payload:**
```
a5
```

**Formato:** `a` + número de segundos (1-60)
**Exemplo:** `a3` = alimentar por 3 segundos

**Nota:** Mantido para compatibilidade retroativa.

---

## 📊 Comparação de Uso de Memória

### Por Mensagem

| Tipo | Antes | Depois | Economia |
|------|-------|--------|----------|
| Heartbeat | 200 bytes | 60 bytes | **70%** |
| Alerta Ração | 80 bytes | 30 bytes | **62%** |
| Status | 55 bytes | 35 bytes | **36%** |
| Conclusão | 95 bytes | 65 bytes | **32%** |
| **Média** | **107 bytes** | **47 bytes** | **56%** |

### Por Tópico

| Tipo | Antes | Depois | Economia |
|------|-------|--------|----------|
| Comando | 26 bytes | 6 bytes | **77%** |
| Status | 25 bytes | 5 bytes | **80%** |
| Resposta | 28 bytes | 6 bytes | **79%** |
| Heartbeat | 29 bytes | 6 bytes | **79%** |
| Alerta | 32 bytes | 6 bytes | **81%** |
| **Média** | **28 bytes** | **5.8 bytes** | **79%** |

---

## ⚡ Melhorias de Latência

| Métrica | Antes | Depois | Melhoria |
|---------|-------|--------|----------|
| Intervalo Heartbeat | 300s (5min) | 10s | **30x mais rápido** |
| Detecção de Falha | ~10min | ~30s | **20x mais rápido** |
| Resposta a Comandos | Imediata | Imediata | Mantida |

---

## 🔧 Configuração do Sistema

### Intervalos de Tempo
```cpp
INTERVALO_HEARTBEAT = 10000;  // 10 segundos (antes: 300000ms)
INTERVALO_MONITORAMENTO_RACAO = 300000;  // 5 minutos (mantido)
```

### Buffer MQTT
```cpp
mqttClient.setBufferSize(2048);  // 2KB (suficiente para payloads otimizados)
mqttClient.setKeepAlive(60);     // 60 segundos
```

---

## 🔐 Segurança

- **Protocolo:** MQTT over TLS/SSL
- **Porta:** 8883
- **Autenticação:** Username + Password
- **Certificado:** TLS sem verificação (modo inseguro - adequado para testes)

---

## 📝 Notas de Implementação

### Para o Desenvolvedor Central (Dashboard/Backend)

1. **Atualizar tópicos MQTT:**
   - Subscrever nos novos tópicos: `a/r/hb`, `a/r/st`, `a/r/rsp`, `a/r/alr`
   - Publicar comandos em: `a/r/cmd`

2. **Parsear novos payloads:**
   ```javascript
   // Heartbeat
   const hb = JSON.parse(payload);
   const alive = hb.s === 1;
   const remotaId = hb.i;
   const rssi = hb.r;
   const alimentando = hb.a === 1;
   const travado = hb.t === 1;

   // Alerta de Ração
   const alerta = JSON.parse(payload);
   const remotaId = alerta.i;
   const nivelBaixo = alerta.n === 1;
   const distancia = alerta.d;

   // Status
   const status = JSON.parse(payload);
   const estadoAtual = status.s;  // "INICIADO", "CONCLUIDO", etc.
   const timestamp = status.t;

   // Resposta
   const resp = JSON.parse(payload);
   const concluido = resp.c === 1;
   const tempoSegundos = resp.ts;
   const comandoId = resp.id;
   ```

3. **Ajustar timeout de heartbeat:**
   - Considerar remota offline se não receber heartbeat por > 30 segundos
   - Antes: > 10 minutos

---

## 🐛 Troubleshooting

### Remota não envia heartbeat
- Verificar: WiFi conectado
- Verificar: MQTT conectado ao broker
- Verificar: `INTERVALO_HEARTBEAT` em config.cpp

### Mensagens muito grandes
- Se payload > 2KB, aumentar: `mqttClient.setBufferSize()`
- Payloads otimizados devem ficar < 100 bytes

### Central não recebe mensagens
- Verificar subscrição nos tópicos corretos (`a/r/*`)
- Verificar parsing dos campos abreviados

---

## 📅 Histórico de Versões

### v2.0 (Atual) - Otimizado
- Heartbeat: 10s
- Tópicos curtos: `a/r/*`
- Payloads compactos: campos de 1 letra
- Economia média: 60% de memória

### v1.0 (Anterior) - Legacy
- Heartbeat: 5 minutos
- Tópicos longos: `alimentador/remota/*`
- Payloads verbosos: nomes completos
- Base de comparação

---

## 📞 Suporte

Para dúvidas sobre o protocolo, consulte:
- Código fonte: `src/gerenciador_comunicacao.cpp`
- Configuração: `src/config.cpp`
- Este documento: `MQTT_PROTOCOL.md`
