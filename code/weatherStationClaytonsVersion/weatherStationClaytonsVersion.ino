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
#include <RTClib.h>
#include <Adafruit_PM25AQI.h>

// Component Declarations //
Adafruit_Si7021 temperatureSensor = Adafruit_Si7021();
Adafruit_BMP280 pressureSensor;
Adafruit_PM25AQI aqi = Adafruit_PM25AQI();
PM25_AQI_Data aqi_data;

// DISPLAY
LiquidCrystal_I2C lcd(0x27, 20, 4); //Specifies the display to have 4 rows with 20 characters per row. These numbers should not be reduced.
int currentScreen = 0;
const int displayScreens = 4; //Specifies the number of different display screens to cycle through

#define rainRatePin (16)

#define windSpeedPin (18)
#define windHeadingPin (69)

#define SD_CARD_SS_PIN (4)

#define REQ_BUF_SZ   20   // Used for handling GET requests from clients

typedef struct weatherdata_t {
  float temperature;
  float pressure;
  float humidity;
  float aqi_pm25;
  float aqi_pm10;
  float airQuality;
  float windSpeed;
  int windHeading;
  float rainRate;
  String fireSafetyRating;
  String aqiLabel;
} weatherdata_t;

typedef struct dailydata_t {
  String date;
  float high_temp;
  float low_temp;
  float avg_pressure;
  float high_humidity;
  float high_aqi;
  float avg_windSpeed;
  float total_rainRate;
  String worst_fireSafetyRating;
  String worst_aqiLabel;
} dailydata_t;

// Global Data Variables //
weatherdata_t data;
dailydata_t dailyData;
dailydata_t prevDailyData;

#define windHeadingOffset 0

int windInterruptCounter;
int windInterruptBounce;

float totalWindSpeed = 0;
int windSpeedMeasurementsCount = 0;
volatile float windHr = 0;
volatile float windHrTotal = 0;
int windHrCount = 0;

int raininterruptCounter;
int rainInterruptBounce;

float ignitionComponent;
float rain24Hrs;
float rainYear;

volatile float rainHr = 0;
volatile float rain24Hr = 0;
const float volumePerRainTip = 0.00787401575; //Volume of water that will cause a tip in the rain sensor, should be calibrated
const int rainBounceMin = 500; //Min amount of time in millis that a rain tip must wait after its predecessor to be counted
volatile float lastRainTip = millis();

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

  attachInterrupt(digitalPinToInterrupt(rainRatePin), rainTip, FALLING); // whenever rain causes a tip, this interrupt is called
  attachInterrupt(digitalPinToInterrupt(windSpeedPin), isr_rotation, FALLING); // whenever wind causes a rotation, this interrupt is called

  updateData();
}

