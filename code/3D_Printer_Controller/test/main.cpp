#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <KeyPad.h>
#include "header_files/hapticControl.h"

hapticControl HC = hapticControl(MagneticSensorSPI(AS5048_SPI, 5), BLDCMotor(7), BLDCDriver6PWM(15, 16, 17, 4, 21, 22), 30, 5);

int wifiConnect();
int initWebSocket(void);
void webSocketEvent(WStype_t, uint8_t *, size_t);
char IntToChar(int);
void ParseAndSave(String);

String PATH = "/websocket";
String url = "https://*printers_host*"; // TO DO

const char *ap_ssid = "3d-printer-controller";
const char *ap_pass = "I~7hK5IV=yo89v+h<>&x";

// WebServer on port 80
WebServer server(80);
WebSocketsClient webSocket;

// saves data onto flash
Preferences prefs;

void handleRoot()
{
  String page = R"rawliteral(
  <!DOCTYPE html><html>
  <head><meta charset="UTF-8"><title>ESP32 Setup</title></head>
  <body>
    <h2>Wifi settings:</h2>
    <form action="/save" method="POST" enctype="text/plain">
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
      <td><input type="text" name="key9"></td>
      <td><input type="text" name="keyA"></td>
      <td><input type="text" name="keyB"></td>
    </tr>
    <tr>
      <td><input type="text" name="keyC"></td>
      <td><input type="text" name="keyD"></td>
      <td><input type="text" name="keyE"></td>
      <td><input type="text" name="keyF"></td>
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
byte rowPins[4] = {14, 33, 13, 26}; 
byte colPins[4] = {27, 25, 32, 12};

Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void handleSave()
{
  ParseAndSave(server.arg("plain"));

  server.send(200, "text/html",
    "<h3>Saved succesfully! Device will now try to connect to wifi…</h3>");
  delay(2000);

  wifiConnect();
}

void setup()
{
  Serial.begin(115200);

  prefs.begin("config", true);
  String ssid = prefs.getString("wifi_ssid");
  String pass = prefs.getString("wifi_pass");
  prefs.end();
  Serial.print("Prefs ssid: "); Serial.println(ssid);

  if(ssid == "") // first startup
  {
    Serial.println("first_startup");
    // Setting up access point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_pass);
    Serial.print("Access Point IP: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();
  }
  else
  {
    wifiConnect();
  }

  initWebSocket();
  HC.init();

}

long time_past = 0;

void loop()
{
  server.handleClient();
  webSocket.loop();
  HC.loop();


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
  if(millis() - time_past > 1000)
  { 
    Serial.println("Step count: ");
    Serial.println(HC.step_count);
    time_past = millis();
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

int wifiConnect()
{
  prefs.begin("config", true);
  String ssid = prefs.getString("ssid");
  String pass = prefs.getString("pass");
  prefs.end();

  //Serial.print("ssid: "); Serial.println(ssid);
  //Serial.print("pass: "); Serial.println(pass);

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

void ParseAndSave(String stream)
{
  int arg_count = 20;
  int start[arg_count];
  int end[arg_count];
  int pos = 0;

  for(int i = 0; i < arg_count; i++)
  {
    start[i] = stream.indexOf('=', pos) + 1;
    end[i] = stream.indexOf('\n', pos);
    pos = end[i] + 1;
    //Serial.print("pos: "); Serial.println(pos);
  }

  String ssid = stream.substring(start[0], end[0]);
  String pass = stream.substring(start[1], end[1]);
  String HOST = stream.substring(start[2], end[2]);
  String PORT = stream.substring(start[3], end[4]);
  ssid.trim();
  pass.trim();
  HOST.trim();
  PORT.trim(); // .trim() deletes all invisible characters

  prefs.begin("config", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.putString("HOST", HOST);
  prefs.putInt("PORT", stream.substring(start[3], end[3]).toInt());

  for(int i = 4; i < 20; i++) // save keys values
  {
    String arg = "key" + (String)IntToChar(i - 4);

    String gcode = stream.substring(start[i], end[i]);
    gcode.trim();
    prefs.putString(((String)IntToChar(i - 4)).c_str(), gcode);
    //Serial.printf("%s: %s\n",arg, gcode);

  }
  prefs.end();
}



