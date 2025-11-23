#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Keypad.h>
#include <Preferences.h>

#include "header_files/MCP23017.h"
#include "header_files/Display.h"
#include "header_files/Display.h"

// This class inherits from Keypad and overrides pin operations to use MCP23017 if provided

class Keypad_MCP : public Keypad
{
public:
    Keypad_MCP(char *userKeymap, byte *row, byte *col, byte numRows, byte numCols, MCP23017 *ext = nullptr)
        : Keypad(userKeymap, row, col, numRows, numCols), extender(ext) {}

    virtual void pin_mode(byte pinNum, byte mode) override {
        if (extender) extender->pinMode(pinNum, mode);
        else pinMode(pinNum, mode);
    }
    virtual void pin_write(byte pinNum, boolean level) override {
        if (extender) extender->digitalWrite(pinNum, level);
        else digitalWrite(pinNum, level);
    }
    virtual int pin_read(byte pinNum) override {
        if (extender) return extender->digitalRead(pinNum);
        return digitalRead(pinNum);
    }

private:
    MCP23017 *extender;
};


class printerControl
{
public:
    /**
     * printer control class constructor
     * @param rowPins array of 4 bytes representing the row pins of the keypad
     * @param colPins array of 4 bytes representing the column pins of the keypad
     */
    printerControl(byte rowPins[4], byte colPins[4], Display *disp = nullptr);
    /**
     * Initializes the websocket connection to the printer server
     * @param HOST the IP address of the printer server
     * @param PORT the port number of the printer server
     * @param PATH the websocket path
     * @param url the origin URL for the websocket connection
     */
    bool init();
    /**
     * Main loop to be called in main loop() function
     */
    void loop();

    // printer data:
    float ext_temp;
    float ext_target;
    float bed_temp;
    float bed_target;
    /**
     * 
     * 
     */

private:
    /**
     * websocket event handler
     * @param type the type of websocket event
     * @param payload the payload of the websocket message
     * @param length the length of the payload
     */
    void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);

    MCP23017 extender;
    Keypad_MCP kpd;
    WebSocketsClient webSocket;
    Display *display;
    String gcode[16];
    //hapticControl knob; TO DO
    String PATH;
    String url;
    // websocket display update flag (set by websocket handler, handled in loop())
    //volatile int ws_event = 0; // 0 = none, 1 = connected, 2 = disconnected
    // request to send subscribe message from main loop (avoid calling sendTXT in callback)
    //volatile bool ws_subscribe_request = false;
};
