#include "header_files/hapticControl.h"
 
hapticControl::hapticControl(MagneticSensorSPI sensor_init, BLDCMotor motor_init, BLDCDriver6PWM driver_init, float supply_voltage, float voltage_limit, int encoder_PIN0, int encoder_PIN1)
  : sensor(sensor_init), motor(motor_init), driver(driver_init), supply_voltage(supply_voltage), voltage_limit(voltage_limit),
    encoder(encoder_PIN0, encoder_PIN1, RotaryEncoder::LatchMode::TWO03)
{
  num_steps = 0;                // start in smooth mode (index 0 in step table)
  step_size = _2PI / 16.0f;
  num_steps_old = num_steps;
  step_count_old = step_count;
  last_torque = 0.0f;
  voltage_filter_alpha = 0.8f;  // Filter strength: lower=smoother, higher=responsive (0.2-0.5)
  _btn_prev_state = HIGH;
  _btn_last_ms    = 0;
  _btn_step_idx   = 0;          // index 0 = smooth mode (0 steps)
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
  // power supply voltage [V] — must match actual battery/supply voltage for correct PWM scaling
  driver.voltage_power_supply = supply_voltage;
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
  pinMode(HC_BTN_PIN, INPUT_PULLUP);
  // set motion control loop to be used
  // set the torque control type
  // Set motor parameters for proper FOC
  //motor.phase_resistance = 12.5; // 12.5 Ohms
  motor.torque_controller = TorqueControlType::voltage;
  // Set motion control loop
  motor.controller = MotionControlType::torque;
  // Motor voltage limit derived from torque budget: Vq_max = MAX_TORQUE * R_phase / Kt
  motor.voltage_limit = HC_MAX_TORQUE * HC_R_PHASE / HC_KT;  // ≈ 2.87 V
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
  last_torque = 0.0f;
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

    setNumSteps();

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

    // All haptic logic operates in torque [N·m]
    // Conversion to voltage at output: Vq = torque * HC_R_PHASE / HC_KT
    float target_torque;

    // Smooth mode: velocity damping for continuous resistance
    if (num_steps == 0) {
      // damping [N·m·s/rad]: full HC_MAX_TORQUE reached at velocity_max
      const float velocity_min = 0.5f;   // [rad/s] deadband — below this: zero torque
      const float velocity_max = 15.0f;  // [rad/s] saturation — above this: full HC_MAX_TORQUE
      const float damping = HC_MAX_TORQUE / velocity_max;  // 0.002 N·m·s/rad
      float abs_velocity = abs(motor.shaft_velocity);

      if (abs_velocity < velocity_min) {
        target_torque = 0.0f;
      } else if (abs_velocity < velocity_max) {
        float scaling_factor = (abs_velocity - velocity_min) / (velocity_max - velocity_min);
        target_torque = -damping * motor.shaft_velocity * scaling_factor;
      } else {
        target_torque = -damping * motor.shaft_velocity;  // saturated at HC_MAX_TORQUE
      }
      // Heavier filtering for smooth mode (velocity noise)
      target_torque = 0.08f * target_torque + 0.75f * last_torque;
    }
    // Detent mode: step-based haptic feedback
    else {
      float normalized_pos = between_steps_pos / step_size; // 0.0 – 1.0
      // Peak detent torque as fraction of HC_MAX_TORQUE
      const float detent_fraction = 0.2f;  // 0.2 * 40 mN·m = 8 mN·m peak
      target_torque = -HC_MAX_TORQUE * detent_fraction * sin(_2PI * normalized_pos);

      // Position deadband: zero torque near step center, linear ramp outward
      const float pos_deadband_min = 0.08f;
      const float pos_deadband_max = 0.15f;
      float dist_from_center = abs(normalized_pos - 0.5f);

      if (dist_from_center < pos_deadband_min) {
        target_torque = 0.0f;
      } else if (dist_from_center < pos_deadband_max) {
        float scaling_factor = (dist_from_center - pos_deadband_min) / (pos_deadband_max - pos_deadband_min);
        target_torque *= scaling_factor;
      }
      // else: full torque

      // Low-pass filter
      target_torque = voltage_filter_alpha * target_torque + (1.0f - voltage_filter_alpha) * last_torque;
    }

    last_torque = target_torque;

    // Convert torque → voltage: Vq = τ * R_phase / Kt
    motor.move(target_torque * HC_R_PHASE / HC_KT);
  }
}

// Step table cycled by the KEY button: 0 (smooth) → 16 → 12 → 8 → 0 …
static const int HC_STEPS_TABLE[]  = {0, 16, 12, 8};
static const int HC_STEPS_COUNT    = 4;

void hapticControl::setNumSteps()
{
  uint8_t state = digitalRead(HC_BTN_PIN);  // LOW when pressed (INPUT_PULLUP)

  // Detect falling edge (button press)
  if (state == LOW && _btn_prev_state == HIGH) {
    uint32_t now = millis();
    if (now - _btn_last_ms > 50) {           // 50 ms debounce
      _btn_step_idx = (_btn_step_idx + 1) % HC_STEPS_COUNT;
      num_steps     = HC_STEPS_TABLE[_btn_step_idx];
      _btn_last_ms  = now;
    }
  }
  _btn_prev_state = state;
}

/*void hapticControl::encoderInit(RotaryEncoder &encoder)
{
  // Store the address of the passed encoder
  this->encoder = &encoder;
}*/