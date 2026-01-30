// schedule_service.cpp
#include "services/schedule_service.h"
#include "config.h"
#include "core/ClockService.h"
#include "hardware/feeder_service.h"
#include "services/log_service.h"

ScheduleService scheduleService;

ScheduleService::ScheduleService() : lastFeedDay(0), lastFeedIndex(255) {}

bool ScheduleService::begin(ClockService* clockSvc, FeederService* feederSvc, LogService* logSvc) {
    this->clock = clockSvc;
    this->feeder = feederSvc;
    this->log = logSvc;

    if (!load()) {
        LOG_WARN("Não foi possível carregar agendamentos");
        LOG_INFO("Criando agendamentos padrão");
        // Agenda padrão
        setMeal(0, 8, 0, 100, true);   // Café: 08:00 - 100g
        setMeal(1, 13, 0, 150, true);  // Almoço: 13:00 - 150g
        setMeal(2, 18, 0, 120, true);  // Jantar: 18:00 - 120g
        save();
    }
    return true;
}

bool ScheduleService::load() {
    if (!prefs.begin(NVS_SCHEDULE_NAMESPACE, true)) {
        LOG_ERROR("Erro ao abrir NVS para agendamentos");
        return false;
    }

    for (int i = 0; i < 3; i++) {
        String prefix = "meal" + String(i);
        meals[i].hour = prefs.getUChar((prefix + "_hour").c_str(), 0);
        meals[i].minute = prefs.getUChar((prefix + "_minute").c_str(), 0);
        meals[i].qty = prefs.getUShort((prefix + "_qty").c_str(), 100);
        meals[i].enabled = prefs.getBool((prefix + "_enabled").c_str(), true);
    }

    prefs.end();
    LOG_SUCCESS("Agendamentos carregados da NVS");
    printMeals();
    return true;
}

bool ScheduleService::save() {
    if (!prefs.begin(NVS_SCHEDULE_NAMESPACE, false)) {
        LOG_ERROR("Erro ao abrir NVS para salvar agendamentos");
        return false;
    }

    for (int i = 0; i < 3; i++) {
        String prefix = "meal" + String(i);
        prefs.putUChar((prefix + "_hour").c_str(), meals[i].hour);
        prefs.putUChar((prefix + "_minute").c_str(), meals[i].minute);
        prefs.putUShort((prefix + "_qty").c_str(), meals[i].qty);
        prefs.putBool((prefix + "_enabled").c_str(), meals[i].enabled);
    }

    prefs.end();
    LOG_SUCCESS("Agendamentos salvos na NVS");
    return true;
}

void ScheduleService::loop() {
    static uint32_t lastCheck = 0;
    
    if (millis() - lastCheck < 1000) return;
    lastCheck = millis();
    
    checkMeals();
}

bool ScheduleService::checkMeals() {
    if (!clock || !clock->isInitialized()) {
        static unsigned long lastWarn = 0;
        if (millis() - lastWarn > 60000) {  // Avisar apenas a cada 1 min
            LOG_WARN("RTC não sincronizado - verificação de refeições desabilitada");
            lastWarn = millis();
        }
        return false;
    }

    uint32_t today = clock->getTimestamp() / 86400;


    for (uint8_t i = 0; i < 3; i++) {
        if (!meals[i].enabled) continue;

        if (shouldFeedNow(meals[i].hour, meals[i].minute)) {
            if (today != lastFeedDay || i != lastFeedIndex) {
                LOG_SEPARATOR();
                LOG_SECTION("⏰ REFEIÇÃO AGENDADA");
                LOG_KV("Refeição", String(i));
                LOG_KV("Horário", String(meals[i].hour) + ":" + (meals[i].minute < 10 ? "0" : "") + String(meals[i].minute));
                LOG_KV("Quantidade", String(meals[i].qty) + "g");

                if (log) {
                    log->addLog(
                        clock->getTimestamp(),
                        meals[i].qty,
                        true,  // assumindo sucesso - será atualizado pelo feeder
                        "schedule"  // FONTE: agendamento automático
                    );
                }

                if (feeder) {
                    feeder->dispense(meals[i].qty, "schedule");
                }

                lastFeedDay = today;
                lastFeedIndex = i;
                return true;
            }
        }
    }

    return false;
}


bool ScheduleService::shouldFeedNow(uint8_t hour, uint8_t minute) {
    if (!clock) return false;

    return (clock->getHour() == hour && clock->getMinute() == minute);

}

bool ScheduleService::setMeal(uint8_t index, uint8_t hour, uint8_t minute, uint16_t qty, bool enabled) {
    if (index >= 3) return false;
    
    meals[index].hour = hour;
    meals[index].minute = minute;
    meals[index].qty = qty;
    meals[index].enabled = enabled;
    
    return true;
}

Meal ScheduleService::getMeal(uint8_t index) {
    if (index >= 3) return Meal();
    return meals[index];
}

void ScheduleService::printMeals() {
    LOG_INFO("Agendamentos configurados:");
    for (int i = 0; i < 3; i++) {
        String time = String(meals[i].hour) + ":" + (meals[i].minute < 10 ? "0" : "") + String(meals[i].minute);
        String info = time + " → " + String(meals[i].qty) + "g " +
                     (meals[i].enabled ? "[ATIVA]" : "[INATIVA]");
        LOG_KV("Refeição " + String(i), info);
    }
}
