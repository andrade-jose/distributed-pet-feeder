#ifndef FEEDER_SERVICE_H
#define FEEDER_SERVICE_H

#include <Arduino.h>

class ClockService;
class LogService;
class MQTTService;

class FeederService {
public:
    FeederService();

    // Novo begin() com injeção de dependências
    bool begin(ClockService* clock, LogService* log);

    bool dispense(uint16_t quantity, const char* source = "manual");
    bool isDispensing();
    void loop();
    void testServo();

private:
    void moveServo(uint16_t quantity);
    bool checkSensor();

    // Dependências
    ClockService* clock;
    LogService* log;

    bool dispensing;
    uint32_t dispenseStartTime;
    uint16_t currentQuantity;
    String feedSource;
};

extern FeederService feederService;

#endif
