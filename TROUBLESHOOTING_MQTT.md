# 🔧 Troubleshooting - MQTT não conecta

## 📊 Sintomas Observados

```
[MQTT] MQTT não conectado - configuração de horário não enviada
✅ Configuração salva: Remota 1, Refeição 3, 22:30, 40g
```

✅ **Funcionando:**
- Interface web
- Salvamento em Preferences
- Sincronização com display LCD

❌ **Não Funcionando:**
- Conexão MQTT (broker ou cliente)

---

## 🔍 Diagnóstico Passo a Passo

### **Passo 1: Verificar Serial Monitor**

Conecte o ESP32 via USB e abra o Serial Monitor (115200 baud). Procure por estas mensagens:

#### ✅ **Inicialização Correta** (deve aparecer)
```
=== Inicializando Sistema ===
[WiFi] WiFi conectado!
[WiFi] IP: 192.168.x.xxx
=== Inicializando Gerenciador MQTT ===
=== Iniciando Broker MQTT Local ===
BROKER MQTT INICIADO COM SUCESSO!
   IP: 192.168.x.xxx
   Porta: 1883
   Máximo de clientes: 6
===============================
Aguardando broker estabilizar...
Criando cliente MQTT...
Cliente MQTT criado com sucesso
Configurando cliente MQTT para: 192.168.x.xxx:1883
Cliente MQTT configurado com sucesso
Tentando conectar ao broker...
[MQTT] Conectando ao broker MQTT local... Conectado!
```

#### ❌ **Erros Possíveis**

**Erro 1: WiFi não conectado**
```
[WiFi] WiFi não conectado
[MQTT] WiFi não conectado - impossível iniciar broker!
```
**Solução**: Verifique credenciais WiFi no `config.h`

---

**Erro 2: Broker não inicia**
```
[MQTT] ERRO: Falha ao iniciar broker MQTT!
```
**Solução**:
- Reinicie o ESP32
- Verifique se a porta 1883 não está em uso
- Verifique memória disponível

---

**Erro 3: Cliente não criado**
```
[MQTT] ERRO CRÍTICO: Falha ao alocar mqttClient!
```
**Solução**:
- Problema de memória
- Reinicie o ESP32
- Verifique se há outros processos consumindo RAM

---

**Erro 4: mqttClient não inicializado**
```
[MQTT] ERRO: mqttClient não inicializado!
```
**Solução**:
- Problema na inicialização do cliente
- Verifique se o `init()` foi chamado corretamente
- Reinicie o sistema

---

**Erro 5: Falha ao conectar**
```
[MQTT] Conectando ao broker MQTT local... Falhou (código -2)
```
**Códigos de erro:**
- `-4`: MQTT_CONNECTION_TIMEOUT (timeout na conexão)
- `-3`: MQTT_CONNECTION_LOST (conexão perdida)
- `-2`: MQTT_CONNECT_FAILED (falha na conexão)
- `-1`: MQTT_DISCONNECTED (desconectado)
- `1-5`: Erros de protocolo/autenticação

**Solução**:
- Aguarde o broker estabilizar (delay de 1s já implementado)
- Verifique se o IP está correto
- Reinicie o ESP32

---

## 🛠️ Soluções Rápidas

### **Solução 1: Reiniciar o ESP32**
```cpp
// Pressione o botão RESET físico ou use:
ESP.restart();
```

### **Solução 2: Aumentar delay de estabilização**

**Arquivo:** `gerenciador_mqtt.cpp` linha 59

**ANTES:**
```cpp
delay(1000);  // 1 segundo
```

**DEPOIS:**
```cpp
delay(2000);  // 2 segundos - mais tempo para broker estabilizar
```

### **Solução 3: Adicionar retry na inicialização**

**Arquivo:** `gerenciador_mqtt.cpp` linha 75

**ADICIONAR APÓS `conectar()`:**
```cpp
// Tentativas adicionais se falhar
int tentativas = 0;
while (!conectado && tentativas < 5) {
    DEBUG_MQTT_PRINTF("Tentativa %d/5...\n", tentativas + 1);
    delay(2000);
    conectar();
    tentativas++;
}

if (!conectado) {
    DEBUG_MQTT_PRINTLN("AVISO: MQTT não conectou após 5 tentativas");
    DEBUG_MQTT_PRINTLN("Sistema continuará funcionando, mas sem MQTT");
}

return conectado;
```

