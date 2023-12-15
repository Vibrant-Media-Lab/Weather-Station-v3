# Weather-Station-v3
Arduino project by Emmanuelle Nika Brent, Aidan Razzi Reilly and Clayton "Ken" Sakalousky, building upon previous Pitt Vibrant Media Lab Weather Station module projects.

This weather station module is a compact, portable, and fully functioning weather monitoring system. It can track a variety of metrics that are stored locally and can be accessed remotely. The module is centered around an Arduino Mega, which receives readings from peripheral sensors and processes them to an onboard memory system. The module also acts as a web server which clients can connect to to see the most up-to-date readings, as well as interact with the historic data via line graphs. 

The goal of this project is to be transparent enough for beginners to implement, and for more knowledgable users to make changes to the source code and hardware. The project is open source, and as such all programs and instructions will be made publicly available without cost. 

## Weather Data
The sensors used in this project can track a variety of data. The following measurements are used in this project: 

* **Temperature** in Celsius
* **Barometric Pressure** in Pa
* **Humidity** in percent
* **Particulate Matter (PM) 2.5** in micrograms per cubic meter
* **Particulate Matter (PM) 10** in micrograms per cubic meter
* **Air Quality Index (AQI)**
* **AQI Level** 
* **Wind Speed** in miles per hour
* **Wind Heading**
* **Rain Fall** in inches per hour
* **Fire Safety Rating**

The above values are measured every hour, and they are used to calculate the following every 24 hours: 
* **Daily High Temperature** in Celsius
* **Daily Low Temperaure** in Celsius
* **Daily Average Pressure** in Pa
* **Daily High Humidity** in percent
* **Daily High Air Quality Index (AQI)**
* **Daily Worst AQI Level**
* **Daily Average Wind Speed** in miles per hour
* **Daily Total Rain Fall** in inches
* **Daily Worst Fire Safety Rating**

The data is logged with timestamps to a CSV file, stored on an SD card.

## Design Overview
### Dependencies/Inclusions
* string.h
* math.h
* SoftwareSerial.h
* Ethernet.h
* SD.h
* ArduinoJson.h (this may not be necessary anymore in the most recent version of the build)
* Wire.h
* SPI.h
* Adafruit_Si7021.h
* Adafruit_BMP280.h
* Adafruit_Sensor.h
* LiquidCrystal_I2C.h
* RTClib.h

### Components
The core unit of this module is an Arduino Mega.

* **Ethernet Shield with MicroSD storage**  
*Model: Ethernet Shield W5100(HanRun HR911105A 17/32)*

* **LCD Display**  
*Model: ADAFRUIT HD44780*

* **System Temperature and Barometric Pressure**  
*Model: ADAFRUIT BMP280*

* **ExternalTemperature and Relative Humidity**  
*Model: ADAFRUIT Si7021*

* **Air Quality Sensor and Breadboard Adapter Kit**  
*Model: ADAFRUIT PMS5003*

* **Anemoeter and Wind Direction**  
*Model: DAVIS ANEMOMETER FOR VANTAGE PRO2*

* **Rain Fall**  
*Model: DAVIS AEROCONE COLLECTOR FOR VANTAGE PRO2*

## Files
### Final Build
The files for the final build can be found in `code\weatherStationClaytonsVersion\`:
* **weatherStationClaytonsVersion.ino:** *The complete Arduino sketch to be uploaded. Collects data from the sensors and stores them to an onboard MicroSD card. Also contains the web server capabilites for handling clients*
* **log.csv:** *The log file which hourly and daily data is stored to with timestamps*
* **index.htm:** *Homepage of the website which shows the current hourly data and the previous day's roundup. Should be stored on the MicroSD card to be accessed and served to clients by the Arduino sketch*
* **graphs.htm:** *Graphs page of the website, allows users to interact with the historic stored data. Should be stored on the MircoSD card to be accessed and served to clients by the Arduino sketch*
* **i_style.css:** *Stylesheet for `index.htm`. Should be stored on the MircoSD card to be accessed and served to clients by the Arduino sketch*
* **g_style.css:** *Stylesheet for `graphs.htm`. Should be stored on the MircoSD card to be accessed and served to clients by the Arduino sketch*
* **index_js.js:** *JavaScript file for `index.htm`, parses the CSV file to display the most recent hourly and daily data, and reloads the page once an hour to update the data. Should be stored on the MircoSD card to be accessed and served to clients by the Arduino sketch*
* **graph_js.js** *JavaScript file for `graphs.htm`, converts the data into arrays to be used as inputs for line graphs. Should be stored on the MircoSD card to be accessed and served to clients by the Arduino sketch*
* **charts.js** *Locally stored JavaScript file of the ApexCharts library, which is used to create the graphs. This library is quite large, so if the weather station is connected to the Internet and can simply import the library, that is recommended. If not, this file should be stored on the MircoSD card to be accessed and served to clients by the Arduino sketch*
* **ppparse.js** *Locally stored JavaScript file of the Papa Parse library, which is used to parse the CSV file to an array of JSON objects that can be used by the other code. This library loads relatively quickly, but it is still recommended to use the version imported from the Internet if possible. If not, this file should be stored on the MircoSD card to be accessed and served to clients by the Arduino sketch*

### Component Tests
The directory `\code\componentTests\` contains subdirectories with Arduino sketches to test the functionality of the various components included in the project individually. 

## Future Improvements
As this is an open source project, anyone is able to use this code to develop their own weather station, and we welcome feedback and suggestions on the current product!
As it stands, there are a few known improvements that can be made to this build: 
* The readings of the rain fall, wind speed, wind heading, and air quality data have not be thoroughly tested, and may require debugging. 
* The code has gone through many iterations, and as such there may be redundant or unnecessary code in the Arduino sketch that can be cleaned up.
