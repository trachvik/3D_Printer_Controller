#include "header_files/hapticControl.h"

hapticControl HC = hapticControl(MagneticSensorSPI(AS5048_SPI, 5), BLDCMotor(7), BLDCDriver6PWM(15, 16, 17, 4, 21, 22), 5);
RotaryEncoder encoder(35, 39, RotaryEncoder::LatchMode::TWO03);
long time_past = 0;

void setup()
{
  Serial.begin(115200);
  // initialize HC and pass the rotary encoder to it
  HC.encoderInit(encoder);
  HC.init();
}

void loop()
{
  HC.loop();

  if(millis() - time_past > 1000)
  { 
    Serial.println("Step Count: ");
    Serial.println(HC.step_count);
    time_past = millis();
  }
}
