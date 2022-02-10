/**
 * ArduinoACePCalendar : "ACePController.cpp"
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

#include <SD.h>
#include "ACePController.h"
#include "imagedata.h"

#define DISPLAY_WIDTH   600
#define DISPLAY_HEIGHT  448
#define TARGET_FILESIZE ((uint32_t)DISPLAY_WIDTH * DISPLAY_HEIGHT / 2)

#define ACEP_RESET_PIN  8
#define ACEP_DC_PIN     9
#define ACEP_CS_PIN     10
#define ACEP_BUSY_PIN   7

#define SD_CS_PIN       4
#define SD_CD_PIN       5

#define waitShort()     delay(50)
#define waitLong()      delay(200)


PROGMEM static const uint8_t initialzeSequence1[] = {
    // cmd,  data, ...
    3, 0x00, 0xEF, 0x08,
    5, 0x01, 0x37, 0x00, 0x23, 0x23,
    2, 0x03, 0x00,
    4, 0x06, 0xC7, 0xC7, 0x1D,
    2, 0x30, 0x3C,
    2, 0x40, 0x00,
    2, 0x50, 0x37,
    2, 0x60, 0x22,
    5, 0x61, DISPLAY_WIDTH >> 8, DISPLAY_WIDTH & 0xFF, DISPLAY_HEIGHT >> 8, DISPLAY_HEIGHT & 0xFF,
    2, 0xE3, 0xAA,
    0
}; 

PROGMEM static const uint8_t initialzeSequence2[] = {
    // cmd,  data, ...
    2, 0x50, 0x37,
    0
};

PROGMEM static const uint8_t displayStartSequence[] = {
    // cmd,  data, ...
    5, 0x61, DISPLAY_WIDTH >> 8, DISPLAY_WIDTH & 0xFF, DISPLAY_HEIGHT >> 8, DISPLAY_HEIGHT & 0xFF,
    1, 0x10,
    0
};

PROGMEM static const uint8_t sleepSequence[] = {
    // cmd,  data, ...
    2, 0x07, 0xA5,
    0
};


PROGMEM static const uint8_t testDatePattern1[] = {
    IMG_ID_BLANK, IMG_ID_BLANK, IMG_ID_BLANK, IMG_ID_BLANK, IMG_ID_BLANK,
    IMG_ID_BLANK, IMG_ID_BLANK, IMG_ID_BLANK, IMG_ID_BLANK, IMG_ID_BLANK,
    IMG_ID_KANJI_SUN, IMG_ID_KANJI_MON, IMG_ID_KANJI_TUE, IMG_ID_KANJI_WED,
};

PROGMEM static const uint8_t testDatePattern2[] = {
    IMG_ID_NUMBER_0, IMG_ID_NUMBER_1, IMG_ID_NUMBER_2, IMG_ID_NUMBER_3, IMG_ID_NUMBER_4,
    IMG_ID_NUMBER_5, IMG_ID_NUMBER_6, IMG_ID_NUMBER_7, IMG_ID_NUMBER_8, IMG_ID_NUMBER_9,
    IMG_ID_KANJI_THU, IMG_ID_KANJI_FRI, IMG_ID_KANJI_SAT, IMG_ID_KANJI_YEAR,
};

/*---------------------------------------------------------------------------*/

void ACePController::setup()
{
    pinMode(ACEP_RESET_PIN, OUTPUT);
    pinMode(ACEP_DC_PIN, OUTPUT);
    pinMode(ACEP_CS_PIN, OUTPUT);
    pinMode(ACEP_BUSY_PIN, INPUT); 
    pinMode(SD_CD_PIN, INPUT);
    pinMode(SD_CS_PIN, OUTPUT);
    isInitialized = false;
}

void ACePController::initialize()
{
    digitalWrite(ACEP_CS_PIN, HIGH);
    digitalWrite(SD_CS_PIN, HIGH);
    digitalWrite(ACEP_RESET_PIN, LOW);
    waitShort();
    digitalWrite(ACEP_RESET_PIN, HIGH);
    waitLong();
    waitACePBusyHigh(100); // time limit = 5 secs
    if (digitalRead(ACEP_BUSY_PIN) != HIGH) {
        return;
    }

    SPI.begin();
    applyACePSequence(initialzeSequence1);
    waitShort();
    applyACePSequence(initialzeSequence2);
    isInitialized = true;
}

void ACePController::setDate(uint16_t year, uint8_t month, uint8_t day)
{
    placeDigits(&dateLetters[3], year, 4);
    dateLetters[4] = IMG_ID_KANJI_YEAR;
    placeDigits(&dateLetters[6], month, 2);
    dateLetters[7] = IMG_ID_KANJI_MONTH;
    placeDigits(&dateLetters[9], day, 2);
    dateLetters[10] = IMG_ID_KANJI_DAY;
    dateLetters[11] = IMG_ID_BRACKET_L;
    uint8_t youbi = IMG_ID_KANJI_SUN + calculateYoubi(year, month, day);
    dateLetters[12] = youbi;
    dateLetters[13] = IMG_ID_BRACKET_R;
    switch (youbi) {
        case IMG_ID_KANJI_SUN: 
            fgColor = RED;
            break;
        case IMG_ID_KANJI_SAT:
            fgColor = BLUE;
            break;
        default:
            fgColor = BLACK;
            break;
    }
}

