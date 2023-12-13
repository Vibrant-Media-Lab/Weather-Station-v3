// arrays for each column of data in the CSV file - global variables

// hourly data
var timestampArray = [];
var tempArray = [];
var pressureArray = [];
var humidityArray = [];
var aqiArray = [];
var pm2_5Array = [];
var pm10Array = [];
var aqiLabelArray = [];
var windSpeedArray = [];
var windHeadingArray = [];
var rainRateArray = [];
var fireSafetyArray = [];

// daily data
var dateArray = [];
var highTempArray = [];
var lowTempArray = [];
var avgPressureArray = [];
var highHumidityArray = [];
var highAqiArray = [];
var worstAqiLabelArray = [];
var avgWindSpeedArray = [];
var totRainFallArray = [];
var worstFireSafetyArray = [];



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

// callback function used in graphs.htm, has access to the parsed data, call all other functions from here
// arguments: array of JSON objects
// returns: nothing
function containerFunction(data){
    createDataArrays(data);
    trackButtonClicks();

    //checkAvailability();
}


// create arrays with each individual data type; store to the global variables so they are accessible for making graphs
// arguments: array of JSON objects
// returns: nothing
function createDataArrays(data){

    // reset the arrays to be empty to make sure that data is not just being added on to the end every time this is called
    timestampArray = [];
    tempArray = [];
    pressureArray = [];
    humidityArray = [];
    aqiLabelArray = [];
    aqiArray = [];
    pm2_5Array = [];
    pm10Array = [];
    windSpeedArray = [];
    windHeadingArray = [];
    rainRateArray = [];
    fireSafetyArray = [];

    dateArray = [];
    highTempArray = [];
    lowTempArray = [];
    avgPressureArray = [];
    highHumidityArray = [];
    highAqiArray = [];
    worstAqiLabelArray = [];
    avgWindSpeedArray = [];
    totRainFallArray = [];
    worstFireSafetyArray = [];
    


    let current_data_length = data.length - 1;  // subtract 1 to get length because of empty line at the end

    // for each object in the array, add the reading for that data type to the end of the appropriate array

    // hourly data
    for (let i = 0; i < current_data_length; i++){
        timestampArray.push(data[i]["timestamp"]);
        tempArray.push(data[i]["temperature"]);
        pressureArray.push(data[i]["pressure"]);
        humidityArray.push(data[i]["humidity"]);
        aqiLabelArray.push(data[i]["aqi_label"]);
        aqiArray.push(data[i]["aqi"]);
        pm2_5Array.push(data[i]["pm25"]);
        pm10Array.push(data[i]["pm10"]);
        windSpeedArray.push(data[i]["wind_speed"]);
        windHeadingArray.push(data[i]["wind_heading"]);
        rainRateArray.push(data[i]["rain_rate"]);
        fireSafetyArray.push(data[i]["fire_safety_rating"]);
        
    }

    // daily data
    for (let i = 0; i < current_data_length; i+=24){
        dateArray.push(data[i]["prev_date"]);
        highTempArray.push(data[i]["prev_day_high_temp"]);
        lowTempArray.push(data[i]["prev_day_low_temp"]);
        avgPressureArray.push(data[i]["prev_day_avg_pressure"]);
        highHumidityArray.push(data[i]["prev_day_high_humidity"]);
        highAqiArray.push(data[i]["prev_day_high_aqi"]);
        worstAqiLabelArray.push(data[i]["prev_day_worst_aqi_label"]);
        avgWindSpeedArray.push(data[i]["prev_day_avg_wind_speed"]);
        totRainFallArray.push(data[i]["prev_day_total_rain"]);
        worstFireSafetyArray.push(data[i]["prev_day_worst_fine_safety"]);
    }
}


