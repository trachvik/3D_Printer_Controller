#include "header_files/MCP23017.h"

MCP23017::MCP23017()
    : Adafruit_MCP23X17()
{
}

bool MCP23017::init()
{
    if (!begin_I2C()) {
        Serial.println("Error inicializing MCP23017!");
        return false;
    }
    return true;
}