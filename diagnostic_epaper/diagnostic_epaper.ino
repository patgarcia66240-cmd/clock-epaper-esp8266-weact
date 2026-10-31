
#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h>

// ============================================================
// WEACT STUDIO E-PAPER 2.9" NOIR ET BLANC
// Carte : Wemos D1 mini / ESP8266
// Interface : SPI materiel
//
// Cablage conforme au schema fourni.
// Pilote candidat : GxEPD2_290_BS
// ============================================================

// ---------------- BROCHES ESP8266 ---------------------------

constexpr uint8_t PIN_EPD_CS   = D8;  // GPIO15 - Bleu
constexpr uint8_t PIN_EPD_DC   = D2;  // GPIO4  - Blanc
constexpr uint8_t PIN_EPD_RST  = D1;  // GPIO5  - Orange
constexpr uint8_t PIN_EPD_BUSY = D0;  // GPIO16 - Violet

// SPI materiel ESP8266 :
// DIN / SDA -> D7 / GPIO13 (Bleu)
// CLK / SCL -> D5 / GPIO14 (Jaune)

// ---------------- PILOTE E-PAPER ----------------------------

GxEPD2_BW<
    GxEPD2_290_BS,
    GxEPD2_290_BS::HEIGHT
> display(
    GxEPD2_290_BS(
        PIN_EPD_CS,
        PIN_EPD_DC,
        PIN_EPD_RST,
        PIN_EPD_BUSY
    )
);

// ---------------- OUTILS DE DIAGNOSTIC ----------------------

void afficherEtatBusy(const char* etape)
{
    Serial.print(F("[BUSY] "));
    Serial.print(etape);
    Serial.print(F(" : "));

    Serial.println(
        digitalRead(PIN_EPD_BUSY) == HIGH
            ? F("HIGH (1)")
            : F("LOW (0)")
    );
}

void afficherConfiguration()
{
    Serial.println();
    Serial.println(F("======================================"));
    Serial.println(F(" WEACT E-PAPER 2.9 - TEST ESP8266"));
    Serial.println(F(" Pilote : GxEPD2_290_BS"));
    Serial.println(F(" SPI : D7/MOSI et D5/SCK"));
    Serial.println(F("======================================"));

    Serial.println(F("VCC  : 3V3  [Rouge]"));
    Serial.println(F("GND  : GND  [Noir]"));
    Serial.println(F("DIN  : D7   [Jaune]"));
    Serial.println(F("CLK  : D5   [Vert]"));
    Serial.println(F("CS   : D8   [Bleu]"));
    Serial.println(F("DC   : D2   [Blanc]"));
    Serial.println(F("RST  : D1   [Orange]"));
    Serial.println(F("BUSY : D0   [Violet]"));

    Serial.printf(
        "Frequence CPU : %u MHz\n",
        ESP.getCpuFreqMHz()
    );

    Serial.printf(
        "Memoire libre : %u octets\n",
        ESP.getFreeHeap()
    );
}

// ---------------- TEST D'AFFICHAGE --------------------------

void afficherPageTest()
{
    Serial.println(F("[4] Preparation de la page..."));

    display.setRotation(1);
    display.setFullWindow();
    display.setTextColor(GxEPD_BLACK);
    display.setFont(NULL);

    Serial.println(F("[5] Demarrage du rafraichissement..."));

    display.firstPage();

    do
    {
        display.fillScreen(GxEPD_WHITE);

        display.setCursor(10, 25);
        display.println(F("WEACT 2.9"));

        display.setCursor(10, 50);
        display.println(F("ESP8266 OK"));

        display.setCursor(10, 75);
        display.println(F("TEST SPI"));

        display.setCursor(10, 100);
        display.println(F("NOIR ET BLANC"));

        display.setCursor(10, 125);
        display.println(F("D1 MINI"));

    } while (display.nextPage());

    Serial.println(F("[6] Sequence d'affichage terminee."));
}

// ---------------- INITIALISATION ----------------------------

void setup()
{
    Serial.begin(115200);
    delay(1500);

    afficherConfiguration();

    pinMode(PIN_EPD_BUSY, INPUT);
    afficherEtatBusy("Avant SPI");

    Serial.println(F("[1] Initialisation SPI..."));

    SPI.begin();

    Serial.println(F("[2] Initialisation du pilote..."));

    // Parametres :
    // 115200 = vitesse serie pour le diagnostic
    // true   = messages de diagnostic de la bibliotheque
    // 50     = delai du reset en millisecondes
    // false  = pas de reset externe supplementaire
    display.init(115200, true, 50, false);

    Serial.println(F("[3] Retour de display.init()."));
    afficherEtatBusy("Apres initialisation");

    afficherPageTest();

    afficherEtatBusy("Apres rafraichissement");

    Serial.printf(
        "Memoire libre apres test : %u octets\n",
        ESP.getFreeHeap()
    );

    Serial.println(F("[7] Mise en veille du panneau..."));

    display.hibernate();

    Serial.println(F("[8] TEST TERMINE."));
}

// ---------------- BOUCLE PRINCIPALE -------------------------

void loop()
{
    // Pas de rafraichissement repetitif.
    delay(1000);
}

