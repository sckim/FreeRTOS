#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

#include <bluefruit.h>
#include "config.h"

#define MAX_CHUNK_SIZE 20  // BLE_CHARACTERISTIC_LENGTH와 동일
#define DATA_START_PACKET 0x10
#define DATA_CHUNK_PACKET 0x11
#define DATA_END_PACKET 0x12
#define MAX_BUFFER_SIZE 2048  // 최대 버퍼 크기 정의

// ble_handler.h에 큐 추가
extern QueueHandle_t xBLEEventQueue;

// BLE 이벤트 타입 정의
typedef enum {
    BLE_EVENT_CONNECTED,
    BLE_EVENT_DISCONNECTED,
    BLE_EVENT_DATA_SEND,
    BLE_EVENT_ADC_DONE
} BLEEventType_t;

// BLE 이벤트 구조체
typedef struct {
    BLEEventType_t eventType;
    uint8_t data[20];  // BLE_CHARACTERISTIC_LENGTH
    uint16_t length;
} BLEEvent_t;

class BLEHandler {   
private:
    BLEService dataService;
    BLECharacteristic dataCharacteristic;
    bool isBLEConnected;
    static BLEHandler* instance;

    static void onBLEDataReceived(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len);
    void processReceivedData(uint8_t* data, uint16_t len);

    // 대용량 데이터 전송을 위한 멤버 추가
    uint8_t largeDataBuffer[MAX_BUFFER_SIZE];
    size_t largeDataLength;
    size_t currentChunkIndex;
    bool isTransmittingLargeData;
    
    void sendLargeData(const uint8_t* data, size_t length);
    void sendNextChunk();

    // 콜백 함수들을 static으로 선언
    static void connect_callback(uint16_t conn_handle);
    static void disconnect_callback(uint16_t conn_handle, uint8_t reason);

public:
    BLEHandler();
    void init();
    void handleConnection(uint16_t conn_handle);
    void handleDisconnection(uint16_t conn_handle, uint8_t reason);
    void sendNotification(const uint8_t* data, size_t length);
    bool isDeviceConnected() const;
    void setConnected(bool connected);
};

#endif // BLE_HANDLER_H