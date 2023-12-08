/**
 * Arduino Weather Station
 * Demonstration Build: 17 November 2023
 * 
 * Funtionality -
 *  - Standard sensors
 *  - SD write
 *  - Create web server
 *  - Handle server clients
 */
// General Includes //
#include <string.h>
#include <math.h>
#include <SoftwareSerial.h>
#include <Ethernet.h>
#include <SD.h>
#include <ArduinoJson.h>

// Component Includes //
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Si7021.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"

// Component Declarations //
SoftwareSerial airQualitySerial(14, 15);

Adafruit_Si7021 temperatureSensor = Adafruit_Si7021();

Adafruit_BMP280 pressureSensor;

// DISPLAY
LiquidCrystal_I2C lcd(0x27, 20, 4); //Specifies the display to have 4 rows with 20 characters per row. These numbers should not be reduced.
int currentScreen = 0;
const int displayScreens = 4; //Specifies the number of different display screens to cycle through

#define rainRatePin (16)

#define windSpeedPin (18)
#define windHeadingPin (69)

#define SD_CARD_SS_PIN (4)

#define REQ_BUF_SZ   20   // Used for handling GET requests from clients

// Aliases and Definitions //
#define dataUpdateTime 10000
#define dataWriteTime 60000
#define dataUploadTime 360000

typedef struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
} airqualitydata_t;

SoftwareSerial aqSerial(9, 10);

typedef struct weatherdata_t {
  float temperature;
  float pressure;
  float humidity;
  float pm25;
  float pm10;
  float aqi;
  float windSpeed;
  int windHeading;
  float rainRate;
  String fireSafetyRating;
  String aqiLabel;
} weatherdata_t;

// Global Data Variables //
airqualitydata_t aq;
weatherdata_t data;

#define windHeadingOffset 0

int windInterruptCounter;
int windInterruptBounce;
int raininterruptCounter;

float ignitionComponent;
float rain24Hrs;
float rainYear;

String serializedData, timeStamp;
File logFile;
File root;
File file;
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer

String readString;  // used to store and print GET requests, for debugging

// Timing //
RTC_PCF8523 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

DateTime now;

// Flags //
bool hasSi;
bool hasBmp;
bool hasSD;
bool hasEnet;

// Network //
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 12);      // will need to be changed depending on the router; must be in the range of the subnet
EthernetServer server(80);


void setup() {
  beginSerial();
  beginSD();
  beginEnet();
  beginSensors();
  beginRTC();
  prepareLCD();
  updateData();
}

void loop() {
  now = rtc.now();

  if(now.minute() == 0 && now.second() == 0){
    updateData();
    if(hasSD) writeToSD();
  }


  // find client and serve them the web page
  findEthernetClient();

  delay(1000);
}

// Initialize Components //

/**
 * Opens a serial output to the connected console, for debugging use. Busy waits until the
 * connection opens correctly.
 * Arguments: None
 * Return: None
 */
void beginSerial() {
  Serial.begin(9600);
  while(!Serial) {
    delay(10);
  }

  aqSerial.begin(9600);
  while(!aqSerial) {
  	delay(10);
  }
}

/**
 * Opens the onboard SD card storage and gets a reference to the datalog file. This reference
 * is stored in the global 'log' variable. If no SD is found or the file can't be opened, sets
 * the appropriate global flags to prevent SD access.
 * For debugging: option to print the files stored on the SD card
 * Arguments: None
 * Return: None
 */
void beginSD() {
  if(SD.begin(SD_CARD_SS_PIN)) {
    logFile = SD.open("log.txt", FILE_WRITE);
    hasSD = 1;
    /*
    root = SD.open("/");
    Serial.println("Files found on SD card: ");
    printDirectory(root, 0);
  */
  } else {
    hasSD = 0;
  }
}

