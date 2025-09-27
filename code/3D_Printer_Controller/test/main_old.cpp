#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>   // pro uložení do NVS
#include <WebSocketClient.h>

char connect(const char*, const char*);
char initWebSocket(void);

int PORT = 7125;
char* PATH = "/websocket";
String HOST;

const char *ap_ssid = "MyESP32AP";
const char *ap_password = "testpassword";

//String key_gcode[16];

// WebServer na portu 80
WebServer server(80);

WebSocketClient webSocketClient;
WiFiClient client;

// Trvalé uložení nastavení
Preferences prefs;

void handleRoot() {
  // Jednoduchá HTML stránka s formulářem
  String page = R"rawliteral(
  <!DOCTYPE html><html>
  <head><meta charset="UTF-8"><title>ESP32 Setup</title></head>
  <body>
    <h2>Nastavení Wi-Fi a proměnných</h2>
    <form action="/save" method="POST">
      SSID Wi-Fi: <input type="text" name="ssid"><br>
      Heslo Wi-Fi: <input type="password" name="pass"><br><br>
      WebSocket IP: <input type="text" name="HOST"><br>
      <input type="text" name="key"><br>

  
      <input type="submit" value="Uložit">
    </form>
  </body></html>
  )rawliteral";

  server.send(200, "text/html", page);
}

void handleSave() {
  // Ověření, že všechna pole existují | podle me zbytecne
  //if (server.hasArg("ssid") && server.hasArg("pass") &&
      //server.hasArg("HOST"))
  //{

  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

    /*for(int i = 0; i < 15; i++)
    {
        key_gcode[i] = server.arg("key" + (char)i);
    }*/

    // Uložení do NVS
    prefs.begin("config", false);
    prefs.putString("wifi_ssid", ssid);
    prefs.putString("wifi_pass", pass);
    prefs.putString("HOST",  server.arg("HOST"));
    /*for(int i = 1; i < 16; i++)
    {
        prefs.putString("key" + (char)i, server.arg("key" + (char)i));
    }*/
   prefs.putString("key",  server.arg("key"));

    prefs.end();

    server.send(200, "text/html",
      "<h3>Uloženo! ESP32 se nyní pokusí připojit k Wi-Fi…</h3>");
    delay(2000);

    // Pokus o připojení k cílové síti
      connect(ssid.c_str(), pass.c_str());
      initWebSocket();
  //}
  //else {
   // server.send(400, "text/plain", "Chybí některé údaje!");
  //}
}


#include <Arduino.h>
#include <WiFi.h>

const char *ssid = "MyESP32AP";
const char *password = "testpassword";

void setup()
{
  Serial.begin(115200);

  // Spuštění dočasné Wi-Fi
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.print("SoftAP běží. IP: ");
  Serial.println(WiFi.softAPIP());

  // Registrace obsluhy stránek
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  
}

void loop()
{
  server.handleClient();

  // Po úspěšném připojení můžeš v hlavní smyčce
  // použít uložené proměnné:
  if (WiFi.status() == WL_CONNECTED) {
    // Např. jen jednorázové vypsání
    static bool once = false;
    if (!once) {
      prefs.begin("config", true);
      Serial.println("== Uložená konfigurace ==");
      Serial.println("WebSocket IP: " + prefs.getString("HOST"));
      Serial.print("User define keys: " + prefs.getString("key"));
      /*for(int i = 0; i < 15; i++)
      {
        Serial.print(prefs.getString("key" +(char)i) + " ");
      }*/
      Serial.println();
      Serial.print  ("Wi-Fi IP: ");
      Serial.println(WiFi.localIP());
      prefs.end();
      once = true;
    }
  }
}

char initWebSocket()
{
  HOST = prefs.getString("HOST");
  if(!client.connect("*printers_host*", 7125)) {
    Serial.println("Connection failed.");
    return 0;
  }

  Serial.println("Connected.");
  webSocketClient.path = "/websocket";
  webSocketClient.host = "*printers_host*"; // .data() return non constant char*

  if (!webSocketClient.handshake(client)) {
    Serial.println("Handshake failed.");
    return 0;
  }
  Serial.println("Handshake successful");
  return 1;
}

char connect(const char* ssid, const char* pass)
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



