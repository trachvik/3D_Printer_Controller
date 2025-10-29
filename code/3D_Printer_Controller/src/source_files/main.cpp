#include <Arduino.h>

#include "header_files/wifiConnect.h"
#include "header_files/hapticControl.h"
#include "header_files/printerControl.h"

hapticControl HC(MagneticSensorSPI(AS5048_SPI, 5), BLDCMotor(7), BLDCDriver6PWM(15, 16, 17, 4, 21, 22), 5);
RotaryEncoder encoder(35, 39, RotaryEncoder::LatchMode::TWO03);

byte rowPins[4] = {14, 33, 13, 26}; //connect to the row pinouts of the keypad
byte colPins[4] = {27, 25, 32, 12}; //connect to the column pinouts of the keypad

printerControl PC(rowPins,colPins);
bool printerControlinit = true;

wifiConnect wifiCon;

void setup()
{
  Serial.begin(115200);
  wifiCon.init();
  // This prevents PC.init() from beeing called before saving values to prefs
  if(wifiCon.config_saved)
  {
    PC.init();
    printerControlinit = false;
  }
  // initialize HC and pass the rotary encoder to it
  HC.encoderInit(encoder);
  HC.init();
}

//long time_past = 0;

void loop()
{
  if(wifiCon.config_saved && printerControlinit)
  {
    PC.init();
    printerControlinit = false;
  }
  wifiCon.server.handleClient();
  HC.loop();
  PC.loop();
}
