#include <Arduino.h>
#include "config.h"
#include "models.h"

#include "core/ClockService.h"
#include "services/schedule_service.h"
#include "services/log_service.h"
#include "hardware/feeder_service.h"
#include "comm/mqtt_service.h"

ClockService clockService;

void setup() {
    Serial.begin(115200);
    delay(800);

    LOG_SEPARATOR_DOUBLE();
    LOG_SECTION("🚀 REMOTE PET FEEDER - ESP32-S3");
    LOG_SEPARATOR_DOUBLE();

    LOG_KV("Device ID", DEVICE_ID);
    LOG_KV("Remote ID", String(REMOTE_ID));
    LOG_KV("Firmware", "v1.0.0");
    LOG_SEPARATOR();

    // MQTT (conecta WiFi primeiro)
    LOG_SUBSECTION("🌐 Inicializando Comunicação e WiFi");
    mqttService.begin(&clockService, &logService);

    // CLOCK SERVICE (após WiFi conectado)
    LOG_SUBSECTION("⏱ Inicializando ClockService (NTP)");
    if (!clockService.init()) {
        LOG_WARN("Clock rodando sem NTP — modo não sincronizado");
    } else {
        LOG_SUCCESS("Clock inicializado");
    }

    // LOG SERVICE
    LOG_SUBSECTION("📝 Inicializando Logs");
    logService.begin(&clockService);

    // FEEDER
    LOG_SUBSECTION("🍖 Inicializando Alimentador");
    feederService.begin(&clockService, &logService);

    // SCHEDULE
    LOG_SUBSECTION("📅 Inicializando Agendamentos");
    scheduleService.begin(&clockService, &feederService, &logService);

    LOG_SEPARATOR_DOUBLE();
    LOG_SUCCESS("Sistema 100% inicializado!");
    LOG_SEPARATOR_DOUBLE();
}

void loop() {
    clockService.update();
    mqttService.loop();
    scheduleService.loop();
    feederService.loop();
    logService.loop();

    delay(50);
}
