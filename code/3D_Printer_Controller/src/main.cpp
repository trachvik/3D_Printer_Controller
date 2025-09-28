#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <KeyPad.h>

int wifiConnect(const char*, const char*);
int initWebSocket(void);
void webSocketEvent(WStype_t, uint8_t *, size_t);
char IntToChar(int);

String PATH = "/websocket";
String url = "https://*printers_host*"; // TO DO

const char *ap_ssid = "3d-printer-controller";
const char *ap_pass = "I~7hK5IV=yo89v+h<>&x";

// WebServer on port 80
WebServer server(80);
WebSocketsClient webSocket;

// saves data onto flash
Preferences prefs;

void handleRoot() {
  String page = R"rawliteral(
  <!DOCTYPE html><html>
  <head><meta charset="UTF-8"><title>ESP32 Setup</title></head>
  <body>
    <h2>Wifi settings:</h2>
    <form action="/save" method="POST">
      SSID Wi-Fi: <input type="text" name="ssid"><br>
      Password: <input type="password" name="pass"><br><br>
    <h2>Printer settings:</h2>
      IP: <input type="text" name="HOST"><br>
      PORT: <input type="text" name="PORT"><br>
      <br>
      <h2>User defined keys:<h2/>
<table border="1" cellpadding="4">
    <tr>
      <td><input type="text" name="key0"></td>
      <td><input type="text" name="key1"></td>
      <td><input type="text" name="key2"></td>
      <td><input type="text" name="key3"></td>
    </tr>
    <tr>
      <td><input type="text" name="key4"></td>
      <td><input type="text" name="key5"></td>
      <td><input type="text" name="key6"></td>
      <td><input type="text" name="key7"></td>
    </tr>
    <tr>
      <td><input type="text" name="key8"></td>
      <td><input type="text" name="keyA"></td>
      <td><input type="text" name="keyB"></td>
      <td><input type="text" name="keyC"></td>
    </tr>
    <tr>
      <td><input type="text" name="keyD"></td>
      <td><input type="text" name="keyE"></td>
      <td><input type="text" name="keyF"></td>
      <td><input type="text" name="keyG"></td>
    </tr>
  </table>
  
      <input type="submit" value="Save">
    </form>
  </body></html>
  )rawliteral";

  server.send(200, "text/html", page);
}
//keypad

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
{'0','1','2','3'},
{'4','5','6','7'},
{'8','9','A','B'},
{'C','D','E','F'}
};
byte rowPins[4] = {14, 27, 23, 26};
byte colPins[4] = {19, 32, 33, 18};

Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void handleSave()
{
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  // saves data onto flash memory
  prefs.begin("config", false);
  prefs.putString("wifi_ssid", ssid);
  prefs.putString("wifi_pass", pass);
  prefs.putString("HOST",  server.arg("HOST"));
  prefs.putInt("PORT", server.arg("PORT").toInt());
  prefs.putString("key",  server.arg("key"));

  for(int i = 0; i < 16; i++) // TO DO longer gcodes
  {
    String arg = "key" + (String)IntToChar(i);
    //Serial.print("arg: ");Serial.println(IntToChar(i));
    prefs.putString(((String)IntToChar(i)).c_str(), server.arg(arg));
    Serial.printf("%s: %s\n", arg, prefs.getString(((String)IntToChar(i)).c_str()));
  }

  server.send(200, "text/html",
    "<h3>Saved succesfully! Device will now try to connect to wifi…</h3>");
  delay(2000);

  wifiConnect(ssid.c_str(), pass.c_str());
}

void setup()
{
  Serial.begin(115200);

  prefs.begin("config", true);
  String ssid = prefs.getString("wifi_ssid");
  String pass = prefs.getString("wifi_pass");
  prefs.end();
  Serial.print("Prefs ssid: "); Serial.println(ssid);

  //if(ssid == "") // first startup
  //{
    Serial.println("first_startup");
    // Setting up access point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_pass);
    Serial.print("SoftAP běží. IP: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();
  //}
 // else
  //{
  //  wifiConnect(ssid.c_str(), pass.c_str());
  //}

  initWebSocket();

}

void loop()
{
  server.handleClient();
  webSocket.loop();


  char key = kpd.getKey();
  if (key)
  {
    prefs.begin("config", true);
    String gcode = prefs.getString(((String)key).c_str());
    prefs.end();
    String send = "{\"jsonrpc\": \"2.0\",\"method\": \"printer.gcode.script\",\"params\": {\"script\": \"" + gcode + "\"},\"id\": 7466}";
    webSocket.sendTXT(send);
    Serial.println(key);
  }
}

int initWebSocket()
{
  prefs.begin("config", true);
  String HOST = prefs.getString("HOST");
  int PORT = prefs.getInt("PORT");
  prefs.end();

  String header = "Origin: " + url;
  webSocket.setExtraHeaders(header.c_str());
  webSocket.begin(HOST, PORT, PATH);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(15000);

  return 1;
}

int wifiConnect(const char* ssid, const char* pass)
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass); 

  Serial.println("Waiting for wifi");
  int timeout_s = 30;
  while (WiFi.status() != WL_CONNECTED && timeout_s-- > 0) {
      delay(1000);
      Serial.print(".");
  }

  if(WiFi.status() != WL_CONNECTED)
  {
    Serial.println("unable to connect, check your credentials");
    return 0;
  }
  else
  {
    Serial.println("Connected to the WiFi network");
    Serial.println(WiFi.localIP());
    return 1;
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length)
{
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.println("[WSc] Connected!");
      //webSocket.sendTXT("{\"jsonrpc\": \"2.0\",\"method\": \"printer.gcode.script\",\"params\": {\"script\": \"M106 S255\"},\"id\": 7466}");
      break;
    case WStype_TEXT:
      webSocket.sendTXT("{\"jsonrpc\": \"2.0\",\"method\": \"printer.objects.subscribe\",\"params\":{\"objects\": {\"heater_bed\": [\"temperature\", \"target\"], \"extruder\": [\"temperature\",\"target\"]}},\"id\": 5434}");
      //is this the right place to send text to websocket??
      JsonDocument filter;
      filter["result"]["status"] = true;
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
        float ext_temp = doc["result"]["status"]["extruder"]["temperature"];
        float ext_target = doc["result"]["status"]["extruder"]["target"];
        float bed_temp = doc["result"]["status"]["heater_bed"]["temperature"];
        float bed_target = doc["result"]["status"]["heater_bed"]["target"];
        // Print values.
        //Serial.printf("Extruder Temperature: %.2f --> %.2f,  Bed Temperature: %.2f --> %.2f \n", ext_temp, ext_target, bed_temp, bed_target);

      //Serial.printf("[WSc] Text: %s\n", payload);
      break;
  }
 /*if(type == WStype_TEXT)
 {

 }*/

}

char IntToChar(int a)
{
  char ret;
  if(a < 10 && a >= 0) ret = a + 48;
  else if(a > 9 && a < 16) ret = a + 55;
  return ret;
}




