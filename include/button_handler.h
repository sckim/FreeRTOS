#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>
#include "config.h"

typedef enum {
    BUTTON_NO_EVENT,
    BUTTON_SHORT_PRESS,    // 500ms 이하
    BUTTON_MEDIUM_PRESS,   // 500ms ~ 2000ms
    BUTTON_LONG_PRESS      // 2000ms 이상
} ButtonEvent_t;

class ButtonHandler {
private:
    static volatile bool buttonPressed;
    static volatile unsigned long pressStartTime;
    const int pin;
    
    static const unsigned long SHORT_PRESS = SHORT_PRESS_TIME;    // 500ms
    static const unsigned long MIDDLE_PRESS = LONG_PRESS_TIME;  // 2000ms
    
    static ButtonHandler* instance;
    static void buttonISR();

public:
    ButtonHandler(int buttonPin);

    void begin();
    static ButtonHandler* getInstance() { return instance; }
};

extern QueueHandle_t xButtonEventQueue;

#endif // BUTTON_HANDLER_H