#include "header_files/hapticControl.h"

hapticControl HC = hapticControl(MagneticSensorSPI(AS5048_SPI, 5), BLDCMotor(7), BLDCDriver6PWM(15, 16, 17, 4, 21, 22), 30, 5);

long time_past = 0;

void setup()
{
  Serial.begin(115200);
  HC.init();
}

void loop()
{
  HC.loop();

  if(millis() - time_past > 1000)
  { 
    Serial.println("Step count: ");
    Serial.println(HC.step_count);
    time_past = millis();
  }
}

