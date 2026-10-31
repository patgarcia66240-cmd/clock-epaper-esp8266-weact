#include "SaintCalendar.h"
#include <stdio.h>
#include <string.h>

const char* const SaintCalendar::WDAY_NAMES[7] = {
    "Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"
};

const char* const SaintCalendar::MONTH_NAMES[12] = {
    "Janvier", "F\351vrier", "Mars", "Avril", "Mai", "Juin",
    "Juillet", "Ao\373t", "Septembre", "Octobre", "Novembre", "D\351cembre"
};

const char* const SaintCalendar::angels1[31] = {
    "!Jour de l'an","Basile","*Genevi\350ve","*Odilon",
    "Edouard","*M\351laine","Raymond","Lucien",
    "*Alix","Guillaume","*Pauline","*Tatiana",
    "*Yvette","*Nina","R\351mi","Marcel",
    "*Roseline","*Prisca","Marius","S\351bastien",
    "*Agn\350s","Vincent","Banard","Fran\347ois de Sales",
    "!Conversion de Paul","*Paule","*Ang\350le","Thomas d'Aquin",
    "Gildas","*Martine","*Marcelle"
};

const char* const SaintCalendar::angels2[30] = {
    "*Ella","Pr\351sentation","Blaise","*V\351ronique",
    "*Agathe","*Gaston","*Eug\350nie","*Jacqueline",
    "*Apolline","Arnaud","Notre Dame de Lourdes","F\351lix",
    "*B\351atrice","Valentin","Claude","*Julienne",
    "Alexis","*Bernadette","Gabin","*Aim\351e",
    "Damien","*Isabelle","Lazare","Modeste",
    "Rom\351o","Nestor","*Honorine","Romain",
    "Auguste Chapdelaine",
    "!Anniv. bissextile"
};

const char* const SaintCalendar::angels3[31] = {
    "Albin","Charles","Gwenol\351","Rom\351o",
    "Th\351ophile","*Agn\350s","*F\351licit\351","Ryan",
    "*Francoise","Vivien","*Rosine","*Justine",
    "*L\351andre","*Eve","Louise","*B\351n\351dicte",
    "Patrick","Cyrille","Joseph","*Alexandra",
    "Cl\351mence","*L\351a","*R\351becca","*Catherine",
    "Humbert","*Larissa","Albert","Gontran",
    "*Gladys","Am\351d\351e","*Acace"
};

const char* const SaintCalendar::angels4[30] = {
    "Hugues","*Sandrine","Richard","Isidore",
    "*Ir\350ne","Marcellin","Jean-Baptiste de la Salle","*Julie",
    "Gautier","Fulbert","Stanislas","Jules",
    "*Ida","Maxime","Paterne","Benoit-Joseph",
    "*Anicet","Parfait","*Emma","*Odette",
    "Anselme","Alexandre","Georges","Fid\350le",
    "Marc","*Alida","*Zita","*Val\351rie",
    "Catherine de Sienne","Robert"
};

const char* const SaintCalendar::angels5[31] = {
    "!F\352te du travail","Boris","Philippe-Jacques","Sylvain",
    "*Judith","*Prudence","*Gis\350le","!Armistice 1945",
    "!Pac\364me","*Solange","*Estelle","Achille",
    "*Rolande","Mathias","*Denise","Honor\351",
    "Pascal","Eric","Yves","Bernardin",
    "Constantin","Emile","Didier","Donatien",
    "*Sophie","B\351renger","Augustin","Germain",
    "Aymar","Ferdinand","!Visitation de la Ste Vierge"
};

const char* const SaintCalendar::angels6[30] = {
    "Justin","Alcide","Kevin","*Clothide","Boniface","Claude","Gilbert","M\351dard","Ephrem",
    "Landry","Barnab\351","Guy","Antoine","*Elis\351e","*Germaine","R\351gis",
    "Herv\351","*L\351once","*Jude","*Silv\350re","Louis","Alban","*Audrey",
    "Jean","Prosper","Anthelme","Fernand","*Ir\351n\351e","Pierre","Martial"
};

const char* const SaintCalendar::angels7[31] = {
    "Thierry","*Eug\351nie","Thomas","*Isabelle","Antoine-Marie","*Maria","Roger",
    "*Priscille","*Irma","Ulric","Benoit","Olivier","Henri","Camille",
    "Bonaventure","*Carmen","*Charlotte","Fr\351d\351ric"," ","Aur\350le","Victor",
    "*Madeleine","*Brigitte","*Christine","Christophe","*Anne","F\351lix","Samson",
    "*Marthe","*Juliette","Ignace"
};

const char* const SaintCalendar::angels8[31] = {
    "Alphonse","Julien","*Lydie","*Darcy",
    "Abel","Sauveur","Ga\353tan","Dominique","*B\351n\351dicte","Laurent","*Claire",
    "*Chantal","Hyppolyte","Maximilien","Assomption","*Armel","*Hyacinthe",
    "Eudes"," ","Bernard","Christophe","*Marie-Reine","*Rose","Barth\351l\351my","Louis",
    "C\351saire","*Monique","Augustin","*Sabine","Fiacre","Aristide"
};

