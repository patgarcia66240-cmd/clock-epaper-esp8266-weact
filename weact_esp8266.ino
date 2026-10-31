// =====================================================
// Horloge e-Paper WeAct Studio 2.9" (296 x 128) - ESP8266 (D1 mini)
// Brochage WeAct ESP8266 (broche e-paper -> couleur du fil -> D1 mini) :
//   BUSY -> violet -> D0 (GPIO16)
//   RES/RST -> orange -> D1 (GPIO5)
//   D/C -> blanc -> D2 (GPIO4)
//   CS -> bleu -> D8 (GPIO15)
//   SCL/CLK -> vert -> D5 (GPIO14)
//   SDA/DIN -> jaune -> D7 (GPIO13)
//   GND -> noir -> GND
//   VCC -> rouge -> 3V3 (jamais 5V)
// Bouton optionnel : D6 (GPIO12) vers GND.
// Pilote : GxEPD2_290_BS (DEPG0290BS, à confirmer avec la référence imprimée sur l'écran).
// =====================================================
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include "Fonts/Roboto15.h"
#include "Fonts/Roboto24.h"
#include "Fonts/Roboto64.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <NTP.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#include "SaintCalendar.h"
// Ecran : brochage WeAct ESP8266 dans GxEPD2_display_selection_new_style.h (CS=D8, DC=D2, RST=D1, BUSY=D0)
#include "GxEPD2_display_selection_new_style.h"

#include "bitmaps.h"
#include "MoonPhase.h"
#include "SunriseSunset.h"

WiFiUDP wifiUdp;
NTP ntp(wifiUdp);

// =====================================================
// Constantes
// =====================================================
// Version automatique : date et heure de compilation (mises a jour a chaque build)
static constexpr char FW_BUILD[] = __DATE__ " " __TIME__;
static constexpr uint32_t SERIAL_BAUDRATE = 115200;
static constexpr unsigned long ONE_SECOND_MS = 1000UL;
static constexpr unsigned long NTP_RETRY_INTERVAL_MS = 30000UL;  // 30 s entre deux tentatives apres echec
static constexpr unsigned long NTP_SYNC_INTERVAL_MS = 3600000UL;       // 1h
static constexpr unsigned long FULL_REFRESH_INTERVAL_MS = 21600000UL;  // 6h
static constexpr uint32_t SPLASH_DURATION_MS = 2500;
static constexpr uint16_t PARTIAL_UPDATES_BEFORE_FULL = 30; // full refresh apres 30 fenetres partielles

static constexpr int PAGE_BUTTON_PIN = D6;              // Bouton externe sur D6 (GPIO12) vers GND, actif a LOW
// Meteo (Open-Meteo, sans cle). Coordonnees : Saint-Martin-de-Crau, a verifier
static constexpr char WEATHER_URL[] = "https://api.open-meteo.com/v1/forecast?latitude=43.636&longitude=4.830&current=temperature_2m,weather_code,relative_humidity_2m,wind_speed_10m&timezone=Europe%2FParis";
static constexpr unsigned long WEATHER_SHOW_MS = 10000UL;  // duree d'affichage de la meteo

static constexpr char OTA_HOSTNAME[] = "e-paper-clock";
static constexpr char OTA_PASSWORD[] = "123456";              // mets un mot de passe pour proteger la mise a jour

static constexpr int SCREEN_W = 296;
static constexpr int SCREEN_H = 128;

static constexpr int DATE_TOP = 16;

static constexpr int TIME_X = 48;
static constexpr int TIME_Y = 24;
static constexpr int TIME_W = 248;   // assez large pour l'indicateur d'erreur a droite
static constexpr int TIME_H = 70;
static constexpr int TIME_BASELINE_Y = 78;

static constexpr int SAINT_TOP = 121;
static constexpr int SAINT_AREA_Y = 94;   // juste sous la zone de l'heure (24 + 70)
static constexpr int SAINT_AREA_H = 32;   // garde la meme ligne basse (125)

static constexpr int H_MARGIN = 6;

static constexpr size_t DATE_BUFFER_SIZE = 64;
static constexpr size_t SAINT_BUFFER_SIZE = 80;

// =====================================================
// Etat global
// =====================================================
struct ClockState
{
  uint8_t lastMinute = 255;
  uint8_t lastDay = 255;

  bool ntpReady = false;
  bool screenReady = false;

