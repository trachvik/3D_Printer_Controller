#include "header_files/MCP23017.h"
#include <Arduino.h>
#include <Wire.h>

MCP23017::MCP23017()
    : Adafruit_MCP23X17()
{
}

bool MCP23017::init()
{
    // Ensure the TwoWire bus is started before Adafruit creates the I2C device
    // (some Adafruit BusIO helpers expect the bus to be initialized).
    Wire.begin();
    // optional: set speed if you want (uncomment/change as needed)
    // Wire.setClock(400000);

    if (!begin_I2C()) {
        Serial.println("Chyba inicializace MCP23017!");
        return false;
    }
    return true;
}