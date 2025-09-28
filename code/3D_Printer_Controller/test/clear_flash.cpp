#include <Preferences.h>
Preferences prefs;

void setup()
{
    prefs.begin("config", false);
    prefs.clear();   // clears all saved keys from "config"
    prefs.end();
}

void loop()
{

}
