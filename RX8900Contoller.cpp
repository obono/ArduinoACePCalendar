/**
 * ArduinoACePCalendar : "RX8900Controller.cpp"
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

#include "RX8900Controller.h"

#define I2C_ADDRESS 0x32

enum : uint8_t {
    REG_SECOND = 0,
    REG_MINUTE,
    REG_HOUR,
    REG_WEEK,
    REG_DAY,
    REG_MONTH,
    REG_YEAR,
    REG_RAM,
    REG_ALARM_MINUTE,
    REG_ALARM_HOUR,
    REG_ALARM_DAY_YOUBI,
    REG_TIMER_COUNTER_0,
    REG_TIMER_COUNTER_1,
    REG_EXTENTION,
    REG_FLAG,
    REG_CONTROL,
    REG_TEMPERATURE = 0x17,
    REG_BACKUP,
};

#define dec2bcd(value)  (((value) / 10) << 4 | ((value) % 10))
#define bcd2dec(value)  (((value) >> 4) * 10 + ((value) & 15))

/*---------------------------------------------------------------------------*/

void RX8900Controller::setup(void)
{
    delay(1000);
    Wire.begin();
    uint8_t flag = readByte(REG_FLAG);
    uint8_t data[] = { 0b00101010, 0b00000000, 0b11001001 };
    writeBytes(REG_EXTENTION, data, 3);
    writeByte(REG_BACKUP, 0b00000000);
    isInitialized = true;
    if (flag & 0b00000010) {
        restoreDefault();
    }
}

bool RX8900Controller::getDate(uint16_t &year, uint8_t &month, uint8_t &day)
{
    if (!isInitialized) {
        return false;
    }
    uint8_t data[3];
    readBytes(REG_DAY, data, 3);
    day = bcd2dec(data[0]);
    month = bcd2dec(data[1]);
    year = bcd2dec(data[2]) + 2000;
    return true;
}

bool RX8900Controller::setDate(uint16_t year, uint8_t month, uint8_t day)
{
    if (!isInitialized || year < 2000 || year > 2099 ||
            month == 0 || month > 12 || day == 0 || day > 31) {
        return false;
    }
    year -= 2000;
    uint8_t data[] = { dec2bcd(day), dec2bcd(month), dec2bcd(year) };
    writeBytes(REG_DAY, data, 3);
    return true;

}

bool RX8900Controller::getTime(uint8_t &hour, uint8_t &minute, uint8_t &second)
{
    if (!isInitialized) {
        return false;
    }
    uint8_t data[3];
    readBytes(REG_SECOND, data, 3);
    second = bcd2dec(data[0]);
    minute = bcd2dec(data[1]);
    hour = bcd2dec(data[2]);
    return true;
}

bool RX8900Controller::setTime(uint8_t hour, uint8_t minute, uint8_t second)
{
    if (!isInitialized || hour >= 24 || minute >= 60 || second >= 60) {
        return false;
    }
    uint8_t data[] = { dec2bcd(second), dec2bcd(minute), dec2bcd(hour) };
    writeBytes(REG_SECOND, data, 3);
    return true;
}

bool RX8900Controller::getAlarm(uint8_t &hour, uint8_t &minute)
{
    if (!isInitialized) {
        return false;
    }
    uint8_t data[2];
    readBytes(REG_ALARM_MINUTE, data, 2);
    minute = bcd2dec(data[0]);
    hour = bcd2dec(data[1]);
    return true;
}

bool RX8900Controller::setAlarm(uint8_t hour, uint8_t minute)
{
    if (!isInitialized || hour >= 24 || minute >= 60) {
        return false;
    }
    uint8_t data[] = { dec2bcd(minute), dec2bcd(hour), 0b10000000 };
    writeBytes(REG_ALARM_MINUTE, data, 3);
    return true;
}

bool RX8900Controller::suspendAlarm(void)
{
    if (!isInitialized) {
        return false;
    }
    writeByte(REG_FLAG, 0b00000000);
    return true;
}

uint8_t RX8900Controller::getImageIndex(void)
{
    return isInitialized ? readByte(REG_RAM) : 0;
}

bool RX8900Controller::setImageIndex(uint8_t index)
{
    if (!isInitialized) {
        return false;
    }
    writeByte(REG_RAM, index);
}

/*---------------------------------------------------------------------------*/

uint8_t RX8900Controller::readByte(uint8_t reg)
{
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(I2C_ADDRESS, 1);
    return Wire.read();
}

void RX8900Controller::readBytes(uint8_t reg, uint8_t *pData, uint8_t len)
{
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(I2C_ADDRESS, len);
    while (len--) {
        *pData++ = Wire.read();
    }
}


void RX8900Controller::writeByte(uint8_t reg, uint8_t data)
{
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
}

void RX8900Controller::writeBytes(uint8_t reg, uint8_t *pData, uint8_t len)
{
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(reg);
    while (len--) {
        Wire.write(*pData++);
    }
    Wire.endTransmission();
}

void RX8900Controller::restoreDefault(void)
{
    writeByte(REG_CONTROL, 0b11000000);
    setTime(0, 0, 0);
    setDate(2022, 1, 1);
    setImageIndex(0);
    setAlarm(3, 30);
    writeByte(REG_CONTROL, 0b11001000);
}
