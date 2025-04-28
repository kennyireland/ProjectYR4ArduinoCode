#include <WiFi.h>        // Library to connect the ESP32 to a Wi-Fi network
#include <HTTPClient.h>  // Library to send HTTP requests from the ESP32
#include "esp_camera.h"  // Library to control the ESP32-CAM camera module
#include "camera_pins.h" // Library that defines the camera pin configuration

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
/* 
  Has PSRAM which is extra memory added seperate to normal RAM.
  Added to prevent running out of memory when storing images or processing large data.
*/
#define CAMERA_MODEL_ESP_EYE  // Selects ESP-EYE camera model 

// ===========================
// WiFi credentials
// ===========================
const char *ssid = "Kenphone";
const char *password = "tcsv6hi7bsdd548";
/*  
  constant pointer to a character array which holds the server url  
  where sensor data will be sent, contans the IP address of server,
  port number server is listening on for incoming connectiions and
  the API endpoint where images are been sent
*/
const char* cameraUrl = "http://192.168.148.99:8000/api/camera-upload"; // server url with IP, Port, API endpoint

//void startCameraServer();
//void setupLedFlash(int pin);

void setup() {
  Serial.begin(115200); // Set baud rate 
  Serial.setDebugOutput(true); // Enable detailed debug messages on the Serial Monitor
  Serial.println();

  camera_config_t config;               // Create a camera configuration structure
  config.ledc_channel = LEDC_CHANNEL_0; // LEDC channel for generating XCLK (camera clock)
  config.ledc_timer = LEDC_TIMER_0;     // LEDC timer for XCLK
  config.pin_d0 = Y2_GPIO_NUM;   // Data pin 0
  config.pin_d1 = Y3_GPIO_NUM;   // Data pin 1
  config.pin_d2 = Y4_GPIO_NUM;   // Data pin 2
  config.pin_d3 = Y5_GPIO_NUM;   // Data pin 3
  config.pin_d4 = Y6_GPIO_NUM;   // Data pin 4
  config.pin_d5 = Y7_GPIO_NUM;   // Data pin 5
  config.pin_d6 = Y8_GPIO_NUM;   // Data pin 6
  config.pin_d7 = Y9_GPIO_NUM;   // Data pin 7
  config.pin_xclk = XCLK_GPIO_NUM;  // XCLK pin (external clock to the camera)
  config.pin_pclk = PCLK_GPIO_NUM;  // PCLK pin (pixel clock output from camera)
  config.pin_vsync = VSYNC_GPIO_NUM;// VSYNC pin (vertical sync signal)
  config.pin_href = HREF_GPIO_NUM;  // HREF pin (horizontal sync signal)
  config.pin_sccb_sda = SIOD_GPIO_NUM; // SCCB data line (camera control bus)
  config.pin_sccb_scl = SIOC_GPIO_NUM; // SCCB clock line (camera control bus)
  config.pin_pwdn = PWDN_GPIO_NUM;     // Power down pin (optional)
  config.pin_reset = RESET_GPIO_NUM;   // Reset pin (optional)
  config.xclk_freq_hz = 20000000;      // Frequency of XCLK (20 MHz)
  config.frame_size = FRAMESIZE_UXGA;  // Frame size (UXGA 1600x1200 resolution)
  config.pixel_format = PIXFORMAT_JPEG;  // Set pixel format to JPEG (good for streaming)
  //config.pixel_format = PIXFORMAT_RGB565;  // Alternative format for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY; // Only grab frame buffer when empty
  config.fb_location = CAMERA_FB_IN_PSRAM;   // Store frame buffer in PSRAM
  config.jpeg_quality = 12;   // JPEG quality (lower is better quality, range 0-63)
  config.fb_count = 1;        // Number of frame buffers

 // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
 // for larger pre-allocated frame buffer.
 // if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  /*} else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }*/

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);      // Set GPIO13 as input with internal pull-up (for buttons on ESP-EYE)
  pinMode(14, INPUT_PULLUP);      // Set GPIO14 as input with internal pull-up (for buttons on ESP-EYE)
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err); // Print error code if initialization fails
    return;                                                   // Stop further execution                
  }

  sensor_t *s = esp_camera_sensor_get();                      // Get pointer to the camera sensor settings
  
  // initial sensors are flipped vertically and colors are a bit saturated
  // Adjust sensor settings if the camera model is OV3660
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back(image vertically)
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the colour saturation
  }
  // drop down frame size for higher initial frame rate 
  // Lower frame size if using JPEG for faster capture
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }
/*
#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif
*/
  WiFi.begin(ssid, password); // Start connecting to Wi-Fi with SSID and password
  WiFi.setSleep(false); // Disable Wi-Fi sleep mode for faster response

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {  // Wait until Wi-Fi is connected
    delay(500);
    Serial.print(".");    // Print dots while waiting
  }
  Serial.println("");    
  Serial.println("WiFi connected");

  //startCameraServer(); // (Optional) Start camera server if needed

  Serial.print("ESP32 connected with IP address: 'http://");
  Serial.print(WiFi.localIP());       // Print the local IP address
  //Serial.println("' to connect");   // (Optional) Finish the IP display with a message
}

