#include <SimpleFOC.h>
#include <RotaryEncoder.h>

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
    hapticControl(MagneticSensorSPI sensor_init, BLDCMotor motor_init, BLDCDriver6PWM driver_init, int voltage_limit, int encoder_PIN0, int encoder_PIN1);
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
    int voltage_limit;
    int num_steps_old;
    int step_count_buffer;
    float start_angle;
    float step_size;
    MagneticSensorSPI sensor;
    BLDCMotor motor;
    BLDCDriver6PWM driver;
    RotaryEncoder encoder;
};