  unsigned long previousTickMillis = 0;
  unsigned long lastNtpSyncMillis = 0;
  unsigned long lastNtpAttemptMillis = 0;
  unsigned long lastFullRefreshMillis = 0;
  uint16_t partialUpdates = 0;
  bool forceNtpSync = false;
  bool showingWeather = false;
  unsigned long weatherShownMillis = 0;
  bool skyPage = false;
};

ClockState state;

// =====================================================
// Mesures texte
// =====================================================
struct TextMetrics
{
  int16_t x1 = 0;
  int16_t y1 = 0;
  uint16_t w = 0;
  uint16_t h = 0;
};

// =====================================================
// SPI écran
// =====================================================
void setupHSPI()
{
  // SPI materiel de l'ESP8266 (CLK = D5, DIN = D7). Vitesse reduite a 500 kHz (test) :
  // les fils dupont longs corrompent les donnees a vitesse plus elevee.
  display.epd2.selectSPI(SPI, SPISettings(1000000, MSBFIRST, SPI_MODE0));
}

// =====================================================
// Outils affichage
// =====================================================
void clearFullScreen()
{
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());
}

void showSplashBitmap(uint32_t durationMs)
{
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.drawInvertedBitmap(0, 0, epd_bitmap_296, display.width(), display.height(), GxEPD_BLACK);
  } while (display.nextPage());

  delay(durationMs);
}

void print02d(uint8_t value)
{
  char buffer[3];
  snprintf(buffer, sizeof(buffer), "%02u", value);
  display.print(buffer);
}

// =====================================================
// Formatage texte
// =====================================================
void buildDateString(char* out, size_t outSize, time_t t)
{
  if (out == nullptr || outSize == 0)
  {
    return;
  }

  if (t == 0)
  {
    out[0] = '\0';
    return;
  }

  const uint8_t wd = weekday(t); // 1..7
  const uint8_t dd = day(t);
  const uint8_t mm = month(t);
  const int yyyy = year(t);

  const char* weekdayName = SaintCalendar::getWeekdayName(wd - 1); // 0..6
  const char* monthName = SaintCalendar::getMonthName(mm);         // 1..12

  snprintf(out, outSize, "%s %u %s %d", weekdayName, dd, monthName, yyyy);
  out[outSize - 1] = '\0';
}

bool buildSaintString(char* out, size_t outSize, time_t t)
{
  if (out == nullptr || outSize == 0)
  {
    return false;
  }

  if (t == 0)
  {
    out[0] = '\0';
    return false;
  }

  return SaintCalendar::getSaint(day(t), month(t), year(t), out, outSize);
}

// =====================================================
// Mesure / centrage
// =====================================================
TextMetrics measureText(const char* text, const GFXfont* font)
{
  TextMetrics m;
  display.setFont(font);
  display.getTextBounds(text != nullptr ? text : "", 0, 0, &m.x1, &m.y1, &m.w, &m.h);
  return m;
}

int computeCenteredX(uint16_t areaWidth, const TextMetrics& m)
{
  return ((int)areaWidth - (int)m.w) / 2 - m.x1;
}

// =====================================================
// Auto-fit saint sur 2 tailles
// Retourne la police choisie ET les métriques déjà calculées
// pour éviter un double appel à measureText() dans drawSaint().
// =====================================================
const GFXfont* chooseSaintFont(const char* saint, TextMetrics& outMetrics)
{
  if (saint == nullptr)
  {
    outMetrics = measureText("", &Roboto_Bold_18);
    return &Roboto_Bold_18;
  }

  TextMetrics m24 = measureText(saint, &Roboto_Bold_24);
  if ((int)m24.w <= (SCREEN_W - 2 * H_MARGIN))
  {
    outMetrics = m24;
    return &Roboto_Bold_24;
  }

  outMetrics = measureText(saint, &Roboto_Bold_18);
  return &Roboto_Bold_18;
}

// =====================================================
// Rendu date
// =====================================================
// Ecrit l'heure HH:MM a sa position (utilise par drawTime et drawAll)
// Vrai si l'heure est fiable : Wi-Fi connecte et synchro NTP recente
bool clockHealthy()
{
  return WiFi.status() == WL_CONNECTED &&
         state.ntpReady &&
         (millis() - state.lastNtpSyncMillis) < 2 * NTP_SYNC_INTERVAL_MS;
}

// Petit "!" en haut a droite de la zone de l'heure si l'heure n'est plus fiable
void printStatusIcon()
{
  if (clockHealthy())
    return;

  display.setFont(&Roboto_Bold_18);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(TIME_X + TIME_W - 18, TIME_Y + 18);
  display.print("!");
}

