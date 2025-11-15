#pragma once
#include <Arduino.h>
#include <RTClib.h>

class ClockService {
private:
    RTC_DS3231 rtc;
    bool initialized;

    int currentHour;
    int currentMinute;
    int currentSecond;
    int currentDay;
    int currentMonth;
    int currentYear;

public:
    ClockService();

    bool init();
    void update();

    // Getters
    int getHour() const { return currentHour; }
    int getMinute() const { return currentMinute; }
    int getSecond() const { return currentSecond; }
    int getDay() const { return currentDay; }
    int getMonth() const { return currentMonth; }
    int getYear() const { return currentYear; }

    // Formatação
    String getTimeFormatted();      // HH:MM:SS
    String getDateFormatted();      // DD/MM/YYYY
    String getTimeShort();          // HH:MM

    // Setters (para configuração manual se necessário)
    bool setDateTime(int year, int month, int day, int hour, int minute, int second);

    bool isInitialized() const { return initialized; }
};
