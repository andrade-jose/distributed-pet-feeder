# 📶 CONFIGURAÇÃO WiFi - CENTRAL ALIMENTADOR

**Versão:** 1.0.0  
**Atualizado:** 02/08/2025  
**Status:** ✅ Funcionando

---

## 🏠 **CONFIGURAÇÃO ATUAL**

### **Credenciais Ativas:**
- **SSID:** `Coelhoandrade`  
- **Status:** ✅ Conectado automaticamente
- **IP:** `192.168.15.26`
- **Qualidade:** Excelente (-45 dBm)

---

## 🔧 **MÉTODOS DE CONFIGURAÇÃO**

### **1. 📱 Via Interface LCD (Recomendado)**

**Navegação:**
```
Menu Principal
├── WiFi (ENTER)
    ├── Info WiFi         # Status atual da conexão
    ├── Configurar WiFi   # Mudar credenciais  
    └── Voltar
```

**Passos para trocar WiFi:**
1. **Menu Principal** → "WiFi" → ENTER
2. **Info WiFi** → Ver status atual
3. **Configurar WiFi** → Lista redes disponíveis  
4. **Selecionar rede** → UP/DOWN para escolher
5. **ENTER** → Digite senha character por character
6. **Confirmar** → Sistema salva e conecta automaticamente

### **2. 💻 Via Código (Desenvolvimento)**

**Editar `include/config.h`:**
```cpp
// WiFi Configuration  
#define DEFAULT_WIFI_SSID           "SuaRedeWiFi"
#define DEFAULT_WIFI_PASSWORD       "SuaSenha123"
```

**Para aplicar:**
1. Modificar credenciais no código
2. `pio run --target upload`
3. Sistema conecta automaticamente

---

## 📋 **CARACTERÍSTICAS DO SISTEMA WiFi**

### **✅ Funcionalidades:**
- **Conexão automática** na inicialização
- **Reconexão automática** se perder sinal
- **Persistência** de credenciais na EEPROM
- **Scan de redes** disponíveis via interface
- **Qualidade de sinal** em tempo real
- **Sincronização NTP** automática após conectar

### **📊 Status de Conectividade:**
- **Monitor contínuo** de qualidade do sinal
- **Reconexão inteligente** em caso de queda
- **Timeout configurável** para tentativas
- **Debug detalhado** via Serial Monitor

---

## �️ **TROUBLESHOOTING**

### **Problema: Não conecta WiFi**
```bash
# Verificar no Serial Monitor:
WiFi conectado: Coelhoandrade (192.168.15.26)
```

**Soluções:**
1. **Verificar credenciais** via interface LCD
2. **Aproximar do roteador** para melhor sinal
3. **Reiniciar ESP32** (botão RESET)
4. **Verificar se rede existe** (fazer scan)

### **Problema: Conecta mas perde conexão**
**Sintomas:**
```bash
Evento: WiFi desconectado
Tentando reconectar...
```

**Soluções:**
1. **Verificar estabilidade** do roteador
2. **Verificar interferências** (outros dispositivos)
3. **Ajustar posição** do ESP32
4. **Reiniciar roteador** se necessário

### **Problema: IP não obtido**
**Sintomas:**
```bash
WiFi conectado mas sem IP
```

**Soluções:**
1. **Verificar DHCP** do roteador
2. **Aguardar** alguns segundos (timeout automático)
3. **Reconectar** via interface

---

## 📊 **INFORMAÇÕES TÉCNICAS**

### **Configurações Atuais:**
```cpp
// Timeouts e tentativas
#define WIFI_CONNECT_TIMEOUT        15000    // 15 segundos
#define WIFI_RECONNECT_ATTEMPTS     3        // 3 tentativas
#define WIFI_SCAN_MAX_NETWORKS      10       // Máximo 10 redes

// Monitoramento
#define WIFI_CHECK_INTERVAL         5000     // Verificar a cada 5s
#define WIFI_SIGNAL_SAMPLES         5        // Média de 5 amostras
```

