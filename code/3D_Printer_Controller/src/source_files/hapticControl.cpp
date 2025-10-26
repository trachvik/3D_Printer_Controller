#include "header_files/hapticControl.h"
 
hapticControl::hapticControl(MagneticSensorSPI sensor_init, BLDCMotor motor_init, BLDCDriver6PWM driver_init, int voltage_limit): 
  sensor(sensor_init), motor(motor_init), driver(driver_init), voltage_limit(voltage_limit)
{
  step_size = _2PI/(float)num_steps;
  num_steps_old = num_steps;
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
  driver.pwm_frequency = 32000;
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
  _delay(1000); // <----- Is this necessary?

  start_angle = sensor.getAngle();
}

void hapticControl::loop()
{
  sensor.update();
  // main FOC algorithm function
  motor.loopFOC();

  setNumSteps();

  // Preserve position when num_steps is changed
  if(num_steps != num_steps_old)
  {
    step_count_old = step_count;
    step_size = _2PI/(float)num_steps;
    start_angle  = sensor.getAngle();
    num_steps_old = num_steps;
  }

  float angle_rel = (sensor.getAngle() - start_angle);

  // compute signed step count (round to nearest step)
  float step_count_f = ((float)num_steps/_2PI) * angle_rel;
  step_count = (int)round(step_count_f) + step_count_old;

  while(angle_rel > _2PI) angle_rel -= _2PI;
  while(angle_rel < 0) angle_rel += _2PI;

  int step_count_abs = ((float)num_steps/_2PI) * angle_rel;
  float between_steps_pos = angle_rel - step_count_abs * step_size + step_size/2; // add step_size/2 to stabilize center of step

  // Use smooth sinusoidal transition for better stability
  float normalized_pos = between_steps_pos / step_size; // 0 to 1
  float target_voltage = -motor.voltage_limit * sin(_2PI * normalized_pos);

  motor.move(target_voltage);

}

void hapticControl::setNumSteps()
{
  // Update encoder state
  encoder->tick();
  int curPos = encoder->getPosition() / 2;
  // compute coarse relative position in blocks of 16 (adjust as needed)
  int relPos = abs(curPos - (curPos - curPos % 16)) + 1;  // 16 ... number of step increments
  // store into the object's num_steps member
  num_steps = relPos * 4;
}

void hapticControl::encoderInit(RotaryEncoder &encoder)
{
  // Store the address of the passed encoder
  this->encoder = &encoder;
}