void printTimeText()
{
  display.setFont(&Roboto_Bold_64);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(TIME_X + 6, TIME_BASELINE_Y);
  print02d(hour(now()));
  display.print(":");
  print02d(minute(now()));
}

void drawDate()
{
  if (!state.ntpReady || state.showingWeather)
    return;

  char dateBuffer[DATE_BUFFER_SIZE];
  buildDateString(dateBuffer, sizeof(dateBuffer), now());

  const GFXfont* font = &Roboto_Bold_18;
  TextMetrics m = measureText(dateBuffer, font);

  const int areaY = 0;
  const int areaH = 24;   // ne touche plus la zone de l'heure (commence a 24)

  display.setPartialWindow(0, areaY, SCREEN_W, areaH);
  display.firstPage();
  do
  {
    display.fillRect(0, areaY, SCREEN_W, areaH, GxEPD_WHITE);

    display.setFont(font);
    display.setTextColor(GxEPD_BLACK);

    const int x = computeCenteredX(SCREEN_W, m);
    display.setCursor(x, DATE_TOP);
    display.print(dateBuffer);

  } while (display.nextPage());

  // Compte ce rafraîchissement partiel pour déclencher le full refresh à temps.
  state.partialUpdates++;
}

// =====================================================
// Rendu heure
// =====================================================
void drawTime()
{
  if (!state.ntpReady || state.showingWeather)
    return;

  display.setPartialWindow(TIME_X, TIME_Y, TIME_W, TIME_H);
  display.firstPage();
  do
  {
    display.fillRect(TIME_X, TIME_Y, TIME_W, TIME_H, GxEPD_WHITE);

    printTimeText();
    printStatusIcon();

  } while (display.nextPage());

  state.partialUpdates++;
}

// =====================================================
// Rendu saint
// =====================================================
void drawSaint()
{
  if (!state.ntpReady || state.showingWeather)
    return;

  char saintBuffer[SAINT_BUFFER_SIZE];
  buildSaintString(saintBuffer, sizeof(saintBuffer), now());

  TextMetrics m;
  const GFXfont* saintFont = chooseSaintFont(saintBuffer, m); // métriques déjà calculées

  display.setPartialWindow(0, SAINT_AREA_Y, SCREEN_W, SAINT_AREA_H);
  display.firstPage();
  do
  {
    display.fillRect(0, SAINT_AREA_Y, SCREEN_W, SAINT_AREA_H, GxEPD_WHITE);

    display.setFont(saintFont);
    display.setTextColor(GxEPD_BLACK);

    const int x = computeCenteredX(SCREEN_W, m);
    display.setCursor(x, SAINT_TOP);
    display.print(saintBuffer);

  } while (display.nextPage());

  // Compte ce rafraîchissement partiel pour limiter les images fantômes.
  state.partialUpdates++;
}

// =====================================================
// Rendu global — one-pass full window
// Tout est dessiné en une seule passe full refresh,
// sans partial windows séparés. Evite 3 cycles e-paper inutiles.
// =====================================================
void drawAll()
{
  if (!state.ntpReady)
    return;

  // Préparer les buffers
  char dateBuffer[DATE_BUFFER_SIZE];
  buildDateString(dateBuffer, sizeof(dateBuffer), now());

  char saintBuffer[SAINT_BUFFER_SIZE];
  buildSaintString(saintBuffer, sizeof(saintBuffer), now());

  TextMetrics saintMetrics;
  const GFXfont* saintFont = chooseSaintFont(saintBuffer, saintMetrics);

  const GFXfont* dateFont = &Roboto_Bold_18;
  const TextMetrics dateMetrics = measureText(dateBuffer, dateFont);

  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    // Heure
    printTimeText();
    printStatusIcon();

    // Date
    display.setFont(dateFont);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(computeCenteredX(SCREEN_W, dateMetrics), DATE_TOP);
    display.print(dateBuffer);

    // Saint
    display.setFont(saintFont);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(computeCenteredX(SCREEN_W, saintMetrics), SAINT_TOP);
    display.print(saintBuffer);

  } while (display.nextPage());

  state.screenReady = true;
  state.lastFullRefreshMillis = millis();
  state.partialUpdates = 0;
}