void loop() {
  now = rtc.now();

  if(now.minute() == 0 && now.second() == 0){
    updateData();
    if(hasSD) writeToSD();
    delay(1000);
  }


  // find client and serve them the web page
  findEthernetClient();
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

  Serial3.begin(9600);
  while(!Serial3) {
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

    //Properly specifies the pins to be used for rain ticks and wind ticks, and reserves them for interrupts
    pinMode(rainRatePin, INPUT_PULLUP);
    pinMode(windSpeedPin, INPUT_PULLUP); 
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

// Interrupt functions for rain rate and wind speed //

/**
* Interrupt function to be called whenever the rain gauge is tipped, updates the rain fall totals
* rainfall per day = rain24hr
**/
void rainTip(){
  if(millis() - lastRainTip > rainBounceMin){
    rainHr += volumePerRainTip;
    rain24Hr += volumePerRainTip;

    lastRainTip = millis();
  }
}

/**
* On interupt, increments the wind interrupt rotation count
* Arguments: None
* Return: None
* CALLED EVERY TIME THE WIND CAUSES A SINGLE ROTATION (not hourly)
*/
void isr_rotation() {
  if(millis() - windInterruptBounce > 15) {
    windInterruptCounter++;
    windInterruptBounce = millis();
  }

  // TODO: add average wind through the hour
  windHr += windHrCount * (3600.0 / 8000.0);
  windHrTotal += windHr;
  windHrCount++;
  
  
  float totalWindSpeed = 0;
  int windSpeedMeasurementsCount = 0;
  
}

// Updata Data fields //

/**
 * CALLED EVERY HOUR IN loop()
 * Updates the global data struct
 * Arguments: None
 * Return: None
 */
void updateData() {

  // update the hourly data readings
  if(hasSi || hasBmp) updateTemperature();
  if(hasBmp) updatePressure();
  if(hasSi) updateHumidity();
  updateWindHeading();
  updateWindSpeed();
  updateRainRate();
  updateAirQualityIndex();
  updateFireSafetyRating();

  //update the daily data readings

  now = rtc.now();

  // at midnight, move the previous day's values to the stored struct so they stay consistent 
  // throughout the next day, then clear the updating struct to calculate the new days values
  
  if (now.hour() == 0){
    // copy the end of day values of the daily data
    float prevHighTemp = dailyData.high_temp;
    float prevLowTemp = dailyData.low_temp;
    float prevAvgPressure = dailyData.avg_pressure;
    float prevHighHumidity = dailyData.high_humidity;
    float prevHighAQI = dailyData.high_aqi;
    float prevAvgWindSpeed = dailyData.avg_windSpeed;
    float prevTotalRain = dailyData.total_rainRate;
    String prevWorstFireRating = dailyData.worst_fireSafetyRating;
    String prevWorstAQILabel = dailyData.worst_aqiLabel;

    //get the previous day's date to attach to the data
    DateTime prevDay = (now - TimeSpan(1, 0, 0, 0));
    prevDailyData.date = String(prevDay.year()) + "-" + String(prevDay.month()) + "-" + String(prevDay.day());

    // store the previous day's values
    prevDailyData.high_temp = prevHighTemp;
    prevDailyData.low_temp = prevLowTemp;
    prevDailyData.avg_pressure = prevAvgPressure;
    prevDailyData.high_humidity = prevHighHumidity;
    prevDailyData.high_aqi = prevHighAQI;
    prevDailyData.avg_windSpeed = prevAvgWindSpeed;
    prevDailyData.total_rainRate = prevTotalRain;
    prevDailyData.worst_fireSafetyRating = prevWorstFireRating;
    prevDailyData.worst_aqiLabel = prevWorstAQILabel;

    // clear the previous day's data
    dailyData.high_temp = data.temperature;
    dailyData.low_temp = data.temperature;
    dailyData.avg_pressure = data.pressure;
    dailyData.high_humidity = data.humidity;
    dailyData.high_aqi = data.airQuality;
    dailyData.avg_windSpeed = data.windSpeed;
    dailyData.total_rainRate = data.rainRate;
    dailyData.worst_fireSafetyRating = data.fireSafetyRating;
    dailyData.worst_aqiLabel = data.aqiLabel;

    // reset wind speed on the hour. Doesn't follow the above style, but easier to read in a rough draft.
    totalWindSpeed - 0;
    windSpeedMeasurementsCount = 0;

  }
  else {
  //update the daily values each hour: 
    int currentHour = now.hour(); // the number of values currently included in the calculations
    int total = currentHour + 1; // the new value to use for calculating averages

    // high temperature
    if (data.temperature > dailyData.high_temp){
      dailyData.high_temp = data.temperature;
    }

    // low temperature
    if (data.temperature < dailyData.low_temp){
      dailyData.low_temp = data.temperature;
    }

    // average air pressure
    dailyData.avg_pressure = ((dailyData.avg_pressure * currentHour) + data.pressure) / total;

    // highest humidity
    if (data.humidity > dailyData.high_humidity){
      dailyData.high_humidity = data.humidity;
    }

    // worst air quality and AQI label
    if (data.airQuality > dailyData.high_aqi){
      dailyData.high_aqi = data.airQuality;
      dailyData.worst_aqiLabel = data.aqiLabel;
    }

    // average wind speed
    dailyData.avg_windSpeed = ((dailyData.avg_windSpeed * currentHour) + data.windSpeed) / total;

    // total rain fall
    dailyData.total_rainRate += data.rainRate;

    // worst fire safety rating
    if (data.fireSafetyRating == "VERY LOW"){
      dailyData.worst_fireSafetyRating = data.fireSafetyRating;
    }
    else if(data.fireSafetyRating == "LOW" && dailyData.worst_fireSafetyRating != "VERY LOW"){
      dailyData.worst_fireSafetyRating = data.fireSafetyRating;
    }
    else if (data.fireSafetyRating == "MODERATE" && (dailyData.worst_fireSafetyRating != "LOW" && dailyData.worst_fireSafetyRating != "VERY LOW")){
      dailyData.worst_fireSafetyRating = data.fireSafetyRating;
    }
    else if (data.fireSafetyRating == "HIGH" && (dailyData.worst_fireSafetyRating == "VERY HIGH" )){
      dailyData.worst_fireSafetyRating = data.fireSafetyRating;
    }
  }
  
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

void updateAirQualityIndex(){
  //char[] aqiLabel;
  // AQI breakpoints and corresponding concentrations for PM10
  double breakpoints_pm10[] = {0, 54, 154, 254, 354, 424, 504, 604};
  int aqi_values_pm10[] = {0, 50, 100, 150, 200, 300, 400, 500};

  // AQI breakpoints and corresponding concentrations for PM2.5
  double breakpoints_pm25[] = {0, 12, 35.4, 55.4, 150.4, 250.4, 350.4, 500.4};
  int aqi_values_pm25[] = {0, 50, 100, 150, 200, 300, 400, 500};

  // Calculate AQI for PM10
  for (int i = 0; i < 7; ++i) {
      if (aqi_data.pm10_standard <= breakpoints_pm10[i + 1]) {
          data.aqi_pm10 = (int)(((aqi_values_pm10[i + 1] - aqi_values_pm10[i]) / (breakpoints_pm10[i + 1] - breakpoints_pm10[i])) * (aqi_data.pm10_standard - breakpoints_pm10[i]) + aqi_values_pm10[i]);
          break;
      }
  }

  // Calculate AQI for PM2.5
  for (int i = 0; i < 7; ++i) {
      if (aqi_data.pm25_standard <= breakpoints_pm25[i + 1]) {
          data.aqi_pm25 = (int)(((aqi_values_pm25[i + 1] - aqi_values_pm25[i]) / (breakpoints_pm25[i + 1] - breakpoints_pm25[i])) * (aqi_data.pm25_standard - breakpoints_pm25[i]) + aqi_values_pm25[i]);
          break;
      }
  }

  if(data.aqi_pm10 > data.aqi_pm25){
    data.airQuality = data.aqi_pm10;
  }else{
    data.airQuality = data.aqi_pm25;
  }

  if (data.airQuality <= 50) {
        data.aqiLabel = "Good";
    } else if (data.airQuality <= 100) {
        data.aqiLabel = "Moderate";
    } else if (data.airQuality <= 150) {
        data.aqiLabel = "Unhealthy for Sensitive Groups";
    } else if (data.airQuality <= 200) {
        data.aqiLabel = "Unhealthy";
    } else if (data.airQuality <= 300) {
        data.aqiLabel = "Very Unhealthy";
    } else {
        data.aqiLabel = "Hazardous";
    }
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
  
  attachInterrupt(digitalPinToInterrupt(windSpeedPin), isr_rotation, FALLING);
  
  int i = 1000;
  while(i >= 0) {
    unsigned long start = millis();
    i -= (millis() - start);
  }
  
  detachInterrupt(digitalPinToInterrupt(windSpeedPin));
  data.windSpeed = windInterruptCounter * .75; // current wind speed

  // accumulate total wind speed and count measurements (this function is called once per hour)
  totalWindSpeed += windHr;
  windSpeedMeasurementsCount++;
  
}

float calculateAverageWindSpeed() 
{
  if (windSpeedMeasurementsCount == 0) return 0;
  return windHrTotal / windSpeedMeasurementsCount;
}

void updateRainRate(){
  data.rainRate = rainHr;
  rainHr = 0;
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




// Data Packaging and Output //

/**
 * Write CSV data out to the onboard storage (SD Card)
 * Arguments: None
 * Return: None
 */
 //Can't write directly to JSON because SD cards only allow for up to 3 character file extensions
 //CSV chosen over text file because ensures more uniform formatting
void writeToSD() {
  logFile = SD.open("log.csv", FILE_WRITE);

  logFile.print(now.year(), DEC);
  logFile.print("-");
  logFile.print(now.month(), DEC);
  logFile.print("-");
  logFile.print(now.day(), DEC);
  logFile.print(" ");
  logFile.print(now.hour(), DEC);
  logFile.print(":");
  logFile.print(now.minute(), DEC);
  logFile.print(":");
  logFile.print(now.second(), DEC);
  logFile.print(",");

  logFile.print(data.temperature);
  logFile.print(",");
	logFile.print(data.pressure);
  logFile.print(",");
	logFile.print(data.humidity);
  logFile.print(",");
  logFile.print(data.aqi_pm25);
  logFile.print(",");
  logFile.print(data.aqi_pm10);
  logFile.print(",");
  logFile.print(data.airQuality);
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
  logFile.print(",");

  logFile.print(prevDailyData.date);
  logFile.print(",");

  logFile.print(prevDailyData.high_temp);
  logFile.print(",");
  logFile.print(prevDailyData.low_temp);
  logFile.print(",");
  logFile.print(prevDailyData.avg_pressure);
  logFile.print(",");
  logFile.print(prevDailyData.high_humidity);
  logFile.print(",");
  logFile.print(prevDailyData.high_aqi);
  logFile.print(",");
  logFile.print(prevDailyData.worst_aqiLabel);
  logFile.print(",");
  logFile.print(prevDailyData.avg_windSpeed);
  logFile.print(",");
  logFile.print(prevDailyData.total_rainRate);
  logFile.print(",");
  logFile.print(prevDailyData.worst_fireSafetyRating);
  

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
      lcd.print("Air Quality: "); lcd.print(data.airQuality); lcd.print( "mg/m^2");
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
