#include <WebServer.h>
#include <Preferences.h>
#include <WiFi.h>

class wifiConnect
{
public:
    wifiConnect();
    void init();
    // true after a successful /save handling — main() can use this to delay other inits
    //bool configAvailable();
    WebServer server;
    bool config_saved;

private:
    void handleRoot();
    void handleSave();
    void ParseAndSave(String stream);
    void connect(String ssid, String pass);

    Preferences prefs;
    const char *ap_ssid = "3d-printer-controller";
    const char *ap_pass = "I~7hK5IV=yo89v+h<>&x";
};