#include <Arduino.h>
#include <Wire.h>

#include "header_files/wifiConnect.h"
#include "header_files/hapticControl.h"
#include "header_files/printerControl.h"

hapticControl HC(MagneticSensorSPI(AS5048_SPI, 5), BLDCMotor(7), BLDCDriver6PWM(15, 16, 17, 4, 21, 22), 5);
RotaryEncoder encoder(35, 39, RotaryEncoder::LatchMode::TWO03);

//pins on MCP23017
byte rowPins[4] = {11, 10, 9, 8}; //connect to the row pinouts of the keypad
byte colPins[4] = {12, 13, 14, 15}; //connect to the column pinouts of the keypad

printerControl PC(rowPins,colPins);
bool printerControlinit = true;

wifiConnect wifiCon;
//wifiCon->knob.hapticControl(MagneticSensorSPI(AS5048_SPI, 5), BLDCMotor(7), BLDCDriver6PWM(15, 16, 17, 4, 21, 22), 5);

//hw_timer_t * timer = NULL;
//void IRAM_ATTR onTimer() //IRMA_ATTR ensures the function is placed in IRAM and can be called from an interrupt and is much faster than FLASH
//{
  // Zavolá naši rychlou smyčku pro motor
  //HC.loop();
  //Serial.println("Interrupt");
//}

void setup()
{
  Serial.begin(115200);
  // Ensure I2C bus is initialized early to avoid 'bus is not initialized' errors
  Wire.begin();
  //Wire.setClock(400000);
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

  // timer interrupt setup
  //timer = timerBegin(1000);
  //timerAttachInterrupt(timer, &onTimer);
 // timerAlarm(timer, 150,true, 0);
  //timerStart(timer);
 // timerAlarmWrite(timer, 1000, true); // once every 1000 microseconds (1ms)
  //timerAlarmEnable(timer);
}

//long time_past = 0;

void loop()
{
  if(wifiCon.config_saved && printerControlinit)
  {
    PC.init();
    printerControlinit = false;
  }
  if(!wifiCon.config_saved) wifiCon.server.handleClient(); //run http server until data are saved
  HC.loop();
 // wifiCon.server.handleClient();

  PC.loop();
  /*int position;
  switch(HC.encoder_val)
  {
    case 1:
      position = HC.step_count*100;
      break;
    case 2:
      position = HC.step_count*50;
      break;
    case 3:
      position = HC.step_count*10;
      break;
    case 4:
      position = HC.step_count*5;
      break;
    case 5:
      position = HC.step_count;
      break;
  }
  //int position = (HC.encoder_val)*HC.step_count;
  if(HC.step_count != HC.step_count_old) 
  {
    PC.move_axis(MOVE_X, position);
    HC.step_count_old = HC.step_count;
  }*/
}
