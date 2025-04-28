let express = require('express'); // imports Express framework to create routes and handle requests
let router = express.Router(); // creates a mini router which can be used to define API endpoints(GET,POST)
let latestImageBuffer = null; // this is dynamically updated with latest image from my camera
//const multer = require('multer');
//const storage = multer.memoryStorage();
//const Upload = multer({ storage: storage });
// had to run npm install multer in terminal 

/* Route to receive sensor data (acceleromter & heart rate)*/
router.post('/sensor-data', async function (req, res, next) { //defines post route at "/sensor-data"// async keyword, allows handling of asynchronous operations e.g database queries
    try {
        const { heartrate, x, y, z } = req.body; // Extracts values (heart rate) from the incoming JSON body of the request.

        if (!heartrate || x === undefined || y === undefined || z === undefined) { // Condition to check if  heartrate is missing or null. !accelerometer returns true if the accelerometer data is missing, and !heartrate returns true if heart rate data is missing.
            return res.status(400).json({ error: 'No heart rate data' }); // stops request and sends 400 status code Bad Request error. sends JSON responce with error message
        }

        console.log('Received Sensor Data:', { heartrate, x, y, z }); // Logs the received data to the console for debugging purposes.

        //Process data here

        res.json({ message: 'Accelerometer data received successfully' }); // Sends a successful JSON response back to the client to confirm the data was received correctly.

        /* Part of Try catch statement to handle errors inside async function.
           If an error occurs in the try block, the catch block catches it and
           passes the error to Express’s built-in error handler using next(error).
           Express automatically processes it and sends a standard error response to the client.
        */
    } catch (error) {
        next(error);
    }
});

// Middleware to parse raw JPEG image data for /camera-upload
// This will ensure the body is treated as raw binary data (image/jpeg)
// Middleware to parse incoming JPEG image data for /camera-upload as a raw buffer (up to 10MB in size)
// Sending raw binary and using express.raw() is the smartest choice for an ESP32 sending camera images.
router.use('/camera-upload', express.raw({ type: 'image/jpeg', limit: '10mb' }));

router.post('/camera-upload', async function (req, res) {
    try {
        const imageBuffer = req.body                     // Extracts the raw image buffer data from the request body
        if (!imageBuffer || imageBuffer.length === 0) {  // Checks if no image data was received (empty or undefined)
            return res.status(400).json({ error: 'No image received' });  // Sends a 400 Bad Request error if no image data
        }

        latestImageBuffer = imageBuffer; // Stores the latest image data
        
        console.log(`Received image of this size: ${imageBuffer.length} bytes`); // Logs the size of the received image buffer in bytes for debugging

        res.json({ message: 'Image Received Successfully' });
    } catch (error) {
        next(error);
    }
});
/*
// route to serve the latest image (GET request)
router.get('/camera-latest', async function (req, res) {
    try {
        if (!latestImageBuffer) {
            return res.status(404).json({ error: 'No Image Available' }); // if no image available return a 404
        }
        res.setHeder('Content-type', 'image/jpeg'); // set the content type to JPEG image
        res.send(latestImageBuffer);
    } catch (error) {
        console.error("Error in sending image:", error); // log any potential error
        res.status(500).json({ error: "internal server error" }); // return a generic error if sumthing goes wrong
    }
});
*/
module.exports = router;