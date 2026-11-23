/*
 * Romberg balance capture — ESP32 + MPU6050
 *
 * Pins: SDA=21, SCL=22, buzzer=25
 * Rate: 50 Hz for 30 s
 *
 * After each test, builds accelerometer CSV (ax,ay,az) and POSTs it to the
 * Next.js web app for review (gyro is omitted for this project):
 *   POST {API_BASE}/api/ingest
 *
 * Edit WIFI_* and API_BASE before flashing.
 * API_BASE must be your computer's LAN IP (not localhost), e.g.:
 *   http://192.168.1.163:3000
 *
 * The website cleans/queues the CSV; you choose whether to save it to Neon.
 * Send any character over Serial to start a test.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ---- edit these before flashing ----
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char *API_BASE = "http://192.168.1.163:3000"; // Mac LAN IP + Next.js port
// ------------------------------------

Adafruit_MPU6050 mpu;

#define SDA_PIN 21
#define SCL_PIN 22
#define BUZZER_PIN 25

const unsigned long TEST_DURATION = 30000;
const unsigned long SAMPLE_INTERVAL = 20; // 50 Hz

void beep(unsigned int count = 1) {
  for (unsigned int i = 0; i < count; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    if (i + 1 < count) {
      delay(100);
    }
  }
}

void connectWifi() {
  Serial.print("Connecting to WiFi ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi failed — tests will still run but POST will fail.");
  }
}

String escapeJson(const String &input) {
  String out;
  out.reserve(input.length() + 64);
  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    if (c == '\\') {
      out += "\\\\";
    } else if (c == '"') {
      out += "\\\"";
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      // skip CR
    } else {
      out += c;
    }
  }
  return out;
}

bool postCsvToWebApp(const String &csvContent, int sampleCount) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("POST skipped: WiFi not connected");
    return false;
  }

  String url = String(API_BASE) + "/api/ingest";
  String body = "{";
  body += "\"durationMs\":30000,";
  body += "\"sampleCount\":";
  body += String(sampleCount);
  body += ",\"csvContent\":\"";
  body += escapeJson(csvContent);
  body += "\"}";

  HTTPClient http;
  http.setTimeout(15000);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  Serial.print("POSTing CSV to ");
  Serial.println(url);
  int code = http.POST(body);
  String response = http.getString();
  http.end();

  Serial.print("HTTP status: ");
  Serial.println(code);
  if (response.length() > 0) {
    Serial.println(response);
  }

  // 202 Accepted = queued for review in the web app
  return code >= 200 && code < 300;
}

void runRombergTest() {
  Serial.println("ROMBERG_TEST_START");
  beep(1);

  String csv = "ax,ay,az\n";
  int sampleCount = 0;

  unsigned long startTime = millis();
  unsigned long nextSample = startTime;

  while (millis() - startTime < TEST_DURATION) {
    unsigned long now = millis();

    if ((long)(now - nextSample) >= 0) {
      if ((long)(now - nextSample) >= (long)SAMPLE_INTERVAL) {
        nextSample = now;
      }

      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);

      // Serial debug still includes time_ms + gyro; uploaded CSV is accel-only
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

      char line[64];
      snprintf(
          line,
          sizeof(line),
          "%.4f,%.4f,%.4f\n",
          a.acceleration.x,
          a.acceleration.y,
          a.acceleration.z);
      csv += line;
      sampleCount++;

      nextSample += SAMPLE_INTERVAL;
    }
  }

  Serial.println("ROMBERG_TEST_END");
  Serial.print("Samples: ");
  Serial.println(sampleCount);

  bool ok = postCsvToWebApp(csv, sampleCount);
  if (ok) {
    Serial.println("CSV queued in web app for review (ax,ay,az).");
    beep(2);
  } else {
    Serial.println("CSV upload failed.");
    beep(4);
  }
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

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  connectWifi();

  Serial.println("MPU6050 ready.");
  Serial.println("Send any character to start the Romberg test.");
}

void loop() {
  if (Serial.available() > 0) {
    while (Serial.available() > 0) {
      Serial.read();
    }
    runRombergTest();
  }
}
