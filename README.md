# 🐾 Distributed Pet Feeder (ESP32 + MQTT)

Este repositório contém a implementação **real** do sistema distribuído com **Central (ESP32)** e **Remota (ESP32)** usando MQTT. O foco aqui é o que está implementado nos diretórios `Central_Pet_Feeder` e `remote - feeder`.

## ✅ O que está implementado

### Central (ESP32) — `Central_Pet_Feeder`
- Gateway MQTT entre dashboard/controle e as remotas.
- Protocolo com comandos, status, telemetria e retain.
- Operação com TLS (certificado CA local em `include/mqtt_cert.h`).
- Configuração via `include/config.h` (Wi‑Fi e MQTT).

Docs principais:
- `Central_Pet_Feeder/CONFIGURACAO.md`
- `Central_Pet_Feeder/PROTOCOLO_MQTT.md`
- `Central_Pet_Feeder/PROTOCOLO_LOGS_OFFLINE.md`

### Remota (ESP32) — `remote - feeder`
- Servo para alimentação (0° fechado / 90° aberto).
- Sensor Hall para confirmação de posição.
- Botão físico para controle manual (travar/destravar).
- LED de status + heartbeat MQTT.
- MQTT com TLS.

Doc principal:
- `remote - feeder/README.md`

## ⚠️ Nota importante sobre tópicos MQTT

Os dois módulos **não usam exatamente o mesmo esquema de tópicos**:
- A Central usa `petfeeder/central/*` e `petfeeder/remote/{id}/*`.
- A Remota usa `alimentador/remota/*`.

Se você pretende integrar os dois, alinhe os tópicos e o formato das mensagens.

## 🚀 Como começar (resumo)

1) Configure Wi‑Fi e MQTT nos respectivos `config.h`.  
2) Compile e envie via PlatformIO.  
3) Use o monitor serial para validar conexão e mensagens.

## 📁 Estrutura relevante

```
distributed-pet-feeder/
├── Central_Pet_Feeder/    # Central (ESP32)
├── remote - feeder/       # Remota (ESP32)
└── README.md
```