// =====================================================
// WiFi / NTP
// =====================================================
bool connectWiFiAndInitNTP()
{
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180); // 3 min max en portail de configuration

  if (!wifiManager.autoConnect("E-Paper o'Clock"))
  {
    Serial.println("Portail Wi-Fi expire, pas de connexion");
    return false;
  }

  Serial.print("WiFi OK, IP: ");
  Serial.println(WiFi.localIP());

  // Mise a jour a distance (OTA)
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  if (strlen(OTA_PASSWORD) > 0)
    ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();

  ntp.ruleDST("CEST", Last, Sun, Mar, 2, 120);
  ntp.ruleSTD("CET", Last, Sun, Oct, 3, 60);
  ntp.begin();

  Serial.println("NTP initialise");
  return true;
}

time_t localTimeFromUtc(time_t utc);

bool syncTimeFromNTP()
{
  ntp.update();

  const time_t newTime = ntp.epoch();
  // Seuil : 1er janvier 2024. En dessous, la reponse NTP est invalide (ex. 1970).
  if (newTime < 1704067200UL)
  {
    Serial.println("Heure NTP invalide");
    return false;
  }

  setTime(localTimeFromUtc(newTime));

  state.lastMinute = minute(now());
  state.lastDay = day(now());
  state.lastNtpSyncMillis = millis();
  state.ntpReady = true;

  Serial.print("Synchro NTP OK: ");
  Serial.println(ntp.formattedTime("%A %d %B %Y %T"));

  return true;
}

bool initialSync(uint32_t timeoutMs = 15000)
{
  const unsigned long start = millis();

  while (millis() - start < timeoutMs)
  {
    if (syncTimeFromNTP())
    {
      return true;
    }
    // Pompe les événements WiFi pendant l'attente
    // sans bloquer avec delay()
    const unsigned long retryStart = millis();
    while (millis() - retryStart < 500)
    {
      yield();
    }
  }

  return false;
}

// =====================================================
// Horloge locale
// =====================================================
// Redessine seulement ce qui a change : date et saint si le jour change, heure si la minute change
void redrawChanged(bool dayChanged, bool minuteChanged)
{
  if (dayChanged)
  {
    drawDate();
    drawSaint();
  }

  if (minuteChanged || dayChanged)
    drawTime();
}

void tickOneSecond()
{
  if (!state.ntpReady || now() == 0)
    return;



  const uint8_t currentMinute = minute(now());
  const uint8_t currentDay = day(now());

  const bool minuteChanged = currentMinute != state.lastMinute;
  const bool dayChanged = currentDay != state.lastDay;

  state.lastMinute = currentMinute;
  state.lastDay = currentDay;
  redrawChanged(dayChanged, minuteChanged);
}

void handleTick()
{
  const unsigned long nowMs = millis();

  while (nowMs - state.previousTickMillis >= ONE_SECOND_MS)
  {
    state.previousTickMillis += ONE_SECOND_MS;
    tickOneSecond();
  }
}

bool shouldSyncNTP(unsigned long nowMs)
{
  if (state.forceNtpSync)
    return true;

  if (!state.ntpReady)
    return (nowMs - state.lastNtpAttemptMillis) >= NTP_RETRY_INTERVAL_MS;

  return (nowMs - state.lastNtpSyncMillis) >= NTP_SYNC_INTERVAL_MS;
}

// Recupere la temperature et le code meteo. Retourne false en cas d'echec.
bool fetchWeather(int& tempC, int& code, int& humidity, int& windKmh)
{
  // ESP8266 : pas de bundle de certificats par defaut. On accepte le certificat sans verification
  // (lecture publique Open-Meteo, aucune donnee sensible envoyee).
  WiFiClientSecure client;
  client.setInsecure(); // Certificat non vérifié : connexion HTTPS non authentifiée.

  HTTPClient http;
  if (!http.begin(client, WEATHER_URL))
    return false;

  http.setTimeout(8000);
  const int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    http.end();
    return false;
  }

  const String body = http.getString();
  http.end();

  // Analyse les nombres directement dans le buffer JSON pour éviter
  // les allocations temporaires répétées de String::substring().
  const char* currentJson = strstr(body.c_str(), "\"current\":{");
  if (currentJson == nullptr)
    return false;

  auto readNumber = [](const char* json, const char* key, double& value) -> bool {
    const char* p = strstr(json, key);
    if (p == nullptr)
      return false;
    p = strchr(p, ':');
    if (p == nullptr)
      return false;
    ++p;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
      ++p;
    char* end = nullptr;
    value = strtod(p, &end);
    return end != p;
  };

  double value = 0.0;
  if (!readNumber(currentJson, "\"temperature_2m\"", value))
    return false;
  tempC = (int)lround(value);

  if (!readNumber(currentJson, "\"weather_code\"", value))
    return false;
  code = (int)value;

  humidity = readNumber(currentJson, "\"relative_humidity_2m\"", value)
                 ? (int)value : -1;
  windKmh = readNumber(currentJson, "\"wind_speed_10m\"", value)
                ? (int)lround(value) : -1;
  return true;
}

