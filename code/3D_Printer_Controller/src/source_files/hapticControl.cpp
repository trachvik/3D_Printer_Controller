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
    // main FOC algorithm function
    motor.loopFOC();

    setNumSteps(); // TO DO interrupt? or here in task?

    // Preserve position when num_steps is changed
    if(num_steps != num_steps_old)
    {
      step_count_buffer = step_count;
      step_size = _2PI/(float)(num_steps > 0 ? num_steps : 1);  // Avoid division by zero
      start_angle  = sensor.getAngle();
      num_steps_old = num_steps;
    }

    float angle_rel = (sensor.getAngle() - start_angle);

    // compute signed step count (round to nearest step)
    float step_count_f = (num_steps > 0) ? ((float)num_steps/_2PI) * angle_rel : 0;
    step_count = (int)round(step_count_f) + step_count_buffer;

    while(angle_rel > _2PI) angle_rel -= _2PI;
    while(angle_rel < 0) angle_rel += _2PI;

    int step_count_abs = (num_steps > 0) ? ((float)num_steps/_2PI) * angle_rel : 0;
    float between_steps_pos = angle_rel - step_count_abs * step_size + step_size/2; // add step_size/2 to stabilize center of step

    float target_voltage;

    // Smooth mode: velocity damping for continuous resistance
    if (num_steps == 0) {  // Use num_steps == 0 to enable smooth mode
      float damping_coefficient = 0.3f;  // Adjust resistance: 0.1 = light, 0.5 = heavy
      target_voltage = -damping_coefficient * motor.shaft_velocity;
      
      // Smooth velocity deadband with linear ramp to avoid bumps
      float velocity_min = 0.5f;   // Below this: zero torque
      float velocity_max = 3.0f;   // Above this: full damping
      float abs_velocity = abs(motor.shaft_velocity);
      
      if (abs_velocity < velocity_min) {
        target_voltage = 0.0f;
      } else if (abs_velocity < velocity_max) {
        // Linear ramp from 0 to full damping
        float scaling_factor = (abs_velocity - velocity_min) / (velocity_max - velocity_min);
        target_voltage *= scaling_factor;
      }
      // else: full damping (no scaling)
      
      // Heavier filtering for smooth mode to eliminate velocity noise
      // Options: 0.05-0.95 (very smooth), 0.1-0.9 (smooth), 0.2-0.8 (moderate)
      target_voltage = 0.08f * target_voltage + 0.75f * last_voltage;
    } 
    // Detent mode: step-based haptic feedback
    else {
      // Use smooth sinusoidal transition for better stability
      float normalized_pos = between_steps_pos / step_size; // 0 to 1
      //float damping_factor = 1 - (0.6 * ((float)num_steps / 20.0)); // 0.4 for 20 steps 0.88 for 4 steps
      float damping_factor = 0.2f;
      target_voltage = -motor.voltage_limit * damping_factor * sin(_2PI * normalized_pos);
      
      // Position deadband with ramp: reduce torque near step center
      float pos_deadband_min = 0.08f;  // Inner zone: zero torque (5% of step around center)
      float pos_deadband_max = 0.15f;  // Outer zone: full torque (15% of step around center)
      float dist_from_center = abs(normalized_pos - 0.5f);  // Distance from step center
      
      if (dist_from_center < pos_deadband_min) {
        target_voltage = 0.0f;
      } else if (dist_from_center < pos_deadband_max) {
        // Linear ramp from 0 to full torque
        float scaling_factor = (dist_from_center - pos_deadband_min) / (pos_deadband_max - pos_deadband_min);
        target_voltage *= scaling_factor;
      }
      // else: full torque (no scaling)
      
      // Normal filtering for detent mode
      target_voltage = voltage_filter_alpha * target_voltage + (1.0 - voltage_filter_alpha) * last_voltage;
    }
    
    last_voltage = target_voltage;

    motor.move(target_voltage);
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