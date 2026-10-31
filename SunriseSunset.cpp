#include "SunriseSunset.h"
#include <math.h>
#include <stdio.h>

SunriseSunset::SunriseSunset(double lat, double lon, int utcOffset)
    : latitude(lat), longitude(lon), utcOffsetHours(utcOffset) {}

void SunriseSunset::setLocation(double lat, double lon) {
    latitude = lat;
    longitude = lon;
}

void SunriseSunset::setUtcOffset(int hours) {
    utcOffsetHours = hours;
}

bool SunriseSunset::isLeapYear(int year) {
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

int SunriseSunset::dayOfYear(int year, int month, int day) {
    static const int daysBeforeMonth[] = {
        0,   // Jan
        31,  // Feb
        59,  // Mar
        90,  // Apr
        120, // May
        151, // Jun
        181, // Jul
        212, // Aug
        243, // Sep
        273, // Oct
        304, // Nov
        334  // Dec
    };

    int n = daysBeforeMonth[month - 1] + day;
    if (month > 2 && isLeapYear(year)) {
        n += 1;
    }
    return n;
}

double SunriseSunset::toRadians(double degrees) {
    return degrees * PI / 180.0;
}

double SunriseSunset::toDegrees(double radians) {
    return radians * 180.0 / PI;
}

double SunriseSunset::normalizeDegrees(double value) {
    double result = fmod(value, 360.0);
    if (result < 0.0) {
        result += 360.0;
    }
    return result;
}

double SunriseSunset::normalizeHours(double value) {
    double result = fmod(value, 24.0);
    if (result < 0.0) {
        result += 24.0;
    }
    return result;
}

bool SunriseSunset::calculateSolarTimeUtcHours(int year, int month, int day, bool isSunrise, double& utcHours) const {
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        return false;
    }

    if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) {
        return false;
    }

    const int N = dayOfYear(year, month, day);

    const double lngHour = longitude / 15.0;
    const double t = isSunrise
        ? N + ((6.0 - lngHour) / 24.0)
        : N + ((18.0 - lngHour) / 24.0);

    const double M = (0.9856 * t) - 3.289;

    double L = M
             + (1.916 * sin(toRadians(M)))
             + (0.020 * sin(toRadians(2.0 * M)))
             + 282.634;

    L = normalizeDegrees(L);

    double RA = toDegrees(atan(0.91764 * tan(toRadians(L))));
    RA = normalizeDegrees(RA);

    const double Lquadrant = floor(L / 90.0) * 90.0;
    const double RAquadrant = floor(RA / 90.0) * 90.0;
    RA = RA + (Lquadrant - RAquadrant);

    RA /= 15.0;

    const double sinDec = 0.39782 * sin(toRadians(L));
    const double cosDec = cos(asin(sinDec));

    const double cosH =
        (cos(toRadians(ZENITH)) - (sinDec * sin(toRadians(latitude))))
        / (cosDec * cos(toRadians(latitude)));

    if (cosH > 1.0) {
        // Le soleil ne se lève pas ce jour-là
        return false;
    }

    if (cosH < -1.0) {
        // Le soleil ne se couche pas ce jour-là
        return false;
    }

    double H = isSunrise
        ? 360.0 - toDegrees(acos(cosH))
        : toDegrees(acos(cosH));

    H /= 15.0;

    const double T = H + RA - (0.06571 * t) - 6.622;
    utcHours = normalizeHours(T - lngHour);

    return true;
}

SunriseSunset::TimeHM SunriseSunset::utcHoursToLocalTime(double utcHours) const {
    double localHours = utcHours + static_cast<double>(utcOffsetHours);
    localHours = normalizeHours(localHours);

    int totalMinutes = static_cast<int>(localHours * 60.0 + 0.5);

    if (totalMinutes >= 24 * 60) {
        totalMinutes -= 24 * 60;
    }

    TimeHM result;
    result.hour = static_cast<uint8_t>(totalMinutes / 60);
    result.minute = static_cast<uint8_t>(totalMinutes % 60);
    result.valid = true;

    return result;
}

SunriseSunset::TimeHM SunriseSunset::getSunrise(int year, int month, int day) const {
    double utcHours = 0.0;
    if (!calculateSolarTimeUtcHours(year, month, day, true, utcHours)) {
        return {0, 0, false};
    }
    return utcHoursToLocalTime(utcHours);
}

SunriseSunset::TimeHM SunriseSunset::getSunset(int year, int month, int day) const {
    double utcHours = 0.0;
    if (!calculateSolarTimeUtcHours(year, month, day, false, utcHours)) {
        return {0, 0, false};
    }
    return utcHoursToLocalTime(utcHours);
}

void SunriseSunset::formatTime(const TimeHM& time, char* buffer, size_t bufferSize) const {
    if (buffer == nullptr || bufferSize < 6) {
        return;
    }

    if (!time.valid) {
        snprintf(buffer, bufferSize, "--:--");
        return;
    }

    snprintf(buffer, bufferSize, "%02u:%02u", time.hour, time.minute);
}