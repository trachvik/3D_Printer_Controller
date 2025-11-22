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

    // Static instance pointer used by C-style handler wrappers
    static wifiConnect *instance;

private:
    void handleRoot();
    void handleSave();
    void ParseAndSave(String stream);
    void connect(String ssid, String pass);

    // C-style static handlers with no captures
    static void handleRootStatic();
    static void handleSaveStatic();

    Preferences prefs;
    const char *ap_ssid = "3d-printer-controller";
    const char *ap_pass = "I~7hK5IV=yo89v+h<>&x";
};