#ifndef SUNRISESUNSET_H
#define SUNRISESUNSET_H

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

class SunriseSunset {
public:
    struct TimeHM {
        uint8_t hour;
        uint8_t minute;
        bool valid;
    };

    SunriseSunset(double lat, double lon, int utcOffsetHours = 0);

    void setLocation(double lat, double lon);
    void setUtcOffset(int hours);

    TimeHM getSunrise(int year, int month, int day) const;
    TimeHM getSunset(int year, int month, int day) const;

    void formatTime(const TimeHM& time, char* buffer, size_t bufferSize) const;

private:
    double latitude;
    double longitude;
    int utcOffsetHours;

    static constexpr double ZENITH = 90.833; // lever/coucher standard

    static bool isLeapYear(int year);
    static int dayOfYear(int year, int month, int day);

    static double toRadians(double degrees);
    static double toDegrees(double radians);
    static double normalizeDegrees(double value);
    static double normalizeHours(double value);

    bool calculateSolarTimeUtcHours(int year, int month, int day, bool isSunrise, double& utcHours) const;
    TimeHM utcHoursToLocalTime(double utcHours) const;
};

#endif