### **Persistência:**
- **Namespace:** `wifi_config`
- **Chaves:** `ssid`, `password`, `auto_connect`
- **Backup:** Automático na EEPROM

---

## 🔗 **INTEGRAÇÃO COM OUTROS SISTEMAS**

### **Após Conexão WiFi:**
1. ✅ **Sincronização NTP** automática
2. ✅ **Conexão MQTT** automática  
3. ✅ **Atualização interface** em tempo real
4. ✅ **Enable funcionalidades** que dependem de internet

### **Durante Funcionamento:**
- **Monitoramento contínuo** da qualidade
- **Logs detalhados** no Serial Monitor
- **Indicação visual** no LCD (status em tempo real)
- **Recovery automático** em caso de problemas

---

**💡 Para configuração via interface, use os botões UP/DOWN/ENTER. Para desenvolvimento, edite `config.h` diretamente.** 
        ├── Rede2: VizinhoWiFi [****]
        └── Manual (digitar SSID)
```

---

### 💾 **Opção 3: Configuração Programática**

**Para automação** - Via código no setup():

```cpp
void setup() {
    // ... inicializações ...
    
    // Conectar a rede específica
    WiFiManager::connect("MinhaRede", "minhaSenha");
    
    // ... resto do setup ...
}
```

---

## 📋 **Exemplo Prático**

### **Para testar rapidamente:**

1. **Edite `config.h`:**
   ```cpp
   #define DEFAULT_WIFI_SSID           "iPhone de João"
   #define DEFAULT_WIFI_PASSWORD       "123456789"
   ```

2. **Compile e upload:**
   ```bash
   pio run --target upload
   ```

3. **Monitor serial:**
   ```bash
   pio device monitor
   ```

4. **Você verá:**
   ```
   === Inicializando WiFi Manager ===
   Usando credenciais padrão do config.h
   Tentando conectar automaticamente...
   Conectando ao WiFi: iPhone de João
   WiFi conectado: iPhone de João (192.168.1.100)
   ```

---

## 🔄 **Como o Sistema Funciona**

### **Prioridade de Credenciais:**
1. **Credenciais salvas** (via interface LCD)
2. **Credenciais do config.h** (se não há salvas)
3. **Manual** (via interface)

### **Persistência:**
- ✅ **Salva automaticamente** quando configura via LCD
- ✅ **Carrega automaticamente** na próxima inicialização
- ✅ **Reconecta automaticamente** se perder conexão

### **Status em Tempo Real:**
- **Tela HOME**: Mostra se conectado
- **Tela WiFi**: Mostra SSID, IP, qualidade do sinal
- **Serial Monitor**: Log detalhado de conexão

---

## 🚨 **Dicas Importantes**

### **Para Desenvolvimento:**
```cpp
// config.h - deixe suas credenciais para teste
#define DEFAULT_WIFI_SSID           "SuaRede"
#define DEFAULT_WIFI_PASSWORD       "SuaSenha"
```

### **Para Produção:**
```cpp
// config.h - deixe vazio para usar apenas interface
#define DEFAULT_WIFI_SSID           ""
#define DEFAULT_WIFI_PASSWORD       ""
```

### **Comandos Úteis:**
```cpp
WiFiManager::connect("rede", "senha");    // Conectar
WiFiManager::disconnect();                // Desconectar  
WiFiManager::clearCredentials();          // Limpar salvos
WiFiManager::scanNetworks();              // Escanear redes
```

---

## 🎯 **Resumo Rápido**

**Para configurar WiFi AGORA:**
1. Abra `include/config.h`
2. Encontre `DEFAULT_WIFI_SSID` e `DEFAULT_WIFI_PASSWORD`
3. Coloque seu SSID e senha
4. Compile: `pio run`
5. Upload: `pio run --target upload`
6. **Conecta automaticamente!** ✅

**O sistema lembra da configuração e reconecta sozinho nas próximas vezes!** 📶
