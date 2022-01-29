/**
 * ArduinoACePCalendar : "RX8900Controller.h"
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
#include <Wire.h>

class RX8900Controller
{
public:
    RX8900Controller() : isInitialized(false)
    {}
    ~RX8900Controller()
    {}

    void setup(void);
    bool getDate(uint16_t &year, uint8_t &month, uint8_t &day);
    bool setDate(uint16_t year, uint8_t month, uint8_t day);
    bool getTime(uint8_t &hour, uint8_t &minute, uint8_t &second);
    bool setTime(uint8_t hour, uint8_t minute, uint8_t second);
    bool getAlarm(uint8_t &hour, uint8_t &minute);
    bool setAlarm(uint8_t hour, uint8_t minute);
    bool suspendAlarm(void);
    uint8_t getImageIndex(void);
    bool setImageIndex(uint8_t index);

private:
    uint8_t readByte(uint8_t reg);
    void readBytes(uint8_t reg, uint8_t *pData, uint8_t len);
    void writeByte(uint8_t reg, uint8_t data);
    void writeBytes(uint8_t reg, uint8_t *pData, uint8_t len);
    void restoreDefault(void);
    bool isInitialized;
};
