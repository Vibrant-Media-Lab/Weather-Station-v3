#include <Adafruit_PM25AQI.h>
#include <string.h>

Adafruit_PM25AQI aqi = Adafruit_PM25AQI();
PM25_AQI_Data data;
int aqi_pm10, aqi_pm25, airQuality;
String aqiLabel;

void setup() {
  // Wait for serial monitor to open
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("Adafruit PMSA003I Air Quality Sensor");

  // Wait one second for sensor to boot up!
  delay(1000);

  // If using serial, initialize it and set baudrate before starting!
  // Uncomment one of the following
  Serial3.begin(9600);
  //pmSerial.begin(9600);

  // There are 3 options for connectivity!
  // if (! aqi.begin_I2C()) {      // connect to the sensor over I2C
  if (! aqi.begin_UART(&Serial3)) { // connect to the sensor over hardware serial
  //if (! aqi.begin_UART(&pmSerial)) { // connect to the sensor over software serial 
    Serial.println("Could not find PM 2.5 sensor!");
    while (1) delay(10);
  }

  Serial.println("PM25 found!");
}

void loop() { 
  if (! aqi.read(&data)) {
    Serial.println("Could not read from AQI");
    delay(500);  // try again in a bit!
    return;
  }
  updateAirQualityIndex();
  Serial.println("AQI reading success");
  Serial.print(F("AQI = "));
  Serial.println(airQuality);
  Serial.print(F("Condtion = "));
  Serial.println(aqiLabel);
  delay(2000);
}

void updateAirQualityIndex(){
  // AQI breakpoints and corresponding concentrations for PM10
  double breakpoints_pm10[] = {0, 54, 154, 254, 354, 424, 504, 604};
  int aqi_values_pm10[] = {0, 50, 100, 150, 200, 300, 400, 500};

  // AQI breakpoints and corresponding concentrations for PM2.5
  double breakpoints_pm25[] = {0, 12, 35.4, 55.4, 150.4, 250.4, 350.4, 500.4};
  int aqi_values_pm25[] = {0, 50, 100, 150, 200, 300, 400, 500};

  // Calculate AQI for PM10
  for (int i = 0; i < 7; ++i) {
      if (data.pm10_standard <= breakpoints_pm10[i + 1]) {
          aqi_pm10 = (int)(((aqi_values_pm10[i + 1] - aqi_values_pm10[i]) / (breakpoints_pm10[i + 1] - breakpoints_pm10[i])) * (data.pm10_standard - breakpoints_pm10[i]) + aqi_values_pm10[i]);
          break;
      }
  }

  // Calculate AQI for PM2.5
  for (int i = 0; i < 7; ++i) {
      if (data.pm25_standard <= breakpoints_pm25[i + 1]) {
          aqi_pm25 = (int)(((aqi_values_pm25[i + 1] - aqi_values_pm25[i]) / (breakpoints_pm25[i + 1] - breakpoints_pm25[i])) * (data.pm25_standard - breakpoints_pm25[i]) + aqi_values_pm25[i]);
          break;
      }
  }

  if(aqi_pm10 > aqi_pm25){
    airQuality = aqi_pm10;
  }else{
    airQuality = aqi_pm25;
  }

  if (airQuality <= 50) {
        aqiLabel = "Good";
    } else if (airQuality <= 100) {
        aqiLabel = "Moderate";
    } else if (airQuality <= 150) {
        aqiLabel = "Unhealthy for Sensitive Groups";
    } else if (airQuality <= 200) {
        aqiLabel = "Unhealthy";
    } else if (airQuality <= 300) {
        aqiLabel = "Very Unhealthy";
    } else {
        aqiLabel = "Hazardous";
    }
}