const char* weatherText(int code)
{
  if (code == 0) return "Ciel d\351gag\351";
  if (code <= 3) return "Nuageux";
  if (code == 45 || code == 48) return "Brouillard";
  if (code >= 51 && code <= 67) return "Pluie";
  if (code >= 71 && code <= 77) return "Neige";
  if (code >= 80 && code <= 82) return "Averses";
  if (code >= 95) return "Orage";
  return "Inconnu";
}

// =====================================================
// Icones dessinees a la main
// =====================================================
// Mise a l'echelle d'une dimension : s = 100 pour taille normale, 160 pour 1,6 fois plus grand
int sc(int v, int s)
{
  return (v * s + 50) / 100;
}

void drawCloudIcon(int cx, int cy, int s)
{
  display.fillCircle(cx + sc(-6, s), cy + sc(2, s), sc(5, s), GxEPD_BLACK);
  display.fillCircle(cx + sc(2, s), cy + sc(-1, s), sc(7, s), GxEPD_BLACK);
  display.fillCircle(cx + sc(9, s), cy + sc(3, s), sc(4, s), GxEPD_BLACK);
  display.fillRect(cx + sc(-6, s), cy + sc(2, s), sc(15, s), sc(5, s), GxEPD_BLACK);
}

void drawWeatherIcon(int cx, int cy, int code, int s)
{
  if (code == 0)
  {
    display.fillCircle(cx, cy, sc(6, s), GxEPD_BLACK);
    for (int k = 0; k < 8; ++k)
    {
      const double a = k * PI / 4.0;
      display.drawLine(cx + (int)round(cos(a) * sc(9, s)), cy + (int)round(sin(a) * sc(9, s)),
                       cx + (int)round(cos(a) * sc(12, s)), cy + (int)round(sin(a) * sc(12, s)), GxEPD_BLACK);
    }
  }
  else if (code <= 3)
  {
    drawCloudIcon(cx, cy, s);
  }
  else if (code == 45 || code == 48)
  {
    for (int k = 0; k < 3; ++k)
      display.drawFastHLine(cx - sc(10, s), cy + sc(-6 + k * 6, s), sc(20, s), GxEPD_BLACK);
  }
  else if (code >= 71 && code <= 77)
  {
    drawCloudIcon(cx, cy - sc(4, s), s);
    display.fillCircle(cx + sc(-6, s), cy + sc(11, s), sc(2, s), GxEPD_BLACK);
    display.fillCircle(cx, cy + sc(13, s), sc(2, s), GxEPD_BLACK);
    display.fillCircle(cx + sc(6, s), cy + sc(11, s), sc(2, s), GxEPD_BLACK);
  }
  else if (code >= 95)
  {
    drawCloudIcon(cx, cy - sc(4, s), s);
    display.drawLine(cx + sc(1, s), cy + sc(4, s), cx + sc(-3, s), cy + sc(10, s), GxEPD_BLACK);
    display.drawLine(cx + sc(-3, s), cy + sc(10, s), cx + sc(2, s), cy + sc(10, s), GxEPD_BLACK);
    display.drawLine(cx + sc(2, s), cy + sc(10, s), cx + sc(-2, s), cy + sc(15, s), GxEPD_BLACK);
  }
  else
  {
    drawCloudIcon(cx, cy - sc(4, s), s);
    for (int k = 0; k < 3; ++k)
      display.drawLine(cx + sc(-6 + k * 6, s), cy + sc(9, s), cx + sc(-9 + k * 6, s), cy + sc(14, s), GxEPD_BLACK);
  }
}
// Soleil a l'horizon avec une fleche : haut = lever, bas = coucher
void drawSunIcon(int cx, int cy, int r, bool rising)
{
  for (int dy = 0; dy <= r; ++dy)
  {
    const int half = (int)sqrt((double)(r * r - dy * dy));
    display.drawFastHLine(cx - half, cy - dy, 2 * half + 1, GxEPD_BLACK);
  }
  display.drawFastHLine(cx - r - 3, cy + 1, 2 * r + 7, GxEPD_BLACK);

  const int ax = cx + r + 6;
  if (rising)
  {
    display.drawLine(ax, cy, ax, cy - 7, GxEPD_BLACK);
    display.drawLine(ax - 3, cy - 4, ax, cy - 7, GxEPD_BLACK);
    display.drawLine(ax + 3, cy - 4, ax, cy - 7, GxEPD_BLACK);
  }
  else
  {
    display.drawLine(ax, cy - 7, ax, cy, GxEPD_BLACK);
    display.drawLine(ax - 3, cy - 3, ax, cy, GxEPD_BLACK);
    display.drawLine(ax + 3, cy - 3, ax, cy, GxEPD_BLACK);
  }
}

