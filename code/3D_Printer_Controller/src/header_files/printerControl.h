#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Keypad.h>
#include <Preferences.h>
#include "header_files/MCP23017.h"
//#include "header_files/hapticControl.h"
 enum mode
{
    //IDLE,
    MOVE_X,
    MOVE_Y,
    MOVE_Z,
    SET_TEMP,
    SET_TOOL,
    LIGH_SWITCH
        /**
         * 
         * 
         */
};

class printerControl
{
public:
    /**
     * printer control class constructor
     * @param rowPins array of 4 bytes representing the row pins of the keypad
     * @param colPins array of 4 bytes representing the column pins of the keypad
     * @param gcode array of 16 strings representing the gcode commands assigned to each key
     */
    printerControl(byte rowPins[4], byte colPins[4]);
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

//private:
/**
 * websocket event handler
 * @param type the type of websocket event
 * @param payload the payload of the websocket message
 * @param length the length of the payload
 */
    void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    WebSocketsClient webSocket;
    MCP23017 extender;
    // Keypad subclass that uses MCP23017 extender for pin operations when provided
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

    Keypad_MCP kpd;
    String gcode[16];
    //hapticControl *knob;
    String PATH;
    String url;

    /*typedef enum mode
    {
        MOVE_AXIS,
        SET_TEMP,
        SET_TOOL,
        LIGH_SWITCH
    };*/
    mode MODE;
    void knob_action();
    void move_axis(mode axis, int position);
};