### **Solução 4: Verificar ordem de inicialização**

**Arquivo:** `principal.cpp`

**Verificar se está nesta ordem:**
```cpp
void setup() {
    Serial.begin(115200);

    // 1. Display primeiro
    Display::init();

    // 2. Botões
    Botoes::inicializar();

    // 3. Tempo/RTC
    GerenciadorTempo::inicializar();

    // 4. WiFi Config Portal (se necessário)
    GerenciadorWiFiConfig::inicializar();

    // 5. WiFi normal
    GerenciadorWifi::inicializar();

    // ⚠️ AGUARDAR WIFI CONECTAR!
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        timeout++;
    }

    // 6. Só iniciar MQTT após WiFi conectar
    if (WiFi.status() == WL_CONNECTED) {
        GerenciadorMQTT::inicializar();
    } else {
        DEBUG_PRINTLN("ERRO: WiFi não conectou - MQTT não será iniciado");
    }

    // 7. Web server
    GerenciadorWeb::inicializar();

    // 8. Telas
    GerenciadorTelas::inicializar();
}
```

### **Solução 5: Verificar memória disponível**

**Adicionar no início do `setup()`:**
```cpp
void setup() {
    Serial.begin(115200);
    DEBUG_PRINTF("Memória livre ao iniciar: %d bytes\n", ESP.getFreeHeap());

    // ... resto do código
}
```

**Memória mínima necessária:**
- Mínimo: 80.000 bytes (80 KB)
- Recomendado: 100.000+ bytes (100+ KB)

Se a memória estiver baixa, considere:
- Reduzir tamanho dos buffers JSON
- Desabilitar debug temporariamente
- Otimizar uso de String

---

## 🧪 Testes de Diagnóstico

### **Teste 1: Verificar se broker está rodando**

**Via Serial Monitor:**
```
[MQTT] BROKER MQTT INICIADO COM SUCESSO!
[MQTT] Broker Status: ✅ Rodando
```

**Via Interface Web:**
```
http://192.168.x.xxx/api/status
```

Procure por:
```json
{
  "broker_rodando": true,
  "broker_ip": "192.168.x.xxx",
  "broker_porta": 1883
}
```

### **Teste 2: Verificar conexão do cliente**

**Via Serial Monitor:**
```
[MQTT] Cliente MQTT configurado para: 192.168.x.xxx:1883
[MQTT] Conectando ao broker MQTT local... Conectado!
```

**Via Interface Web:**
```json
{
  "mqtt": "Conectado",
  "mqtt_status": "Conectado"
}
```

### **Teste 3: Testar comando manual**

**Via Interface Web:**
1. Acesse http://192.168.x.xxx
2. Clique em "Configurar Remotas"
3. Clique em "Remota 1"
4. Clique em "Alimentar Agora"

**Verificar no Serial Monitor:**
```
[MQTT] Enviando comando GERAL para Remota 1...
[MQTT] Comando geral enviado com sucesso para Remota 1
```

Se aparecer:
```
[MQTT] MQTT não conectado - comando geral não enviado
```

O problema está confirmado!

---

## 📝 Checklist de Verificação

- [ ] WiFi está conectado?
- [ ] Broker MQTT iniciou com sucesso?
- [ ] Cliente MQTT foi criado?
- [ ] Cliente MQTT está conectado ao broker?
- [ ] Memória disponível é suficiente (>80KB)?
- [ ] Ordem de inicialização está correta?
- [ ] Delay de estabilização é suficiente?
- [ ] Não há erros no Serial Monitor?
- [ ] Interface web mostra broker rodando?
- [ ] Comandos MQTT são enviados com sucesso?

---

## 🔄 Fluxo Correto de Inicialização

