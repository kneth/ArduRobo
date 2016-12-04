#include <Scheduler.h>
#include <Scheduler/Semaphore.h>
#include <Ardumoto.h>

Ardumoto Moto;

Semaphore light_mutex;
Semaphore motor_mutex;

struct motor_command {
  int left;
  int right;
};
typedef struct motor_command MotorCommand;

MotorCommand mc;

void control_motor() {
  Serial.print("control_motor ");
  motor_mutex.wait();
  // kopierer værdier - command_center overskriver dem
  int left = mc.left;
  int right = mc.right;
  motor_mutex.signal();

  Serial.println(left, DEC);
  Serial.println(right, DEC);
  Moto.setSpeed('A', left);
  Moto.setSpeed('B', right);
  delay(250);
}

int light_value;

#define ANALOG_SENSOR_PIN A0
#define DIGITAL_SENSOR_PIN 3
#define LEDPIN 13

void light_sensor() {
  int switch_state = digitalRead(DIGITAL_SENSOR_PIN);  
  if (switch_state == LOW) {
    digitalWrite(LEDPIN, HIGH);
  } else {
    digitalWrite(LEDPIN, LOW);
  }

  light_mutex.wait();
  light_value = analogRead(ANALOG_SENSOR_PIN);
  light_mutex.signal();
  delay(250);
}

void command_center() {
  const int max = 100;
  static int last_proportional = 0;
  static int integral = 0;
  
  unsigned int position;
  light_mutex.wait();
  position = light_value;
  light_mutex.signal();
  Serial.println(light_value, DEC);

  int proportional = ((int)position) - 511; // lyssensor mellem 0 og 1023
  int derivative = proportional - last_proportional;
  integral += proportional;
  last_proportional = proportional;

  int power_difference = proportional/20 + integral/10000 + derivative*3/2;
  Serial.print("power_difference = ");
  Serial.println(power_difference, DEC);
  if (power_difference > max)
    power_difference = max;
  if (power_difference < -max)
    power_difference = -max;

  motor_mutex.wait();
  if (power_difference < 0) {
    mc.left = max+power_difference;
    mc.right = max;
  } else {
    mc.left = max;
    mc.right = max-power_difference;
  }
  motor_mutex.signal();
  
  delay(500);
}

void setup() {
  Serial.begin(115200);
  Serial.println("setup");
  Moto.begin();
  Scheduler.start(NULL, light_sensor);
  Scheduler.start(NULL, control_motor);
}

void loop() {
  command_center();
}
