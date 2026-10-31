#ifndef SAINTCALENDAR_H
#define SAINTCALENDAR_H

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

class SaintCalendar {
public:
    static const char* getWeekdayName(uint8_t weekday);
    static const char* getMonthName(uint8_t month);

    static bool getSaint(uint8_t day, uint8_t month, int year, char* buffer, size_t bufferSize);
    static const char* getSaintRaw(uint8_t day, uint8_t month, int year);

private:
    static bool isLeapYear(int year);
    static uint8_t daysInMonth(uint8_t month, int year);
    static bool isValidMonth(uint8_t month);
    static bool isValidDay(uint8_t day, uint8_t month, int year);
    static bool formatSaint(const char* saint, char* buffer, size_t bufferSize);

    static const char* const WDAY_NAMES[7];
    static const char* const MONTH_NAMES[12];

    static const char* const angels1[31];
    static const char* const angels2[30];
    static const char* const angels3[31];
    static const char* const angels4[30];
    static const char* const angels5[31];
    static const char* const angels6[30];
    static const char* const angels7[31];
    static const char* const angels8[31];
    static const char* const angels9[30];
    static const char* const angels10[31];
    static const char* const angels11[30];
    static const char* const angels12[31];

    static const char* const* const SAINTS_BY_MONTH[12];
};

#endif