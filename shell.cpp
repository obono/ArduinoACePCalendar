/**
 * ArduinoACePCalendar : "shell.cpp"
 *
 * Copyright (c) 2022 OBONO
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <arduino.h>
#include <EEPROM.h>
#include "RX8900Controller.h"
#include "ACePController.h"
#include "testpatterndata.h"

#define VERSION "0.2.0"

#define INPUT_BUF_SIZE  32
#define COMMAND_LEN_MAX 8

static void commandNow(char *pArg, uint8_t argLen);
static void commandDate(char *pArg, uint8_t argLen);
static void commandTime(char *pArg, uint8_t argLen);
static void commandAlarm(char *pArg, uint8_t argLen);
static void commandClear(char *pArg, uint8_t argLen);
static void commandIndex(char *pArg, uint8_t argLen);
static void commandLoad(char *pArg, uint8_t argLen);
static void commandExamine(char *pArg, uint8_t argLen);
static void commandHelp(char *pArg, uint8_t argLen);
static void commandVersion(char *pArg, uint8_t argLen);
static void commandQuit(char *pArg, uint8_t argLen);

static void printResult(const bool isOK);
static void printCurrentDate(void);
static void printCurrentTime(void);
static void printAlarmTime(void);
static void printTime(uint8_t hour, uint8_t minute, uint8_t second);
static void printIndexAndPath(uint8_t index, const char *path);
static bool extractNumber(char *p, uint8_t digits, uint16_t &value);

typedef struct {
    const char  name[COMMAND_LEN_MAX];
    void        (*func)(char *, uint8_t);
    const char  *usage;
} Command_T;

PROGMEM static const char usageNow[]     = "Show current date and time.";
PROGMEM static const char usageDate[]    = "Set date by 8 digits (yyyymmdd).";
PROGMEM static const char usageTime[]    = "Set time by 6 digits (HHMMSS).";
PROGMEM static const char usageAlarm[]   = "Set alarm time by 4 digits (HHMM).";
PROGMEM static const char usageClear[]   = "Clear display with color (0-6).";
PROGMEM static const char usageIndex[]   = "Set image index number (0-255).";
PROGMEM static const char usageLoad[]    = "Load image data (0-255 or current).";
PROGMEM static const char usageExamine[] = "Examine function (0-3).";
PROGMEM static const char usageHelp[]    = "Show command help.";
PROGMEM static const char usageVersion[] = "Show version information.";
PROGMEM static const char usageQuit[]    = "Quit shell.";

PROGMEM static const Command_T commandTable[] = {
    { "NOW",     commandNow,     usageNow     },
    { "DATE",    commandDate,    usageDate    },
    { "TIME",    commandTime,    usageTime    },
    { "ALARM",   commandAlarm,   usageAlarm   },
    { "CLEAR",   commandClear,   usageClear   },
    { "INDEX",   commandIndex,   usageIndex   },
    { "LOAD",    commandLoad,    usageLoad    },
    { "EXAMINE", commandExamine, usageExamine },
    { "HELP",    commandHelp,    usageHelp    },
    { "VERSION", commandVersion, usageVersion },
    { "QUIT",    commandQuit,    usageQuit    },
};

extern RX8900Controller rtc;
extern ACePController   acep;
extern bool             isShellEnabled;

static char     inputBuf[INPUT_BUF_SIZE];
static uint8_t  inputPos = 0;

/*---------------------------------------------------------------------------*/

void printShellMessage(void)
{
    Serial.println();
    Serial.println(F("ArduinoACePCalendar Shell"));
    commandVersion(NULL, 0);
    Serial.println();
}

void printShellPrompt(void)
{
    Serial.print(F("> "));
}

