# 🐾 ALIMENTADOR CENTRAL - ESP32 AUTOMATIZADO

**Versão:** 1.0.0  
**Plataforma:** ESP32 Arduino Framework  
**Atualizado:** 02/08/2025  

Sistema central automatizado para controle de até 6 alimentadores remotos via MQTT, com interface LCD e programação de horários.

---

## 🚀 **CARACTERÍSTICAS PRINCIPAIS**

### ✅ **Sistema Completamente Funcional:**
- **Programação automática** de 3 refeições por remota (horário + quantidade)
- **Interface LCD 20x4** com navegação por botões (UP/DOWN/ENTER)
- **Conectividade WiFi** com auto-reconexão e persistência de credenciais
- **Comunicação MQTT** segura via HiveMQ Cloud (SSL/TLS)
- **Monitoramento em tempo real** do status das remotas (heartbeat)
- **Atualização automática** do horário no LCD (sem pressionar botões)
- **Sistema de debug** completo com logs detalhados

### 🏗️ **Arquitetura:**
```
Central ESP32 (Master)
├── WiFi (auto-conecta)
├── NTP (sincronização de hora)
├── MQTT (HiveMQ Cloud SSL)
├── LCD 20x4 I2C
├── 3 Botões (UP/DOWN/ENTER)
└── Controla até 6 Remotas ESP32
```

---

## ⚙️ **CONFIGURAÇÃO RÁPIDA**

### **1. Hardware Necessário:**
- ESP32 DevKit (qualquer modelo)
- LCD 20x4 I2C (endereço 0x27)
- 3 Botões (GPIO 32, 33, 25)
- Módulo RTC DS1307 (opcional - usa NTP)

### **2. Credenciais WiFi/MQTT:**
Edite `include/config.h`:
```cpp
#define WIFI_SSID           "Sua_WiFi"
#define WIFI_PASSWORD       "Sua_Senha"
#define MQTT_SERVER         "9aa85a8cfb4a4ba896f2289aa408ba5a.s1.eu.hivemq.cloud"
#define MQTT_USERNAME       "Central"
#define MQTT_PASSWORD       "Senha1234"
```

### **3. Compilar e Carregar:**
```bash
pio run --target upload
pio device monitor
```

---

## 📖 **DOCUMENTAÇÃO COMPLETA**

### **📁 docs/ - Documentação Detalhada:**

| Arquivo | Descrição |
|---------|-----------|
| **README_SISTEMA.md** | 📋 **Documentação principal completa** |
| HIVEMQ_CONFIG.md | 🌐 Configuração MQTT HiveMQ Cloud |
| CONFIGURAR_WIFI.md | 📡 Setup WiFi e credenciais |
| SISTEMA_CONFIG.md | ⚙️ Sistema de configuração centralizado |
| ESPECIFICACAO_TELAS.md | 🖥️ Interface LCD e navegação |
| SISTEMA_TEMPO.md | ⏰ Sistema de tempo e NTP |
| WIFI_BACKEND.md | 📶 Backend WiFi e auto-reconexão |
| LOGICA_REMOTAS.md | 🔄 Lógica de comunicação MQTT |
| COMPATIBILIDADE_REMOTA.md | 🔧 Setup das remotas |
| MQTT_CENTRAL_CONFIG.md | 📡 Configuração MQTT detalhada |

---

## 🎮 **COMO USAR**

### **Interface LCD - Navegação:**
```
[UP] ↑     - Mover para cima/aumentar valores
[DOWN] ↓   - Mover para baixo/diminuir valores  
[ENTER] →  - Confirmar/entrar no item selecionado
```

### **Menu Principal:**
```
> Configurar Remotas
  Monitorar Status
  Configurar WiFi
  Informações Sistema
```

### **Programar Refeição:**
1. **Selecionar remota** (1-6)
2. **Escolher refeição** (1-3)
3. **Configurar horário** (HH:MM)
4. **Definir quantidade** (5-200g)
5. **Salvar** ✅

