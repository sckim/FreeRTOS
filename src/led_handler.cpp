#include "led_handler.h"

LEDHandler::LEDHandler(int ledPin) : pin(ledPin) {
}

void LEDHandler::begin() {
    pinMode(pin, OUTPUT);
    turnOff();
}

void LEDHandler::turnOn() {
    digitalWrite(pin, HIGH);
}

void LEDHandler::turnOff() {
    digitalWrite(pin, LOW);
}

void LEDHandler::shortBlink() {
    turnOn();
    delay(SHORT_BLINK_DURATION);
    turnOff();
}

void LEDHandler::mediumBlink() {
    turnOn();
    delay(MEDIUM_BLINK_DURATION);
    turnOff();
}

void LEDHandler::longBlink() {
    turnOn();
    delay(LONG_BLINK_DURATION);
    turnOff();
}

void LEDHandler::errorBlink() {
    while (true) {
        turnOn();
        delay(500);
        turnOff();
        delay(500);
    }
}