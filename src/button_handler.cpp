#include "button_handler.h"

volatile bool ButtonHandler::buttonPressed = false;
volatile unsigned long ButtonHandler::pressStartTime = 0;
ButtonHandler* ButtonHandler::instance = nullptr;

ButtonHandler::ButtonHandler(int buttonPin) : pin(buttonPin) {
    instance = this;
}

void ButtonHandler::begin() {
    pinMode(pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin), buttonISR, CHANGE);
}

void ButtonHandler::buttonISR() {
    bool currentState = digitalRead(instance->pin);
    if (currentState == LOW) {  // button pressed
        pressStartTime = millis();
        buttonPressed = true;
    } else {  // button released
        if (buttonPressed) {
            unsigned long pressDuration = millis() - pressStartTime;
            ButtonEvent_t event;
            
            if (pressDuration < SHORT_PRESS) {
                event = BUTTON_SHORT_PRESS;
            } else if (pressDuration < MIDDLE_PRESS) {
                event = BUTTON_MEDIUM_PRESS;
            } else {
                event = BUTTON_LONG_PRESS;
            }
            
            xQueueSendFromISR(xButtonEventQueue, &event, NULL);
            buttonPressed = false;
        }
    }
}