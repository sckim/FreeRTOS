#ifndef LED_HANDLER_H
#define LED_HANDLER_H

#include <Arduino.h>

class LEDHandler {
private:
    const int pin;
    static const unsigned long SHORT_BLINK_DURATION = 200;   // 200ms
    static const unsigned long MEDIUM_BLINK_DURATION = 1000; // 1000ms
    static const unsigned long LONG_BLINK_DURATION = 2000;   // 2000ms

public:
    LEDHandler(int ledPin);
    
    void begin();
    void turnOn();
    void turnOff();
    void shortBlink();
    void mediumBlink();
    void longBlink();
    void errorBlink();  // 에러 상태를 나타내는 무한 점멸
};

#endif // LED_HANDLER_H