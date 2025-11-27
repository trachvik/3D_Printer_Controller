#include "header_files/printerControl.h"

printerControl* printerControl::instance = nullptr; // Inicialization of the static instance pointer

static char keys[4][4] = {
    {'0','1','2','3'},
    {'4','5','6','7'},
    {'8','9','A','B'},
    {'C','D','E','F'}
};

printerControl::printerControl(byte rowPins[4], byte colPins[4], int encoder_PIN0, int encoder_PIN1, Display *disp)
  : kpd(makeKeymap(keys), rowPins, colPins, 4, 4, &extender), display(disp), encoder_PIN0(encoder_PIN0), encoder_PIN1(encoder_PIN1),
    encoder(encoder_PIN0, encoder_PIN1, RotaryEncoder::LatchMode::TWO03)
{
  current_mode = PC_IDLE;
  PATH = "/websocket";
  connected = false;
}

void IRAM_ATTR printerControl::readEncoderISR()
{
  encoder.tick(); 
}

void printerControl::Wrapper()
{
    if (instance != nullptr) {
        instance->readEncoderISR(); // Call the actual ISR method
    }
}

bool printerControl::init()
{
    extender.init();
    Preferences prefs;
    prefs.begin("config", true);
    String HOST = prefs.getString("HOST");
    int PORT = prefs.getInt("PORT");
    //save gcodes to local array:
    for(int i = 0; i < 16; i++)
    {
      //Serial.print("Key: ");Serial.println(prefs.getString(((String)i).c_str()));
      this->gcode[i] = prefs.getString(((String)i).c_str());
      //Serial.printf("gcode[%i]: %s\n", i, gcode[i]);
    }
    prefs.end();

    //webSocketInit:
    url = "http://" + HOST;
    String header = "Origin: " + url;
    webSocket.setExtraHeaders(header.c_str());
    webSocket.begin(HOST, PORT, PATH);

    /**this call passes the address of webSocketEvent into the WebsocketClient object
     * library stores this address in a function-pointer variable
     * when an event occurs, library calls the function through the pointer inside its own routine webSocket.loop()
     */
    //viz
    https://gitlab.fel.cvut.cz/trachvik/bachelor-project/-/blob/main/code/3D_Printer_Controller/lib/arduinoWebSockets-master/src/WebSocketsClient.cpp?ref_type=heads#L341

    // Using a lambda because member functions cannot be used as function pointers directly
    webSocket.onEvent([this](WStype_t type, uint8_t * payload, size_t length) {
        this->webSocketEvent(type, payload, length);
    });
    webSocket.setReconnectInterval(15000);

    instance = this; // Set the static instance pointer to this instance
    // Attach interrupts for the encoder pins
    attachInterrupt(digitalPinToInterrupt(encoder_PIN0), Wrapper, CHANGE);
    attachInterrupt(digitalPinToInterrupt(encoder_PIN1), Wrapper, CHANGE);

    return true;

    //TO DO return 0 if it fails
}

//websocket handler function
void printerControl::webSocketEvent(WStype_t type, uint8_t * payload, size_t length)
{
  switch(type)
  {
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!");
      //display->setCursor(0,0);
      //display->printText("Cannot connect to printer!", 1, true);
      connected = false;
      break;
    case WStype_CONNECTED:
      Serial.println("[WSc] Connected!");
      //display->setCursor(0,0);
      //display->printText("Connected to printer!", 1, true);
      // send subscribe once on connect
      //printer_subscribe();
      connected = true;
      connect_display_flag = true;
      //Defer Display([this](){this->displayShow(); });
      // Set state to indicate we need to send Query
        connection_state = 1;
        last_command_millis = millis();
      break;
    case WStype_TEXT:
      if(connected)
      {
        data = (char*)payload;
        //Defer Display([this](){this->displayShow(); });
        //Defer Parse([this](){this->parse_data(); });
        newDataAvailable = true;
      }
      break;
  }
}

