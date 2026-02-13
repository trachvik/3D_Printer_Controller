#include <Arduino.h>

#include "header_files/wifiConnect.h"
#include "header_files/hapticControl.h"
#include "header_files/printerControl.h"
#include "header_files/Display.h"
//#include <Wire.h>

#define setup_clear_PIN 14
#define encoderHC_PIN0 35
#define encoderHC_PIN1 39
#define encoderPC_PIN0 33
#define encoderPC_PIN1 34


hapticControl HC(MagneticSensorSPI(AS5048_SPI, 5), BLDCMotor(7), BLDCDriver6PWM(15, 16, 17, 4, 12, 32), 5, encoderHC_PIN0, encoderHC_PIN1);

// Pins on the MCP23017 I/O expander
byte rowPins[4] = {11, 10, 9, 8}; //connect to the row pinouts of the keypad
byte colPins[4] = {12, 13, 14, 15}; //connect to the column pinouts of the keypad

Display display;

printerControl PC(rowPins, colPins, encoderPC_PIN0, encoderPC_PIN1, &display);
bool printerControlinit = true;

wifiConnect wifiCon(&display);

void setup()
{
  //pinMode(setup_clear_PIN, INPUT_PULLUP);
  //Serial.begin(115200);

  /*if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    //Serial.println("Display initialized");
    display.clearDisplay();
    display.display();
  } else
  {
    Serial.println("Warning: display.begin() failed");
  }*/

  //wifiCon.init();
  // This prevents PC.init() from beeing called before saving values to prefs
 
  /*if(wifiCon.config_saved)
  {
    PC.init();
    printerControlinit = false;
  }*/
  // initialize HC
  HC.init();

  xTaskCreatePinnedToCore
  (
    hapticControl::startTask, // Call the static function
    "MotorTask",
    2048,         // Change stack size if necessary
    &HC, // Pass the instance as parameter
    1, // reduce priority so it doesn't starve main loop
    NULL,
    1 // pin to core 1
  );
}

//long clear_timeout = 0;

void loop()
{
  /*if(digitalRead(setup_clear_PIN) == LOW && millis() - clear_timeout > 3000) // this clears prefs after 3s hold
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
  /////////////////// Handle haptic control related opearation in printerControl | TO DO HC as object in printerControl?
  if (HC.step_count != HC.step_count_old)
  {
    int sign = (HC.step_count > HC.step_count_old) ? 1 : -1;
    //display.setCursor(0,0);
    //display.printText("step_count: " + String(HC.step_count) + "\n", 1, 1);
    // Serial.println(HC.step_count);
    PC.knobPendingChange(sign);
    HC.step_count_old = HC.step_count;
  }
  if(HC.encoder_val != HC.encoder_val_old)
  {
    PC.setStepSize(HC.encoder_val);
    HC.encoder_val_old = HC.encoder_val;
  }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

  display.printf("step_count: %d\n", HC.step_count);  */
}