// Disque de lune : phase 0 (nouvelle) a 7 (dernier croissant)
void drawMoonDisc(int cx, int cy, int r, int phase)
{
  display.drawCircle(cx, cy, r, GxEPD_BLACK);
  if (phase == 0)
    return;

  const double c = cos(2.0 * PI * phase / 8.0);
  for (int dy = -r; dy <= r; ++dy)
  {
    const double xr = sqrt((double)(r * r - dy * dy));
    const double xt = xr * c;

    // Un trait horizontal par ligne : moins d'appels graphiques que drawPixel().
    int xStart;
    int xEnd;
    if (phase <= 4)
    {
      xStart = (int)ceil(xt);
      xEnd = (int)floor(xr);
    }
    else
    {
      xStart = -(int)floor(xr);
      xEnd = (int)floor(-xt);
    }

    if (xEnd >= xStart)
      display.drawFastHLine(cx + xStart, cy + dy, xEnd - xStart + 1, GxEPD_BLACK);
  }
}
void drawWeather(int tempC, int code, int humidity, int wind, bool ok)
{
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    display.setFont(&Roboto_Bold_18);
    display.setCursor(H_MARGIN, DATE_TOP);
    display.print("M\351t\351o");
    display.drawFastHLine(H_MARGIN, 22, SCREEN_W - 2 * H_MARGIN, GxEPD_BLACK);

    if (ok)
    {
      drawWeatherIcon(225, 44, code, 160);

      display.setFont(&Roboto_Bold_64);
      display.setCursor(H_MARGIN, 96);
      display.print(tempC);

      // Symbole degre dessine a la main : anneau puis "C"
      const int xDeg = display.getCursorX() + 4;
      display.drawCircle(xDeg + 4, 54, 5, GxEPD_BLACK);
      display.drawCircle(xDeg + 4, 54, 4, GxEPD_BLACK);

      display.setFont(&Roboto_Bold_24);
      display.setCursor(xDeg + 12, 96);
      display.print("C");

      display.setFont(&Roboto_Bold_18);
      display.setCursor(170, 92);
      display.print(weatherText(code));

      display.setCursor(H_MARGIN, 120);
      if (humidity >= 0)
      {
        display.print("Humidit\351 ");
        display.print(humidity);
        display.print(" %");
      }
      else
      {
        display.print("Humidit\351 --");
      }

      display.setCursor(170, 120);
      display.print("Vent ");
      if (wind >= 0)
        display.print(wind);
      else
        display.print("--");
      display.print(" km/h");
    }
    else
    {
      display.setFont(&Roboto_Bold_18);
      display.setCursor(18, 76);
      display.print("M\351t\351o indisponible");
    }
  } while (display.nextPage());
}
void showWeather()
{
  int tempC = 0;
  int code = 0;
  int humidity = -1;
  int wind = -1;
  const bool ok = (WiFi.status() == WL_CONNECTED) && fetchWeather(tempC, code, humidity, wind);

  drawWeather(tempC, code, humidity, wind, ok);
  state.showingWeather = true;
  state.skyPage = false;
  state.weatherShownMillis = millis();
}

// Revient a l'horloge apres WEATHER_SHOW_MS
void handleWeatherTimeout()
{
  if (!state.showingWeather)
    return;

  if (millis() - state.weatherShownMillis < WEATHER_SHOW_MS)
    return;

  state.showingWeather = false;
  if (state.ntpReady)
    drawAll();
}

