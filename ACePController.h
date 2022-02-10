/**
 * ArduinoACePCalendar : "ACePController.h"
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

#pragma once

#include <arduino.h>
#include <SPI.h>

enum ACEP_COLOR : uint8_t
{
    BLACK = 0,
    WHITE,
    GREEN,
    BLUE,
    RED,
    YELLOW,
    ORANGE,
};

#define PATH_LEN_MAX        16
#define DATE_LETTERS_LEN    14

class ACePController
{
public:
    ACePController()
        : spiSettings(2000000, MSBFIRST, SPI_MODE0), fgColor(BLACK), bgColor(WHITE), isInitialized(false)
    {}
    ~ACePController()
    {}

    void setup(void);
    void initialize(void);
    void setDate(uint16_t year, uint8_t month, uint8_t day);
    bool clearDisplay(ACEP_COLOR color = WHITE);
    bool displayACePDataFromPGM(
            const uint8_t *pImage, uint16_t width, uint16_t height, bool isDisplayDate = false);
    bool specifyImagePathOfSD(uint8_t index, char *path);
    bool displayACePDataFromSD(const char *path, bool isDisplayDate = false);
    bool displayACePTestPattern(bool isDisplayDate = false);
    void finish(void);

private:
    void placeDigits(uint8_t *p, uint16_t number, uint8_t digits);
    uint8_t calculateYoubi(uint16_t year, uint8_t month, uint8_t day);
    bool isTargetExtension(const char *path);
    void overlapDateLetters(uint8_t *pBuffer, uint16_t y);
    void beginACePTransaction(void);
    void endACePTransaction(void);
    void applyACePSequence(const uint8_t *pSequence);
    void refreshACePScreen(void);
    void sendACePCommand(const uint8_t command);
    void sendACePPgmData(const uint8_t *pData, uint16_t len);
    void sendACePData(const uint8_t *pData, uint16_t len);
    void sendACePData(const uint8_t data);
    void waitACePBusyLow(void);
    void waitACePBusyHigh(uint16_t limit = 0);
    void beginSDTransaction(void);
    void endSDTransaction(void);

    const SPISettings spiSettings;
    uint8_t dateLetters[DATE_LETTERS_LEN];
    ACEP_COLOR fgColor, bgColor;
    bool isInitialized;
};
