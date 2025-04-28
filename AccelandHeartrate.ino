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

#include <Wire.h>                    // Enables I2C communication
#include <WiFi.h>                    // Enables WIfi capabilities
#include <HTTPClient.h>              // Library for sending HTTP requests (GET, POST) to web server
#include <Adafruit_MMA8451.h>        // Library with extra support for MMA8451 accelerometer (sensor)
#include <Adafruit_Sensor.h>         // Library used for Adafruit sensors
#include "MAX30100_PulseOximeter.h"  // Library for MAX30100 (Heart rate and oxygen)

#define SDA_I2C 21  // defining SDA  pin 21    Default pins of the esp32 for I2C communication
#define SCL_I2C 22  // defining SCL  pin 22
//#define REPORTING_PERIOD_MS 1000
#define HTTP_TIMEOUT 5000  // Sets HTTP timeout to 5000ms (5 seconds) to avoid hanging if server unresponsive // to prevent blocking if this the bug.

Adafruit_MMA8451 mma = Adafruit_MMA8451();  // Create MMA8451 accelerometer object to access sensor functions like reading acceleration.

float GtoMeterSQConversion(int rawValue);  // Function prototype, takes one int parameter (rawvalue) and returns a float.

// WiFi details
const char* ssid = "Kenphone";
const char* password = "tcsv6hi7bsdd548";

/*server url I needed to replace with my server url.
  constant pointer to a character array which holds the server url  
  where sensor data will be sent, contans the IP address of server,
  port number server is listening on for incoming connectiions and
  the API endpoint where images are been sent
*/
const char* serverUrl = "http://192.168.148.99:8000/api/sensor-data"; // server url with IP address, Port, API endpoint


/*
 Creates an object of the PulseOximeter class, which serves as a high-level interface to the sensor.
 PulseOximeter is the higher level interface to the sensor
 it offers:
  * beat detection reporting
  * heart rate calculation
  * SpO2 (oxidation level) calculation
 Creates object of the PulseOximeter class.
 pox is the object (instance) created of the PulseOximeter class.
 pox will be used to call the fuctions from the PulseOximeter class.
 examples pox.getHeartRate() or pox.begin().
*/
PulseOximeter pox;  // Class and object.


//unsigned long lastTimeSend = 0;
//const unsigned long sendInterval = 2000;  // 1 second before sending data

/* 
Stores the timestamp of the last time data was sent.
Used later to check if enough time (e.g., 5 seconds) has 
passed before sending new data to server.
*/
unsigned long lastSendTime = 0;  // Varible type unsigned long, Named lastsendTime, initialised to zero. Used when Tracking the last time data was sent
/*
 Holds the current time in milliseconds since the program started, updated each loop.
 Used later to track time intervals for sending data to server.
*/
unsigned long currentMillis = 0;  // Varible type unsigned long, Named currentMillis, To check time for sending data
* /
  //int hr_counter = 0;               // Counter for sensor updates

  /*
 Callback function  (registered below) fired when a pulse is detected
 A callback function is a function passed as an argument to another function
 and is executed when a specific event or condition occurs.
 */
  void onBeatDetected() {   //Function definition
  Serial.println("Beat!");  // prints beat in terminal
}