// =====================================================
// Page lune et soleil (bouton externe D6, deuxieme appui)
// =====================================================
static constexpr double SITE_LAT = 43.636;   // Saint-Martin-de-Crau
static constexpr double SITE_LON = 4.830;
SunriseSunset sun(SITE_LAT, SITE_LON, 1);
MoonPhase moon;

// Dimanche = 0 (formule de Sakamoto)
int dayOfWeekSunday0(int y, int m, int d)
{
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

int lastSundayOf(int y, int m)
{
  for (int d = 31; d >= 25; --d)
    if (dayOfWeekSunday0(y, m, d) == 0) return d;
  return 25;
}

// Heure d'ete en Europe : du dernier dimanche de mars au dernier dimanche d'octobre
int utcOffsetFor(int y, int m, int d)
{
  if (m > 3 && m < 10) return 2;
  if (m < 3 || m > 10) return 1;
  if (m == 3) return (d >= lastSundayOf(y, 3)) ? 2 : 1;
  return (d < lastSundayOf(y, 10)) ? 2 : 1;
}

// Heure UTC -> heure locale (France). ntp.epoch() renvoie l'heure UTC, le fuseau est applique ici.
// Heure d'ete : du dernier dimanche de mars 01:00 UTC au dernier dimanche d'octobre 01:00 UTC.
time_t localTimeFromUtc(time_t utc)
{
  const tm* g = gmtime(&utc);
  if (g == nullptr) return utc;

  const int y = g->tm_year + 1900;
  const int mo = g->tm_mon + 1;
  const int d = g->tm_mday;
  const int h = g->tm_hour;

  int off = 1;
  if (mo > 3 && mo < 10) off = 2;
  else if (mo == 3) off = (d > lastSundayOf(y, 3) || (d == lastSundayOf(y, 3) && h >= 1)) ? 2 : 1;
  else if (mo == 10) off = (d < lastSundayOf(y, 10) || (d == lastSundayOf(y, 10) && h < 1)) ? 2 : 1;

  return utc + off * 3600;
}
void drawSkyPage()
{
  const int y = year();
  const int mo = month();
  const int d = day();

  char riseBuf[8] = "--:--";
  char setBuf[8] = "--:--";
  char dayBuf[32] = "";
  const char* phaseName = "";
  int phase = 0;

  if (state.ntpReady)
  {
    sun.setUtcOffset(utcOffsetFor(y, mo, d));
    const SunriseSunset::TimeHM rise = sun.getSunrise(y, mo, d);
    const SunriseSunset::TimeHM sunset = sun.getSunset(y, mo, d);
    phaseName = moon.getMoonPhaseName(y, mo, d);
    phase = (int)moon.getPhase(y, mo, d);
    sun.formatTime(rise, riseBuf, sizeof(riseBuf));
    sun.formatTime(sunset, setBuf, sizeof(setBuf));

    if (rise.valid && sunset.valid)
    {
      const int minutes = (sunset.hour * 60 + sunset.minute) - (rise.hour * 60 + rise.minute);
      snprintf(dayBuf, sizeof(dayBuf), "Dur\351e du jour : %d h %02d", minutes / 60, minutes % 60);
    }
    else
    {
      snprintf(dayBuf, sizeof(dayBuf), "Dur\351e du jour : --");
    }
  }

  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    display.setFont(&Roboto_Bold_18);
    display.setCursor(H_MARGIN, DATE_TOP);
    display.print("Lune et soleil");
    display.drawFastHLine(H_MARGIN, 22, SCREEN_W - 2 * H_MARGIN, GxEPD_BLACK);

    if (state.ntpReady)
    {
      drawMoonDisc(254, 62, 22, phase);

      display.setFont(&Roboto_Bold_18);
      display.setCursor(H_MARGIN, 50);
      display.print(phaseName);

      drawSunIcon(16, 76, 6, true);
      display.setCursor(36, 80);
      display.print(riseBuf);

      drawSunIcon(150, 76, 6, false);
      display.setCursor(172, 80);
      display.print(setBuf);

      display.setCursor(H_MARGIN, 104);
      display.print(dayBuf);
    }
    else
    {
      display.setFont(&Roboto_Bold_18);
      display.setCursor(H_MARGIN, 76);
      display.print("Heure indisponible");
    }
  } while (display.nextPage());
}
void showSky()
{
  drawSkyPage();
  state.showingWeather = true;
  state.skyPage = true;
  state.weatherShownMillis = millis();
}
// Appui sur le bouton externe D6 : meteo, puis lune et soleil, puis retour a l'horloge
void handleButton()
{
  static bool lastLevel = HIGH;
  static unsigned long lastChangeMs = 0;

  const bool level = digitalRead(PAGE_BUTTON_PIN);
  const unsigned long nowMs = millis();

  if (level != lastLevel && nowMs - lastChangeMs > 50)
  {
    lastChangeMs = nowMs;
    lastLevel = level;

    if (level == LOW)
    {
      if (!state.showingWeather)
      {
        Serial.println("Bouton externe : affichage meteo");
        showWeather();
      }
      else if (!state.skyPage)
      {
        Serial.println("Bouton externe : affichage lune et soleil");
        showSky();
      }
      else
      {
        Serial.println("Bouton externe : retour a l'horloge");
        state.showingWeather = false;
        state.skyPage = false;
        if (state.ntpReady)
        {
          state.forceNtpSync = true;
          drawAll();
        }
      }
    }
  }
}
void handleNtpSync()
{
  const unsigned long nowMs = millis();
  if (!shouldSyncNTP(nowMs))
    return;

  state.lastNtpAttemptMillis = nowMs;
  state.forceNtpSync = false;

  // Pas de tentative NTP sans Wi-Fi : la reconnexion automatique du Wi-Fi
  // se fait en arriere-plan, et la prochaine tentative aura lieu apres le delai de nouvelle tentative
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wi-Fi indisponible, synchro NTP reportee");
    return;
  }

  const bool wasReady = state.ntpReady;
  const time_t oldTime = now();

  if (!syncTimeFromNTP())
    return;

  if (!state.screenReady || !wasReady)
  {
    drawAll();
    return;
  }

  const bool dayChanged =
      day(oldTime) != day(now()) ||
      month(oldTime) != month(now()) ||
      year(oldTime) != year(now());

  const bool minuteChanged =
      hour(oldTime) != hour(now()) ||
      minute(oldTime) != minute(now());

  redrawChanged(dayChanged, minuteChanged);
}

