#ifndef ADC_HANDLER_H
#define ADC_HANDLER_H

#include <Arduino.h>

class ADCHandler {
private:
    const int adcPin;
    int lastValue;
    
public:
    ADCHandler(int pin) : adcPin(pin), lastValue(0) {}
    
    void init() {
        analogReadResolution(12);  // 12비트 해상도 설정
        pinMode(adcPin, INPUT);
    }
    
    int readADC() {
        lastValue = analogRead(adcPin);
        return lastValue;
    }
    
    int getLastValue() const {
        return lastValue;
    }
};

#endif // ADC_HANDLER_H