void setup() {
  Serial.begin(115200);          // Initializes serial communication at 115200 baud rate.
  Wire.begin(SDA_I2C, SCL_I2C);  // Initializes I2C communication with default SDA(Serial data) and SCL(serial clock) pins
  //Serial.println("Scanning for I2C devices..");

  Serial.print("Initializing pulse oximeter..");
  // Initialize the PulseOximeter instance
  // Failures are generally due to an improper I2C wiring, missing power supply
  // or wrong target chip

  /*
 Connects to Wi-Fi using the provided SSID and password. 
 Waits until the connection is established, printing status updates.
*/
  WiFi.begin(ssid, password);                  // Starts Wi-Fi connection with SSID and password
  while (WiFi.status() != WL_CONNECTED) {      // Waits until connected
    delay(1000);                               // Waits 1 second before checking again
    Serial.println("Connecting to Wifi....");  // Prints status while waiting
  }
  Serial.println("Connected to Wifi...");  // Prints once connected

  /*
 Attempts to initialize the pulse oximeter sensor. 
 If initialization fails pox.begin() returns false, prints "FAILED" and halts the program with an infinite loop.
 Otherwise pox.begin() (returns true), prints "SUCCESS".
 Infinite loop prevents running faulty logic or crash later in logic
*/
  if (!pox.begin()) {             // Checks if the Pulse Oximeter sensor initialization fails
    Serial.println("FAILED");
    for (;;)                      // Infinite loop to halt execution
      ;
  } else {
    Serial.println("SUCCESS");
  }

  /*
 Initializes the MMA8451 accelerometer sensor. If initialization fails mma.begin() returns false,
 prints message "Couldnt start" and halts the program with an infinite loop.
 If successful mma.begin() returns false, prints message "MMA8451 found!".
*/
  Serial.println("Adafruit MMA8451 test!");   // Prints a message for start of "Adafruit MMA8451 test!"
  if (!mma.begin()) {                         // Checks if the sensor initialization fails (mma.begin() returns false)
    Serial.println("Couldnt start");
    while (1)                                 // Infinite loop to stop the program if sensor init fails
      ;
  }
  Serial.println("MMA8451 found!");


  /*
  Accelerometer range set to +-2G (MMA8451 has ranges: +-2G, +-4G, +-8G) G = Gravity acceleration
  MMA8451 => In +-2G mode 14-bit Sensor(-2^13 to 2^13 -1 => values -8192 to +8191)
  Each axis (x, y, z) gives a raw value in this range (-8192 to +8191)
  1G = 9.80665 m/s^2
  Sensor outputs 4096 counts per G
  +-2G mode => 1G 8192/2 = 4096
  Acceleration (m/s^2) => (Raw value/4096) * 9.80665
 */

  // +-2g mode more sensitive to smaller movements in acceleration. Better for fall detection and Shaking.
  // +-4/8g best for measuring higher accelerations but less sensitive to smaller movements in acceleration.
  mma.setRange(MMA8451_RANGE_2_G);  // setting sensor to 2g range.
  /*
  mma.getRange returns an integer value that represents the range setting (0 => +-2G, 1 => +-4G, 2 => +-8G).
  2 << mma.getRange() use's bitwise left shift (<<) operator
  Shifts binary representaion of a number to the left by set number of positions
  (X << N) X left by N number of bits or X * 2.
*/
  Serial.print("Range = ");
  Serial.print(2 << mma.getRange());
  Serial.println("G");

  /*Measuring the timing between samples for debugging*/
  //long sampleTime = measureSampleTime();
  //Serial.print("Time between Samples: ");
  //Serial.print(sampleTime);
  //Serial.println(" millisSeconds");


  // The default current for the IR LED is 50mA and it could be changed
  // by uncommenting the following line. Check MAX30100_Registers.h for all the
  // available options.
  // pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

  // Register a callback for the beat detection
  // Registers the onBeatDetected function to be called automatically when a heartbeat is detected.
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {
  currentMillis = millis(); // Assigns the return value of millis() (time in ms since program start) to currentMillis
  pox.update();  // Updates pulse oximeter readings and checks for new data (e.g., heartbeats)
  //hr_counter++; // counter was use instead of delay() blocking function

  mma.read();  // Reads new acceleration data from the MMA8451 sensor
  float x = GtoMeterSQConversion(mma.x);  // Converts raw X-axis acceleration from G to m/s²
  float y = GtoMeterSQConversion(mma.y);  // Converts raw Y-axis acceleration from G to m/s²
  float z = GtoMeterSQConversion(mma.z);  // Converts raw Z-axis acceleration from G to m/s²

  // %lu = unsigned long (currentMillis), %.3f = float with 3 decimal places (x, y, z)
  Serial.printf("TS: %lu\tX: %.3f\tY: %.3f\tZ: %.3f M/S^2\n", currentMillis, x, y, z);// Prints timestamp and converted acceleration values for X, Y, Z axes to the Serial Monitor

  float heartrate = pox.getHeartRate(); // Retrieves the current heart rate (in BPM) from the heart rate sensor
  float spo2 = pox.getSpO2();           // Retrieves the current oxygen saturation level (SpO2 %) from the pulse oximeter

  //Serial.print("Sensor Update Count: ");
  //Serial.println(hr_counter);

  Serial.print("Heart Rate: "); // prints Heart rate
  Serial.println(heartrate);    // Prints the current heart rate (in BPM)
  Serial.print("SpO2: ");       // Prints label for oxygen saturation level
  Serial.println(spo2);         // Prints the current SpO2 level (in %)
/*
 This if statement was to prevent overloading the server with requests
 possible issue with sensor freezing.millis() at start of loop returns the current time in milliseconds
 since the program started.currentMillis = millis(); updates currentMillis with the current time.
 currentMillis holds the current time, lastSendTime stores the time when the data was last sent.
 Then, the if statement compares the current time (currentMillis) with the last sent time (lastSendTime)
 to check if 5 seconds have passed. If it's 5000ms (5 seconds) or more the data is sent to the server.
 Once data sent currentmillis gets assigned to lastSendTime and loop continues till data can be sent again.
 */
  if (currentMillis - lastSendTime >= 5000) {      // Check if 5 seconds have passed since last data send
    if (heartrate > 0 && heartrate < 200) {        // Ensure heart rate is within a valid range (0-200 BPM)
      Serial.println("Sending Data ....");         // Print message before sending data
      sendToServer(heartrate, x, y, z);            // Send heart rate and accelerometer data (x, y, z) to server
    } else {
      Serial.println("Invalid data, Heart rate out of range or zero.");
    }
    lastSendTime = currentMillis;        // Assign the current time to lastSendTime after sending data
  }

  //delay(500);  // blocking dont use
}
// Function for conversion of rawValues to meters per second squared 1G = 9.80665
float GtoMeterSQConversion(int rawValue) {
  return (rawValue / 4096.0) * 9.80665;
}

/* Function to send heart rate and accelerometer data to the server (non-blocking)
   Function will send data to the server without blocking other tasks.
   HTTPClient is used for HTTP requests. 
   Creating an object of the class HTTPClient http; allows the use of methods and properties
   provided by the HTTPClient class to make HTTP requests. The object is used to initialize
   and manage the HTTP connection, Set headers, timeouts and senddata to the server and handle
   the response from the server. 
*/
void sendToServer(float heartrate, float x, float y, float z) { // parameters to be sent and type (floats)
  HTTPClient http;                                              // Create an HTTP Client object for making HTTP requests
  http.setTimeout(HTTP_TIMEOUT);                                // Set the timeout duration for the HTTP request, possibly help issue with freezing sensor

  // Start the HTTP request to the server using the specified URL
  http.begin(serverUrl);  // Initiates the HTTP connection to the server URL
  
  /*
  Prepares the HTTP request by setting the appropriate header for sending data in JSON format.
  Tells the server the data being sent is in JSON format.REST APIs commonly use JSON as the data
  format for requests and responses. Specifying this header tells the server that the body of the
  HTTP request will contain JSON-formatted data (string). The server will then know how to properly
  parse and interpret the data sent in the request. Parsing allows the server to extract the actual
  values from raw JSON data and convert them to usable varibles and objects which the server can work
  such as save to a data base or send a response to client
  */
  http.addHeader("Content-Type", "application/json"); // Adds a header to specify that the data being sent is in JSON format

  // Ready JSON payload with the heartrate data and Accelerometer data
  String payload = "{\"heartrate\": " + String(heartrate, 2) + ", \"x\": " + String(x, 2) + ", \"y\": " + String(y, 2) + ", \"z\": " + String(z, 2) + "}";  // changed so it is a float sent ie: two decimal points //, \"spo2\": " + String(spo2, 2) + "
  //payload.replace(",", ".");                                         // Ensures decimal uses a dot, not a comma

  Serial.print("Sending JSON Payload: ");
  Serial.println(payload);  // Debugging step printing payload



  // Send the post request
  int httpResponseCode = http.POST(payload); // Sends the HTTP POST request with the payload (data) to the server

  // Check for response

/*
  HTTP response code returned by the server will be > 0 meaning success response(HTTP 200) or < 0 which 
  means an error response(HTTP 404 or 500). 
*/
  if (httpResponseCode > 0) {                               // Checks if the server response is greater than 0, indicating success.
    Serial.print("Data sent succesfully, Response code: ");
    Serial.println(httpResponseCode);                       // Print the success message and the HTTP response code
  } else {                                                  // If the response code is not positive, there was an error sending data
    Serial.print("Error sending data, Response code: ");
    Serial.println(httpResponseCode);                       // Print the error message and the HTTP response code
  }

  
  http.end();  // end the http connection
}
