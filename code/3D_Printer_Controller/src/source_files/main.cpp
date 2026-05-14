#include <Arduino.h>

#ifndef BOARD_BLACKPILL
#include "header_files/wifiConnect.h"
#include "header_files/printerControl.h"
#include "header_files/Display.h"
#endif

#include "header_files/hapticControl.h"

// --- Pin definitions ---
#ifdef BOARD_BLACKPILL
  // STM32F411CE BlackPill
  // Driver: TIM1 complementary outputs (hardware dead-time, SimpleFOC auto-detects)
  //   AH=PA8  (TIM1_CH1)   AL=PB13 (TIM1_CH1N)
  //   BH=PA9  (TIM1_CH2)   BL=PB14 (TIM1_CH2N)
  //   CH=PA10 (TIM1_CH3)   CL=PB15 (TIM1_CH3N)
  // AS5048A: SPI1 (MOSI=PA7, MISO=PA6, SCK=PA5), CS=PA4
  #define setup_clear_PIN  PB12
  #define encoderHC_PIN0   PB8
  #define encoderHC_PIN1   PB9
  #define HAPTIC_CS_PIN    PA4
  #define HAPTIC_AH        PA8
  #define HAPTIC_AL        PB13
  #define HAPTIC_BH        PA9
  #define HAPTIC_BL        PB14
  #define HAPTIC_CH        PA10
  #define HAPTIC_CL        PB15
#else
  // ESP32
  #define setup_clear_PIN  14
  #define encoderHC_PIN0   35
  #define encoderHC_PIN1   39
  #define encoderPC_PIN0   33
  #define encoderPC_PIN1   34
  #define HAPTIC_CS_PIN    5
  #define HAPTIC_AH        15
  #define HAPTIC_AL        16
  #define HAPTIC_BH        17
  #define HAPTIC_BL        4
  #define HAPTIC_CH        12
  #define HAPTIC_CL        32
#endif


#ifdef BOARD_BLACKPILL
  // Li-ion single cell: max 4.2V, nominal 3.7V
  // voltage_limit: conservative 1.5V (<<R_phase*I_max = 5.6*0.1 = 0.56V headroom)
  #define SUPPLY_VOLTAGE  4.2f
  #define MOTOR_VOLTAGE_LIMIT  3.0f
#else
  #define SUPPLY_VOLTAGE  4.2f
  #define MOTOR_VOLTAGE_LIMIT  3.0f
#endif

hapticControl HC(MagneticSensorSPI(AS5048_SPI, HAPTIC_CS_PIN), BLDCMotor(11),
                 BLDCDriver6PWM(HAPTIC_AH, HAPTIC_AL, HAPTIC_BH, HAPTIC_BL, HAPTIC_CH, HAPTIC_CL),
                 SUPPLY_VOLTAGE, MOTOR_VOLTAGE_LIMIT, encoderHC_PIN0, encoderHC_PIN1);

#ifndef BOARD_BLACKPILL
// Pins on the MCP23017 I/O expander
byte rowPins[4] = {11, 10, 9, 8}; //connect to the row pinouts of the keypad
byte colPins[4] = {12, 13, 14, 15}; //connect to the column pinouts of the keypad

Display display;

printerControl PC(rowPins, colPins, encoderPC_PIN0, encoderPC_PIN1, &display);
bool printerControlinit = true;

wifiConnect wifiCon(&display);
#endif

void setup()
{
  Serial.begin(115200);
  pinMode(setup_clear_PIN, INPUT_PULLUP);

  // initialize HC
  HC.init();

#ifndef BOARD_BLACKPILL
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
#endif
}

long clear_timeout = 0;

void loop()
{
#ifdef BOARD_BLACKPILL
  // No RTOS on STM32 Arduino — run haptic loop directly (has internal while(1))
  HC.loop();
#else
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
#endif // !BOARD_BLACKPILL
}
