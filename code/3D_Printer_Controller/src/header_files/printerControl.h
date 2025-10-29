#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Keypad.h>
#include <Preferences.h>

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

private:
/**
 * websocket event handler
 * @param type the type of websocket event
 * @param payload the payload of the websocket message
 * @param length the length of the payload
 */
    void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    WebSocketsClient webSocket;
    Keypad kpd;
    String gcode[16];
    //hapticControl knob; TO DO
    String PATH;
    String url;
};
