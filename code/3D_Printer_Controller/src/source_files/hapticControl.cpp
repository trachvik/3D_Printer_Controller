#include "header_files/hapticControl.h"
 
hapticControl::hapticControl(MagneticSensorSPI sensor_init, BLDCMotor motor_init, BLDCDriver6PWM driver_init, int num_steps_init, int voltage_limit): 
  sensor(sensor_init), motor(motor_init), driver(driver_init), num_steps(num_steps_init)
{
  this->voltage_limit = voltage_limit;
  step_size = _2PI/(float)num_steps;
}

void hapticControl::init()
{
    // initialize magnetic sensor hardware
  sensor.init();
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
  //motor.phase_resistance = 12.5; // 12.5 Ohms
  motor.torque_controller = TorqueControlType::voltage;
  // set motion control loop to be used
  motor.controller = MotionControlType::torque;
  //  maximal velocity of the position control
  // default 20
  //motor.velocity_limit = 4;
  // default voltage_power_supply
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
  _delay(1000);

  start_angle = sensor.getAngle();
}

void hapticControl::loop()
{
  sensor.update();
  // main FOC algorithm function
  motor.loopFOC();

  // Motion control function

  angle_rel = (sensor.getAngle() - start_angle);

  // compute signed step count (round to nearest step)
  step_count_f = ((float)num_steps/_2PI) * angle_rel;
  step_count = (int)round(step_count_f);

  while(angle_rel > _2PI) angle_rel -= _2PI;
  while(angle_rel < 0) angle_rel += _2PI;

  step_count_abs = ((float)num_steps/_2PI)*(angle_rel);

  between_steps_pos = angle_rel - step_count_abs * step_size;

  if(between_steps_pos > step_size/2) target_voltage = motor.voltage_limit*((step_size - between_steps_pos)/(step_size/2));
  if(between_steps_pos < step_size/2) target_voltage = -motor.voltage_limit*(between_steps_pos/(step_size/2));

  motor.move(target_voltage);

}