const char* const SaintCalendar::angels9[30] = {
    "Gilles","*Ingrid","Gr\351goire","*Rosalie",
    "*Ra\357ssa","Bertrand","*Reine","!Nativit\351",
    "Alain","*In\350s","Adelphe","Apollinaire",
    "Aim\351","!Croix Glorieuse","Roland","*Edith",
    "Renaud","*Nad\350ge","*Emilie","Davy",
    "Matthieu","Maurice","!Automne","Th\350cle",
    "Hermann","C\364me et Damien","Vincent de Paul","Venceslas",
    "Michel","J\351r\364me"
};

const char* const SaintCalendar::angels10[31] = {
    "Th\351r\350se de l'Enfant J\351sus","L\351ger","G\351rard","Fran\347ois d'Assise",
    "*Fleur","Bruno","Serge","*P\351lagie",
    "Denis","*Ghislaine","Firmin","Wilfried",
    "G\351raud","Juste","*Th\351r\350se d'Avila","*Edwige",
    "Baudoin","Luc","Ren\351","*Adeline",
    "*C\351line","*Elodie","Jean de Capistran","Florentin",
    "Cr\351pin","Dimitri","*Emeline","Jude",
    "Narcisse","Bienvenue","Quentin"
};

const char* const SaintCalendar::angels11[30] = {
    "!Toussaint","!D\351funts","Hubert","Hubert","Charles",
    "Sylvie","Bertille","Carine","Geoffroy",
    "Th\351odore","L\351on","!Armistice 1918","Christian",
    "Brice","Sidoine","Albert","*Marguerite",
    "*Elisabeth","Aude","Tanguy","Edmond",
    "!Pr\351sence de Marie","*C\351cile","Cl\351ment","*Flora",
    "*Catherine","*Delphine","S\351vrin","Jacques de la Marche",
    "Saturnin"
};

const char* const SaintCalendar::angels12[31] = {
    "*Florence","*Viviane","Fran\347ois Xavier","*Barbara",
    "G\351rald","Nicolas","Ambroise","!Immacul\351e Conception",
    "Pierre Fourier","Romaric","Daniel","!Jeanne-Fran\347oise de Chan",
    "*Lucie","Gatien","*Ninon","*Alice",
    "Ga\353l","*Aude","Armand","*Ad\350le",
    "!Hiver","*Fran\347oise Xavi\350re","Cl\351ment","*Flora",
    "!No\353l","Etienne","Jean","Innocents",
    "David","Roger","Sylvestre"
};

const char* const* const SaintCalendar::SAINTS_BY_MONTH[12] = {
    angels1, angels2, angels3, angels4, angels5, angels6,
    angels7, angels8, angels9, angels10, angels11, angels12
};

bool SaintCalendar::isLeapYear(int year) {
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

uint8_t SaintCalendar::daysInMonth(uint8_t month, int year) {
    switch (month) {
        case 1:  return 31;
        case 2:  return isLeapYear(year) ? 29 : 28;
        case 3:  return 31;
        case 4:  return 30;
        case 5:  return 31;
        case 6:  return 30;
        case 7:  return 31;
        case 8:  return 31;
        case 9:  return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default: return 0;
    }
}

bool SaintCalendar::isValidMonth(uint8_t month) {
    return month >= 1 && month <= 12;
}

bool SaintCalendar::isValidDay(uint8_t day, uint8_t month, int year) {
    if (!isValidMonth(month)) {
        return false;
    }
    const uint8_t maxDay = daysInMonth(month, year);
    return day >= 1 && day <= maxDay;
}

const char* SaintCalendar::getWeekdayName(uint8_t weekday) {
    if (weekday > 6) {
        return "";
    }
    return WDAY_NAMES[weekday];
}

const char* SaintCalendar::getMonthName(uint8_t month) {
    if (!isValidMonth(month)) {
        return "";
    }
    return MONTH_NAMES[month - 1];
}

const char* SaintCalendar::getSaintRaw(uint8_t day, uint8_t month, int year) {
    if (!isValidDay(day, month, year)) {
        return nullptr;
    }
    return SAINTS_BY_MONTH[month - 1][day - 1];
}

bool SaintCalendar::formatSaint(const char* saint, char* buffer, size_t bufferSize) {
    if (saint == nullptr || buffer == nullptr || bufferSize == 0) {
        return false;
    }

    if (saint[0] == '*') {
        snprintf(buffer, bufferSize, "Ste %s", saint + 1);
    } else if (saint[0] == '!') {
        snprintf(buffer, bufferSize, "%s", saint + 1);
    } else {
        snprintf(buffer, bufferSize, "St %s", saint);
    }

    buffer[bufferSize - 1] = '\0';
    return true;
}

bool SaintCalendar::getSaint(uint8_t day, uint8_t month, int year, char* buffer, size_t bufferSize) {
    const char* raw = getSaintRaw(day, month, year);
    if (raw == nullptr) {
        if (buffer != nullptr && bufferSize > 0) {
            snprintf(buffer, bufferSize, "Glinglin");
            buffer[bufferSize - 1] = '\0';
        }
        return false;
    }

    return formatSaint(raw, buffer, bufferSize);
}