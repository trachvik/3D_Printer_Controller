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
    /**
     * wifiConnect class constructor
     * @param disp pointer to Display object
     */
    wifiConnect(Display *disp = nullptr);
    /**
     * Initializes WiFi connection and web server
     */
    void init();
    void config_clear();
    WebServer server;
    bool config_saved;

private:
    /**
     * Handles the root web page request
     */
    void handleRoot();
    /**
     * Handles the save configuration request
     */
    void handleSave();
    /**
     * Parses and saves WiFi credentials and other settings
     * @param stream the incoming data stream from the web form
     */
    void ParseAndSave(String stream);
    /**
     * Connects to the specified WiFi network
     */
    void connect(String ssid, String pass);

    Preferences prefs;
    Display *display;
    const char *ap_ssid = "3d-printer-controller";
    const char *ap_pass = "I~7hK5IV=yo89v+h<>&x"; // Strong password for access point mode - change as needed
};