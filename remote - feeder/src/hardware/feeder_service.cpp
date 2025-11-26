// feeder_service.cpp
#include "hardware/feeder_service.h"
#include "config.h"
#include "services/log_service.h"
#include "core/ClockService.h"
#include "comm/mqtt_service.h"
#include <ESP32Servo.h>

Servo servo;
FeederService feederService;

FeederService::FeederService() :
    dispensing(false),
    dispenseStartTime(0),
    currentQuantity(0),
    feedSource("manual")
{}

bool FeederService::begin(ClockService* clockSvc, LogService* logSvc) {
    this->clock = clockSvc;
    this->log = logSvc;

    // Configurar sensor
    pinMode(SENSOR_PIN, INPUT);

    // Configurar servo PDI 6221MG (180°)
    servo.setPeriodHertz(50);        // 50Hz padrão para servos
    servo.attach(SERVO_PIN);         // A biblioteca gerencia os timers automaticamente

    // Posição central (90°) = 1500μs
    servo.writeMicroseconds(1500);
    delay(500); // Aguarda estabilizar

    LOG("✅ Feeder Service inicializado");
    LOG_KV("Servo Pin", String(SERVO_PIN));
    LOG_KV("Posição Inicial", "90° (centro)");

    return true;
}

bool FeederService::dispense(uint16_t quantity, const char* source) {
    if (dispensing) {
        LOG_WARN("Alimentação em progresso, aguarde");
        return false;
    }

    if (quantity == 0 || quantity > 500) {
        LOG_ERROR("Quantidade inválida: " + String(quantity) + "g");
        return false;
    }

    LOG_SUBSECTION("🍖 ALIMENTAÇÃO");
    LOG_KV("Quantidade", String(quantity) + "g");
    LOG_KV("Fonte", source);
    LOG_START("Dispensação de ração");

    dispensing = true;
    currentQuantity = quantity;
    feedSource = source;
    dispenseStartTime = millis();

    moveServo(currentQuantity);

    return true;
}

bool FeederService::isDispensing() {
    return dispensing;
}

void FeederService::loop() {
    if (!dispensing) return;

    unsigned long elapsed = millis() - dispenseStartTime;

    if (elapsed > FEED_TIMEOUT_MS) {
        LOG_ERROR("Timeout na alimentação!");
        LOG_KV("Tempo decorrido", String(elapsed / 1000) + "s");

        // Adicionar log local de falha
        if (log && clock) {
            log->addLog(
                clock->getTimestamp(),
                currentQuantity,
                false,  // delivered = false
                "timeout"
            );
        }

        // Publicar confirmação de falha via MQTT
        mqttService.publishFeedAck(currentQuantity, false, "timeout");

        dispensing = false;
        moveServo(0);
        LOG_SEPARATOR();
        return;
    }

    if (elapsed > 3000) {
        LOG_KV("Tempo de dispensação", String(elapsed / 1000.0, 1) + "s");
        bool success = checkSensor();

        // Adicionar log local
        if (log && clock) {
            log->addLog(
                clock->getTimestamp(),
                currentQuantity,
                success,
                feedSource.c_str()
            );
        }

        // Publicar confirmação via MQTT
        mqttService.publishFeedAck(currentQuantity, success, feedSource.c_str());

        if (success) {
            LOG_COMPLETE("Dispensação de ração");
            LOG_SUCCESS("Alimentação concluída: " + String(currentQuantity) + "g");
        } else {
            LOG_FAILED("Dispensação de ração");
            LOG_ERROR("Falha no sensor durante alimentação");
        }

        dispensing = false;
        moveServo(0);
        LOG_SEPARATOR();
    }
}

void FeederService::moveServo(uint16_t quantity) {
    if (quantity == 0) {
        // Parar: posição central (90° = 1500μs)
        servo.writeMicroseconds(1500);
        LOG_DEBUG("Servo -> PARADO (90° / 1500μs)");
    } else {
        // Ângulo absoluto de 60° (1167μs)
        int targetAngle = 30;
        int micros = map(targetAngle, 0, 180, 500, 2500);
        
        // Garantir limites seguros
        micros = constrain(micros, 500, 2500);
        
        servo.writeMicroseconds(micros);
        LOG_DEBUG("Servo -> " + String(targetAngle) + "° (" + String(micros) + "μs) para " + String(quantity) + "g");
    }
}

bool FeederService::checkSensor() {
    // SIMULAÇÃO: Vamos fazer o sensor sempre retornar true para teste
    // int sensorValue = digitalRead(SENSOR_PIN);
    // bool foodDetected = (sensorValue == HIGH);

    bool foodDetected = true; // Forçar sucesso para teste

    LOG_KV("Sensor HC-SR04", foodDetected ? "✓ DETECTADO" : "✗ NÃO DETECTADO");

    return foodDetected;
}

void FeederService::testServo() {
    LOG_SUBSECTION("🧪 TESTE DO SERVO");

    // Teste 1: Posição 0° (500μs)
    LOG("Teste 1: Movendo para 0° (500μs)");
    servo.writeMicroseconds(500);
    delay(2000);

    // Teste 2: Posição 60° (1167μs) - Posição de alimentação
    LOG("Teste 2: Movendo para 60° (1167μs) - ALIMENTAÇÃO");
    servo.writeMicroseconds(1167);
    delay(2000);

    // Teste 3: Posição 90° (1500μs) - Centro
    LOG("Teste 3: Movendo para 90° (1500μs) - Centro");
    servo.writeMicroseconds(1500);
    delay(2000);

    // Teste 4: Posição 120° (1833μs)
    LOG("Teste 4: Movendo para 120° (1833μs)");
    servo.writeMicroseconds(1833);
    delay(2000);

    // Teste 5: Posição 180° (2500μs)
    LOG("Teste 5: Movendo para 180° (2500μs)");
    servo.writeMicroseconds(2500);
    delay(2000);

    // Voltar ao centro
    LOG("Voltando ao centro (90°)");
    servo.writeMicroseconds(1500);
    delay(1000);

    LOG_SUCCESS("Teste concluído!");
    LOG_SEPARATOR();
}