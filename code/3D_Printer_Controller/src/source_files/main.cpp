#include <Arduino.h>

#include "header_files/wifiConnect.h"
#include "header_files/hapticControl.h"
#include "header_files/printerControl.h"
#include "header_files/Display.h"
//#include <Wire.h>

#define setup_clear_PIN 14
#define led_PIN 27

hapticControl HC(MagneticSensorSPI(AS5048_SPI, 5), BLDCMotor(7), BLDCDriver6PWM(15, 16, 17, 4, 12, 32), 5);
RotaryEncoder encoder(35, 39, RotaryEncoder::LatchMode::TWO03);

// Pins on the MCP23017 I/O expander
byte rowPins[4] = {11, 10, 9, 8}; //connect to the row pinouts of the keypad
byte colPins[4] = {12, 13, 14, 15}; //connect to the column pinouts of the keypad

Display display;

printerControl PC(rowPins, colPins, &display);
bool printerControlinit = true;

wifiConnect wifiCon(&display);

void setup()
{
  pinMode(setup_clear_PIN, INPUT_PULLUP);
  pinMode(led_PIN, OUTPUT);
  Serial.begin(115200);

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("Display initialized");
    display.clearDisplay();
    display.display();
  } else
  {
    Serial.println("Warning: display.begin() failed");
  }

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

  xTaskCreatePinnedToCore
  (
    hapticControl::startTask, // Voláme tu statickou funkci
    "MotorTask",
    2048,         // <--- Změna z 8192 na 2048 (stále budete mít rezervu cca 1700 bajtů)
    &HC, // <--- DŮLEŽITÉ: Zde předáváme odkaz na konkrétní instanci (this)
    1, // reduce priority so it doesn't starve main loop
    NULL,
    1 // pin to core 1
  );
}

long clear_timeout = 0;

void loop()
{
  if(digitalRead(setup_clear_PIN) == LOW && millis() - clear_timeout > 3000) // this clears prefs after 3s hold
  {
    wifiCon.config_clear();
    display.setCursor(0,0);
    display.printText("Config cleared!\n", 1, 1);
    delay(1000);
    wifiCon.init();
  }
  else if(digitalRead(setup_clear_PIN) == HIGH)
  {
    clear_timeout = millis();
  }

  if (HC.step_count != HC.step_count_old)
  {
    display.setCursor(0,0);
    display.printText("step_count: " + String(HC.step_count) + "\n", 1, 1);
    // Serial.println(HC.step_count);
    HC.step_count_old = HC.step_count;
  }
  if(wifiCon.config_saved)
  {
    if(printerControlinit) PC.init();
    else PC.loop();
    printerControlinit = false;
    //Serial.println("FLAG");
  }
  wifiCon.server.handleClient();
  //HC.loop();
  //PC.loop();
}
