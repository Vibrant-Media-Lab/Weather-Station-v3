// set timer to auto refresh the page once every hour to display the most recent data
var timeout = 3600000;
window.setTimeout(poller, timeout);

function poller() {
    window.location = "http://192.168.1.12";    //URL will need to be updated to match where the web page is hosted 
    window.setTimeout(poller, timeout);
}

// use the Papa Parse library to parse data from csv format on SD card to JSON object usable for webpage
// arguments: log file, callback function
// returns: nothing
// the callback function is necessary because of the way that the data is parsed; without sending the array of JSON objects as an argument to a callback function, the results would not be visible outside of this function
function parseData(logFile, callBack) {
    Papa.parse(logFile, {
        download:true,
        header: true,
        complete: function(results) {
            callBack(results.data);
        }
    });
}

// callback function used in intex.htm, has access to the parsed data, call all other functions from here
// arguments: array of JSON objects
// returns: nothing
function mainPageDisplay(data){
    displayCurrentData(data);
}


// display most recent readings in the "Current Weather" section
// arguments: array of JSON objects
// returns: nothing
// creates an unordered list of objects and each data type is a list item to display
function displayCurrentData(data) {

    let current_data_index = data.length - 2;   // use -2 because the array always seems to have an extra blank line at the end

    //debugging for-loop to check that the data is accessible
    /*
    console.log(current_data_index);

    for (let item in data[current_data_index]){
        console.log(item);
        console.log(data[current_data_index][item]);
    }
    */

    // display most recent date and time for the data
    let currTime = document.createElement("h5");
    currTime.id = "curr_time";

    let timestamp = `${data[current_data_index]["timestamp"]}`;
    timestamp = timestamp.split(" ");

    currTime.innerText = `Data collected at ${timestamp[1]} on ${timestamp[0]}`;

    // create ul for hourly data
    let currWeather = document.createElement("ul");
    currWeather.id = "curr_weather";

    // add currWeather to the DOM
    let currWeatherParent = document.getElementById("hourly_live_data");
    currWeatherParent.appendChild(currTime);
    currWeatherParent.appendChild(currWeather);

    // for each sensor measurement, create an li element, set the id and the innerText, and add it to the DOM
    let currTemp = document.createElement("li");
    currTemp.id = "curr_temp";
    currTemp.innerText = `Temperature: ${data[current_data_index]["temperature"]} \u00B0C`;
    currWeather.appendChild(currTemp);

    let currPressure = document.createElement("li");
    currPressure.id = "curr_pressure";
    currPressure.innerText = `Air Pressure: ${data[current_data_index]["pressure"]} Pa`;
    currWeather.appendChild(currPressure);

    let currHumidity = document.createElement("li");
    currHumidity.id = "curr_humidity";
    currHumidity.innerText = `Humidity: ${data[current_data_index]["humidity"]} %`;
    currWeather.appendChild(currHumidity);

    let currAQILabel = document.createElement("li");
    currAQILabel.id = "curr_AQI_label";
    currAQILabel.innerText = `Air Quality Index (AQI) Level: ${data[current_data_index]["aqi_label"]}`;
    currWeather.appendChild(currAQILabel);

    let currAQI = document.createElement("li");
    currAQI.id = "curr_AQI";
    currAQI.innerText = `Air Quality Index (AQI): ${data[current_data_index]["aqi"]}`;
    currWeather.appendChild(currAQI);

    let currPM2_5 = document.createElement("li");
    currPM2_5.id = "curr_PM_2_5";
    currPM2_5.innerText = `PM 2.5: ${data[current_data_index]["pm25"]} \u00B5g/m3`;
    currWeather.appendChild(currPM2_5);

    let currPM10 = document.createElement("li");
    currPM10.id = "curr_PM_10";
    currPM10.innerText = `PM 10: ${data[current_data_index]["pm10"]} \u00B5g/m3`;
    currWeather.appendChild(currPM10);

    let currWindSpeed = document.createElement("li");
    currWindSpeed.id = "curr_wind_speed";
    currWindSpeed.innerText = `Wind Speed: ${data[current_data_index]["wind_speed"]} mph`;
    currWeather.appendChild(currWindSpeed);

    let currWindHeading = document.createElement("li");
    currWindHeading.id = "curr_wind_heading";
    currWindHeading.innerText = `Wind Heading: ${data[current_data_index]["wind_heading"]} \u00B0`;
    currWeather.appendChild(currWindHeading);

    let currRainRate = document.createElement("li");
    currRainRate.id = "curr_rain_rate";
    currRainRate.innerText = `Rain Rate: ${data[current_data_index]["rain_rate"]} inches per hour`;
    currWeather.appendChild(currRainRate);

    let currFireSafety = document.createElement("li");
    currFireSafety.id = "curr_fire_safety";
    currFireSafety.innerText = `Fire Safety Rating: ${data[current_data_index]["fire_safety_rating"]}`;
    currWeather.appendChild(currFireSafety);


    // display the previous date when the daily data collected
    let prevDay = document.createElement("h5");
    prevDay.id = "prev_day";

    prevDay.innerText = `Data collected on ${data[current_data_index]["prev_date"]}`;

    // create ul for daily data
    let dailyWeather = document.createElement("ul");
    dailyWeather.id = "daily_weather";

    // add dailyWeather to the DOM
    let dailyWeatherParent = document.getElementById("prev_day_data");
    dailyWeatherParent.appendChild(prevDay);
    dailyWeatherParent.appendChild(dailyWeather);

    // for each sensor measurement, create an li element, set the id and the innerText, and add it to the DOM
    let highTemp = document.createElement("li");
    highTemp.id = "high_temp";
    highTemp.innerText = `High Temperature: ${data[current_data_index]["prev_day_high_temp"]} \u00B0C`;
    dailyWeather.appendChild(highTemp);

    let lowTemp = document.createElement("li");
    lowTemp.id = "low_temp";
    lowTemp.innerText = `Low Temperature: ${data[current_data_index]["prev_day_low_temp"]} \u00B0C`;
    dailyWeather.appendChild(lowTemp);

    let avgPressure = document.createElement("li");
    avgPressure.id = "avg_pressure";
    avgPressure.innerText = `Average Air Pressure: ${data[current_data_index]["prev_day_avg_pressure"]} Pa`;
    dailyWeather.appendChild(avgPressure);

    let highHumidity = document.createElement("li");
    highHumidity.id = "high_humidity";
    highHumidity.innerText = `High Humidity: ${data[current_data_index]["prev_day_high_humidity"]} %`;
    dailyWeather.appendChild(highHumidity);

    let worstAQILabel = document.createElement("li");
    worstAQILabel.id = "worst_AQI_label";
    worstAQILabel.innerText = `Worst Air Quality Index (AQI) Level: ${data[current_data_index]["prev_day_worst_aqi_label"]}`;
    dailyWeather.appendChild(worstAQILabel);

    let highAQI = document.createElement("li");
    highAQI.id = "high_AQI";
    highAQI.innerText = `Highest Air Quality Index (AQI): ${data[current_data_index]["prev_day_high_aqi"]}`;
    dailyWeather.appendChild(highAQI);    

    let avgWindSpeed = document.createElement("li");
    avgWindSpeed.id = "avg_wind_speed";
    avgWindSpeed.innerText = `Average Wind Speed: ${data[current_data_index]["prev_day_avg_wind_speed"]} mph`;
    dailyWeather.appendChild(avgWindSpeed);

    let totRainRate = document.createElement("li");
    totRainRate.id = "tot_rain_rate";
    totRainRate.innerText = `Total Rain Fall: ${data[current_data_index]["prev_day_total_rain"]} inches`;
    dailyWeather.appendChild(totRainRate);

    let worstFireSafety = document.createElement("li");
    worstFireSafety.id = "worst_fire_safety";
    worstFireSafety.innerText = `Lowest Fire Safety Rating: ${data[current_data_index]["prev_day_worst_fire_safety"]}`;
    dailyWeather.appendChild(worstFireSafety);

}