void printerControl::parse_data()
{
  //if(parse_flag)
  //{
    DynamicJsonDocument doc(4096); // adjust size if needed
    DeserializationError error = deserializeJson(doc, data.c_str());
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      Serial.printf("payload: %s\n", data);
      return;
    }
    // Extract relevant data to root object
    JsonObject root;

    if (doc.containsKey("method")) //subscribe responce
    {
      if(doc["method"] == "notify_status_update")
      {
        root = doc["params"][0].as<JsonObject>();
        //Serial.println("subscribe_flag");
      }
    }
    if (doc.containsKey("result") && doc["result"].is<JsonObject>()) //query responce
    {
      root = doc["result"]["status"].as<JsonObject>();
      //Serial.println("query_flag");
    }

    //parse values from root object
    // extruder
    if (root.containsKey("extruder") && root["extruder"].is<JsonObject>())
    {
      JsonObject ex = root["extruder"].as<JsonObject>();
      if (ex.containsKey("temperature")) ext_temp = ex["temperature"].as<float>();
      if (ex.containsKey("target")) ext_target = ex["target"].as<float>();
    }

    // heater_bed
    if (root.containsKey("heater_bed") && root["heater_bed"].is<JsonObject>())
    {
      JsonObject hb = root["heater_bed"].as<JsonObject>();
      if (hb.containsKey("temperature")) bed_temp = hb["temperature"].as<float>();
      if (hb.containsKey("target")) bed_target = hb["target"].as<float>();
    }

    // position
    if (root.containsKey("toolhead") && root["toolhead"].is<JsonObject>())
    {
      JsonObject hb = root["toolhead"].as<JsonObject>();
      if (hb.containsKey("position"))
      {
        for(int i = 0; i < 4; i++)
          position[i] = hb["position"][i].as<float>();
      }
    }
    // Print values.
      //Serial.printf("Extruder Temperature: %.2f --> %.2f,  Bed Temperature: %.2f --> %.2f \n", ext_temp, ext_target, bed_temp, bed_target);
      //Serial.printf("Position: [%.2f, %.2f, %.2f, %.2f] \n", position[0], position[1], position[2], position[3]);

    //Serial.printf("[WSc] Text: %s\n", payload);
    //display->setCursor(0,10);
    //display->clearDisplay();
    //display->printf("X: %.0f\nY: %.0f\nZ: %.0f\nE: %.0f", position[0], position[1], position[2], position[3]);
    //display->display();

    //parse_flag = false;
    //}
}

String printerControl::dataToSend(bool isQuery)
{
  String data;
  String dataToSend;

  //list of objects to subscribe to:
  //Object -> Object Fields | example: "heater_bed": ["temperature", "target"]
  https://moonraker.readthedocs.io/en/latest/printer_objects/ | list of the objects and their fields
  data += "\"heater_bed\": [\"temperature\", \"target\"],";
  data +=  "\"extruder\": [\"temperature\", \"target\"],";
  data += "\"toolhead\": [\"position\"]"; // target? | position x live_position

  if(isQuery)
  {
    dataToSend = "{\"jsonrpc\": \"2.0\",\"method\": \"printer.objects.query\",\"params\": {\"objects\": {"
      + data + "}}, \"id\": 5434}";
  }
  else
  {
    dataToSend = "{\"jsonrpc\": \"2.0\",\"method\": \"printer.objects.subscribe\",\"params\": {\"objects\": {"
      + data + "}}, \"id\": 5434}";
  }
  return dataToSend;
}

void printerControl::loop()
{
    webSocket.loop();

    // Handle connection states
    if (connected)
    {
        // State 1: Send QUERY
        // Wait at least 50ms since connection established
        if (connection_state == 1 && (millis() - last_command_millis > 50))
        {
            String txt = dataToSend(true);
            webSocket.sendTXT(txt);
            Serial.println("[WSc] Query sent. Waiting for Subscribe...");
            
            connection_state = 2; // Move to next state
            last_command_millis = millis();
        }

        // State 2: Send SUBSCRIBE
        // Wait at least 100ms since last command
        if (connection_state == 2 && (millis() - last_command_millis > 100))
        {
            String txt = dataToSend(false);
            webSocket.sendTXT(txt);
            Serial.println("[WSc] Subscribe sent. Printer Ready.");

            connection_state = 3; // Move to Ready state
            last_command_millis = millis();
        }
    }
    char key = kpd.getKey();
    if (key)
    {
      int index;
      if(key >= '0' && key <= '9')
          index = key - '0';
      else if(key >= 'A' && key <= 'F')
          index = key - 'A' + 10;
      String gcode_send = gcode[index];
      // Always the same id?
      String send = "{\"jsonrpc\": \"2.0\",\"method\": \"printer.gcode.script\",\"params\": {\"script\": \"" + gcode_send + "\"},\"id\": 7466}";
      webSocket.sendTXT(send);
      Serial.println(key);
    }

    if (newDataAvailable)
    {
        parse_data(); 
        newDataAvailable = false;
    }
    //periodically refresh display
    if (millis() - last_display_update > 100)
    {
        displayShow();
        last_display_update = millis();
    }
    change_mode();
    sendKnobBuffer();

}

