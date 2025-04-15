/*
Arduino-MAX30100 oximetry / heart rate integrated sensor library
Copyright (C) 2016  OXullo Intersecans <x@brainrapers.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#include "MAX30100_PulseOximeter.h"

#define SDA_I2C 21  // defining SDA  pin 21    Default pins of the esp32 for I2C communication
#define SCL_I2C 22  // defining SCL  pin 22
#define REPORTING_PERIOD_MS 1000
#define HTTP_TIMEOUT 5000

Adafruit_MMA8451 mma = Adafruit_MMA8451();

float GtoMeterSQConversion(int rawValue);  //Function prototype
//long measureSampleTime();

//wifi details
const char* ssid = "Kenphone";
const char* password = "tcsv6hi7bsdd548";

//server url i needed to replace with my servet url
const char* serverUrl = "http://192.168.97.99:8000/api/sensor-data";

// PulseOximeter is the higher level interface to the sensor
// it offers:
//  * beat detection reporting
//  * heart rate calculation
//  * SpO2 (oxidation level) calculation
PulseOximeter pox;


//unsigned long lastTimeSend = 0;
//const unsigned long sendInterval = 2000;  // 1 second before sending data

unsigned long lastSendTime = 0;   // Track the last time data was sent
unsigned long currentMillis = 0;  // To check time for sending data
int hr_counter = 0;               // Counter for sensor updates

//uint32_t tsLastReport = 0;

// Callback (registered below) fired when a pulse is detected
void onBeatDetected() {
  Serial.println("Beat!");
}


void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_I2C, SCL_I2C);
  //Serial.println("Scanning for I2C devices..");

  Serial.print("Initializing pulse oximeter..");

  // Initialize the PulseOximeter instance
  // Failures are generally due to an improper I2C wiring, missing power supply
  // or wrong target chip


  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wifi....");
  }
  Serial.println("Connected to Wifi...");


  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;)
      ;
  } else {
    Serial.println("SUCCESS");
  }


  Serial.println("Adafruit MMA8451 test!");
  if (!mma.begin()) {
    Serial.println("Couldnt start");
    while (1)
      ;
  }
  Serial.println("MMA8451 found!");
  /*
  Accelerometer range set to +-2G (MMA8451 has ranges: +-2G, +-4G, +-8G) G = Gravity acceleration
  MMA8451 => In +-2G mode 14-bit Sensor(values -8192 to +8191)
  Each axis measurement range from -8192 to +8191
  1G = 9.80665 m/s^2
  Sensor outputs 4096 counts per G
  +-2G mode => 1G 8192/2 = 4096
  Acceleration (m/s^2) => (Raw value/4096) * 9.80665
 */
  mma.setRange(MMA8451_RANGE_2_G);
  /*
  mma.getRange returns an integer value that represents the range setting (0 => +-2G, 1 => +-4G, 2 => +-8G).
  2 << mma.getRange() use's bitwise left shift (<<) operator
  Shifts binary representaion of a number to the left by set number of positions
  (X << N) X left by N number of bits or X * 2.
*/

  Serial.print("Range = ");
  Serial.print(2 << mma.getRange());
  Serial.println("G");

  /*Measuring the timing between samples*/

  //long sampleTime = measureSampleTime();
  //Serial.print("Time between Samples: ");
  //Serial.print(sampleTime);
  //Serial.println(" millisSeconds");


  // The default current for the IR LED is 50mA and it could be changed
  //   by uncommenting the following line. Check MAX30100_Registers.h for all the
  //   available options.
  // pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

  // Register a callback for the beat detection
  pox.setOnBeatDetectedCallback(onBeatDetected);
}
/*
void loop() {
  // Sensor will update with every loop
  pox.update();

  if (millis() - lastTimeSend >= sendInterval) {
      float heartrate = pox.getHeartRate();  // get heartrate readings
      float spo2 = pox.getSpO2();            //getting ox readings

      //if (heartrate > 0) {
      //pox.update();
      Serial.print("Heart rate:");
      Serial.print(heartrate),
      Serial.print("bpm / SpO2:");
      Serial.print(spo2);
      Serial.println("%");

      sendToServer(heartrate);
      //} else {
      //Serial.println("Invalid, data not sent to server");

      lastTimeSend = millis();
    }
}
*/
void loop() {
  currentMillis = millis();
  pox.update();
  hr_counter++;

  mma.read();
  float x = GtoMeterSQConversion(mma.x);
  float y = GtoMeterSQConversion(mma.y);
  float z = GtoMeterSQConversion(mma.z);

  Serial.printf("TS: %lu\tX: %.3f\tY: %.3f\tZ: %.3f M/S^2\n", currentMillis, x, y, z);

  float heartrate = pox.getHeartRate();
  float spo2 = pox.getSpO2();

  Serial.print("Sensor Update Count: ");
  Serial.println(hr_counter);

  Serial.print("Heart Rate: ");
  Serial.println(heartrate);
  Serial.print("SpO2: ");
  Serial.println(spo2);

  if (currentMillis - lastSendTime >= 5000) {
    if (heartrate > 0 && heartrate < 200) {
      Serial.println("Sending Data ....");
      sendToServer(heartrate, x, y, z);
    } else {
      Serial.println("Invalid data, Heart rate out of range or zero.");
    }
    lastSendTime = currentMillis;
  }

  //delay(500);  // blocking dont use
}
// Function for conversion of rawValues to meters per second squared 1G = 9.80665
float GtoMeterSQConversion(int rawValue) {
  return (rawValue / 4096.0) * 9.80665;
}

//, float spo2
// Function to send heartrate to server (non blocking)
void sendToServer(float heartrate, float x, float y, float z) {
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT);  // set time out to prevent blocking

  //Start the http request to the server
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  // Ready JSON payload with the heartrate data
  String payload = "{\"heartrate\": " + String(heartrate, 2) + ", \"x\": " + String(x, 2) + ", \"y\": " + String(y, 2) + ", \"z\": " + String(z, 2) +  "}";  // changed so it is a float is sent ie two decimal points //, \"spo2\": " + String(spo2, 2) + "
  //payload.replace(",", ".");                                         // Ensures decimal uses a dot, not a comma

  Serial.print("Sending JSON Payload: ");
  Serial.println(payload);  // Debugging step printing payload



  // Send the post request
  int httpResponseCode = http.POST(payload);

  // Check for response
  if (httpResponseCode > 0) {
    Serial.print("Data sent succesfully, Response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Error sending data, Response code: ");
    Serial.println(httpResponseCode);
  }

  // end the http connection
  http.end();
}
