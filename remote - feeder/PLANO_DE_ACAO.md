
---

# 📐 **PLANO DE AÇÃO — Refatoração e Novas Funcionalidades da REMOTA (ESP32 + RTC + Offline Logging)**

## 📌 Objetivo Geral

Modernizar e profissionalizar o firmware da REMOTA, adicionando:

* RTC real (DS3231/DS1307)
* Agendamento autônomo offline
* Logs persistentes com timestamp
* Sincronização automática ao voltar online
* Modularização completa (SOLID)
* Remoção de itens obsoletos

---

# 🧩 **Fase 1 — Reestruturação da Arquitetura do Código**

## 🎯 Objetivo:

Deixar o código organizado em módulos, fácil de manter e escalar.

## 🗂️ Criar Novos Módulos

| Arquivo                   | Responsabilidade                   |
| ------------------------- | ---------------------------------- |
| `rtc_service.cpp/.h`      | RTC, hora atual, ajuste via NTP    |
| `schedule_service.cpp/.h` | 3 refeições + NVS + checagem       |
| `feeder_service.cpp/.h`   | Controle do servo/motor e sensores |
| `log_service.cpp/.h`      | Logs offline + NVS + envio MQTT    |
| `mqtt_service.cpp/.h`     | Subscribe, publish e comandos      |
| `system_status.cpp/.h`    | LEDs, estado online/offline        |

## 🧱 Nova Estrutura do `main.cpp`

```cpp
void setup() {
    rtcService.begin();
    scheduleService.load();
    feederService.begin();
    mqttService.begin();
}

void loop() {
    mqttService.loop();
    scheduleService.loop();
    logService.loop();
    feederService.loop();
}
```

## 📦 Criar `models.h`

```cpp
struct Meal {
    uint8_t hour;
    uint8_t minute;
    uint16_t qty;
    bool enabled;
};

struct FeedLog {
    uint32_t timestamp;
    uint16_t qty;
    bool delivered;
    char source[12]; // "mqtt", "rtc_auto", "manual"
};
```

---

# 🧹 **Fase 2 — Remover Itens Desnecessários**

### ❌ Remover completamente:

* `millis()` para controle de horário
* contadores de tempo manuais
* delays grandes no loop
* scheduler improvisado
* logs sem timestamp
* variáveis duplicadas fora de módulos
* funções de servo espalhadas

---

# 🕰️ **Fase 3 — Implementar RTC (Core do Sistema)**

### 🧩 Criar `rtc_service.cpp`

Responsabilidades:

* Inicializar RTC
* Verificar `lostPower()`
* Ajustar horário via NTP quando online
* Retornar `DateTime now()`
* Expor `uint32_t getUnixTime()`

### 🔄 Fluxo de inicialização

1. Inicia RTC
2. Se perdeu energia → define hora padrão
3. Se MQTT conectar → sincroniza via NTP
4. Agenda ativa imediatamente com base no RTC

---

# 📅 **Fase 4 — Sistema de Horário Autônomo**

### 🧩 Criar `schedule_service.cpp`

Funções:

* `load()`: carrega refeições da NVS
* `save()`: salva refeições
* `checkMeals()`: compara RTC com horários
* `shouldFeedNow()`: trava anti-duplicação

### 🔥 Remover:

* toda lógica baseada em `millis`
* bloqueios de tempo por loop
* verificações duplicadas em `main`

---

# 🍖 **Fase 5 — Execução da Alimentação**

### 🧩 Criar `feeder_service.cpp`

Com:

```cpp
bool dispense(int qty);
```

### Destaques:

* retorno booleano (true = sucesso)
* confirmação via sensor (opcional)
* tempo de execução mínimo
* sem delays grandes no loop

---

# 🗂️ **Fase 6 — Sistema de Logs Offline**

### 🧩 Criar `log_service.cpp`

Funções obrigatórias:

```cpp
addLog(timestamp, qty, delivered, source);
saveLogs();
loadLogs();
sendPendingLogsMQTT();
clearLogs();
```

### Armazenamento:

* Usar NVS (`Preferences`)
* Guardar array com até 50 logs

### Envio:

* Ao reconectar ao MQTT, enviar tudo para `petfeeder/logs`

---

# 📡 **Fase 7 — Revisão do Módulo MQTT**

### 🧩 Criar `mqtt_service.cpp`

Responsabilidades:

* Conectar / reconectar
* Assinar tópicos:

  * `petfeeder/<id>/cmd`
  * `petfeeder/logs` (somente central)
* Interpretar comandos:

  * `"schedule"`
  * `"feed"`
* Publicar estados
* Publicar logs offline ao reconectar

### Regras:

* NÃO misturar lógica de alimentação com parse MQTT
* JSON parse isolado
* Publicações organizadas por função

---

# 🧠 **Fase 8 — Sistema de Status (Online / Offline)**

### 🧩 Criar `system_status.cpp`

Funções:

* `setOnline()`
* `setOffline()`
* controle de LEDs
* flag `bool isOnline`

Uso:

```cpp
if (!mqttClient.connected()) systemStatus.setOffline();
else systemStatus.setOnline();
```

---

# 🧪 **Fase 9 — Testes Funcionais**

### **RTC**

* testar `lostPower()`
* boot sem RTC conectado
* sincronização via NTP

### **Horário**

* definir hora manual
* simular troca de minuto
* verificar disparo exato

### **Feeder**

* teste com quantidades variadas
* simular falha de sensor

### **Logs Offline**

* gerar 3 logs
* reiniciar
* conectar MQTT
* verificar envio automático

### **MQTT**

* enviar `"feed"`
* enviar `"schedule"`

---

# 🧼 **Fase 10 — Limpeza Final**

### 🧹 Padronizar prints

```cpp
#define DEBUG 1
#if DEBUG
#define LOG(x) Serial.println(x)
#else
#define LOG(x)
#endif
```

### 🗂️ Colocar defines em `config.h`

* tópicos MQTT
* pinos
* DEVICE_ID
* limites de tempo
* tamanho do buffer de log

### 📄 Atualizar README

---

# 📘 **Resumo Final (Checklist)**

### Arquitetura

* [ ] Criar módulos (RTC, schedule, feeder, logs, MQTT, status)
* [ ] Criar `models.h`
* [ ] Atualizar `main.cpp`

### RTC

* [ ] Implementar módulo
* [ ] Remover `millis()` antigo

### Horários

* [ ] Refatorar para usar RTC
* [ ] Remover scheduler improvisado

### Alimentação

* [ ] Centralizar execução
* [ ] Retorno booleano

### Logs Offline

* [ ] Implementar persistência
* [ ] Sincronizar ao conectar

### MQTT

* [ ] Separar parse
* [ ] Assinar comandos
* [ ] Novo tópico de logs

### Modo Offline

* [ ] LED
* [ ] Flag `isOnline`

### Testes

* [ ] RTC
* [ ] Horário
* [ ] Alimentação
* [ ] MQTT
* [ ] Logs

---