long lastMillis = 0;

void printerControl::change_mode()
{
  int currPos = encoder.getPosition() / 2;
  /*if(millis() - lastMillis > 500)
  {
    Serial.print("Encoder position PC: ");
    Serial.println(currPos);
    lastMillis = millis();
  }*/
  if(currPos != last_encoder_pos)
  {
    int change = (currPos > last_encoder_pos) ? (int)current_mode + 1 : change = (int)current_mode - 1;
    if(change > MODE_COUNT - 1) change = 0;
    else if(change < 0) change = MODE_COUNT - 1;

    current_mode = static_cast<mode>(change);
    last_encoder_pos = currPos;
    //displayShow();
    Serial.print("Current mode: ");
    Serial.println(current_mode);
  }
  
      // delta - for fast rotating?
}

void printerControl::setStepSize(int encoder_val) // encodr val: [1,5]
{
  switch(encoder_val)
  {
      case 1:
          step_size = 1;
          break;
      case 2:
          step_size = 5;
          break;
      case 3:
          step_size = 10;
          break;
      case 4:
          step_size = 25;
          break;
      case 5:
          step_size = 50;
          break;
      default:
          break;
  }
  //displayShow();
}

void printerControl::knobPendingChange(int sign) // TO DO merge knobPendingChange and sendKnobBuffer into one function after including HapticControl?
{
  if (-sign > 0) // Clockwise turn for increasing value
  {
      pending_change += 1; // Zvýšit buffer
  } else
  {
      pending_change -= 1; // Snížit buffer
  }
  last_knobMillis = millis();
}

void printerControl::sendKnobBuffer()
{
  if(millis() - last_knobMillis > 100 && pending_change != 0) // This prevents overflowing the printer with commands when turning the knob too fast
  {
    String data;
    switch(current_mode)
    {
      // TO DO make position buffer for faster knob moves
        case PC_SET_POSITION: // relative move
            // only X axis for now | _CLIENT_LINEAR_MOVE X=-10 F=6000 | F = velocity | TO DO Generic gcode 
            data = "{\"jsonrpc\": \"2.0\",\"method\": \"printer.gcode.script\",\"params\": {\"script\": \"_CLIENT_LINEAR_MOVE X="
            + String(pending_change*step_size)+ " F=6000\"},\"id\": 7466}";
            webSocket.sendTXT(data);
            //Serial.println(pending_change);
            break;
        case PC_SET_TEMPERATURE:
        default:
            break;
    }
    pending_change = 0;
  }
}

void printerControl::displayShow()
{
  display->clearDisplay();
  display->setCursor(0,0);
  if(connected)
  {
    if(connect_display_flag)
    {
      display->print("Connected to printer!");
      display->display();
      //delay(2000);  /// <--- is this delay ok here?
      display->clearDisplay();
      connect_display_flag = false;
    }
    if(current_mode != PC_IDLE)
    {
        display->setCursor(0,15);
        display->print("Step size: "); display->print(step_size);
        //Serial.print("Step size: "); Serial.println(step_size);
    }
    display->setCursor(0,0);
    switch(current_mode)
    {
        case PC_IDLE:
            display->print("IDLE");
            break;
        case PC_SET_POSITION: // for real time position update I have to split position-from websocket and position_print-depending on the haptic knob
            display->print("SET POSITION");
            display->setCursor(0,30);
            display->print("X POSITION: "); display->print(position[0]);
            break;
        case PC_SET_TEMPERATURE:
            display->print("SET TEMPERATURE");
            display->setCursor(0,30);
            break;
        default:
            display->print("UNKNOWN");
            break;
    }
    display->display();
  }
}