```
1. Serial.begin(115200)
   ↓
2. Display::init()
   ↓
3. GerenciadorTempo::inicializar()
   ↓
4. GerenciadorWifi::inicializar()
   ↓
5. AGUARDAR WiFi conectar (até 10 segundos)
   ↓
6. ✅ WiFi conectado?
   ↓ SIM
7. GerenciadorMQTT::inicializar()
   ├─ iniciarBroker()
   ├─ delay(1000)
   ├─ criar mqttClient
   ├─ configurarMQTT()
   └─ conectar()
   ↓
8. ✅ Broker rodando?
   ↓ SIM
9. ✅ Cliente conectado?
   ↓ SIM
10. GerenciadorWeb::inicializar()
    ↓
11. GerenciadorTelas::inicializar()
    ↓
12. ✅ SISTEMA PRONTO!
```

---

## 💡 Dica: Adicionar Logs de Debug

**Adicionar no loop principal** (`principal.cpp`):

```cpp
void loop() {
    static unsigned long ultimoDebug = 0;
    unsigned long agora = millis();

    // Debug a cada 30 segundos
    if (agora - ultimoDebug > 30000) {
        ultimoDebug = agora;

        DEBUG_PRINTLN("========== STATUS ==========");
        DEBUG_PRINTF("WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "Conectado" : "Desconectado");

        if (GerenciadorMQTT::instance) {
            DEBUG_PRINTF("MQTT Broker: %s\n", GerenciadorMQTT::instance->brokerEstaRodando() ? "Rodando" : "Parado");
            DEBUG_PRINTF("MQTT Cliente: %s\n", GerenciadorMQTT::instance->estaConectado() ? "Conectado" : "Desconectado");
            DEBUG_PRINTF("Clientes: %d\n", GerenciadorMQTT::instance->getClientesConectados());
        } else {
            DEBUG_PRINTLN("MQTT instance: nullptr");
        }

        DEBUG_PRINTF("Memória livre: %d bytes\n", ESP.getFreeHeap());
        DEBUG_PRINTLN("============================");
    }

    // ... resto do loop
}
```

---

## ✅ Solução Definitiva Recomendada

Se nenhuma das soluções acima funcionar, implemente esta versão robusta:

**Arquivo:** `gerenciador_mqtt.cpp` - substituir função `inicializar()`

```cpp
void GerenciadorMQTT::inicializar() {
    if (!instance) {
        instance = new GerenciadorMQTT();
    }

    // Verificar WiFi antes de tudo
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_MQTT_PRINTLN("AVISO: WiFi não conectado - aguardando...");

        int tentativas = 0;
        while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
            delay(500);
            DEBUG_MQTT_PRINT(".");
            tentativas++;
        }

        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_MQTT_PRINTLN("\nERRO: WiFi não conectou - MQTT não será iniciado");
            return;
        }

        DEBUG_MQTT_PRINTLN("\nWiFi conectado!");
    }

    // Tentar inicializar com retries
    bool sucesso = false;
    for (int i = 0; i < 3 && !sucesso; i++) {
        DEBUG_MQTT_PRINTF("Tentativa de inicialização MQTT %d/3...\n", i + 1);
        sucesso = instance->init();

        if (!sucesso) {
            DEBUG_MQTT_PRINTLN("Falhou - aguardando antes de tentar novamente...");
            delay(2000);
        }
    }

    if (sucesso) {
        DEBUG_MQTT_PRINTLN("✅ MQTT inicializado com sucesso!");
    } else {
        DEBUG_MQTT_PRINTLN("❌ MQTT falhou após 3 tentativas");
        DEBUG_MQTT_PRINTLN("Sistema continuará funcionando sem MQTT");
    }
}
```

---

## 📞 Ainda não resolveu?

Se após todas essas soluções o MQTT ainda não conectar:

1. **Copie os logs completos** do Serial Monitor desde o início até a tentativa de conexão
2. **Verifique a mensagem de erro específica** que aparece
3. **Teste se o broker inicia** (deve aparecer "BROKER MQTT INICIADO COM SUCESSO!")
4. **Teste se o cliente é criado** (deve aparecer "Cliente MQTT criado com sucesso")
5. **Verifique qual linha falha** exatamente

Com essas informações, será possível identificar o problema exato!

---

**Autor**: Claude Code
**Data**: 2025-01-11
**Versão**: 1.0
