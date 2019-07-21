/*
  ESP8266 Weather Webserver provides near automous display of weather data
  The 'MIT License (MIT) Copyright (c) 2019 by David Bird'. Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
  documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the
  following conditions:
  The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
  software use is visible to an end-user.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
//########################   Weather Display  #############################
// Receives and displays the weather forecast from the Weather Underground
// and then displays a web page with the data using a webserver
//################# LIBRARIES ################
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WiFiManager.h>     // https://github.com/tzapu/WiFiManager
#include <DNSServer.h>
#include <ArduinoJson.h>     // https://github.com/bblanchon/ArduinoJson

//################ VARIABLES ################

//------ NETWORK VARIABLES---------
const char *ssid      = "your_SSID";
const char *password  = "your_PASSWORD";
String Key            = "your_API_KEY";  // See: http://www.wunderground.com/weather/api/d/docs (change here with your KEY)
String City           = "IMELKSHA2";     // NOTE: MUST Be a PWS not a city
String Country        = "UK";
// Only the following country codes are supported by WU: 
// United States Country Code: US, United Kingdom Country Code: GB, France Country Code: FR, Germany Country Code: DE, Italy Country Code: IT

String Units          = "m";               // e = English units m = Metric units h = Hybrid units (UK) s = SI Units
String Conditions     = "conditions";      // See: http://www.wunderground.com/weather/api/d/docs?d=data/index&MR=1
char   wxserver[]     = "api.weather.com"; // Address for WeatherUnderGround

unsigned long    lastConnectionTime = 0;            // Last time you connected to the server, in milliseconds
const unsigned long postingInterval = 300L * 1000L; // Delay between updates, in milliseconds, WU allows 500 requests per-day maximum and no more than 2.8 per-minute!
char buffer[256];
int rx_cnt = 0;

//################ PROGRAM VARIABLES and OBJECTS ################
String webpage, city, country, date_time, observation_time,
       Temperature, Feelslike, Humidity, Heatindex,
       Winddirection, Windspeed, Windgust, Windchill,
       Pressure,
       Dewpoint,
       Precip1hr, Preciptoday,
       solarradiation, icon_filename;

WiFiClient wxclient;

String version = "v2.0";     // Version of this program
ESP8266WebServer server(80); // Start server on port 80 (default for a web-browser, change to your requirements, e.g. 8080 if your Router uses port 80
// To access server from the outside of a WiFi network e.g. ESP8266WebServer server(8266) add a rule on your Router that forwards a
// connection request to http://your_network_ip_address:8266 to port 8266 and view your ESP server from anywhere.
// Example http://yourhome.ip:8266 will be directed to http://192.168.0.40:8266 or whatever IP address your router gives to this server

void setup()
{
  Serial.begin(115200); // initialize serial communications
  WiFiManager wifiManager;
  // New OOB ESP8266 has no Wi-Fi credentials so will connect and not need the next command to be uncommented and compiled in, a used one with incorrect credentials will
  // so restart the ESP8266 and connect your PC to the wireless access point called 'ESP8266_AP' or whatever you call it below in ""
  //wifiManager.resetSettings(); // Command to be included if needed, then connect to http://192.168.4.1/ and follow instructions to make the WiFi connection
  // Set a timeout until configuration is turned off, useful to retry or go to sleep in n-seconds
  wifiManager.setTimeout(180);
  //fetches ssid and password and tries to connect, if connections succeeds it starts an access point with the name called "ESP8266_AP" and waits in a blocking loop for configuration
  if (!wifiManager.autoConnect("ESP8266_AP")) {
    Serial.println(F("failed to connect and timeout occurred"));
    delay(6000);
  }
  // At this stage the WiFi manager will have successfully connected to a network, or if not will try again in 180-seconds
  Serial.println(F("WiFi connected.."));
  //----------------------------------------------------------------------
  server.begin(); Serial.println(F("Webserver started...")); // Start the webserver
  Serial.println("Use this URL to connect: http://" + WiFi.localIP().toString() + "/"); // Print the IP address
  server.on("/", SystemSetup);
  server.on("/homepage", homepage);
  //httpRequest(Conditions);
}

void homepage() {
  httpRequest();
  append_page_header();
  webpage += "<table align='center' style='width: 100%; height: 200px'>";
  webpage += "<tr>";
  webpage += "  <td>";
  webpage += "   <table style='width: 100%'>";
  webpage += "     <tr>";
  webpage += "       <td>";
  webpage += "          <img src='" + icon_filename + "' alt='Weather Symbol' style='width:150px;height:150px;'>";
  webpage += "       </td>";
  webpage += "     </tr>";
  webpage += "   </table>";
  webpage += "  </td>";
  webpage += "  <td>";
  webpage += "   <table style='width: 100%'>";
  webpage += "     <tr>";
  webpage += "       <td class='style3'>" + Temperature + "<sup>&deg;C</sup></td>";
  webpage += "   </table>";
  webpage += "  </td>";
  webpage += "  <td>";
  webpage += "   <table style='width: 100%'>";
  webpage += "     <tr>";
  webpage += "       <td>";
  webpage += "        <table>";
  webpage += "          <tr>";
  webpage += "           <td colspan='2' class='style1'>" + Winddirection + "<sup>&deg;</sup></td>";
  webpage += "          </tr>";
  webpage += "          <tr>";
  webpage += "            <td class='style2'>Gusting:</td>";
  webpage += "            <td class='style2'>" + Windgust + "MPH</td>";
  webpage += "          </tr>";
  webpage += "        </table>";
  webpage += "       </td>";
  webpage += "     </tr>";
  webpage += "   </table>";
  webpage += "  </td>";
  webpage += "  <td>";
  webpage += "   <table class = 'style6'>";
  webpage += "     <tr>";
  webpage += "       <td>Pressure </td>";
  webpage += "       <td>" + Pressure + " hPA</td>";
  webpage += "     </tr> ";
  webpage += "    <tr>";
  webpage += "      <td>Windchill </td>";
  webpage += "      <td>" + Windchill + "<sup>&deg;C</sup></td>";
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td>Dew Point </td>";
  webpage += "      <td>" + Dewpoint + "<sup>&deg;C</sup></td>";
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td>Heat Index </td>";
  webpage += "      <td>" + Heatindex + "<sup>&deg;C</sup></td>";
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td>Humidity </td>";
  webpage += "      <td>" + Humidity + " %</td>";
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td>Rainfall </td>";
  webpage += "      <td>" + Preciptoday + " mm </td>";
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td>Rainfall Rate </td>";
  webpage += "      <td>" + Precip1hr + " mm </td>";
  webpage += "    </tr>";
  webpage += "   </table>";
  webpage += "  </td>";
  webpage += "</tr>";
  webpage += "</table><br>";
  append_page_footer();
  server.send(200, "text/html", webpage);
}

void loop() {
  server.handleClient();
  if (wxclient.available()) {
    Serial.println("\nConnected...");
    String Rxdata = "";
    while (wxclient.available()){
      Rxdata = wxclient.readStringUntil('\r');
    }
    const size_t capacity = sizeof(Rxdata) + 1024;
    Serial.println(Rxdata);
    // Typical response:
    //{"observations":[{"stationID":"IMELKSHA2","obsTimeUtc":"2019-07-21T10:45:00Z","obsTimeLocal":"2019-07-21 11:45:00",
    //"neighborhood":"Melksham","softwareType":"weewx-3.9.2","country":"GB",
    //"solarRadiation":null,"lon":-2.12935376,"realtimeFrequency":null,"epoch":1563705900,"lat":51.36468887,
    //"uv":null,"winddir":270,"humidity":69,"qcStatus":1,
    //"metric":{"temp":21,"heatIndex":21,"dewpt":15,"windChill":21,"windSpeed":3,"windGust":11,"pressure":1021.30,"precipRate":0.0,"precipTotal":0.0,"elev":40}}]}

    DynamicJsonDocument doc(capacity);
    const char* json = Rxdata.c_str();
    
    deserializeJson(doc, json);

    JsonObject observations_0 = doc["observations"][0];
    const char* observations_0_stationID    = observations_0["stationID"]; // "KMAHANOV10"
    const char* observations_0_obsTimeUtc   = observations_0["obsTimeUtc"]; // "2019-07-20T22:18:03Z"
    const char* observations_0_obsTimeLocal = observations_0["obsTimeLocal"]; // "2019-07-20 18:18:03"
    const char* observations_0_neighborhood = observations_0["neighborhood"]; // "1505Broadway"
    const char* observations_0_softwareType = observations_0["softwareType"]; // "Rainwise IP-100"
    const char* observations_0_country      = observations_0["country"]; // "US"
    float observations_0_lon                = observations_0["lon"]; // -70.86485291
    long observations_0_epoch               = observations_0["epoch"]; // 1563661083
    float observations_0_lat                = observations_0["lat"]; // 42.09263229
    int observations_0_winddir              = observations_0["winddir"]; // 202
    int observations_0_humidity             = observations_0["humidity"]; // 67
    int observations_0_qcStatus             = observations_0["qcStatus"]; // 1

    JsonObject observations_0_metric        = observations_0["metric"];
    int observations_0_metric_temp          = observations_0_metric["temp"]; // 32
    int observations_0_metric_heatIndex     = observations_0_metric["heatIndex"]; // 39
    int observations_0_metric_dewpt         = observations_0_metric["dewpt"]; // 24
    int observations_0_metric_windChill     = observations_0_metric["windChill"]; // 32
    int observations_0_metric_windSpeed     = observations_0_metric["windSpeed"]; // 3
    int observations_0_metric_windGust      = observations_0_metric["windGust"]; // 3
    float observations_0_metric_pressure    = observations_0_metric["pressure"]; // 1006.1
    int observations_0_metric_precipRate    = observations_0_metric["precipRate"]; // 0
    int observations_0_metric_precipTotal   = observations_0_metric["precipTotal"]; // 0
    int observations_0_metric_elev          = observations_0_metric["elev"]; // 32

    JsonObject observations_0_imperial      = observations_0["imperial"];
    int observations_0_imperial_temp        = observations_0_imperial["temp"]; // 90
    int observations_0_imperial_heatIndex   = observations_0_imperial["heatIndex"]; // 103
    int observations_0_imperial_dewpt       = observations_0_imperial["dewpt"]; // 77
    int observations_0_imperial_windChill   = observations_0_imperial["windChill"]; // 90
    int observations_0_imperial_windSpeed   = observations_0_imperial["windSpeed"]; // 0
    int observations_0_imperial_windGust    = observations_0_imperial["windGust"]; // 2
    float observations_0_imperial_pressure  = observations_0_imperial["pressure"]; // 29.71
    int observations_0_imperial_precipRate  = observations_0_imperial["precipRate"]; // 0
    int observations_0_imperial_precipTotal = observations_0_imperial["precipTotal"]; // 0
    int observations_0_imperial_elev        = observations_0_imperial["elev"]; // 104

    city             = observations_0_stationID;
    country          = observations_0_country;
    date_time        = observations_0_obsTimeUtc;
    observation_time = observations_0_obsTimeLocal;
    Humidity         = observations_0_humidity;
    Winddirection    =  observations_0_winddir;

    observations_0_stationID    = observations_0["stationID"]; // "KMAHANOV10"
    observations_0_obsTimeUtc   = observations_0["obsTimeUtc"]; // "2019-07-20T22:35:31Z"
    observations_0_obsTimeLocal = observations_0["obsTimeLocal"]; // "2019-07-20 18:35:31"
    observations_0_neighborhood = observations_0["neighborhood"]; // "1505Broadway"
    observations_0_softwareType = observations_0["softwareType"]; // "Rainwise IP-100"
    observations_0_country      = observations_0["country"]; // "US"
    observations_0_lat          = observations_0["lat"]; // 42.09263229
    observations_0_lon          = observations_0["lon"]; // -70.86485291
    observations_0_epoch        = observations_0["epoch"]; // 1563662131
    observations_0_winddir      = observations_0["winddir"]; // 202
    observations_0_humidity     = observations_0["humidity"]; // 67
    observations_0_qcStatus     = observations_0["qcStatus"]; // 1

    if (Units == "m") {
      Temperature = observations_0_metric_temp;
      Heatindex   = observations_0_metric_heatIndex;
      Windspeed   = observations_0_metric_windSpeed;
      Windgust    = observations_0_metric_windGust;
      Windchill   = observations_0_metric_windChill;
      Pressure    = observations_0_metric_pressure;
      Dewpoint    = observations_0_metric_dewpt;
      Preciptoday = observations_0_metric_precipTotal;
      Precip1hr   = observations_0_metric_precipRate;
    }
    else
    {
      Temperature = observations_0_imperial_temp;
      Windchill   = observations_0_imperial_windChill;
      Heatindex   = observations_0_imperial_heatIndex;
      Windspeed   = observations_0_imperial_windSpeed;
      Windgust    = observations_0_imperial_windGust;
      Windchill   = observations_0_imperial_windChill;
      Pressure    = observations_0_imperial_pressure;
      Dewpoint    = observations_0_imperial_dewpt;
      Preciptoday = observations_0_imperial_precipTotal;
      Precip1hr   = observations_0_imperial_precipRate;
    }
    wxclient.flush();
    if (Windchill == 0){
      Windchill = Heatindex;
    }
    Serial.println(Temperature);
    Serial.println(Windchill);
    Serial.println(Heatindex);
    Serial.println(Windspeed);
    Serial.println(Windgust);
    Serial.println(Windchill);
    Serial.println(Pressure);
    Serial.println(Dewpoint);
    Serial.println(Preciptoday);
    Serial.println(Precip1hr);
  }
  if (millis() - lastConnectionTime > postingInterval) {
    httpRequest();
  }
}

// this method makes a HTTP connection to the server:
void httpRequest() {
  if (wxclient.connect(wxserver, 80)) {
    Serial.println("Connecting...");
    // Make a HTTP request:
    //http://api.weather.com/v2/pws/observations/current?stationId=KMAHANOV10&format=json&units=e&apiKey=your_API_key
    String api_parameter1 = "GET /v2/pws/observations/current?stationId=" + City +  "&format=json&units="+Units+"&apiKey=" + Key + " HTTP/1.1";
    String api_parameter2 = "Host: " + (String)wxserver;
    String api_parameter3 = "Connection: close";
    wxclient.println(api_parameter1); Serial.println(api_parameter1);
    wxclient.println(api_parameter2);
    wxclient.println(api_parameter3);
    wxclient.println();
    wxclient.println();
    lastConnectionTime = millis();// note time when the connection was made:
  }
  else
  {
    Serial.println("connection failed");  // if connection failed
  }
}

void append_page_header() {
  webpage  = "<!DOCTYPE html><html><head>";
  webpage += "<meta http-equiv='refresh' content='60'>"; // 30-sec refresh time, test needed to prevent auto updates repeating some commands
  webpage += "<title>Weather Webserver</title><style>";
  webpage += "body {width:1020px;margin:0 auto;font-family:arial;font-size:14px;text-align:center;color:blue;background-color:#F7F2Fd;}";
  webpage += "</style></head><body><h1>Autonomous Weather Server " + version + "</h1>";
  webpage += "<h3>Weather Conditions for " + City + "<br>" + observation_time + "</h1>";
}

void append_page_footer() { // Saves repeating many lines of code for HTML page footers
  webpage += "<head><style>ul{list-style-type:none;margin:0;padding:0;overflow:hidden;background-color:#d8d8d8;font-size:14px;}";
  webpage += "li{float:left;border-right:1px solid #bbb;}last-child {border-right: none;}";
  webpage += "li a{display: block;padding:3px 15px;text-decoration:none;}";
  webpage += "li a:hover{background-color:#FFFFFF;}";
  webpage += "section {font-size:14px;}";
  webpage += "p {background-color:#E3D1E2;}";
  webpage += "h1{background-color:#d8d8d8;}";
  webpage += "h3{color:#9370DB;font-size:24px; line-height: 75%;}";
  webpage += "table{font-family: arial, sans-serif;font-size:16px;border-collapse: collapse;width: 100%;}";
  webpage += "td, th {border: 0px solid black;text-align: left;padding: 2px;}";
  webpage += ".style1 {text-align:center;font-size:30px; background-color:#D8BFD8;}";
  webpage += ".style2 {text-align:center;font-size:16px; background-color:#ADD8E6;}";
  webpage += ".style3 {text-align:center;font-size:60px; background-color:#FFE4B5;}";
  webpage += ".style4 {text-align:center;font-size:16px; background-color:#FFE4B5;}";
  webpage += ".style5 {text-align:center;font-size:20px; background-color:#D9BFD9;}";
  webpage += ".style6 {text-align:center;font-size:14px; background-color:#B0C4DE; width:100%;}";
  webpage += "sup{vertical-align:super;font-size:smaller;}";
  webpage += "</style>";
  webpage += "<ul>";
  webpage += "<li><a href='/homepage'>Home Page</a></li>";
  webpage += "<li><a href='/'>System Setup</a></li>";
  webpage += "</ul>";
  webpage += "<p>&copy; Weather Underground Data and Icons 2017<br>";
  webpage += "&copy;" + String(char(byte(0x40 >> 1))) + String(char(byte(0x88 >> 1))) + String(char(byte(0x5c >> 1))) + String(char(byte(0x98 >> 1))) + String(char(byte(0x5c >> 1)));
  webpage += String(char((0x84 >> 1))) + String(char(byte(0xd2 >> 1))) + String(char(0xe4 >> 1)) + String(char(0xc8 >> 1)) + String(char(byte(0x40 >> 1)));
  webpage += String(char(byte(0x64 / 2))) + String(char(byte(0x60 >> 1))) + String(char(byte(0x62 >> 1))) + String(char(0x6e >> 1)) + "</p>";
  webpage += "</body></html>";
}

void SystemSetup() {
  webpage = ""; // don't delete this command, it ensures the server works reliably!
  append_page_header();
  String IPaddress = WiFi.localIP().toString();
  webpage += "<h3>System Setup, if required enter values then Enter</h3>";
  webpage += "<form action=\"http://" + IPaddress + "\" method=\"POST\">";
  webpage += "City<br><input type='text' name='wu_city' value='" + City + "'><br>";
  webpage += "Country<br><input type='text' name='wu_country' value='" + Country + "'><br>";
  webpage += "<input type='submit' value='Enter'><br><br>";
  webpage += "</form></body>";
  append_page_footer();
  server.send(200, "text/html", webpage); // Send a response to the client to enter their inputs, if needed, Enter=defaults
  if (server.args() > 0 ) { // Arguments were received
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      String Argument_Name   = server.argName(i);
      String client_response = server.arg(i);
      if (Argument_Name == "wu_city") {
        City = client_response;
      }
      if (Argument_Name == "wu_country") {
        Country = client_response;
      }
    }
  }
  webpage = "";
  httpRequest();
}
