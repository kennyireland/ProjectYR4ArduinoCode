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
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS 1000

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
int hr_counter = 0;

//uint32_t tsLastReport = 0;

// Callback (registered below) fired when a pulse is detected
void onBeatDetected() {
  Serial.println("Beat!");
}


void setup() {
  Serial.begin(115200);

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

  // The default current for the IR LED is 50mA and it could be changed
  //   by uncommenting the following line. Check MAX30100_Registers.h for all the
  //   available options.
  // pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

  // Register a callback for the beat detection
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {
  // Make sure to call update as fast as possible
  pox.update();

  hr_counter++;  //increment counter every loop
  Serial.println(hr_counter);

  // Asynchronously dump heart rate
  // For both, a value of 0 means "invalid"
  //if (millis() - tsLastReport > REPORTING_PERIOD_MS) {  // millis time since Arduino started, RPM checks if 1sec has passed
  if (hr_counter >= 1000) {
    float heartrate = pox.getHeartRate();  // get heartrate readings
    float spo2 = pox.getSpO2();            //getting ox readingss
    if (heartrate > 0) {
      //pox.update();
      Serial.print("Heart rate:");
      Serial.print(pox.getHeartRate());
      Serial.print(heartrate),
      Serial.print("bpm / SpO2:");
      //Serial.print(pox.getSpO2());
      Serial.print(spo2);
      Serial.println("%");

      //tsLastReport = millis();  // updates the time stamp

      //add counter instead of 1000 delay
      //255 but 256 use int
      //hr_counter = 0;
      Serial.print("Sending Data ....");
      Serial.println(heartrate);

      Serial.println("Call sending to server...");
      //Send heart rate data to server
      sendToServer(heartrate);
      
    } else {
      Serial.println("Invalid data, Heart is 0, not sent to server");
    }
    hr_counter = 0;
  }
}


// Function to send heartrate to server
void sendToServer(float heartrate) {
  HTTPClient http;

  //Start the http request to the server
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  // Ready JSON payload with the heartrate data
  String payload = "{\"heartrate\": " + String(heartrate, 2) + "}";  // changed so it is a float is sent ie two decimal points
  payload.replace(",", ".");                                         // Ensures decimal uses a dot, not a comma

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

/*
  // Check if it's time to send data to the server
  if (millis() - lastSendTime >= sendInterval) {
    // Calculate and send data if enough readings have been accumulated
    if (hr_counter >= numReadings) {
      float avgHr = hr_total / numReadings;

      Serial.print("Averaged Heart Rate: ");
      Serial.print(avgHr);
      Serial.print(" bpm");

      Serial.println("Sending data to server...");
      sendToServer(avgHr);

      // Reset the counter and total after sending
      hr_counter = 0;
      hr_total = 0;
    }

    // Update the last time the data was sent
    lastSendTime = millis();
  }

  // Other non-blocking tasks can go here if needed
}*/

/*
unsigned long lastSendTime = 0;  // Track the last time data was sent
unsigned long currentMillis = 0; // To check time for sending data
int hr_counter = 0;  // Counter for sensor updates

void loop() {
  currentMillis = millis();  // Get the current time
  pox.update();  // Make sure to call this as often as possible to update sensor

  hr_counter++;  // Increment counter for every loop cycle
  Serial.print("Sensor Update Count: ");
  Serial.println(hr_counter);  // Print the counter value to monitor updates

  // Print the heart rate and SpO2
  float heartrate = pox.getHeartRate();
  float spo2 = pox.getSpO2();
  
  // Print the heart rate and SpO2 to serial for monitoring
  Serial.print("Heart Rate: ");
  Serial.println(heartrate);
  Serial.print("SpO2: ");
  Serial.println(spo2);

  // Only send data periodically, e.g., every 5 seconds
  if (currentMillis - lastSendTime >= 5000) {
    if (heartrate > 0 && heartrate < 200) {  // Valid heart rate
      Serial.println("Sending Data ....");
      sendToServer(heartrate);  // Send heart rate data to server
    } else {
      Serial.println("Invalid data, Heart rate out of range or zero.");
    }

    lastSendTime = currentMillis;  // Update last send time
  }
}
*/