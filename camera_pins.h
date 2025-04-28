/*
 Keeping hardware configurations like GPIO pins in a separate .h file helps keep the main sketch clean and organized.
 Allows for easy changes to the hardware setup without affecting the main logic. It also makes the code more reusable
 across different projects and easier to maintain or debug.
*/
#define CAMERA_MODEL_ESP_EYE   // Define the model as ESP_EYE
#define PWDN_GPIO_NUM  -1      // Power down pin, not used here (-1 means no pin)
#define RESET_GPIO_NUM -1      // Reset pin, not used here (-1 means no pin)
#define XCLK_GPIO_NUM  4       // External clock pin (GPIO 4)
#define SIOD_GPIO_NUM  18      // SCCB Data pin (GPIO 18)
#define SIOC_GPIO_NUM  23      // SCCB Clock pin (GPIO 23)

#define Y9_GPIO_NUM    36  // Data pins for camera (GPIOs 36–39)
#define Y8_GPIO_NUM    37
#define Y7_GPIO_NUM    38
#define Y6_GPIO_NUM    39
#define Y5_GPIO_NUM    35
#define Y4_GPIO_NUM    14
#define Y3_GPIO_NUM    13
#define Y2_GPIO_NUM    34
#define VSYNC_GPIO_NUM 5   // Vertical sync pin (GPIO 5)
#define HREF_GPIO_NUM  27  // Horizontal reference pin (GPIO 27)
#define PCLK_GPIO_NUM  25  // Pixel clock pin (GPIO 25)

#define LED_GPIO_NUM 22    // LED pin (GPIO 22) 