bool ACePController::clearDisplay(ACEP_COLOR color)
{
    if (!isInitialized || color < BLACK || color > ORANGE) {
        return false;
    }
    applyACePSequence(displayStartSequence);
    uint8_t buffer[DISPLAY_WIDTH / 2];
    memset(buffer, color | color << 4, sizeof(buffer));
    beginACePTransaction();
    for (uint16_t y = 0; y < DISPLAY_HEIGHT; y++) {
        sendACePData(buffer, sizeof(buffer));
    }
    endACePTransaction();
    refreshACePScreen();
    return true;
}

bool ACePController::displayACePDataFromPGM(
        const uint8_t *pImage, uint16_t width, uint16_t height, bool isDisplayDate)
{
    if (!isInitialized || !pImage || !width || !height) {
        return false;
    }
    applyACePSequence(displayStartSequence);
    uint8_t buffer[DISPLAY_WIDTH / 2];
    beginACePTransaction();
    for (uint16_t y = 0; y < DISPLAY_HEIGHT; y++) {
        const uint8_t *pSrc = pImage + width / 2 * (y % height);
        for (uint16_t x = 0, srcWidth = width / 2; x < DISPLAY_WIDTH / 2; x += srcWidth) {
            if (srcWidth > DISPLAY_WIDTH / 2 - x) {
                srcWidth = DISPLAY_WIDTH / 2 - x;
            }
            memcpy_P(buffer + x, pSrc, srcWidth);
        }
        if (isDisplayDate) {
            overlapDateLetters(buffer, y);
        }
        sendACePData(buffer, sizeof(buffer));
    }
    endACePTransaction();
    refreshACePScreen();
    return true;
}

bool ACePController::specifyImagePathOfSD(uint8_t index, char *path)
{
    path[0] = '\0';
    if (!isInitialized || digitalRead(SD_CD_PIN) == LOW) {
        return false;
    }

    bool ret = true;
    SD.begin(SD_CS_PIN);
    beginSDTransaction();
    File root = SD.open(F("/")), entry;
    bool isFirst = true;
    while (entry = root.openNextFile()) {
        if (!entry.isDirectory() && entry.size() == TARGET_FILESIZE && isTargetExtension(entry.name())) {
            if (isFirst) {
                strncpy(path, entry.name(), PATH_LEN_MAX);
            }
            if (index == 0) {
                if (!isFirst) {
                    strncpy(path, entry.name(), PATH_LEN_MAX);
                }
                ret = false;
                break;
            }
            isFirst = false;
            index--;
        }
        entry.close();
    }
    root.close();
    endSDTransaction();
    SD.end();
    return ret;
}

bool ACePController::displayACePDataFromSD(const char *path, bool isDisplayDate)
{
    if (!isInitialized || digitalRead(SD_CD_PIN) == LOW) {
        return false;
    }

    SD.begin(SD_CS_PIN);
    beginSDTransaction();
    File dataFile = SD.open(path);
    endSDTransaction();
    if (!dataFile) {
        return false;
    }

    applyACePSequence(displayStartSequence);
    uint8_t buffer[DISPLAY_WIDTH / 2];
    for (uint16_t y = 0; y < DISPLAY_HEIGHT; y++) {
        beginSDTransaction();
        dataFile.read(buffer, sizeof(buffer));
        endSDTransaction();
        if (isDisplayDate) {
            overlapDateLetters(buffer, y);
        }
        beginACePTransaction();
        sendACePData(buffer, sizeof(buffer));
        endACePTransaction();
    }
    beginSDTransaction();
    dataFile.close();
    endSDTransaction();
    SD.end();
    refreshACePScreen();
    return true;
}

bool ACePController::displayACePTestPattern(bool isDisplayDate)
{
    if (!isInitialized) {
        return false;
    }

    applyACePSequence(displayStartSequence);
    uint8_t buffer[DISPLAY_WIDTH / 2];
    beginACePTransaction();
    for (uint8_t color = 0; color < 7; color++) {
        fgColor = (ACEP_COLOR)color;
        bgColor = (fgColor == WHITE || fgColor == YELLOW) ? BLACK : WHITE;
        for (uint16_t y = 0; y < DISPLAY_HEIGHT / 7; y++) {
            memset(buffer, color | color << 4, sizeof(buffer));
            if (isDisplayDate) {
                memcpy_P(dateLetters, testDatePattern1, DATE_LETTERS_LEN);
                overlapDateLetters(buffer, y + IMG_KANJI_OFFS);
                memcpy_P(dateLetters, testDatePattern2, DATE_LETTERS_LEN);
                overlapDateLetters(buffer, y - IMG_KANJI_OFFS);
            }
            sendACePData(buffer, sizeof(buffer));
        }
    }
    endACePTransaction();
    refreshACePScreen();
    return true;
}

