#include "core/ClockService.h"
#include "config.h"

ClockService::ClockService()
    : initialized(false),
      lastNTPUpdate(0),
      currentHour(0), currentMinute(0), currentSecond(0),
      currentDay(1), currentMonth(1), currentYear(2025) {
}

bool ClockService::init() {
    Serial.println("[ClockService] Inicializando NTP...");

    // Configurar NTP
    configTime(NTP_TIMEZONE_OFFSET * 3600, NTP_DAYLIGHT_OFFSET, NTP_SERVER);

    Serial.println("[ClockService] Aguardando sincronização NTP...");

    // Aguardar até 10 segundos para sincronização
    int attempts = 0;
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo) && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    if (attempts >= 20) {
        Serial.println("[ClockService] AVISO: Timeout na sincronização NTP, usando horário local");
        initialized = false;
        return false;
    }

    initialized = true;
    lastNTPUpdate = millis();
    update();  // Primeira leitura

    Serial.printf("[ClockService] NTP sincronizado: %s %s\n",
                  getDateFormatted().c_str(), getTimeFormatted().c_str());
    return true;
}

void ClockService::syncWithNTP() {
    Serial.println("[ClockService] Ressincronizando com NTP...");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        lastNTPUpdate = millis();
        Serial.println("[ClockService] NTP ressincronizado com sucesso");
    } else {
        Serial.println("[ClockService] Falha ao ressincronizar NTP");
    }
}

void ClockService::update() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        // Se não conseguir obter o horário, manter valores anteriores
        if (!initialized) {
            // Usar valores padrão se nunca foi inicializado
            return;
        }
        return;
    }

    currentHour = timeinfo.tm_hour;
    currentMinute = timeinfo.tm_min;
    currentSecond = timeinfo.tm_sec;
    currentDay = timeinfo.tm_mday;
    currentMonth = timeinfo.tm_mon + 1;  // tm_mon é 0-11
    currentYear = timeinfo.tm_year + 1900;  // tm_year é anos desde 1900

    // Ressincronizar periodicamente com NTP
    if (initialized && (millis() - lastNTPUpdate >= NTP_UPDATE_INTERVAL)) {
        syncWithNTP();
    }
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

unsigned long ClockService::getTimestamp() const {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return 0;
    }
    return mktime(&timeinfo);
}