// debugging function to check scope of arrays; make sure that the global variables are working as intended
/*
function checkAvailability(){
    console.log(timestampArray);
    console.log(tempArray);
    console.log(pressureArray);
    console.log(humidityArray);
    console.log(aqiLabelArray);
    console.log(aqiArray);
    console.log(pm2_5Array);
    console.log(pm10Array);
    console.log(windSpeedArray);
    console.log(windHeadingArray);
    console.log(rainRateArray);
    console.log(fireSafetyArray);

    console.log(dateArray);
    console.log(highTempArray);
    console.log(lowTempArray);
    console.log(avgPressureArray);
    console.log(highHumidityArray);
    console.log(highAqiArray);
    console.log(worstAqiLabelArray);
    console.log(avgWindSpeedArray);
    console.log(totRainFallArray);
    console.log(worstFireSafetyArray);
    

}
*/

// function to create the graphs on the page, uses the ApexCharts library
// arguments: array of data type to make graph from, string of data type to use for axis label and title
// returns: nothing
function graphData(dataArray, dataType){
    //console.log(dataArray);

    // clear the previous graph that was displayed, if any
    graph = document.getElementById('graph');
    graph.innerHTML = "";
    

    // the data does not have timestamps, so instead create array of counts for the number of readings taken
    let x_data = [];
    for(let i = 1; i <= dataArray.length; i ++){
        x_data.push(i);
    }

    // set the parameters of the chart
    var options = {
        chart: {
          type: 'line'
        },
        series: [{
          name: dataType,
          data: dataArray
        }],
        xaxis: {
          categories: x_data
        }
      }

      // create the chart in the graph div using the parameters defined above
      var chart = new ApexCharts(graph, options);
      
      // draw the chart
      chart.render();
}

// create event listeners to the ul of data types on the page; when a "button" is clicked, create a graph of that data type
// arguments: none
// returns: nothing
function trackButtonClicks(){

    // reminder of the IDs used for each element in the HTML
    /*
    <li id="tempButton">Temperature</li>
    <li id="pressureButton">Air Pressure</li>
    <li id="humidityButton">Humidity</li>
    <li id="aqiButton">Air Quality Index</li>
    <li id="airQuality2.5Button">PM 2.5</li>
    <li id="airQuality10Button">PM 10</li>
    <li id="windSpeedButton">Wind Speed</li>
    <li id="windHeadingButton">Wind Heading</li>
    <li id="rainRateButton">Rain Rate</li> 
    */
   
    // for each "button", add an event listener that calls the graphData function with the correct array and data type

    let tempButton = document.getElementById("tempButton");
    tempButton.addEventListener("click", (e) => { graphData(tempArray, "Temperature") });

    let pressureButton = document.getElementById("pressureButton");
    pressureButton.addEventListener("click", (e) => { graphData(pressureArray, "Air Pressure") });

    let humidityButton = document.getElementById("humidityButton");
    humidityButton.addEventListener("click", (e) => { graphData(humidityArray, "Humidity") });

    let aqiButton = document.getElementById("aqiButton");
    aqiButton.addEventListener("click", (e) => { graphData(aqiArray, "AQI") });

    let pm2_5Button = document.getElementById("airQuality2.5Button");
    pm2_5Button.addEventListener("click", (e) => { graphData(pm2_5Array, "PM 2.5")});

    let pm10Button = document.getElementById("airQuality10Button");
    pm10Button.addEventListener("click", (e) => { graphData(pm10Array, "PM 10")});

    let windSpeedButton = document.getElementById("windSpeedButton");
    windSpeedButton.addEventListener("click", (e) => { graphData(windSpeedArray, "Wind Speed") });

    let windHeadingButton = document.getElementById("windHeadingButton");
    windHeadingButton.addEventListener("click", (e) => { graphData(windHeadingArray, "Wind Heading") });

    let rainRateButton = document.getElementById("rainRateButton");
    rainRateButton.addEventListener("click", (e) => { graphData(rainRateArray, "Rain Rate") });
    
}
