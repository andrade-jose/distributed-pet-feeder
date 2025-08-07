# Sistema de Configuração Centralizado ✅

## Arquivo config.h Implementado

O sistema agora possui um **arquivo de configuração centralizado** que permite alterar todas as definições sem mexer no código principal.

## 📁 include/config.h - Configurações Centralizadas

### 🔧 **Hardware**
```cpp
// Pinos dos botões
#define UP_BUTTON_PIN       32
#define DOWN_BUTTON_PIN     33
#define ENTER_BUTTON_PIN    25

// LCD I2C
#define LCD_I2C_ADDR        0x27
#define LCD_COLS            20
#define LCD_ROWS            4
```

### ⏱️ **Timing e Timeouts**
```cpp
#define BUTTON_DEBOUNCE_MS          200     // Debounce dos botões
#define INFO_SCREEN_TIMEOUT         10000   // Timeout telas de info
#define WIFI_RECONNECT_INTERVAL     30000   // Intervalo reconexão WiFi
#define MAIN_LOOP_DELAY_MS          50      // Delay loop principal
```

### 🍽️ **Configurações das Refeições**
```cpp
#define MAX_REMOTAS                 6
#define REFEICOES_PER_REMOTA       3
#define MIN_QUANTIDADE_GRAMAS      5
#define MAX_QUANTIDADE_GRAMAS      200
#define STEP_QUANTIDADE_GRAMAS     5

// Horários padrão
#define REFEICAO_1_PADRAO          "08:00"
#define REFEICAO_2_PADRAO          "14:30"
#define REFEICAO_3_PADRAO          "20:00"
#define QUANTIDADE_PADRAO          40
```

### 📡 **Comunicação**
```cpp
#define SERIAL_BAUD_RATE           115200
#define WIFI_MAX_NETWORKS          20
#define UDP_LOCAL_PORT             4210
#define MQTT_PORT                  1883
```

### 🐛 **Debug Configurável**
```cpp
#define DEBUG_ENABLED              true
#define DEBUG_WIFI                 true
#define DEBUG_BUTTONS              false

// Macros automáticas
#define DEBUG_PRINT(x)         // Ativo só se DEBUG_ENABLED
#define DEBUG_PRINTLN(x)       // Ativo só se DEBUG_ENABLED
#define DEBUG_WIFI_PRINTLN(x)  // Ativo só se DEBUG_WIFI
```

### 💾 **Armazenamento**
```cpp
#define PREFS_WIFI_NAMESPACE       "wifi"
#define PREFS_WIFI_SSID            "ssid"
#define PREFS_WIFI_PASSWORD        "password"
```

### 📊 **Versão do Sistema**
```cpp
#define SYSTEM_VERSION             "1.0.0"
#define SYSTEM_BUILD_DATE          __DATE__
#define SYSTEM_BUILD_TIME          __TIME__
```

## 🔄 Arquivos Atualizados

### ✅ **Todos os headers atualizados:**
- `buttons.h` - Usa `UP_BUTTON_PIN`, `BUTTON_DEBOUNCE_MS`
- `display.h` - Usa `LCD_I2C_ADDR`, `LCD_COLS`, `LCD_ROWS`
- `screen_manager.h` - Usa `REFEICOES_PER_REMOTA`, `INFO_SCREEN_TIMEOUT`
- `wifi_manager.h` - Usa `WIFI_MAX_NETWORKS`

### ✅ **Todos os .cpp atualizados:**
- `main.cpp` - Usa `SERIAL_BAUD_RATE`, `MAIN_LOOP_DELAY_MS`, debug macros
- `buttons.cpp` - Usa pinos e debug do config.h
- `display.cpp` - Usa configurações LCD do config.h

## 🎯 Como Usar o Sistema de Configuração

### **1. Alterar Pinos dos Botões**
```cpp
// Em config.h - apenas alterar aqui!
#define UP_BUTTON_PIN       15    // Era 32
#define DOWN_BUTTON_PIN     16    // Era 33
#define ENTER_BUTTON_PIN    17    // Era 25
```

### **2. Alterar Endereço do LCD**
```cpp
// Em config.h
#define LCD_I2C_ADDR        0x3F  // Era 0x27
```

### **3. Alterar Timeouts**
```cpp
// Em config.h
#define INFO_SCREEN_TIMEOUT         5000    // 5s em vez de 10s
#define BUTTON_DEBOUNCE_MS          100     // 100ms em vez de 200ms
```

### **4. Alterar Configurações de Refeição**
```cpp
// Em config.h
#define REFEICOES_PER_REMOTA       4       // 4 refeições em vez de 3
#define QUANTIDADE_PADRAO          50      // 50g em vez de 40g
#define MAX_QUANTIDADE_GRAMAS      300     // Máximo 300g
```

### **5. Controlar Debug**
```cpp
// Em config.h - desabilitar debug para produção
#define DEBUG_ENABLED              false
#define DEBUG_WIFI                 false
```

## 💡 Vantagens do Sistema

### ✅ **Facilidade de Manutenção**
- **Uma única linha** para alterar qualquer configuração
- **Sem necessidade** de procurar valores pelo código
- **Compilação automática** com novos valores

### ✅ **Diferentes Ambientes**
```cpp
// Desenvolvimento
#define DEBUG_ENABLED              true
#define INFO_SCREEN_TIMEOUT         3000    // Timeout rápido para teste

// Produção  
#define DEBUG_ENABLED              false
#define INFO_SCREEN_TIMEOUT         10000   // Timeout normal
```

### ✅ **Hardware Flexível**
```cpp
// ESP32 DevKit
#define UP_BUTTON_PIN       32

// ESP32-C3 (pinos diferentes)
#define UP_BUTTON_PIN       2
```

### ✅ **Debug Inteligente**
- **DEBUG_PRINT()** só compila se `DEBUG_ENABLED = true`
- **Zero overhead** em produção quando debug desabilitado
- **Debug específico** por módulo (WiFi, Buttons, etc.)

## 🚀 Resultado Final

Agora você pode **configurar todo o sistema** editando apenas o arquivo `config.h`:

- 🔧 **Pinos do hardware**
- ⏱️ **Timeouts e delays**
- 🍽️ **Configurações das refeições**
- 📡 **Parâmetros de comunicação**
- 🐛 **Níveis de debug**
- 📊 **Informações de versão**

**Tudo em um só lugar, sem mexer no código principal!** ✨

### **Exemplo de uso:**
1. Mudar `UP_BUTTON_PIN` de 32 para 15 no `config.h`
2. Compilar
3. **Todo o sistema** usa automaticamente o novo pino

Perfeito para diferentes versões de hardware ou ambientes de desenvolvimento! 🎯
