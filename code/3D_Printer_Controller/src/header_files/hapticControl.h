#include <SimpleFOC.h>
#include <RotaryEncoder.h>

// GM3506 motor parameters
#define HC_R_PHASE    5.6f    // phase winding resistance [Ω] (±5%)
#define HC_KT         0.078f  // torque constant [N·m/A]
// Peak haptic torque budget — derived driver voltage = MAX_TORQUE * R_PHASE / KT ≈ 2.87 V
#define HC_MAX_TORQUE 0.040f  // [N·m]

// Built-in KEY button (active LOW, INPUT_PULLUP)
#ifdef BOARD_BLACKPILL
  #define HC_BTN_PIN  PA0   // BlackPill KEY button
#else
  #define HC_BTN_PIN  0     // ESP32 BOOT/IO0 button
#endif

/**
 * This class implements a haptic control system using a BLDC motor, magnetic sensor, and rotary encoder for step adjustment.
 * BLDC motor, magnetic sensor are initialized via constructor parameters alongside motor voltage limit. The rotary encoder is initialized via a separate init method.
 */

class hapticControl
{
public:
    int step_count; // current step count
    int step_count_old;
    int num_steps; // number of steps in full rotation
    int encoder_val; // current encoder value [1,5]
    int encoder_val_old;
    /** 
     * hapticControl class constructor
     * @param sensor_init       initilializes MagneticSensorSPI sensor instance
     * @param motor_init        initilializes BLDCMotor motor instance
     * @param driver_init       initilializes BLDCDriver6PWM driver instance
     * @param num_steps_init    initial number steps in one full rotation
     * @param voltage_limit     voltage limit for the motor
     * @param encoder_PIN0      rotary encoder pin 0
     * @param encoder_PIN1      rotary encoder pin 1
     */
    hapticControl(MagneticSensorSPI sensor_init, BLDCMotor motor_init, BLDCDriver6PWM driver_init, float supply_voltage, float voltage_limit, int encoder_PIN0, int encoder_PIN1);
    /**
     * This function has to be called in the setup() part of the main code
     */
    void init();
    /**
     * This function has to be called in the loop() part of the main code
     */
    void loop();
    /**
     * Update num_steps from a rotary encoder
     *
     */
    /**
     * Static task starter for FreeRTOS
     */
    static void startTask(void* _this);
private:
    void setNumSteps();
    float supply_voltage;  // actual battery/supply voltage [V]
    float voltage_limit;   // max Vq applied to motor [V]
    int num_steps_old;
    int step_count_buffer;
    float start_angle;
    float step_size;
    MagneticSensorSPI sensor;
    BLDCMotor motor;
    BLDCDriver6PWM driver;
    RotaryEncoder encoder;
    float last_torque;          // Last applied torque [N·m] for low-pass filter
    float voltage_filter_alpha; // Low-pass filter coefficient (0-1)
    uint8_t  _btn_prev_state;   // previous button read (HIGH/LOW)
    uint32_t _btn_last_ms;      // timestamp of last accepted press [ms]
    uint8_t  _btn_step_idx;     // index into {16, 12, 8} step table
};