/**
* Creates the EthernetShield web server using the given MAC and IP addresses
* Prints the IP address for where to find the web page
* Arguments: None
* Return: None
**/
void beginEnet() {

  Ethernet.init(10);

  Serial.println("Server setup");

  Ethernet.begin(mac, ip);
  server.begin();

  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

}

/**
Set up RTC
**/
void beginRTC() {
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
 
  // When the RTC was stopped and stays connected to the battery, it has
  // to be restarted by clearing the STOP bit. Let's do this to ensure
  // the RTC is running.
  rtc.start();

}

/**
 * Attemts to open attached sensors for future data measurement. Sets a series of global flags
 * based on which are present and which data metrics can be read.
 * Arguments: None
 * Return: None
 */
void beginSensors() {
	hasSi = temperatureSensor.begin();
	hasBmp = pressureSensor.begin();
}

/**
 * Initializes, clears, and backlights the LCD. To the user, this should appear as if the LCD turns off for about 2 seconds.
 */
void prepareLCD() {
    lcd.init();
    lcd.init();
    lcd.clear();
    lcd.backlight();
}

/**
* Debugging function for printing the names and sizes of the files stored on the SD card.
* Recursive function for nested directories
* Arguments: File directory to look in,  number of tabs to indent for easier readability of nested directories
* Return: None
**/
void printDirectory(File dir, int numTabs) {
   while (true) {

      File entry = dir.openNextFile();
      if (! entry) {
         // no more files
         break;
      }
      for (uint8_t i = 0; i < numTabs; i++) {
         Serial.print('\t');
      }
      Serial.print(entry.name());
      if (entry.isDirectory()) {
         Serial.println("/");
         printDirectory(entry, numTabs + 1);
      } else {
         // files have sizes, directories do not
         Serial.print("\t\t");
         Serial.println(entry.size(), DEC);
      }
      entry.close();
   }
}

// Updata Data fields //

/**
 * Updates the global data struct
 * Arguments: None
 * Return: None
 */
void updateData() {
  if(hasSi || hasBmp) updateTemperature();
  if(hasBmp) updatePressure();
  if(hasSi) updateHumidity();
  updateWindHeading();
  updateWindSpeed();
  updateParticulateMatter();
  updateAirQualityIndex();
  updateFireSafetyRating();
}

/**
 * Updates the global temperature data, using the Si7021 sensor if one exists and the BMP280
 * otherwise.
 * Arguments: None
 * Return: None
 */
void updateTemperature() {
  if(hasSi) data.temperature = temperatureSensor.readTemperature();
  else if(hasBmp) data.temperature = pressureSensor.readTemperature();
}

/**
 * Updates the global pressure data, using the BMP280 sensor
 * Arguments: None
 * Return: None
 */
void updatePressure() {
  data.pressure = pressureSensor.readPressure();
}

/**
 * Updates the global humidity data, using the Si7021 sensor
 * Arguments: None
 * Return: None
 */
void updateHumidity() {
  data.humidity = temperatureSensor.readHumidity();
}

/**
 * Updates the global air quality data, using the PM2.5 sensor
 * Arguments: None
 * Return: None
 */
void updateParticulateMatter() {
  if(readPMSdata(&aqSerial)) {
    data.pm25 = aq.pm25_standard;
    data.pm10 = aq.pm10_standard;
	}
}

