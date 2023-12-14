# Weather-Station-v3
Arduino project by Emmanuelle Nika Brent, Aidan Razzi Reilly and Clayton "Ken" Sakalousky, building upon previous Pitt Vibrant Media Lab Weather Station module projects.

This weather station module is a compact, portable, and fully functioning weather monitoring system. It can track a variety of metrics that are stored locally and can be accessed remotely. The module is centered around an Arduino Mega, which receives readings from peripheral sensors and processes them to an onboard memory system. The module also acts as a web server which clients can connect to to see the most up-to-date readings, as well as interact with the historic data via line graphs. 

The goal of this project is to be transparent enough for beginners to implement, and for more knowledgable users to make changes to the source code and hardware. The project is open source, and as such all programs and instructions will be made publicly available without cost. 

### Weather data
The sensors used in this project can track a variety of data. The following measurements are used in this project: 

* **Temperature** in Celsius
* **Pressure** in Pa
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

### Components

* Arduino Ethernet Shield
* Adafruit Si7021 Temperature and Humidity Sensor Breakout Board
* Adafruit BMP280 I2C or SPI Barometric Pressure & Altitude Sensor - STEMMA QT
* PM2.5 Air Quality Sensor and Breadboard Adapter Kit - PMS5003
* Davis AeroCone Rain Gauge
* Wind speed and direction sensor
* Arduino LCD
