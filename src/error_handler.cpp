#include "config.h"
#include "error_handler.h"

void errorHandler(const char* message) {
    DEBUG_PRINTF("%s\n", message);
    while (true) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
    }
}