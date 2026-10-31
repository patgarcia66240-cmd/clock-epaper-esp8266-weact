#ifndef MOONPHASE_H
#define MOONPHASE_H

#include <Arduino.h>

class MoonPhase {
public:
    enum Phase : uint8_t {
        NEW_MOON = 0,
        WAXING_CRESCENT,
        FIRST_QUARTER,
        WAXING_GIBBOUS,
        FULL_MOON,
        WANING_GIBBOUS,
        LAST_QUARTER,
        WANING_CRESCENT
    };

    MoonPhase() = default;

    Phase getPhase(int year, int month, int day) const;
    const char* getMoonPhaseName(int year, int month, int day) const;

private:
    static double calculateJulianDay(int year, int month, int day);
    static Phase phaseFromJulianDay(double julianDay);

    static const char* const PHASE_NAMES[8];
};

#endif