### **Monitoramento Automático:**
- ✅ **Horário atual** atualiza automaticamente no LCD
- ✅ **Status das remotas** monitorado continuamente
- ✅ **Comandos automáticos** enviados nos horários programados
- ✅ **Debug logs** mostram todas as ações no Serial Monitor

---

## 🔧 **DESENVOLVIMENTO**

### **Estrutura do Código:**
```
src/
├── principal.cpp           # Loop principal e inicialização
├── gerenciador_telas.cpp   # Interface LCD e menus
├── gerenciador_tempo.cpp   # Tempo, NTP e agendamentos  
├── gerenciador_wifi.cpp    # WiFi e MQTT
├── display.cpp             # Controle do LCD
├── botoes.cpp             # Leitura dos botões
└── menu.cpp               # Sistema de menus
```

### **Configuração Centralizada:**
Todas as configurações em `include/config.h`:
- Pinos do hardware
- Credenciais WiFi/MQTT  
- Timeouts e delays
- Configurações de debug
- Parâmetros das refeições

### **Sistema de Debug:**
```cpp
#define DEBUG_ENABLED       true    // Debug geral
#define DEBUG_WIFI          true    // Debug WiFi/MQTT
#define DEBUG_BUTTONS       false   // Debug botões
```

---

## 📊 **STATUS ATUAL**

### ✅ **100% Implementado e Funcionando:**
- [x] Interface LCD completa com todos os menus
- [x] Sistema WiFi com auto-reconexão
- [x] Comunicação MQTT segura (HiveMQ Cloud)
- [x] Programação de horários e quantidades
- [x] Envio automático de comandos para remotas
- [x] Monitoramento de status (heartbeat)
- [x] Atualização automática do horário no LCD
- [x] Sistema de configuração centralizado
- [x] Debug completo com logs detalhados

### 🔄 **Funcionamento Automático:**
- **Verificação de horários:** A cada 30 segundos
- **Heartbeat remotas:** Monitoramento contínuo
- **Atualização LCD:** Automática quando hora muda
- **Reconexão WiFi:** Automática se desconectar
- **Reconexão MQTT:** Automática com backoff

---

## 🌐 **MQTT - TÓPICOS UTILIZADOS**

### **Comandos (Central → Remota):**
```json
Tópico: alimentador/remota/comando
Payload: {
  "acao": "alimentar",
  "remota_id": 1,
  "quantidade": 40,
  "tempo": 4
}
```

### **Status (Remota → Central):**
```json
Tópico: alimentador/remota/1/status  
Payload: {
  "status": "alimentando",
  "quantidade": 40,
  "tempo_restante": 3
}
```

### **Heartbeat (Remota → Central):**
```json
Tópico: alimentador/remota/heartbeat
Payload: {
  "remota_id": 1,
  "timestamp": 1234567890,
  "status": "ok"
}
```

---

## 🚨 **TROUBLESHOOTING**

### **Problema: LCD não funciona**
```
Solução: Verificar conexões I2C e endereço (0x27)
```

### **Problema: WiFi não conecta**
```
Solução: Verificar credenciais em config.h
```

### **Problema: MQTT não conecta**
```
Solução: 
1. Verificar hora NTP sincronizada
2. Verificar credenciais MQTT
3. Verificar conexão internet
```

### **Problema: Remotas não respondem**
```
Solução:
1. Verificar se remotas estão conectadas
2. Verificar tópicos MQTT
3. Ver logs de debug no Serial Monitor
```

---

## 🔗 **LINKS ÚTEIS**

- **🌐 HiveMQ Console:** https://console.hivemq.cloud/
- **📊 MQTT Test Client:** https://www.hivemq.com/demos/websocket-client/
- **📖 PlatformIO:** https://platformio.org/
- **🔧 ESP32 Docs:** https://docs.espressif.com/

---

**🎯 Sistema pronto para produção! Documentação completa na pasta `docs/`.**