void handleSerialInput(char data)
{
    if (inputPos < INPUT_BUF_SIZE && (
            data == ' ' && inputPos > 0 && inputBuf[inputPos - 1] != ' ' ||
            data >= '0' && data <= '9' || data >= 'A' && data <= 'Z' || data >= 'a' && data <= 'z')) {
        inputBuf[inputPos] = data;
        inputPos++;
        Serial.print(data);
    } else if (data == '\b' && inputPos > 0) {
        inputPos--;
        Serial.print(F("\b \b"));
    } else if (data == '\n' || data == '\r') {
        Serial.println();
        if (inputPos > 0) {
            uint8_t commandLen = 0;
            while (commandLen < inputPos && inputBuf[commandLen] != ' ') {
                commandLen++;
            }
            bool isMatched = false;
            for (uint8_t i = 0; i < sizeof(commandTable) / sizeof(commandTable[0]); i++) {
                if (strncasecmp_P(inputBuf, &commandTable[i].name[0], commandLen) == 0) {
                    char *pArg = &inputBuf[commandLen + 1];
                    uint8_t argLen = 0;
                    while (commandLen + 1 + argLen < inputPos && pArg[argLen] != ' ') {
                        argLen++;
                    }
                    ((void (*)(char *, uint8_t))pgm_read_ptr(&commandTable[i].func))(pArg, argLen);
                    isMatched = true;
                    break;
                }
            }
            if (!isMatched) {
                Serial.println(F("Invalid command."));
            }
            inputPos = 0;
        }
        if (isShellEnabled) {
            printShellPrompt();
        }
    }
}

/*---------------------------------------------------------------------------*/

static void commandNow(char *pArg, uint8_t argLen)
{
    printCurrentDate();
    Serial.print(' ');
    printCurrentTime();
    Serial.println();
}

static void commandDate(char *pArg, uint8_t argLen)
{
    if (argLen == 0) {
        printCurrentDate();
        Serial.println();
        return;
    }
    uint16_t year, month, day;
    bool isOK = argLen == 8 && extractNumber(pArg, 4, year) && extractNumber(pArg + 4, 2, month) &&
            extractNumber(pArg + 6, 2, day) && rtc.setDate(year, month, day);
    printResult(isOK);
}

static void commandTime(char *pArg, uint8_t argLen)
{
    if (argLen == 0) {
        printCurrentTime();
        Serial.println();
        return;
    }
    uint16_t hour, minute, second;
    bool isOK = argLen == 6 && extractNumber(pArg, 2, hour) && extractNumber(pArg + 2, 2, minute) &&
            extractNumber(pArg + 4, 2, second) && rtc.setTime(hour, minute, second);
    printResult(isOK);
}

static void commandAlarm(char *pArg, uint8_t argLen)
{
    if (argLen == 0) {
        printAlarmTime();
        Serial.println();
        return;
    }
    uint16_t hour, minute;
    bool isOK = argLen == 4 && extractNumber(pArg, 2, hour) && extractNumber(pArg + 2, 2, minute) &&
            rtc.setAlarm(hour, minute);
    printResult(isOK);
}

static void commandClear(char *pArg, uint8_t argLen)
{
    uint16_t color = WHITE;
    if (argLen > 0) {
        extractNumber(pArg, argLen, color);
    }
    bool isOK = acep.clearDisplay((ACEP_COLOR)color);
    printResult(isOK);
}

static void commandIndex(char *pArg, uint8_t argLen)
{
    uint16_t index = 0;
    if (argLen == 0) {
        index = rtc.getImageIndex();
        char path[PATH_LEN_MAX];
        if (acep.specifyImagePathOfSD(index, path)) {
            index = 0;
        }
        printIndexAndPath(index, path);
        return;
    }
    bool isOK = extractNumber(pArg, argLen, index) && index <= UINT8_MAX;
    if (isOK) {
        rtc.setImageIndex(index);
    }
    printResult(isOK);
}

