#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Keypad.h>
#include <Preferences.h>
#include <RotaryEncoder.h>
#include <functional>

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

enum mode
{
    PC_IDLE,
    PC_SET_POSITION,
    PC_SET_TEMPERATURE,
    /**
     * Additional modes can be added here
     */
    MODE_COUNT
};

class printerControl
{
public:
    /**
     * printer control class constructor
     * @param rowPins array of 4 bytes representing the row pins of the keypad
     * @param colPins array of 4 bytes representing the column pins of the keypad
     * @param encoder_PIN0 rotary encoder pin 0
     * @param encoder_PIN1 rotary encoder pin 1
     * @param disp pointer to Display object (optional)
     */
    printerControl(byte rowPins[4], byte colPins[4], int encoder_PIN0, int encoder_PIN1, Display *disp = nullptr);
    /**
     * Initializes the websocket connection to the printer server
     */
    bool init();
    /**
     * Main loop to be called in main loop() function
     */
    void loop();
    /**
     * Handles pending changes from the rotary encoder
     */
    void knobPendingChange(int sign);
    /**
     * Sends the buffered knob changes to the printer server
     */
    void sendKnobBuffer();
    /**
     * Sets the step size based on the encoder value
     */
    void setStepSize(int encoder_val);
    bool connect_display_flag;

    int step_size; // TO DO initialize

private:
    // printer data:
    float ext_temp;
    float ext_target;
    float bed_temp;
    float bed_target;
    float position[4];
    /**
     * 
     * 
     */

    /**
     * websocket event handler
     * @param type the type of websocket event
     * @param payload the payload of the websocket message
     * @param length the length of the payload
     */
    void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    /**
     * Prepares data to send over websocket
     * @param isQuery true if preparing a query, false for subscription
     */
    String dataToSend(bool isQuery);
    //bool parse_flag;
    /**
     * Parses incoming websocket data
     */
    void parse_data();
    /**
     * Changes the current mode of the printer control based on enumerated modes
     */
    void change_mode();
    bool newDataAvailable;
    String data;
    bool connected;
    unsigned long last_knobMillis = 0;
    unsigned long last_display_update = 0;
    int pending_change = 0;
    int connection_state = 0;           // 0=Disconnected, 1=NeedsQuery, 2=QuerySent, 3=Ready
    unsigned long last_command_millis = 0;

    static printerControl* instance; // Pointer on this instance for static ISR wrapper
    static void Wrapper();        // Static ISR wrapper
    void IRAM_ATTR readEncoderISR(); // Actual ISR method
    int encoder_PIN0;
    int encoder_PIN1;
    /**
     * Displays the current status on the OLED display
     */
    void displayShow();
    mode current_mode;
    MCP23017 extender;
    RotaryEncoder encoder;
    int last_encoder_pos;
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

// Helper class for RAII-style deferred execution which is used in webSocketEvent so it is more lightweight and less error-prone
/*class Defer
{
private:
    std::function<void()> callback;
public:
    // In constructor we store the function to be called later
    Defer(std::function<void()> cb) : callback(cb) {}

    // V destruktoru funkci zavoláme.
    ~Defer()
    {
        if (callback)
        {
            callback();
        }
    }
};*/
