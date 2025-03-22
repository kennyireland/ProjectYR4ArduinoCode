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
#include <Wifi.h>
#include <HTTPCLIENT.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS     1000

//wifi details
const char* ssid = "Kenphone";
const char* password = "tcsv6hi7bsdd548";

//server url i need to replace with my servet url or ip
const char* serveUrl = "http://172.24.32.1:8000/sensor-data";

// PulseOximeter is the higher level interface to the sensor
// it offers:
//  * beat detection reporting
//  * heart rate calculation
//  * SpO2 (oxidation level) calculation
PulseOximeter pox;

uint32_t tsLastReport = 0;

// Callback (registered below) fired when a pulse is detected
void onBeatDetected()
{
    Serial.println("Beat!");
}

void setup()
{
    Serial.begin(115200);

    Serial.print("Initializing pulse oximeter..");

    // Initialize the PulseOximeter instance
    // Failures are generally due to an improper I2C wiring, missing power supply
    // or wrong target chip
    if (!pox.begin()) {
        Serial.println("FAILED");
        for(;;);
    } else {
        Serial.println("SUCCESS");
    }

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to Wifi....");
    }
    Serial.println("Connected to Wifi...");

    // The default current for the IR LED is 50mA and it could be changed
    //   by uncommenting the following line. Check MAX30100_Registers.h for all the
    //   available options.
    // pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

    // Register a callback for the beat detection
    pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop()
{
    // Make sure to call update as fast as possible
    pox.update();

    // Asynchronously dump heart rate and oxidation levels to the serial
    // For both, a value of 0 means "invalid"
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        Serial.print("Heart rate:");
        Serial.print(pox.getHeartRate());
        Serial.print("bpm / SpO2:");
        Serial.print(pox.getSpO2());
        Serial.println("%");

        tsLastReport = millis();

        // send heart rate data to server
        sendToServer(heartRate);
    }


  delay(1000); //wait for 1 second before reading again

}
//function to send heartrate to server 

void sendToServer(float heartRate){
  HTTPClient http;

  // start the http request to the server
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  //prepare JSON payload withthee heartrate data
  String payload = "{\"heartrate\": " + String(heartrate) + "}";

  //send the post request
  int httpResponceCode = http.POST(payload);

  //check for responce 
  if (httpResponceCode > 0) {
    Serial.print("Data sent succesfully, Responce code: ");
    Serial.println(httpResponceCode);
  }else {
    Serial.print("Error sending data, Responce code: ");
    Serial.println(httpResponceCode);
  }

    // end the http connection
    http.end();
}



