void ACePController::finish(void)
{
    waitShort();
    applyACePSequence(sleepSequence);
    waitShort();
    digitalWrite(ACEP_RESET_PIN, LOW);
    SPI.end();
    isInitialized = false;
}

/*---------------------------------------------------------------------------*/

void ACePController::placeDigits(uint8_t *p, uint16_t number, uint8_t digits)
{
    bool isFirst = true;
    while (digits--) {
        uint8_t rem = number % 10;
        *p-- = (!isFirst && number == 0) ? IMG_ID_BLANK : IMG_ID_NUMBER_0 + rem;
        number /= 10;
        isFirst = false;
    }
}

uint8_t ACePController::calculateYoubi(uint16_t year, uint8_t month, uint8_t day)
{
    if (month < 3) {
        year--;
        month += 12;
    }
    return (year + (year / 4) - (year / 100) + (year / 400) + (month * 13 + 8) / 5 + day) % 7;
}

bool ACePController::isTargetExtension(const char *path)
{
    for (int i = 0; i < PATH_LEN_MAX - 4; i++, path++) {
        if (memcmp_P(path, F(".ACP"), 4) == 0) {
            return true;
        }
    }
    return false;
}

void ACePController::overlapDateLetters(uint8_t *pBuffer, uint16_t y)
{
    if (y >= IMG_NUMBER_H) {
        return;
    }
    pBuffer += (DISPLAY_WIDTH - IMG_LETTER_W * DATE_LETTERS_LEN) / 4;
    for (uint8_t *pLetter = dateLetters; pLetter < dateLetters + DATE_LETTERS_LEN; pLetter++) {
        const uint8_t *pImg;
        if (*pLetter < 10) {
            pImg = &imgNumber[*pLetter][y * IMG_LETTER_W / 4];
        } else if (*pLetter < 20 && y >= IMG_KANJI_OFFS) {
            pImg = &imgKanji[*pLetter - 10][(y - IMG_KANJI_OFFS) * IMG_LETTER_W / 4];
        } else {
            pBuffer += IMG_LETTER_W / 2;
            continue;
        }
        for (uint8_t i = 0; i < IMG_LETTER_W / 4; i++, pImg++) {
            uint8_t b = pgm_read_byte(pImg);
            for (uint8_t j = 0; j < 2; j++, pBuffer++) {
                for (uint8_t k = 0; k < 2; k++) {
                    if (b & 2) {
                        *pBuffer &= 0xF << k * 4;
                        *pBuffer |= ((b & 1) ? fgColor : bgColor) << (1 - k) * 4;
                    }
                    b >>= 2;
                }
            }
        }
    }
}

void ACePController::beginACePTransaction(void)
{
    digitalWrite(ACEP_CS_PIN, LOW);
    SPI.beginTransaction(spiSettings);
}

void ACePController::endACePTransaction(void)
{
    SPI.endTransaction();
    digitalWrite(ACEP_CS_PIN, HIGH);
}

void ACePController::applyACePSequence(const uint8_t *pSequence)
{
    beginACePTransaction();
    uint8_t len;
    while (len = pgm_read_byte(pSequence++)) {
        sendACePCommand(pgm_read_byte(pSequence++));
        len--;
        sendACePPgmData(pSequence, len);
        pSequence += len;
    }
    endACePTransaction();
}

void ACePController::refreshACePScreen(void)
{
    beginACePTransaction();
    sendACePCommand(0x04);
    waitACePBusyHigh();
    sendACePCommand(0x12);
    waitACePBusyHigh();
    sendACePCommand(0x02);
    endACePTransaction();
    waitACePBusyLow();
    waitLong();
}

void ACePController::sendACePCommand(const uint8_t command)
{
    digitalWrite(ACEP_DC_PIN, LOW);
    SPI.transfer(command);
}

void ACePController::sendACePPgmData(const uint8_t *pData, uint16_t len)
{
    digitalWrite(ACEP_DC_PIN, HIGH);
    while (len-- > 0) {
        SPI.transfer(pgm_read_byte(pData++));
    }
}

void ACePController::sendACePData(const uint8_t *pData, uint16_t len)
{
    digitalWrite(ACEP_DC_PIN, HIGH);
    while (len-- > 0) {
        SPI.transfer(*pData++);
    }
}

void ACePController::sendACePData(const uint8_t data)
{
    digitalWrite(ACEP_DC_PIN, HIGH);
    SPI.transfer(data);
}

void ACePController::waitACePBusyLow(void)
{
    while (digitalRead(ACEP_BUSY_PIN) != LOW) {
        waitShort();
    }
}

void ACePController::waitACePBusyHigh(uint16_t limit)
{
    uint16_t counter = 0;
    while (digitalRead(ACEP_BUSY_PIN) != HIGH && (limit == 0 || counter++ < limit)) {
        waitShort();
    }
}

void ACePController::beginSDTransaction(void)
{
    digitalWrite(SD_CS_PIN, LOW);
}

void ACePController::endSDTransaction(void)
{
    digitalWrite(SD_CS_PIN, HIGH);
}
