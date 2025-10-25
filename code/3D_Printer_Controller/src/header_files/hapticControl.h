#include <SimpleFOC.h>

class hapticControl
{
public:
    int step_count;
    int num_steps;
    hapticControl(MagneticSensorSPI sensor_init, BLDCMotor motor_init, BLDCDriver6PWM driver_init, int num_steps_init, int voltage_limit);
    void init();
    void loop();
private:
    int voltage_limit;
    float target_voltage;
    int step_count_abs;
    float between_steps_pos;
    float step_size;
    float start_angle;
    float target_angle;
    float angle_rel;
    float step_count_f;
    MagneticSensorSPI sensor;
    BLDCMotor motor;
    BLDCDriver6PWM driver;
};