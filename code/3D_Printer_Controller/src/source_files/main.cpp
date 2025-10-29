#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "header_files/hapticControl.h"
#include "header_files/printerControl.h"

hapticControl HC(MagneticSensorSPI(AS5048_SPI, 5), BLDCMotor(7), BLDCDriver6PWM(15, 16, 17, 4, 21, 22), 5);
RotaryEncoder encoder(35, 39, RotaryEncoder::LatchMode::TWO03);

byte rowPins[4] = {14, 33, 13, 26}; //connect to the row pinouts of the keypad
byte colPins[4] = {27, 25, 32, 12}; //connect to the column pinouts of the keypad

printerControl PC(rowPins,colPins);


int wifiConnect();
char IntToChar(int);
void ParseAndSave(String);

String PATH = "/websocket";
String url = "*printers_url*"; // TO DO

const char *ap_ssid = "3d-printer-controller";
const char *ap_pass = "I~7hK5IV=yo89v+h<>&x";

// WebServer on port 80
WebServer server(80);

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
  String ssid = prefs.getString("ssid");
  String pass = prefs.getString("pass");
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

  prefs.begin("config", true);
  String HOST = prefs.getString("HOST");
  int PORT = prefs.getInt("PORT");
  String gcode[16];
  for(int i = 0; i < 16; i++)
  {
    gcode[i] = prefs.getString(((String)IntToChar(i)).c_str());
  }
  prefs.end();

  PC.init(HOST, PORT, PATH, url, gcode);
  // initialize HC and pass the rotary encoder to it
  HC.encoderInit(encoder);
  HC.init();

}

long time_past = 0;

void loop()
{
  server.handleClient();
  HC.loop();
  PC.loop();
}

int wifiConnect()
{
  prefs.begin("config", true);
  String ssid = prefs.getString("ssid");
  String pass = prefs.getString("pass");
  String gcode[16];
  /*for(int i = 0; i < 16; i++)
  {
    String arg = "key" + (String)IntToChar(i);
    gcode[i] = prefs.getString(arg.c_str());
    Serial.printf("Key %s: %s\n", arg.c_str(), gcode[i].c_str());
  }*/
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