// =====================================================
// Full refresh périodique
// =====================================================
bool shouldDoFullRefresh(unsigned long nowMs)
{
  if (!state.screenReady || state.showingWeather)
    return false;

  if (state.partialUpdates >= PARTIAL_UPDATES_BEFORE_FULL)
    return true;

  return (nowMs - state.lastFullRefreshMillis) >= FULL_REFRESH_INTERVAL_MS;
}

void handlePeriodicFullRefresh()
{
  const unsigned long nowMs = millis();
  if (!shouldDoFullRefresh(nowMs))
    return;

  Serial.println("Full refresh periodique de l'ecran");
  drawAll();
}

// =====================================================
// Message d'erreur
// =====================================================
void drawErrorMessage()
{
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&Roboto_Bold_18);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(18, 46);
    display.print("Heure indisponible");
    display.setCursor(18, 76);
    display.print("V\351rifier Wi-Fi / NTP");
  } while (display.nextPage());
}

// =====================================================
// Setup / loop
// =====================================================
void setup()
{
  Serial.begin(SERIAL_BAUDRATE);
  Serial.println();
  Serial.println("=== Demarrage horloge e-paper ===");
  Serial.print("Build : ");
  Serial.println(FW_BUILD);

  pinMode(PAGE_BUTTON_PIN, INPUT_PULLUP);
  setupHSPI();

  display.init(SERIAL_BAUDRATE, true, 2, false);
  display.setRotation(1);

  // Diagnostic : modes de rafraichissement reellement supportes par le pilote
  Serial.print("hasPartialUpdate : ");
  Serial.println(display.epd2.hasPartialUpdate);
  Serial.print("hasFastPartialUpdate : ");
  Serial.println(display.epd2.hasFastPartialUpdate);

  // Effacement complet de l'ecran (les deux buffers) avant le splash
  display.clearScreen();

  // Le splash efface déjà l'écran et reste affiché pendant la connexion Wi-Fi.
  // Cela évite deux rafraîchissements complets redondants au démarrage.
  showSplashBitmap(SPLASH_DURATION_MS);

  const bool wifiOk = connectWiFiAndInitNTP();

  if (wifiOk && initialSync())
  {
    drawAll();
  }
  else
  {
    clearFullScreen();
    drawErrorMessage();
  }

  state.previousTickMillis = millis();
}

void loop()
{
  ArduinoOTA.handle();
  handleButton();
  handleWeatherTimeout();
  handleTick();
  handleNtpSync();
  handlePeriodicFullRefresh();
}