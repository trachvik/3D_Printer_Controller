#include <WebServer.h>
#include <Preferences.h>

class wifiConnect
{
  public:
    wifiConnect();
    void handleRoot();
    void handleSave();
    void ParseAndSave(String stream);


    WebServer server;
    Preferences prefs;
};