static void commandLoad(char *pArg, uint8_t argLen)
{
    uint16_t index;
    if (argLen == 0 || !extractNumber(pArg, argLen, index) || index > UINT8_MAX) {
        index = rtc.getImageIndex();
    }
    char path[PATH_LEN_MAX];
    if (acep.specifyImagePathOfSD(index, path)) {
        index = 0;
    }
    printIndexAndPath(index, path);
    bool isOK = acep.displayACePDataFromSD(path);
    printResult(isOK);
}

static void commandExamine(char *pArg, uint8_t argLen)
{
    uint16_t test = 0;
    if (argLen > 0) {
        extractNumber(pArg, argLen, test);
    }

    bool isOK = false;
    switch (test) {
        case 0:
            uint16_t year;
            uint8_t month, day;
            if (rtc.getDate(year, month, day)) {
                acep.setDate(year, month, day);
            }
            acep.clearDisplay();
            char path[PATH_LEN_MAX];
            acep.specifyImagePathOfSD(rtc.getImageIndex(), path);
            isOK = acep.displayACePDataFromSD(path, true);
            break;
        case 1:
        case 2:
            isOK = acep.displayACePTestPattern(test == 2);
            break;
        case 3:
            isOK = acep.displayACePDataFromPGM(imgTestPattern, IMG_TEST_PATTERN_WIDTH, IMG_TEST_PATTERN_HEIGHT);
            break;
        default:
            break;
    }
    printResult(isOK);
}

static void commandHelp(char *pArg, uint8_t argLen)
{
    for (uint8_t i = 0; i < sizeof(commandTable) / sizeof(commandTable[0]); i++) {
        char text[40];
        strncpy_P(text, &commandTable[i].name[0], COMMAND_LEN_MAX);
        Serial.print(F("    "));
        Serial.print(text);
        for (uint8_t i = strnlen(text, COMMAND_LEN_MAX); i < COMMAND_LEN_MAX; i++) {
            Serial.print(' ');
        }
        strncpy_P(text, (const char *)pgm_read_ptr(&commandTable[i].usage), sizeof(text));
        Serial.println(text);
    }
}

static void commandVersion(char *pArg, uint8_t argLen)
{
    Serial.println(F("Version: " VERSION " (" __DATE__ " " __TIME__ ")"));
}

static void commandQuit(char *pArg, uint8_t argLen)
{
    isShellEnabled = false;
    Serial.println(F("Good bye!"));
    Serial.end();
}

/*---------------------------------------------------------------------------*/

static void printResult(const bool isOK)
{
    Serial.println(isOK ? F("OK") : F("Error"));
}

static void printCurrentDate(void)
{
    uint16_t year;
    uint8_t month, day;
    if (rtc.getDate(year, month, day)) {
        Serial.print(year);
        Serial.print('/');
        Serial.print(month);
        Serial.print('/');
        Serial.print(day);
    }
}

static void printCurrentTime(void)
{
    uint8_t hour, minute, second;
    if (rtc.getTime(hour, minute, second)) {
        printTime(hour, minute, second);
    }
}

static void printAlarmTime(void)
{
    uint8_t hour, minute;
    if (rtc.getAlarm(hour, minute)) {
        printTime(hour, minute, 0);
    }
}

static void printTime(uint8_t hour, uint8_t minute, uint8_t second)
{
    if (hour < 10) {
        Serial.print('0');
    }
    Serial.print(hour);
    Serial.print(':');
    if (minute < 10) {
        Serial.print('0');
    }
    Serial.print(minute);
    Serial.print(':');
    if (second < 10) {
        Serial.print('0');
    }
    Serial.print(second);
}

static void printIndexAndPath(uint8_t index, const char *path)
{
    Serial.print('[');
    Serial.print(index);
    Serial.print(F("] path: "));
    Serial.println(path);
}

static bool extractNumber(char *p, uint8_t digits, uint16_t &value)
{
    value = 0;
    while (digits--) {
        if (*p < '0' || *p > '9') {
            return false;
        }
        value = value * 10 + (*p - '0');
        p++;
    }
    return true;
}