void updateAirQualityIndex(){
  int aqi_pm10, aqi_pm25;
  //char[] aqiLabel;
  // AQI breakpoints and corresponding concentrations for PM10
  double breakpoints_pm10[] = {0, 54, 154, 254, 354, 424, 504, 604};
  int aqi_values_pm10[] = {0, 50, 100, 150, 200, 300, 400, 500};

  // AQI breakpoints and corresponding concentrations for PM2.5
  double breakpoints_pm25[] = {0, 12, 35.4, 55.4, 150.4, 250.4, 350.4, 500.4};
  int aqi_values_pm25[] = {0, 50, 100, 150, 200, 300, 400, 500};

  // Calculate AQI for PM10
  for (int i = 0; i < 7; ++i) {
      if (data.pm10 <= breakpoints_pm10[i + 1]) {
          aqi_pm10 = (int)(((aqi_values_pm10[i + 1] - aqi_values_pm10[i]) / (breakpoints_pm10[i + 1] - breakpoints_pm10[i])) * (data.pm10 - breakpoints_pm10[i]) + aqi_values_pm10[i]);
          break;
      }
  }

  // Calculate AQI for PM2.5
  for (int i = 0; i < 7; ++i) {
      if (data.pm25 <= breakpoints_pm25[i + 1]) {
          aqi_pm25 = (int)(((aqi_values_pm25[i + 1] - aqi_values_pm25[i]) / (breakpoints_pm25[i + 1] - breakpoints_pm25[i])) * (data.pm25 - breakpoints_pm25[i]) + aqi_values_pm25[i]);
          break;
      }
  }

  if(aqi_pm10 > aqi_pm25){
    data.aqi = aqi_pm10;
  }else{
    data.aqi = aqi_pm25;
  }

  if (data.aqi <= 50) {
        data.aqiLabel = "Good";
    } else if (data.aqi <= 100) {
        data.aqiLabel = "Moderate";
    } else if (data.aqi <= 150) {
        data.aqiLabel = "Unhealthy for Sensitive Groups";
    } else if (data.aqi <= 200) {
        data.aqiLabel = "Unhealthy";
    } else if (data.aqi <= 300) {
        data.aqiLabel = "Very Unhealthy";
    } else {
        data.aqiLabel = "Hazardous";
    }
}

bool readPMSdata(Stream *s) {
  if (! s->available()) {
    return false;
  }
  
  // Read a byte at a time until we get to the special '0x42' start-byte
  if (s->peek() != 0x42) {
    s->read();
    return false;
  }
 
  // Now read all 32 bytes
  if (s->available() < 32) {
    return false;
  }
    
  uint8_t buffer[32];    
  uint16_t sum = 0;
  s->readBytes(buffer, 32);
 
  // get checksum ready
  for (uint8_t i=0; i<30; i++) {
    sum += buffer[i];
  }
 
  /* debugging
  for (uint8_t i=2; i<32; i++) {
    Serial.print("0x"); Serial.print(buffer[i], HEX); Serial.print(", ");
  }
  Serial.println();
  */
  
  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i=0; i<15; i++) {
    buffer_u16[i] = buffer[2 + i*2 + 1];
    buffer_u16[i] += (buffer[2 + i*2] << 8);
  }
 
  // put it into a nice struct :)
  memcpy((void *)&aq, (void *)buffer_u16, 30);
 
  if (sum != aq.checksum) {
    Serial.println("Checksum failure");
    return false;
  }
  // success!
  return true;
}

/**
 * Updates the global wind heading data, using the Davis anemometer
 * Arguments: None
 * Return: None
 */
void updateWindHeading() {
  int rawHeading = analogRead(windHeadingPin);
  int mappedHeading = map(rawHeading, 0, 1023, 0, 360);
  int actualHeading = (mappedHeading + windHeadingOffset) % 360;
  if(actualHeading < 0) actualHeading += 360;
  data.windHeading = actualHeading;
}

/**
 * Updates the global wind speed data, using the Davis wind speed gauge. Blocks for 1s.
 * Arguments: None
 * Return: None
 */
void updateWindSpeed() {
  windInterruptCounter = 0;
  
  pinMode(windSpeedPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(windSpeedPin), isr_rotation, FALLING);
  
  int i = 1000;
  while(i >= 0) {
    unsigned long start = millis();
    i -= (millis() - start);
  }
  
  detachInterrupt(digitalPinToInterrupt(windSpeedPin));
  data.windSpeed = windInterruptCounter * .75;
}

