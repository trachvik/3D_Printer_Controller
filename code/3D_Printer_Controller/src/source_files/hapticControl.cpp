#include "header_files/hapticControl.h"
 
hapticControl::hapticControl(MagneticSensorSPI sensor_init, BLDCMotor motor_init, BLDCDriver6PWM driver_init, int voltage_limit, int encoder_PIN0, int encoder_PIN1)
  : sensor(sensor_init), motor(motor_init), driver(driver_init), voltage_limit(voltage_limit),
    encoder(encoder_PIN0, encoder_PIN1, RotaryEncoder::LatchMode::TWO03)
{
  step_size = _2PI/(float)(num_steps > 0 ? num_steps : 1);  // Avoid division by zero
  num_steps_old = num_steps;
  step_count_old = step_count;
  last_voltage = 0.0;
  voltage_filter_alpha = 0.8;  // Filter strength: lower=smoother, higher=responsive (0.2-0.5)
}

void hapticControl::init()
{
  setNumSteps();
    // initialize magnetic sensor hardware
  sensor.init();
  // diagnostic: check sensor read immediately after init
  // link the motor to the sensor
  motor.linkSensor(&sensor);
  // driver config
  // power supply voltage [V]
  driver.voltage_power_supply = voltage_limit;
  // limit the maximal dc voltage the driver can set
  // as a protection measure for the low-resistance motors
  // this value is fixed on startup
  driver.voltage_limit = voltage_limit;
  // pwm frequency to be used [Hz]
  // for atmega328 fixed to 32kHz
  // esp32/stm32/teensy configurable
  // Lower frequency reduces audible noise but may affect smoothness
  // Try: 20000-25000 Hz for balance between performance and noise
  driver.pwm_frequency = 40000;  // Reduced from 32000 to minimize audible noise
  // driver config
  driver.init();
  motor.linkDriver(&driver);
  // set motion control loop to be used
  // set the torque control type
  // Set motor parameters for proper FOC
  //motor.phase_resistance = 12.5; // 12.5 Ohms
  motor.torque_controller = TorqueControlType::voltage;
  // Set motion control loop
  motor.controller = MotionControlType::torque;
  // Set reasonable voltage limit for stable control
  motor.voltage_limit = voltage_limit;
  // use monitoring with serial 
  //Serial.begin(115200);
  // comment out if not needed
  //motor.useMonitoring(Serial);
  // initialize motor
  motor.init();
  // align encoder and start FOC
  motor.initFOC();
  //Serial.println("Motor ready.");
  //start_angle = sensor.getAngle();
  
  // Settle motor at zero torque after calibration
  for (int i = 0; i < 100; i++) {
    motor.loopFOC();
    motor.move(0);
    delayMicroseconds(1000);
  }
  
  _delay(500);

  start_angle = sensor.getAngle();
  last_voltage = 0.0f;  // Ensure filter starts at zero
}
void hapticControl::startTask(void* _this)
{
    // Convert the passed pointer back to hapticControl instance
    hapticControl* controller = (hapticControl*)_this;
    
    static uint32_t lastCheck = 0;
    /*if (millis() - lastCheck > 2000)
    {
        lastCheck = millis();
        
        UBaseType_t space_left = uxTaskGetStackHighWaterMark(NULL);
        Serial.printf("Least amount of stack space that has remained for the task since the task was created: %d bytes\n", space_left);
        
        if (space_left < 200)
        {
            Serial.println("Warning: very low stack space in xTaskCreatePinnedToCore");
        }
    }*/
    
    // Now call the actual loop method
    controller->loop();
}

void hapticControl::loop()
{
  while(1)
  {
    sensor.update();
    motor.loopFOC();
    motor.move(3);
    delay(5);
  }
}

long last_millis = 0;

void hapticControl::setNumSteps() // TO DO interrupt?
{
  // Update encoder state
  encoder.tick();
  int curPos = encoder.getPosition() / 2;
  /*if(millis() - last_millis > 500)
  {
    Serial.print("Encoder position HC : ");
    Serial.println(curPos);
    last_millis = millis();
  }*/

  // compute coarse relative position in blocks of 16 (adjust as needed)
  encoder_val = abs(curPos - (curPos - curPos % 6)) + 1;  // 6 ... number of step increments | numbers [1,6]
  // store into the object's num_steps member
  // encoder_val 1 = smooth mode (0 steps), 2-6 = 4, 8, 12, 16, 20 steps
  if (encoder_val == 1) {
    num_steps = 0;  // Smooth mode
  } else {
    num_steps = (7 - encoder_val) * 4;  // 4, 8, 12, 16, 20 steps
  }
}

/*void hapticControl::encoderInit(RotaryEncoder &encoder)
{
  // Store the address of the passed encoder
  this->encoder = &encoder;
}*/