#include <WebServer.h>
#include <Preferences.h>
#include <WiFi.h>
#include "header_files/Display.h"
/**
 * This class handles WiFi connectivity and handles configuration via a web interface.
 * It uses the Preferences library to store and retrieve WiFi credentials.
 */

class wifiConnect
{
public:
    wifiConnect(Display *disp = nullptr);
    void init();
    void config_clear();
    WebServer server;
    bool config_saved;

private:
    void handleRoot();
    void handleSave();
    void ParseAndSave(String stream);
    void connect(String ssid, String pass);

    Preferences prefs;
    Display *display;
    const char *ap_ssid = "3d-printer-controller";
    const char *ap_pass = "I~7hK5IV=yo89v+h<>&x";
};