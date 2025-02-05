#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// DFU 설정
#define STACK_SIZE_DFU 1024
#define PRIORITY_DFU 1

// GPIO 설정
#define BUTTON_PIN 5
#define LED_PIN LED_BUILTIN
#define SHORT_PRESS_TIME 500   // 1초
#define LONG_PRESS_TIME 2000    // 3초

// 태스크 설정
#define STACK_SIZE_GPIO 1024
#define STACK_SIZE_BLE 4096
#define PRIORITY_GPIO_INPUT 2
#define PRIORITY_GPIO_OUTPUT 2
#define PRIORITY_BLE 1

// BLE 설정
#define UUID16_SVC_DATA_SERVICE 0x180F
#define UUID16_CHR_DATA_CHAR 0x2A19
#define BLE_CHARACTERISTIC_LENGTH 20
#define QUEUE_SIZE 10

// ADC 설정
#define ADC_PIN A0
#define STACK_SIZE_ADC 1024
#define PRIORITY_ADC 3

#define DEBUG 1
#if DEBUG
    #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

#endif // CONFIG_H