void updateFireSafetyRating() {
  long rh = data.humidity / 100.0;
  long temp = data.temperature;
  long emc;
  if(rh < 10) {
    emc = .03229 + .281073*rh - .000578*rh*temp;
  } else if(rh < 50) {
    emc = 2.22749 + .160107*rh - .014784*temp;
  } else {
    emc = 21.0606 + .005565*(pow(rh,2)) - .00035*rh*temp - .483119*rh;
  }

 long mc1 = 1.03*emc;
 long qign = 144.5 - (.266*temp) - (.00058*(pow(temp,2))) - (.01*temp*mc1) + (18.54*(1.0 - exp(-.151*mc1)) + 6.4*mc1);
 long chi = (344.0 - qign) / 10.0;
 int P_I = round((3.6*chi*.0000185) - 100/.99767);
 int P_FI = round(sqrt(data.windSpeed*rh)/data.windSpeed) ;
 int IC = round(.1*P_I*P_FI);
 ignitionComponent = IC;
 
 if(IC < .1) {
  data.fireSafetyRating = "VERY LOW";
 } else if(IC < .25){
  data.fireSafetyRating = "LOW";
 } else if(IC < .4){
  data.fireSafetyRating = "MODERATE";
 } else if(IC < .7) {
  data.fireSafetyRating = "HIGH";
 } else {
  data.fireSafetyRating = "VERY HIGH";
 }
}

/**
* On interupt, increments the wind interrupt rotation count
* Arguments: None
* Return: None
*/
void isr_rotation() {
  if(millis() - windInterruptBounce > 15) {
    windInterruptCounter++;
    windInterruptBounce = millis();
  }
}

// Data Packaging and Output //

/**
 * Write CSV data out to the onboard storage (SD Card)
 * Arguments: None
 * Return: None
 */
 //Can't write directly to JSON because SD cards only allow for up to 3 character file extensions
 //CSV over text file because ensures more uniform formatting
void writeToSD() {
  logFile = SD.open("log.csv", FILE_WRITE);

  logFile.print(data.temperature);
  logFile.print(",");
	logFile.print(data.pressure);
  logFile.print(",");
	logFile.print(data.humidity);
  logFile.print(",");
  logFile.print(data.pm25);
  logFile.print(",");
  logFile.print(data.pm10);
  logFile.print(",");
  logFile.print(data.aqi);
  logFile.print(",");
  logFile.print(data.aqiLabel);
  logFile.print(",");
  logFile.print(data.windSpeed);
  logFile.print(",");
	logFile.print(data.windHeading);
  logFile.print(",");
	logFile.print(data.rainRate);
  logFile.print(",");
	logFile.print(data.fireSafetyRating);


  logFile.println("");
  logFile.flush();
  logFile.close();
}


/**
 * Finds the Ethernet client and connects them to the web server
 * Listens to the GET requests from the client and responds with the correct endpoint
 * Arguments: None
 * Return: None
 */
