#include "header_files/wificonnect.h"
#include "SPIFFS.h"

wifiConnect::wifiConnect(Display *disp)
  : server(80), display(disp)
{
    config_saved = false;
}

void wifiConnect::init()
{
    config_saved = false; // for reseting config via button
    if(!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    prefs.begin("config", false);
    String ssid = prefs.getString("ssid");
    String pass = prefs.getString("pass");
    prefs.end();
    //Serial.print("Prefs ssid: "); Serial.println(ssid);

    if(ssid == "") // first startup
    {
        Serial.println("first_startup");
        display->setCursor(0,0);
        display->printText("Please fill the wifi credentials\nand printer config.", 1, 1);
        // Setting up access point
        WiFi.mode(WIFI_AP);
        WiFi.softAP(ap_ssid, ap_pass);
        Serial.print("Access Point IP: ");
        Serial.println(WiFi.softAPIP());

        server.on("/", [this](){
          this->handleRoot();
        });
        server.on("/save", HTTP_POST, [this](){
          this->handleSave();
        });
        server.begin();
    }
    else
    {
        connect(ssid, pass);
    }
    //TO DO failure handling
}

void wifiConnect::connect(String ssid, String pass)
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass); 

    Serial.println("Waiting for wifi");
    display->setCursor(0,0);
    display->printText("Connecting to WiFi\n", 1, 1);
    int timeout_s = 30;
    while (WiFi.status() != WL_CONNECTED && timeout_s-- > 0)
    {
        delay(1000);
        Serial.print(".");
        display->printText(".", 1, false);
    }
    if(timeout_s <= 0)
    {
        Serial.println("Failed to connect to WiFi");
        display->setCursor(0,0);
        display->printText("Failed to connect to WiFi!\n", 1, 1);
        return;
    }
    display->setCursor(0,0);
    display->printText("Connected to:\n" + ssid, 1, 1);
    WiFi.setSleep(false); // disable wifi sleep to improve stability | may increase power consumption TO DO test if needed
    config_saved = true;
    //TO DO failure handling
}

void wifiConnect::handleRoot()
{
  File file = SPIFFS.open("/config.html", "r");
  if (!file) {
    server.send(500, "text/plain", "Error loading HTML file");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void wifiConnect::handleSave()
{
    ParseAndSave(server.arg("plain"));
    prefs.begin("config", true);
    String ssid = prefs.getString("ssid");
    String pass = prefs.getString("pass");
    prefs.end();

    server.send(200, "text/html",
    "<h3>Saved succesfully! Device will now try to connect to wifi…</h3>");
    delay(2000);

    connect(ssid, pass);
}

void wifiConnect::ParseAndSave(String stream)
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

  // open Preferences in read-write mode so putString() actually writes
  prefs.begin("config", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.putString("HOST", HOST);
  prefs.putInt("PORT", stream.substring(start[3], end[3]).toInt());

  for(int i = 4; i < 20; i++) // save keys values
  {
    String gcode = stream.substring(start[i], end[i]);
    gcode.trim();
    String index = (String)(i - 4);
    prefs.putString(index.c_str(), gcode);
  }
  prefs.end();
}

void wifiConnect::config_clear()
{
    prefs.begin("config", false);
    prefs.clear();
    prefs.end();
}
