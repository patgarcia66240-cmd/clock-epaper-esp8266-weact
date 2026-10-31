#include "MoonPhase.h"
#include <math.h>

namespace {
    constexpr double NEW_MOON_REFERENCE = 2451550.1;
    constexpr double SYNODIC_MONTH = 29.53058867;
}

const char* const MoonPhase::PHASE_NAMES[8] = {
    "Nouvelle lune",
    "Premier croissant",
    "Premier quartier",
    "Gibbeuse croissante",
    "Pleine lune",
    "Gibbeuse d\351croissante",
    "Dernier quartier",
    "Dernier croissant"
};

double MoonPhase::calculateJulianDay(int year, int month, int day) {
    int y = year;
    int m = month;

    if (m <= 2) {
        y -= 1;
        m += 12;
    }

    const int A = y / 100;
    const int B = 2 - A + (A / 4);

    return floor(365.25 * (y + 4716))
         + floor(30.6001 * (m + 1))
         + day + B - 1524.5;
}

MoonPhase::Phase MoonPhase::phaseFromJulianDay(double julianDay) {
    double cycle = (julianDay - NEW_MOON_REFERENCE) / SYNODIC_MONTH;

    cycle = cycle - floor(cycle);
    if (cycle < 0.0) {
        cycle += 1.0;
    }

    const uint8_t index = static_cast<uint8_t>((cycle * 8.0) + 0.5) % 8;
    return static_cast<Phase>(index);
}

MoonPhase::Phase MoonPhase::getPhase(int year, int month, int day) const {
    const double julianDay = calculateJulianDay(year, month, day);
    return phaseFromJulianDay(julianDay);
}

const char* MoonPhase::getMoonPhaseName(int year, int month, int day) const {
    return PHASE_NAMES[getPhase(year, month, day)];
}