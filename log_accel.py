# Had been using this script to run tests. Script saved data to a CSV File (Comma-Separated -Values text file), 
# which could be opened by Excel. Problem was script saved the data in one column and I couldn't plot the data
# correctly, as in I needed each axis and the time stamp in seperate columns. 

import serial   # Allows communication with serial devices (Connects pythin to my ESP32 via USB - com port)
import time     # Used to generate timestamps for the log file (Creat timestamped file name)

#ESP32 COM port
# Script communicates to my ESP32 on COM5 at 115200 Baud rate
SERIAL_PORT = "COM5" # Set to com port of my connection for ESP32 
BAUD_RATE = 115200   # To match ESP32 Baud rate
LOG_FILE = f"accel_log_{time.strftime('%Y%m%d-%H%M%S')}.csv"  # Creates log file with time and date (timestamped filename) year-4,month-2,day-2,hour-24,minutes,seconds

# Open serial connection
# Opens connection to my ESP32 and sets timeout to 1 second - prevents script from freezing if no data is received
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) 

# Open file for writing
with open(LOG_FILE, "w") as file:  # opens CSV file, the "w" sets it in write mode
    print(f"Logging data to {LOG_FILE}...")  # prints message to show logging has started
    while True:
        try:
            line = ser.readline().decode().strip()  # ser.readline()::Read line from serial(ESP32) / decode()::converts bytes to text / strip()::cleans data by to removing any spaces or newlines
            if line:    # if line not empty
                print(line)  # Prints to terminal so can be observed
                file.write(line + "\n")  # Saves to a CSV file
        except KeyboardInterrupt:  # Allows script to be stopped by pressing Ctrl+c
            print("\nLogging stopped by user.")  # prints to terminal
            break       # Exit loop

ser.close()   # Close's serial connection


