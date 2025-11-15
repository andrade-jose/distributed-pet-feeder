#include "core/ClockService.h"

ClockService::ClockService()
    : initialized(false),
      currentHour(0), currentMinute(0), currentSecond(0),
      currentDay(1), currentMonth(1), currentYear(2025) {
}

bool ClockService::init() {
    Serial.println("[ClockService] Inicializando RTC...");

    if (!rtc.begin()) {
        Serial.println("[ClockService] ERRO: RTC não encontrado!");
        initialized = false;
        return false;
    }

    if (rtc.lostPower()) {
        Serial.println("[ClockService] AVISO: RTC perdeu energia, configurando data padrão");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    initialized = true;
    update();  // Primeira leitura
    Serial.println("[ClockService] RTC inicializado com sucesso!");
    return true;
}

void ClockService::update() {
    if (!initialized) return;

    DateTime now = rtc.now();

    currentHour = now.hour();
    currentMinute = now.minute();
    currentSecond = now.second();
    currentDay = now.day();
    currentMonth = now.month();
    currentYear = now.year();
}

String ClockService::getTimeFormatted() {
    char buffer[9];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
    return String(buffer);
}

String ClockService::getDateFormatted() {
    char buffer[11];
    snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", currentDay, currentMonth, currentYear);
    return String(buffer);
}

String ClockService::getTimeShort() {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", currentHour, currentMinute);
    return String(buffer);
}

bool ClockService::setDateTime(int year, int month, int day, int hour, int minute, int second) {
    if (!initialized) {
        Serial.println("[ClockService] RTC não inicializado!");
        return false;
    }

    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    update();

    Serial.printf("[ClockService] Data/hora ajustada: %s %s\n",
                  getDateFormatted().c_str(), getTimeFormatted().c_str());
    return true;
}
