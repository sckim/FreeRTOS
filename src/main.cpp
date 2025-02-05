#include <FreeRTOS.h>
#include <semphr.h>
#include <queue.h>

#include "config.h"
#include "button_handler.h"
#include "led_handler.h"
#include "adc_handler.h"
#include "ble_handler.h"
#include "error_handler.h"

// BLEDfu bledfu;

// 전역 객체 선언
ButtonHandler buttonHandler(BUTTON_PIN);
LEDHandler ledHandler(LED_PIN);
BLEHandler bleHandler;
ADCHandler adcHandler(ADC_PIN);

// FreeRTOS 객체
SemaphoreHandle_t xGPIOSemaphore;
QueueHandle_t xButtonEventQueue = NULL;
QueueHandle_t xBLEEventQueue = NULL;

uint8_t adcDataBuffer[MAX_BUFFER_SIZE];
size_t adcDataLength; // 현재 버퍼에 저장된 데이터 길이

void TaskADC(void *pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    static size_t dataIndex = 0; // 버퍼의 현재 인덱스

    DEBUG_PRINTF("ADC Task Started\n");

    for (;;)
    {
        if (xGPIOSemaphore != NULL && xSemaphoreTake(xGPIOSemaphore, pdMS_TO_TICKS(5)) == pdTRUE)
        {
            // int adcValue = adcHandler.readADC();
            int adcValue = dataIndex;  // for testing

            // 버퍼에 ADC 값 저장
            adcDataBuffer[dataIndex * 2] = (adcValue >> 8) & 0xFF; // 상위 바이트
            adcDataBuffer[dataIndex * 2 + 1] = adcValue & 0xFF;    // 하위 바이트
            dataIndex++;

            // 512개의 ADC 값이 모이면 BLE 전송 이벤트 생성 (1024 bytes)
            if (dataIndex >= 512)
            {
                BLEEvent_t event;
                event.eventType = BLE_EVENT_DATA_SEND;
                event.data[0] = 0x05;          // ADC 데이터 타입
                
                adcDataLength = dataIndex * 2; // 전체 바이트 수 설정
                event.length = adcDataLength;
                // BLE 이벤트 큐에 전송 요청
                if (bleHandler.isDeviceConnected())
                {
                    xQueueSend(xBLEEventQueue, &event, 0);
                }

                // 버퍼 인덱스 초기화
                dataIndex = 0;
                // DEBUG_PRINTF("512 ADC samples collected and queued for BLE transmission\n");
            }

            xSemaphoreGive(xGPIOSemaphore);
        }
        // DEBUG_PRINTF("ADC Done\n");
        // 100ms 주기로 실행
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}

// GPIO 출력 태스크
void TaskOutput(void *pvParameters)
{
    ButtonEvent_t receivedEvent;

    for(;;)
    {
        if (xQueueReceive(xButtonEventQueue, &receivedEvent, portMAX_DELAY) == pdTRUE)
        {
            switch(receivedEvent)
            {
                case BUTTON_SHORT_PRESS:
                    DEBUG_PRINTF("Short press\n");
                    ledHandler.shortBlink();
                    break;
                case BUTTON_MEDIUM_PRESS:
                    DEBUG_PRINTF("Medium press\n");
                    ledHandler.mediumBlink();
                    break;
                case BUTTON_LONG_PRESS:
                    DEBUG_PRINTF("Long press\n");   
                    ledHandler.longBlink();
                    break;
                case BUTTON_NO_EVENT:
                    DEBUG_PRINTF("No event\n");
                    break;
            }
        }
    }
}

// BLE 태스크
void TaskBLE(void *pvParameters)
{
    DEBUG_PRINTF("BLE Task Started\n");
    BLEEvent_t receivedEvent;

    for (;;)
    {
        if (xQueueReceive(xBLEEventQueue, &receivedEvent, portMAX_DELAY) == pdTRUE)
        {
            if (bleHandler.isDeviceConnected())
            {
                switch (receivedEvent.eventType)
                {
                case BLE_EVENT_CONNECTED:
                    DEBUG_PRINTF("BLE Connected\n");
                    break;
                case BLE_EVENT_DISCONNECTED:
                    DEBUG_PRINTF("BLE Disconnected\n");
                    break;
                case BLE_EVENT_ADC_DONE:
                    DEBUG_PRINTF("ADC done\n");
                    break;  
                case BLE_EVENT_DATA_SEND:
                    if (receivedEvent.data[0] == 0x05)
                    { // ADC 데이터인 경우
                        // adcDataBuffer의 데이터를 BLE로 전송
                        // DEBUG_PRINTF("Sending ADC data\n");
                        // for(int i=0; i<512; i++)
                        // {
                        //     DEBUG_PRINTF("%0X, ", adcDataBuffer[i]);
                        // }
                        // DEBUG_PRINTF("\n");
                        // bleHandler.sendNotification(adcDataBuffer, adcDataLength);
                    }
                    else
                    {
                        // 일반적인 이벤트 데이터 전송
                        bleHandler.sendNotification(receivedEvent.data, receivedEvent.length);
                    }
                    break;
                }
            }
        }
    }
}

// // DFU 모드로 강제 진입
// void enterDFUMode()
// {
//     // nRF52 특정 레지스터 설정
//     NRF_POWER->GPREGRET = 0x68;
//     NVIC_SystemReset();
// }

void setup()
{
    Serial.begin(115200);
// 장치에 따라서 아래 코드가 없으면 Halt 상태에 빠질 수 있음
#if defined(ARDUINO_NRF52840_ADAFRUIT) || \
    defined(ARDUINO_NRF52_ADAFRUIT) ||    \
    defined(ARDUINO_AVR_LEONARDO) ||      \
    defined(ARDUINO_SAMD_ZERO)
    while (!Serial)
        delay(10);
#endif
    DEBUG_PRINTF("Starting BLE Sensor Node\n");

    // bledfu.begin();

    // FreeRTOS 객체 생성
    xButtonEventQueue = xQueueCreate(QUEUE_SIZE, sizeof(ButtonEvent_t));
    if (xButtonEventQueue == NULL)
    {
        errorHandler("Failed to create queue");
    }

    // BLE 이벤트 큐 생성
    xBLEEventQueue = xQueueCreate(10, sizeof(BLEEvent_t));
    if (xBLEEventQueue == NULL)
    {
        errorHandler("Failed to create BLE event queue");
    }

    xGPIOSemaphore = xSemaphoreCreateMutex();
    if (xGPIOSemaphore == NULL)
    {
        errorHandler("Failed to create semaphore");
    }

    buttonHandler.begin();
    ledHandler.turnOff();
    adcHandler.init();
    bleHandler.init();

    // 태스크 생성
    // xTaskCreate(TaskButton, "GeneralInput", STACK_SIZE_GPIO, NULL, PRIORITY_GPIO_INPUT, NULL);
    xTaskCreate(TaskOutput, "GeneralOutput", STACK_SIZE_GPIO, NULL, PRIORITY_GPIO_OUTPUT, NULL);
    xTaskCreate(TaskADC, "ADC", STACK_SIZE_ADC, NULL, PRIORITY_ADC, NULL);
    xTaskCreate(TaskBLE, "BLE", STACK_SIZE_BLE, NULL, PRIORITY_BLE, NULL);
}

void loop()
{
    // FreeRTOS 사용시 loop는 비워둠
}

// To do
// BLE 연결
// BLE 수신
// BLE 송신
// ADC를 Queue로 전송할지 고민.
// Central mode와 Peripheral mode를 구분해서 사용할지 고민.