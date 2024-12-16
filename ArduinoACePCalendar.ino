/**
 * ArduinoACePCalendar : "ArduinoACePCalendar.ino"
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
#include <avr/sleep.h>
#include "RX8900Controller.h"
#include "ACePController.h"

#define ALARM_WAKE_PIN      2
#define SHELL_ENABLE_PIN    3

#define SERIAL_BAUD_RATE    9600

void printShellMessage(void);
void printShellPrompt(void);
void handleSerialInput(char data);

RX8900Controller    rtc;
ACePController      acep;
bool                isShellEnabled;

/*---------------------------------------------------------------------------*/

void setup(void)
{
    pinMode(ALARM_WAKE_PIN, INPUT_PULLUP);
    pinMode(SHELL_ENABLE_PIN, INPUT_PULLUP);
    isShellEnabled = digitalRead(SHELL_ENABLE_PIN) == HIGH;
    bool isAlarmWake = digitalRead(ALARM_WAKE_PIN) == LOW;
    if (isShellEnabled) {
        Serial.begin(SERIAL_BAUD_RATE);
        printShellMessage();
    }
    rtc.setup();
    acep.setup();
    acep.initialize();
    if (isShellEnabled) {
        printShellPrompt();
    }
    if (isAlarmWake) {
        doToday();
    }
}

void loop(void)
{
    if (digitalRead(ALARM_WAKE_PIN) == LOW) {
        doToday();
    }
    if (isShellEnabled) {
        int serialData;
        while ((serialData = Serial.read()) != -1) {
            handleSerialInput(serialData);
        }
        delay(100);
    } else {
        acep.finish();
        sleep();
        acep.initialize();
    }
}

static void doToday(void)
{
    rtc.suspendAlarm();
    uint16_t year;
    uint8_t month, day;
    if (rtc.getDate(year, month, day)) {
        acep.setDate(year, month, day);
    }
    acep.clearDisplay();
    uint8_t index = rtc.getImageIndex();
    char path[PATH_LEN_MAX];
    if (acep.specifyImagePathOfSD(index, path)) {
        index = 0;
    }
    rtc.setImageIndex(index + 1);
    acep.displayACePDataFromSD(path, true);
}

static void wakeUp(void)
{
    // do nothing
}

static void sleep(void)
{
    uint8_t adcsraBak = ADCSRA;
    ADCSRA = 0;
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    noInterrupts();
    sleep_bod_disable();
    sleep_enable();
    interrupts();
    attachInterrupt(0, wakeUp, FALLING);
    sleep_cpu();
    // go to sleep...
    sleep_disable();
    detachInterrupt(0);
    ADCSRA = adcsraBak;
}