void findEthernetClient() {
  
  EthernetClient client = server.available();
  if(client) {

    bool requestEnded = true;

    while(client.connected()) {
    
      if(client.available()) {
        char c = client.read();

        // buffer first part of HTTP request in HTTP_req array (string)
        // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
        if (req_index < (REQ_BUF_SZ - 1)) {
            HTTP_req[req_index] = c;          // save HTTP request character
            req_index++;
        }

        /*
        //read char by char HTTP request; useful for debugging, but not necessary for server to work
        if (readString.length() < 100) {
          //store characters to string 
          readString += c; 
        }
        */

        if(c == '\n' && requestEnded) {

          //Serial.println(readString); //print to serial monitor for debuging 

          // for each endpoint, respond with the correct content type and open the correct file on the SD card to read

          if (StrContains(HTTP_req, "GET / ") || StrContains(HTTP_req, "GET /index.htm")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connnection: close");
            client.println();
            file = SD.open("index.htm");        // open web page file for home page
          }
          else if (StrContains(HTTP_req, "GET /graphs.htm")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connnection: close");
            client.println();
            file = SD.open("graphs.htm");        // open web page file for graphs page
          }
          else if (StrContains(HTTP_req, "GET /i_style.css")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/css");
            client.println("Connnection: close");
            client.println();
            file = SD.open("i_style.css");        // open CSS file for home page
          }
          else if (StrContains(HTTP_req, "GET /g_style.css")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/css");
            client.println("Connnection: close");
            client.println();
            file = SD.open("g_style.css");        // open CSS file for graphs page
          }
          else if (StrContains(HTTP_req, "GET /ppparse.js")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/x-javascript");
            client.println("Connnection: close");
            client.println();
            file = SD.open("ppparse.js");        // open Papa Parse library file
          }
          else if (StrContains(HTTP_req, "GET /charts.js")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/x-javascript");
            client.println("Connnection: close");
            client.println();
            file = SD.open("charts.js");        // open ApexCharts library file
          }
          else if (StrContains(HTTP_req, "GET /index_js.js")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/x-javascript");
            client.println("Connnection: close");
            client.println();
            file = SD.open("index_js.js");        // open JS file for home page
          }
          else if (StrContains(HTTP_req, "GET /graph_js.js")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/x-javascript");
            client.println("Connnection: close");
            client.println();
            file = SD.open("graph_js.js");        // open JS file for graphs page
          }
          else if (StrContains(HTTP_req, "GET /log.csv")) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/csv");
            client.println("Connnection: close");
            client.println();
            file = SD.open("log.csv");        // open log file
          }

          // if the file was found on the SD card, respond to the client with the requested content
          if (file) {
            while(file.available()) {
                client.write(file.read()); // send file to client
            }
            file.close();
          }
          // reset buffer index and all buffer elements to 0
          req_index = 0;
          StrClear(HTTP_req, REQ_BUF_SZ);
          break;
        }

        if(c == '\n') {
          requestEnded = true;
        } else if(c != '\r') {
          requestEnded = false;
        }
      }
    }

    delay(1);
    client.stop();
    readString="";
  }
}

// sets every element of str to 0 (clears array)
// use in findEthernetClient for handling GET requests from client
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
// use in findEthernetClient for handling GET requests from client
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}

void updateDisplay() {

  lcd.clear();
  lcd.setCursor(0,0);

  switch(currentScreen) {
    //General display, with ext. temp., rel. humidity, air pressure, and air quality
    case 0:
      lcd.print("Temp: "); lcd.print(data.temperature); lcd.print(" F");
      lcd.setCursor(0,1);
      lcd.print("Humidity: "); lcd.print(data.humidity); lcd.print(" %");
      lcd.setCursor(0,2);
      lcd.print("Pressure: "); lcd.print(data.pressure); lcd.print(" hectoPascals");
      lcd.setCursor(0,3);
      lcd.print("Air Quality: "); lcd.print(data.aqi); lcd.print( "mg/m^2");
      break;
    
    //Wind & rain
    case 1:
      lcd.print("Rain Rate: "); lcd.print(data.rainRate); lcd.print("in/hr");
      lcd.setCursor(0,1);
      lcd.print("Wind: "); lcd.print(data.windSpeed); lcd.print(" MPH "); lcd.print(data.windHeading);
      break;

    //Rain history
    case 2:
      lcd.print("RAIN IN LAST 24 HRS:"); 
      lcd.setCursor(0,1); lcd.print(rain24Hrs);
      lcd.setCursor(0,2); lcd.print("RAIN IN LAST YEAR:"); 
      lcd.setCursor(0,3); lcd.print(rainYear);
      break;
      
    //Fire safety and ignition component
    case 3:
      lcd.print("FIRE DANGER RATING:"); 
      lcd.setCursor(0,1); lcd.print(data.fireSafetyRating);
      lcd.setCursor(0,2); lcd.print("IGNITION COMP.: "); 
      lcd.setCursor(0,3); lcd.print(ignitionComponent); lcd.print("%");
      break;
      
    default:
      break;
  }
}