void loop() {
  // Do nothing. Everything is done in another task by the web server.
  captureImageAndSend(); // Call the function to capture an image and send it to the server.
  delay(10000);          // Wait for 10 seconds before calling the function again. (Pauses the ESP32 for 10 seconds) could have used millis to keep ESP32 (Esp-Eye) responsive.
}

/*
  esp_camera_fb_get() is the function call to the function that grabs the latest image from the
  camera and returns a pointer to the frame buffer(fb). The frame buffer(fb) now contains the image data.
  The frame buffer is an area of memory that temporarily holds the image data. In ESP32 camera modules,
  the raw image(like JPEG) is stored in the frame buffer before being processed, sent over the network
  or saved somewhere. The pointer fb got from esp_camera_fb_get() points directly to this memory block
  which contains the image. Using pointer fb allows access to the saved image in the memory block.
*/
// Function to capture an image and send it to the backend server
void captureImageAndSend() {
  camera_fb_t* fb = esp_camera_fb_get();      // Capture an image and get the frame buffer pointer
  if (!fb) {                                  // Check if the capture failed (no frame)
    Serial.println("Camera capture failed");  // Print error message if capture failed
    return;                                   // Exit the function if no frame is captured 
  }

/*   
  Creating an object named 'http' from the HTTPClient class.
  This allows the created 'http' object to use methods like .begin() to start the connection.
*/
  HTTPClient http;                              // Create an object named 'http' from the HTTPClient class
  http.begin(cameraUrl);                        // Start a connection to the backend server Camera upload endpoint
  http.addHeader("Content-Type", "image/jpeg"); // Set the content type to JPEG image
  
/*
  http.POST() is a method from the HTTPClient class that sends an HTTP POST request to the server.
  fb stands for the frame buffer so fb->buf is the image data that was captured by the camera.
  It points to the buffer containing the JPEG image data. fb->len is the length of the data
  (how many bytes of image data are being sent). Post method sends the image data(fb->buf) as the
  body of the POST request to the server at the cameraUrl.The server then processes the image and
  returns a response, which is stored in httpResponseCode varible.
  
*/
  int httpResponseCode = http.POST(fb->buf, fb->len);  // Send captured image to server and varible(httpResponseCode) to store response code.


/*
 HTTP response code returned by the server will be > 0 meaning success response(HTTP 200) or < 0 which 
 means an error response(HTTP 404 or 500). 
*/
  if (httpResponseCode > 0) {                                 // Checks if the server response is greater than 0, indicating success. 
    Serial.print("Image sent succesfully, Response code: ");  // Prints success message 
    Serial.println(httpResponseCode);                         // Print server response if successful (success code)
  } else {
    Serial.print("Error sending image, Response code: ");     // Prints error message  
    Serial.println(httpResponseCode);                         // Print server response if sending failed (error code)
  }

  http.end();                // Close the HTTP connection
/*
  Returns the captured image frame buffer(fb) back to the camera driver so camera can reuse the memory.
  for the next image capture instead of keeping old data and wasting memory. Frees up memory and prevents
  memory leaks. The camera driver is the software that makes the camera on the ESP32 work properly.
*/
  esp_camera_fb_return(fb);  
}
