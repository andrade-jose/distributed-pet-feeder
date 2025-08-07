# 🍽️ Alimentador Automático de Pets - ESP32 REMOTA

Sistema inteligente de alimentação automática para pets com controle remoto via MQTT, baseado em arquitetura distribuída CENTRAL/REMOTA.

## 📋 Índice

- [Visão Geral](#-visão-geral)
- [Arquitetura do Sistema](#-arquitetura-do-sistema)
- [Hardware](#-hardware)
- [Funcionalidades](#-funcionalidades)
- [Comunicação MQTT](#-comunicação-mqtt)
- [Instalação](#-instalação)
- [Configuração](#-configuração)
- [Uso do Sistema](#-uso-do-sistema)
- [Monitoramento](#-monitoramento)
- [Troubleshooting](#-troubleshooting)

## 🎯 Visão Geral

Este projeto implementa um **alimentador automático inteligente** controlado remotamente via **MQTT**. O sistema utiliza uma arquitetura distribuída onde:

- **ESP32 CENTRAL**: Envia comandos de alimentação
- **ESP32 REMOTA**: Executa os movimentos físicos (este repositório)

### ⭐ Características Principais

- ✅ **Controle por Tempo**: Alimentação baseada em duração (segundos)
- ✅ **Confirmação de Posição**: Sensor Hall para verificação precisa
- ✅ **Comunicação Segura**: MQTT com SSL/TLS
- ✅ **Controle Manual**: Botão físico para travamento/destravamento
- ✅ **Monitoramento**: LED de status e heartbeat automático
- ✅ **Arquitetura Robusta**: Sistema distribuído tolerante a falhas

## 🏗️ Arquitetura do Sistema

```
┌─────────────────┐    MQTT/SSL    ┌─────────────────┐
│   ESP32 CENTRAL │ ────────────► │   ESP32 REMOTA  │
│                 │               │                 │
│ • Interface Web │               │ • Servo Control │
│ • Agendamentos  │               │ • Sensor Hall   │
│ • Controle      │               │ • Alimentação   │
└─────────────────┘               └─────────────────┘
         │                                 │
         │                                 │
    ┌─────────┐                      ┌──────────┐
    │ HiveMQ  │                      │ Hardware │
    │ Cloud   │                      │ Físico   │
    └─────────┘                      └──────────┘
```

## 🔧 Hardware

### Componentes Principais

| Componente | Modelo | Pino | Função |
|------------|--------|------|--------|
| **Microcontrolador** | ESP32-D0WD-V3 | - | Controle principal |
| **Servo Motor** | PDI 6221MG | GPIO 5 | Movimento de alimentação |
| **Sensor Hall** | A3144 | GPIO 4 | Confirmação de posição |
| **Botão** | Push Button | GPIO 18 | Controle manual |
| **LED Status** | LED comum | GPIO 13 | Indicador visual |

### Posições do Servo

- **0° (FECHADO)**: Posição de descanso/bloqueio
- **90° (ABERTO)**: Posição de alimentação ativa

### Conexões Físicas

```
ESP32          Componente
─────          ──────────
GPIO 5    ──►  Servo (PWM)
GPIO 4    ──►  Sensor Hall (Digital)
GPIO 18   ──►  Botão (Pull-up interno)
GPIO 13   ──►  LED Status
GND       ──►  GND comum
3.3V      ──►  VCC sensores
5V        ──►  VCC servo
```

## ⚙️ Funcionalidades

### 🍽️ Sistema de Alimentação

- **Controle por Tempo**: 1-60 segundos de duração
- **Sequência**: 0°→90° (manter tempo) →0°
- **Confirmação**: Sensor Hall verifica posições
- **Segurança**: Sem ciclos repetitivos desnecessários

### 🔒 Controle Manual

- **1º Clique**: Trava servo em 90° (alimentação contínua)
- **2º Clique**: Destrava e volta para 0° (posição segura)
- **Pausa Inteligente**: Alimentação automática pode ser pausada

### 📡 Comunicação

- **WiFi**: Conexão automática com reconexão
- **MQTT**: Broker HiveMQ Cloud com SSL/TLS
- **Heartbeat**: Status a cada 30 segundos
- **Respostas**: Confirmação de comandos executados

## 📡 Comunicação MQTT

### Tópicos MQTT

| Tipo | Tópico | Direção | Descrição |
|------|--------|---------|-----------|
| **Comando** | `alimentador/remota/comando` | Central → Remota | Comandos de alimentação |
| **Status** | `alimentador/remota/status` | Remota → Central | Status operacional |
| **Resposta** | `alimentador/remota/resposta` | Remota → Central | Confirmação de conclusão |
| **Heartbeat** | `alimentador/remota/heartbeat` | Remota → Central | Sinal de vida |

### Formatos de Mensagem

#### Comando de Alimentação
```json
{
  "acao": "alimentar",
  "tempo": 5,
  "remota_id": 1
}
```

#### Comandos Simples
- `PING` - Teste de comunicação
- `STATUS` - Solicitar status atual
- `STOP` - Parar alimentação

#### Comandos Legados
- `a5` - Alimentar por 5 segundos
- `a10` - Alimentar por 10 segundos

#### Resposta de Status
```json
{
  "status": "DISPONIVEL",
  "timestamp": 1234567890
}
```

#### Heartbeat
```json
{
  "status": "ALIVE",
  "remota_id": 1,
  "uptime": 1234567890,
  "wifi_rssi": -45,
  "free_heap": 200000,
  "alimentacao_ativa": false,
  "servo_travado": false
}
```

## 🚀 Instalação

### Pré-requisitos

- **PlatformIO IDE** ou **VS Code + PlatformIO Extension**
- **ESP32 Development Board**
- **Componentes de hardware** listados acima

### Dependências

```ini
[env:esp32c3]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    ESP32Servo@^0.13.0
    PubSubClient@^2.8.0
    WiFi@^2.0.0
    WiFiClientSecure@^2.0.0
```

### Passos de Instalação

1. **Clone o repositório**
```bash
git clone <repository-url>
cd Alimentador_remota
```

2. **Abra no PlatformIO**
```bash
code .
```

3. **Configure credenciais** (ver seção Configuração)

4. **Compile e upload**
```bash
pio run --target upload
```

## ⚙️ Configuração

### WiFi
```cpp
const char* WIFI_SSID = "SUA_REDE_WIFI";        
const char* WIFI_PASSWORD = "SUA_SENHA_WIFI";
```

### MQTT
```cpp
const char* MQTT_SERVER = "SEU_BROKER.hivemq.cloud";  
const int MQTT_PORT = 8883;                     
const char* MQTT_CLIENT_ID = "ESP32_Remota_001"; 
const char* MQTT_USERNAME = "SEU_USUARIO";          
const char* MQTT_PASSWORD = "SUA_SENHA";
```

### Hardware (Opcional)
```cpp
const int PINO_SERVO = 5;      
const int PINO_HALL = 4;       
const int PINO_BOTAO = 18;      
const int PINO_LED_STATUS = 13;
```

## 🎮 Uso do Sistema

### Inicialização

1. **Power On**: ESP32 inicia e conecta WiFi/MQTT
2. **Teste Automático**: Componentes são testados
3. **Status**: LED indica conexão (fixo=OK, piscando=erro)
4. **Pronto**: Sistema aguarda comandos

### Operação Normal

1. **Comando Recebido**: Via MQTT da Central
2. **Validação**: Parâmetros verificados (1-60s)
3. **Execução**: Servo 0°→90° por tempo especificado →0°
4. **Confirmação**: Sensor Hall valida posições
5. **Resposta**: Status enviado para Central

### Controle Manual

- **Alimentação Contínua**: Pressionar botão (trava em 90°)
- **Parar/Voltar**: Pressionar novamente (volta para 0°)
- **Emergência**: Botão funciona mesmo durante alimentação automática

## 📊 Monitoramento

### LED de Status

- **Fixo**: WiFi e MQTT conectados ✅
- **Piscando**: Problemas de conexão ⚠️

### Serial Monitor

```
📊 Status: 3s/5s | Restante: 2s | Posição: 90°(ABERTO) | Confirmação: ✅CONFIRMADO | Servo: LIVRE | WiFi: OK | MQTT: OK
```

### Logs Importantes

- `🍽️ INICIANDO ALIMENTAÇÃO` - Comando recebido
- `✅ ALIMENTAÇÃO CONCLUÍDA!` - Processo finalizado
- `⚠️ Servo fora de posição!` - Erro de posicionamento
- `💓 Heartbeat enviado: ALIVE` - Sinal de vida

## 🔧 Troubleshooting

### Problemas Comuns

#### LED Piscando
**Causa**: Sem conexão WiFi/MQTT
**Solução**: 
- Verificar credenciais WiFi
- Confirmar broker MQTT disponível
- Checar conexão de internet

#### Servo Não Move
**Causa**: Hardware ou configuração
**Solução**:
- Verificar conexão GPIO 5
- Confirmar alimentação 5V do servo
- Testar com comandos manuais

#### Sensor Hall Não Detecta
**Causa**: Posicionamento ou hardware
**Solução**:
- Verificar conexão GPIO 4
- Posicionar ímã próximo ao sensor
- Testar com multímetro

#### Não Recebe Comandos
**Causa**: MQTT ou tópicos
**Solução**:
- Verificar tópicos MQTT
- Confirmar inscrição em `alimentador/remota/comando`
- Testar com MQTT client externo

### Comandos de Teste

```bash
# Monitor serial
pio device monitor

# Upload forçado
pio run --target upload --upload-port COM3

# Limpeza
pio run --target clean
```

## 📋 Status do Projeto

- ✅ **Hardware**: Totalmente implementado
- ✅ **Software**: Sistema completo funcionando
- ✅ **Comunicação**: MQTT SSL/TLS operacional
- ✅ **Testes**: Validação completa realizada
- ✅ **Documentação**: Guias completos disponíveis

## 🤝 Contribuição

1. Fork o projeto
2. Crie uma branch para sua feature
3. Commit suas mudanças
4. Push para a branch
5. Abra um Pull Request


## 📞 Suporte

Para dúvidas ou suporte:
- Abra uma **Issue** no GitHub
- Consulte os **logs do serial monitor**
- Verifique a **documentação técnica**

---
