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

    // create ul
    let currWeather = document.createElement("ul");
    currWeather.id = "curr_weather";

    // add currWeather to the DOM
    let currWeatherParent = document.getElementById("live_data");
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
}