#include <AS5048A.h>
#include <Arduino.h>

int getRelativeSteps(void);

AS5048A angleSensor(SS, false); // false - turn of debugging

long time_past = 0;
int past_pos = 0;
int rel_pos = 0;

int steps_per_revolution = 30;
int step_count = 0;
int range = 16384; // 14-bit sensor

void setup()
{
	Serial.begin(115200);
	angleSensor.begin();
    past_pos = angleSensor.getRawRotation();
}

void loop()
{
    step_count = getRelativeSteps();

    // print to serial monitor
    if(millis() - time_past > 1000)
    {
        Serial.print("Absolute position: ");
        Serial.println(angleSensor.getRawRotation());
        //Serial.print("Relative position: ");
        //Serial.println(rel_pos);
        Serial.print("Step couter: ");
        Serial.println(step_count);
        //Serial.print("delta: ");
        //Serial.println(delta);
        //Serial.print("State: ");
        //angleSensor.printState();
        //Serial.print("Errors: ");
        //Serial.println(angleSensor.getErrors());

        time_past = millis();
    }
}

int getRelativeSteps()
{
    int curr_pos = angleSensor.getRawRotation();
    int delta = curr_pos - past_pos;

    if (delta > range/2) delta -= range;
    else if (delta < -range/2) delta += range;

    rel_pos -= delta; 
    past_pos = curr_pos;
    return (rel_pos*steps_per_revolution)/range;
}