#include "header_files/printerControl.h"

printerControl* printerControl::instance = nullptr; // Inicializace statického ukazatele na nullptr

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
  url = "*printers_url*"; // TO DO
}

void IRAM_ATTR printerControl::readEncoderISR()
{
  encoder.tick(); 
}

void printerControl::Wrapper()
{
    if (instance != nullptr) {
        instance->readEncoderISR(); // Tady voláme tu skutečnou metodu
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

    instance = this; // Nastavení ukazatele na aktuální instanci
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
      display->setCursor(0,0);
      display->printText("Cannot connect to printer!", 1, true);
      break;
    case WStype_CONNECTED:
      Serial.println("[WSc] Connected!");
      display->setCursor(0,0);
      display->printText("Connected to printer!", 1, true);
      // send subscribe once on connect
      printer_subscribe();
      break;
    case WStype_TEXT:
      // Parse incoming notify_status_update which typically looks like:
      // {"jsonrpc":"2.0","method":"notify_status_update","params":[ {"heater_bed":{...}, "extruder":{...} }, <time>]}
      data = (char*)payload;
      //Serial.println(data);
      //Serial.println((char*)payload);
      //parse_data((char*)payload);

      parse_flag = true;
      break;
  }
}

void printerControl::parse_data()
{
  if(parse_flag)
  {
    DynamicJsonDocument doc(1024); // adjust size if needed
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
    display->setCursor(0,10);
    display->clearDisplay();
    display->printf("X: %.0f\nY: %.0f\nZ: %.0f\nE: %.0f", position[0], position[1], position[2], position[3]);
    display->display();

    parse_flag = false;
    }
}

void printerControl::printer_subscribe()
{
  String data;

  //list of objects to subscribe to:
  //Object -> Object Fields | example: "heater_bed": ["temperature", "target"]
  https://moonraker.readthedocs.io/en/latest/printer_objects/ | list of the objects and their fields
  data += "\"heater_bed\": [\"temperature\", \"target\"],";
  data +=  "\"extruder\": [\"temperature\", \"target\"],";
  data += "\"toolhead\": [\"position\"]"; // target? | position x live_position

  String txt_subcribe = "{\"jsonrpc\": \"2.0\",\"method\": \"printer.objects.subscribe\",\"params\": {\"objects\": {"
    + data + "}}, \"id\": 5434}";

  String txt_query = "{\"jsonrpc\": \"2.0\",\"method\": \"printer.objects.query\",\"params\": {\"objects\": {"
    + data + "}}, \"id\": 5434}";

  webSocket.sendTXT(txt_query);
  webSocket.sendTXT(txt_subcribe); // force report at the start (responces are send only when changes in values occur)

}

void printerControl::loop()
{
    webSocket.loop();
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

    parse_data();
    change_mode();
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
    int change;
    if(currPos > last_encoder_pos)
    {
      change = (int)current_mode + 1;
      if(change > MODE_COUNT) change = 0;
    }
    else if(currPos < last_encoder_pos)
    {
      change = (int)current_mode - 1;
      if(change < 0) change = MODE_COUNT;
    }
    current_mode = static_cast<mode>(change);
    last_encoder_pos = currPos;
    //Serial.print("Current mode: ");
    //Serial.println(current_mode);
  }
  
      // delta - for fast rotating?
}

