````md
# 🐾 Alimentador Automático de Pets – Central + Remotas (ESP32 + MQTT)

Sistema distribuído com ESP32 para alimentação automática de pets, usando arquitetura **Central/Remota** com comunicação **MQTT segura** e controle via **LCD + botões físicos**.

## 🔧 Componentes

- **Central (ESP32):**
  - LCD 20x4 com botões UP/DOWN/ENTER
  - Programação de até 3 refeições por remota
  - Sincronização automática de hora (NTP)
  - Monitoramento das remotas em tempo real

- **Remota (ESP32):**
  - Servo motor para liberar ração
  - Sensor Hall para confirmar posição
  - Botão manual para controle local
  - Envia status e heartbeat para a central

## 🔗 Comunicação

- **MQTT Broker:** HiveMQ Cloud (SSL/TLS)
- **Tópicos principais:**
  - `alimentador/remota/comando`
  - `alimentador/remota/status`
  - `alimentador/remota/heartbeat`
  - `alimentador/remota/resposta`

## 🚀 Como Usar

1. Clone este repositório:
   ```bash
   git clone <url>
   cd alimentador-automatico
````

2. Acesse `central/` ou `remota/` e configure Wi-Fi e MQTT em `config.h`.
3. Compile e envie o código via PlatformIO.
4. Use o monitor serial para debug e verificação.

## 📁 Estrutura

```
alimentador-automatico/
├── central/   → Código da interface e controle
├── remota/    → Código das unidades físicas
└── README.md  → Visão geral do projeto
```

## 📋 Requisitos

* PlatformIO + VS Code
* Conta no HiveMQ Cloud
* Placas ESP32 DevKit
* Acesso à rede Wi-Fi

---
