#include "ble_handler.h"

BLEHandler::BLEHandler():
      dataService(UUID16_SVC_DATA_SERVICE),
      dataCharacteristic(UUID16_CHR_DATA_CHAR),
      isBLEConnected(false),
      isTransmittingLargeData(false),
      largeDataLength(0),
      currentChunkIndex(0) {
    instance = this;
}

BLEHandler* BLEHandler::instance = nullptr;

void BLEHandler::init() {
    Bluefruit.begin();
    Bluefruit.setName("Feather52 FreeRTOS");
    Bluefruit.setTxPower(4);

    // 정적 멤버 함수를 콜백으로 등록
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

    dataService.begin();

    dataCharacteristic.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY | CHR_PROPS_WRITE);
    dataCharacteristic.setPermission(SECMODE_OPEN, SECMODE_OPEN);
    dataCharacteristic.setFixedLen(BLE_CHARACTERISTIC_LENGTH);
    dataCharacteristic.setWriteCallback(onBLEDataReceived);
    dataCharacteristic.begin();

    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addService(dataService);
    Bluefruit.Advertising.addName();

    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);
    Bluefruit.Advertising.setFastTimeout(30);
    Bluefruit.Advertising.start(0);
}

void BLEHandler::handleConnection(uint16_t conn_handle) {
    isBLEConnected = true;
    // DEBUG_PRINTF("BLE Connected\n");
    
    BLEEvent_t event;
    event.eventType = BLE_EVENT_CONNECTED;
    xQueueSend(xBLEEventQueue, &event, 0);
}

void BLEHandler::handleDisconnection(uint16_t conn_handle, uint8_t reason) {
    isBLEConnected = false;
    // DEBUG_PRINTF("BLE Disconnected\n");
    
    BLEEvent_t event;
    event.eventType = BLE_EVENT_DISCONNECTED;
    xQueueSend(xBLEEventQueue, &event, 0);
}

// 정적 콜백 함수 구현
void BLEHandler::connect_callback(uint16_t conn_handle) {
    if (instance != nullptr) {
        instance->handleConnection(conn_handle);
    }
}

void BLEHandler::disconnect_callback(uint16_t conn_handle, uint8_t reason) {
    if (instance != nullptr) {
        instance->handleDisconnection(conn_handle, reason);
    }
}

void BLEHandler::sendNotification(const uint8_t* data, size_t length) {
    if (!isBLEConnected) return;
    
    if (length > 512) {
        sendLargeData(data, length);
    } else {
        dataCharacteristic.notify(data, length);
    }
}

void BLEHandler::sendLargeData(const uint8_t* data, size_t length) {
    if (isTransmittingLargeData) return;
    if (length > MAX_BUFFER_SIZE) {
        DEBUG_PRINTF("Data too large for buffer\n");
        return;
    }

    memcpy(largeDataBuffer, data, length);
    largeDataLength = length;
    currentChunkIndex = 0;
    isTransmittingLargeData = true;
    
    uint8_t startPacket[MAX_CHUNK_SIZE] = {0};
    startPacket[0] = DATA_START_PACKET;
    startPacket[1] = (uint8_t)(length >> 24);
    startPacket[2] = (uint8_t)(length >> 16);
    startPacket[3] = (uint8_t)(length >> 8);
    startPacket[4] = (uint8_t)(length);
    
    dataCharacteristic.notify(startPacket, MAX_CHUNK_SIZE);
    delay(10);
    
    sendNextChunk();
}

void BLEHandler::sendNextChunk() {
    if (!isTransmittingLargeData) return;
    
    size_t dataOffset = currentChunkIndex * (MAX_CHUNK_SIZE - 2);
    size_t remainingBytes = 0;
    
    if (largeDataLength > dataOffset) {
        remainingBytes = largeDataLength - dataOffset;
    }
    
    if (remainingBytes == 0) {
        uint8_t endPacket[MAX_CHUNK_SIZE] = {0};
        endPacket[0] = DATA_END_PACKET;
        dataCharacteristic.notify(endPacket, MAX_CHUNK_SIZE);
        
        isTransmittingLargeData = false;
        largeDataLength = 0;
        return;
    }
    
    uint8_t chunkPacket[MAX_CHUNK_SIZE] = {0};
    chunkPacket[0] = DATA_CHUNK_PACKET;
    chunkPacket[1] = currentChunkIndex;
    
    size_t chunkSize = min(MAX_CHUNK_SIZE - 2, remainingBytes);
    
    memcpy(&chunkPacket[2], 
           &largeDataBuffer[dataOffset], 
           chunkSize);
    
    dataCharacteristic.notify(chunkPacket, MAX_CHUNK_SIZE);
    currentChunkIndex++;
    
    delay(10);
    sendNextChunk();
}

bool BLEHandler::isDeviceConnected() const {
    return isBLEConnected;
}

void BLEHandler::setConnected(bool connected) {
    isBLEConnected = connected;
}

void BLEHandler::onBLEDataReceived(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
    DEBUG_PRINTF("Data received from client\n");
    
    if (len > 0 && instance != nullptr) {
        DEBUG_PRINTF("Received %d bytes: ", len);
        for (uint16_t i = 0; i < len; i++) {
            DEBUG_PRINTF("%02X ", data[i]);
        }
        DEBUG_PRINTF("\n");
        
        instance->processReceivedData(data, len);
    }
}

void BLEHandler::processReceivedData(uint8_t* data, uint16_t len) {
    if (len == 0) return;

    uint8_t response[BLE_CHARACTERISTIC_LENGTH] = {0};
    
    switch (data[0]) {
        case 0x01: {  // LED 제어 명령
            DEBUG_PRINTF("LED control\n");
            digitalWrite(LED_PIN, data[1] ? HIGH : LOW);
            break;
        }
        case 0x02: {  // 시스템 상태 요청
            DEBUG_PRINTF("Ssytem status\n");
            response[0] = 0x02;  // 응답 타입
            response[1] = 0x00;  // 시스템 상태
            sendNotification(response, sizeof(response));
            break;
        }
        case 0x03: {  // 대용량 데이터 전송 요청
            DEBUG_PRINTF("Large data transfer\n");
            uint8_t testBuffer[1000];
            for(int i = 0; i < 1000; i++) {
                testBuffer[i] = (uint8_t)(i & 0xFF);
            }
            sendNotification(testBuffer, sizeof(testBuffer));
            break;
        }
        case 0x04: {  // 시스템 진단 데이터 요청
            DEBUG_PRINTF("Diagnostic\n");
            uint8_t diagnosticData[MAX_CHUNK_SIZE] = {0};
            diagnosticData[0] = 0x04;
            diagnosticData[1] = isTransmittingLargeData ? 1 : 0;
            diagnosticData[2] = currentChunkIndex;
            diagnosticData[3] = (largeDataLength >> 8) & 0xFF;
            diagnosticData[4] = largeDataLength & 0xFF;
            dataCharacteristic.notify(diagnosticData, sizeof(diagnosticData));
            break;
        }
        case 0x05: {  // ADC 데이터 요청
            response[0] = 0x05;
            
            int adcValue = analogRead(ADC_PIN);
            response[1] = (adcValue >> 8) & 0xFF;
            response[2] = adcValue & 0xFF;
            DEBUG_PRINTF("ADC Value: %d\n", adcValue);
            sendNotification(response, sizeof(response));
            break;
        }
        default: {
            DEBUG_PRINTF("Unknown command received\n");
            response[0] = 0xFF;
            response[1] = 0x01;
            sendNotification(response, sizeof(response));
            break;
        }
    }
}