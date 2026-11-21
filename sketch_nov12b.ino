
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

#define SDA_PIN 21
#define SCL_PIN 22
#define BUZZER_PIN 25

const unsigned long TEST_DURATION = 30000;
const unsigned long SAMPLE_INTERVAL = 20; // 50 Hz

void beep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
}

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!mpu.begin()) {
    Serial.println("MPU6050 not detected!");
    while (true) {
      delay(100);
    }
  }

  Serial.println("MPU6050 ready.");
  delay(2000);

  Serial.println("Starting Romberg test...");
  beep();

  Serial.println("time_ms,ax,ay,az,gx,gy,gz");

  unsigned long startTime = millis();
  unsigned long nextSample = startTime;

  while (millis() - startTime < TEST_DURATION) {
    unsigned long now = millis();

    if ((long)(now - nextSample) >= 0) {
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);

      Serial.print(now - startTime);
      Serial.print(",");
      Serial.print(a.acceleration.x, 4);
      Serial.print(",");
      Serial.print(a.acceleration.y, 4);
      Serial.print(",");
      Serial.print(a.acceleration.z, 4);
      Serial.print(",");
      Serial.print(g.gyro.x, 4);
      Serial.print(",");
      Serial.print(g.gyro.y, 4);
      Serial.print(",");
      Serial.println(g.gyro.z, 4);

      nextSample += SAMPLE_INTERVAL;
    }
  }

  beep();
  Serial.println("Test complete.");
}

void loop() {
}
