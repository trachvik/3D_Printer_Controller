#include "header_files/wificonnect.h"

char IntToChar(int a);

wifiConnect::wifiConnect()
  : server(80)
{
}

void wifiConnect::handleRoot()
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

void wifiConnect::handleSave()
{
  ParseAndSave(server.arg("plain"));

  server.send(200, "text/html",
    "<h3>Saved succesfully! Device will now try to connect to wifi…</h3>");
  delay(2000);

  wifiConnect();
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

char IntToChar(int a)
{
  char ret;
  if(a < 10 && a >= 0) ret = a + 48;
  else if(a > 9 && a < 16) ret = a + 55;
  return ret;
}
