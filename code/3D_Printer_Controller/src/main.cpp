#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <KeyPad.h>

WebSocketsClient webSocket;
WiFiClient client;

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
      //mam text posilat zde nebo jinde, propadne jaky zvolit interval?
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
        Serial.printf("Extruder Temperature: %.2f --> %.2f,  Bed Temperature: %.2f --> %.2f \n", ext_temp, ext_target, bed_temp, bed_target);


      //Serial.printf("[WSc] Text: %s\n", payload);
      break;
  }
 /*if(type == WStype_TEXT)
 {

 }*/

}

void setup() {
  Serial.begin(115200);
  WiFi.begin("*fill_your_ssid*", "*fill_your_pass*");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi connected");

  //client.connect("*printers_host*", 7125); // <-------------

  webSocket.setExtraHeaders("Origin: *printers_url*");
  webSocket.begin("*printers_host*", 7125, "/websocket"); // <----------------  bez toho nefunguje, proc, co to dela?

    /*client.print(
    String("GET /websocket HTTP/1.1\r\n") +
    "Host: " + "*printers_host*" + ":" + "7125" + "\r\n" +
    "Upgrade: websocket\r\n" +
    "Connection: Upgrade\r\n" +
    "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n" +
    "Sec-WebSocket-Version: 13\r\n" +
    "Origin: http://" + "*printers_host*" + "\r\n" +
    "\r\n"
  );*/
  //webSocket.sendHeader(webSocket); // WSclient_t   <-------------------
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(15000);

}

void loop() {
  webSocket.loop();
  /*while (client.available()) {
    String line = client.readStringUntil('\n');
    Serial.println("⬅️ " + line);
  }*/
}