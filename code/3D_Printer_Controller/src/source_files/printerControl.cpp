#include "header_files/printerControl.h"

static char keys[4][4] = {
    {'0','1','2','3'},
    {'4','5','6','7'},
    {'8','9','A','B'},
    {'C','D','E','F'}
};

printerControl::printerControl(byte rowPins[4], byte colPins[4])
  : kpd(makeKeymap(keys), rowPins, colPins, 4, 4, &extender)
{
  PATH = "/websocket";
  url = "*printers_url*"; // TO DO
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

    return true;

    //TO DO return 0 if it fails
}

//websocket handler function
void printerControl::webSocketEvent(WStype_t type, uint8_t * payload, size_t length)
{
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.println("[WSc] Connected!");
      // send subscribe once on connect
      webSocket.sendTXT("{\"jsonrpc\": \"2.0\",\"method\": \"printer.objects.subscribe\",\"params\":{\"objects\": {\"heater_bed\": [\"temperature\", \"target\"], \"extruder\": [\"temperature\",\"target\"]}},\"id\": 5434}");
      //webSocket.sendTXT("{\"jsonrpc\": \"2.0\",\"method\": \"printer.gcode.script\",\"params\": {\"script\": \"M106 S255\"},\"id\": 7466}");
      break;
    case WStype_TEXT:
      //webSocket.sendTXT("{\"jsonrpc\": \"2.0\",\"method\": \"printer.objects.subscribe\",\"params\":{\"objects\": {\"heater_bed\": [\"temperature\", \"target\"], \"extruder\": [\"temperature\",\"target\"]}},\"id\": 5434}");
      //is this the right place to send text to websocket??
      JsonDocument filter;
      filter["params"] = true;
      char* data = (char*)payload;
      JsonDocument doc;

      DeserializationError error = deserializeJson(doc, data, DeserializationOption::Filter(filter));
        // Test if parsing succeeds.
      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
        // Fetch values.
        //
        // Most of the time, you can rely on the implicit casts.
        // In other case, you can do doc["time"].as<long>();
      // payload shape: params is an array where params[0] is an object
      // containing both "heater_bed" and "extruder" objects.
      // Access the nested keys directly so the original single-index
      // pattern remains similar but targets the correct fields.
      ext_temp = doc["params"][0]["extruder"]["temperature"];
      ext_target = doc["params"][0]["extruder"]["target"];
      bed_temp = doc["params"][0]["heater_bed"]["temperature"];
      bed_target = doc["params"][0]["heater_bed"]["target"];
        // Print values.
        //Serial.printf("Extruder Temperature: %.2f --> %.2f,  Bed Temperature: %.2f --> %.2f \n", ext_temp, ext_target, bed_temp, bed_target);

      //Serial.printf("[WSc] Text: %s\n", payload);
      break;
  }
 /*if(type == WStype_TEXT)
 {

 }*/

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
}

