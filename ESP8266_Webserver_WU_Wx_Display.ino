/* 
 ESP8266 Weather Webserver provides near automous display of weather data
 The 'MIT License (MIT) Copyright (c) 2016 by David Bird'. Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
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
//#include <HttpClient.h>
#include <TextFinder.h>
#include <DNSServer.h>
//################ VARIABLES ################

//------ NETWORK VARIABLES---------
const char *ssid      = "your_wifi_ssid_here";
const char *password  = "your_wifi_password_here";
String Key            = "xxxxxxxxxxxxxxxx";         // See: http://www.wunderground.com/weather/api/d/docs (change here with your KEY)
String City           = "Bath";
String Country        = "UK";
String Conditions     = "conditions";               // See: http://www.wunderground.com/weather/api/d/docs?d=data/index&MR=1
char   wxserver[]     = "api.wunderground.com";     // Address for WeatherUnderGround
unsigned long    lastConnectionTime = 0;            // Last time you connected to the server, in milliseconds
const unsigned long postingInterval = 300L * 1000L; // Delay between updates, in milliseconds, WU allows 500 requests per-day maximum and no more than 10 per-minute!
char buffer[256];
int rx_cnt = 0;

//################ PROGRAM VARIABLES and OBJECTS ################
String webpage, city, country, date_time, observation_time,
       forecast, temperatureC, feelslikeC, temperatureF, feelslikeF, relhumidity, heat_indexC, heat_indexF, 
       wind_desc, wind_direction, wind_direction_deg, wind_speedMPH, wind_speedKPH, wind_gustMPH, wind_gustKPH, windchillC, windchillF, windchill_string,
       pressureMB, pressureIN, pressure_trend,
       dewpointC, dewpointF,
       visibilityMI, visibilityKM,
       precip_1hr_in, precip_1hr_metric, precip_1hr_string, precip_today, precip_today_in, precip_today_metric,
       solarradiation, icon_filename;
       
WiFiClient wxclient;
TextFinder finder(wxclient);
String version = "v1.0";     // Version of this program
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
  // wifiManager.resetSettings(); // Command to be included if needed, then connect to http://192.168.4.1/ and follow instructions to make the WiFi connection
  // Set a timeout until configuration is turned off, useful to retry or go to sleep in n-seconds
  wifiManager.setTimeout(180);
  //fetches ssid and password and tries to connect, if connections succeeds it starts an access point with the name called "ESP8266_AP" and waits in a blocking loop for configuration
  if(!wifiManager.autoConnect("ESP8266_AP")) {
    Serial.println(F("failed to connect and timeout occurred"));
    delay(6000);
    ESP.reset(); //reset and try again
    delay(180000);
  }
  // At this stage the WiFi manager will have successfully connected to a network, or if not will try again in 180-seconds
  Serial.println(F("WiFi connected.."));
  //----------------------------------------------------------------------
  server.begin(); Serial.println(F("Webserver started...")); // Start the webserver
  Serial.println("Use this URL to connect: http://"+WiFi.localIP().toString()+"/");// Print the IP address
  server.on("/", SystemSetup);
  server.on("/homepage", homepage);
  //httpRequest(Conditions);
}

void homepage(){
  httpRequest(Conditions);
  append_page_header(); 
  webpage += "<table align='center' style='width: 100%; height: 200px'>";
  webpage += "<tr>";
  webpage += "  <td>";
  webpage += "  <table style='width: 100%'>";
  webpage += "    <tr>";
  webpage += "      <td>";
  webpage += "         <img src='"+icon_filename+"' alt='Weather Symbol' style='width:150px;height:150px;'>";
  webpage += "      </td>";
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td class='style5'>"+forecast+"</td>";
  webpage += "    </tr>";
  webpage += "  </table>";
  webpage += "  </td>";
  webpage += "  <td>";
  webpage += "  <table style='width: 100%'>";
  webpage += "    <tr>";
  webpage += "      <td class='style3'>"+temperatureC+"<sup>&deg;C</sup></td>";
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td class='style4'>Feels like</td>";
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td class='style4'>"+feelslikeC+"<sup>&deg;C</sup></td>";
  webpage += "    </tr>";
  webpage += "  </table>";
  webpage += "  </td>";
  webpage += "  <td>";
  webpage += "  <table style='width: 100%'>";
  webpage += "    <tr>";
  webpage += "      <td>";
  webpage += "      <table>";
  webpage += "        <tr>";
  webpage += "         <td colspan='2' class='style1'>"+wind_direction+"</td>";
  webpage += "          </tr>";
  webpage += "        <tr>";
  webpage += "         <td colspan='2' class='style1'>("+wind_direction_deg+"<sup>&deg;</sup>)</td>";
  webpage += "        </tr>";
  webpage += "        <tr>";
  webpage += "          <td class='style2'>Wind Description:</td>";
  webpage += "          <td class='style2'>"+wind_desc+"</td>";
  webpage += "        </tr>";
  webpage += "        <tr>";
  webpage += "          <td class='style2'>Gusting:</td>";
  webpage += "          <td class='style2'>"+wind_gustMPH+" MPH</td>";
  webpage += "        </tr>";
  webpage += "      </table>";
  webpage += "      </td>";
  webpage += "    </tr>";
  webpage += "  </table>";
  webpage += "  </td>";
  webpage += "  <td>";
  webpage += "  <table class='style6'>"; // style='width: 100%'
  webpage += "    <tr>";
  webpage += "      <td>Pressure</td>";
  webpage += "      <td>"+pressureMB+" hPA</td>";
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td>Pressure Trend</td>";
  webpage += "      <td>"+pressure_trend+"</td>";
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td>Visibility</td>";
  webpage += "      <td>"+visibilityMI+" Mile(s)</td>";
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td>Windchill</td>";
  webpage += "      <td>"+(windchillC=="NA"?windchillC:windchillC+"<sup>&deg;C</sup>"+"</td>");
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td>Dew Point</td>";
  webpage += "      <td>"+dewpointC+"<sup>&deg;C</sup></td>";
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td>Heat Index</td>";
  webpage += "      <td>"+(heat_indexC=="NA"?heat_indexC:heat_indexC+"<sup>&deg;C</sup>"+"</td>");
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td>Humidity</td>";
  webpage += "      <td>"+relhumidity+"%</td>";
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td>Rainfall</td>";
  webpage += "      <td>"+precip_today_metric+" mm</td>";
  webpage += "    </tr>";
  webpage += "    <tr>";
  webpage += "      <td>Solar Radiation</td>";
  webpage += "      <td>"+solarradiation+"</td>";
  webpage += "    </tr>";
  webpage += "    </table>";
  webpage += "  </td>";
  webpage += "</tr>";
  webpage += "</table><br>";
  append_page_footer();
  server.send(200, "text/html", webpage);
}

void loop() {
  server.handleClient();
  Serial.print(".");delay(1000);
  if (wxclient.connected() && wxclient.available()) {
    Serial.println("\nConnected...");
    if (finder.find("\"city\":")){
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
         city = buffer;
         Serial.println("              City: "+city);
      }
    }
    finder.find("\"country\":");
    if(finder.getString("\"","\"",buffer,sizeof(buffer))){
      country = buffer;
      Serial.println("           Country: "+country);
    }
    if (finder.find("observation_time\":")){
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        observation_time = buffer;
        Serial.println("  Observation time: "+observation_time);
      }
    }
    if (finder.find("local_time_rfc822\":\"")){
      date_time = wxclient.readStringUntil('+');
        Serial.println("         Date-Time: "+date_time);
    }
    if (finder.find("\"weather\":")) {
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        forecast = buffer;
        Serial.println("          Forecast: "+forecast);
      }
    }
    if(finder.find("temp_f")){      
      float temp = finder.getFloat();
      temperatureF = String(temp,1);
      Serial.println("       Temperature: "+temperatureF+char(176)+"C");
    }
    if(finder.find("temp_c")){      
      float temp = finder.getFloat();
      temperatureC = String(temp,1);
      Serial.println("       Temperature: "+temperatureC+char(176)+"C");
    }
    if(finder.find("relative_humidity")){      
      int relative_humidity = finder.getValue();
      relhumidity  = String(relative_humidity);
      Serial.println("     Rel. Humidity: "+relhumidity +"%");
    }
    if (finder.find("\"wind_string\":")) {
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        wind_desc = buffer;
        Serial.println("        Wind Desc.: "+wind_desc);
      }
    }
    if (finder.find("\"wind_dir\":")) {
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        wind_direction = buffer;
        Serial.print("    Wind Direction: "+wind_direction);
      }
    }
    if(finder.find("\"wind_degrees\":")) {
      float WindDirDeg = finder.getFloat();      
      wind_direction_deg = String(WindDirDeg,0);
      Serial.println("("+wind_direction_deg+"-deg"+")");
    }
    if(finder.find("\"wind_mph\":")) {
      float WindSpeed = finder.getFloat();
      wind_speedMPH = String(WindSpeed,0);
      Serial.println("        Wind Speed: "+wind_speedMPH+" mph");
    }
    if(finder.find("\"wind_gust_mph\":")) {
      float WindGust = finder.getFloat();
      wind_gustMPH = String(WindGust,0);
      Serial.println("   Wind gusting to: "+wind_gustMPH+" mph");
    }
    if(finder.find("\"wind_kph\":")) {
      float WindSpeed = finder.getFloat();
      wind_speedKPH = String(WindSpeed,0);
      Serial.println("        Wind Speed: "+wind_speedKPH+" kph");
    }
    if(finder.find("\"wind_gust_kph\":")) {
      float WindGust = finder.getFloat();
      wind_gustKPH = String(WindGust,0);
      Serial.println("   Wind gusting to: "+wind_gustKPH+" kph");
    }
    if (finder.find("\"pressure_mb\":")) {
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        pressureMB = buffer;     
        Serial.println("       Pressure mB: "+pressureMB+" hPA");
      }
    }
    if (finder.find("\"pressure_in\":")) {
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        pressureIN = buffer;     
        Serial.println("        Pressure \": "+pressureIN+" hPA");
      }
    }
    if (finder.find("\"pressure_trend\":")) {
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        pressure_trend = buffer;
        if (pressure_trend == "-") pressure_trend = "Falling";
        if (pressure_trend == "0") pressure_trend = "Steady";
        if (pressure_trend == "+") pressure_trend = "Rising";
        Serial.println("    Pressure trend: ("+pressure_trend+")");
      }
    }
    if (finder.find("dewpoint_f\":")) { 
      float dewpF = finder.getFloat();
      dewpointF = String(dewpF,1);
      Serial.println("      Dew Point(F): "+dewpointF+char(176)+"F");
    }
    if (finder.find("dewpoint_c\":")) { 
      float dewpC = finder.getFloat();
      dewpointC = String(dewpC,1);
      Serial.println("      Dew Point(C): "+dewpointC+char(176)+"C");
    }
    if(finder.find("heat_index_f\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        heat_indexF = buffer;
        Serial.println("     Heat Index(F): "+(heat_indexF=="NA"?heat_indexF:heat_indexF+char(176)+"F"));
      }
    }
    if(finder.find("heat_index_c\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        heat_indexC = buffer;
        Serial.println("     Heat Index(C): "+(heat_indexC=="NA"?heat_indexC:heat_indexC+char(176)+"C"));
      }
    }
    if(finder.find("windchill_string\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        windchill_string = buffer;
        Serial.println("     Wind Chill(C): "+windchill_string);
      }
    }
    if(finder.find("windchill_f\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        windchillF = buffer;
        Serial.println("     Wind Chill(C): "+(windchillF=="NA"?windchillF:windchillF+char(176)+"F"));
      }
    }
    if(finder.find("windchill_c\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        windchillC = buffer;
        Serial.println("     Wind Chill(F): "+(windchillC=="NA"?windchillC:windchillC+char(176)+"C"));
      }
    }
    if(finder.find("feelslike_f\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        feelslikeF = buffer;
        Serial.println("        Feels like: "+feelslikeF+char(176)+"F");
      }
    }
    if(finder.find("feelslike_c\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        feelslikeC = buffer;
        Serial.println("        Feels like: "+feelslikeC+char(176)+"C");
      }
    }
    if(finder.find("visibility_mi\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        visibilityMI = buffer;
        Serial.println("        Visibility: "+visibilityMI+" Miles");
      }
    }
    if(finder.find("visibility_km\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        visibilityKM = buffer;
        Serial.println("        Visibility: "+visibilityKM+" Km");
      }
    }
    if(finder.find("solarradiation\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        solarradiation = buffer;
        Serial.println("   Solar Radiation: "+solarradiation);
      }
    }
    if(finder.find("precip_1hr_string\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        precip_1hr_string = buffer;
        Serial.println("Last hour rain(in): "+precip_1hr_string);
      }
    }
    if(finder.find("precip_1hr_in\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        precip_1hr_in = buffer;
        Serial.println("Last hour rain(in): "+precip_1hr_in);
      }
    }
    if(finder.find("precip_1hr_metric\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        precip_1hr_metric = buffer;
        Serial.println("Last hour rain(mm): "+precip_1hr_metric+"mm");
      }
    }
    if(finder.find("precip_today_string\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        precip_today = buffer;
        Serial.println("    Rainfall today: "+precip_today);
      }
    }
    if(finder.find("precip_today_in\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        precip_today_in = buffer;
        Serial.println("Rainfall today(in): "+precip_today_in);
      }
    }
    if(finder.find("precip_today_metric\":")){      
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        precip_today_metric = buffer;
        Serial.println("Rainfall today(mm): "+precip_today_metric);
      }
    }
    // "icon_url":"http://icons.wxug.com/i/c/k/cloudy.gif",
    if(finder.find("icon_url\":")){
      if(finder.getString("\"","\"",buffer,sizeof(buffer))){
        icon_filename = buffer;
        Serial.println("     Icon Filename: "+icon_filename);
      }
    } 
    Serial.println();Serial.print("(");
    Serial.print(rx_cnt++);
    Serial.print(")");
    wxclient.flush();
  }
  if (millis() - lastConnectionTime > postingInterval) {
    httpRequest(Conditions);
  }
}

// this method makes a HTTP connection to the server:
void httpRequest(String Request) {
  if (wxclient.connect(wxserver, 80)) {
    Serial.println("Connecting...");
    // Make a HTTP request:
    String api_parameter1 = "GET /api/" + Key + "/" + Request + "/q/" + Country + "/" + City + ".json HTTP/1.1"; 
    String api_parameter2 = "Host: " + (String)wxserver;
    String api_parameter3 = "Connection: close";
    wxclient.println(api_parameter1);Serial.println(api_parameter1);
    wxclient.println(api_parameter2);
    wxclient.println(api_parameter3);
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
  webpage += "<h3>Weather Conditions for " + City + "<br>"+observation_time+"</h1>";
}

void append_page_footer(){ // Saves repeating many lines of code for HTML page footers
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
  webpage += "  <li><a href='/homepage'>Home Page</a></li>";
  webpage += "  <li><a href='/'>System Setup</a></li>";
  webpage += "</ul>";
  webpage += "<p>&copy; Weather Underground Data and Icons 2017<br>";
  webpage += "&copy;"+String(char(byte(0x40>>1)))+String(char(byte(0x88>>1)))+String(char(byte(0x5c>>1)))+String(char(byte(0x98>>1)))+String(char(byte(0x5c>>1)));
  webpage += String(char((0x84>>1)))+String(char(byte(0xd2>>1)))+String(char(0xe4>>1))+String(char(0xc8>>1))+String(char(byte(0x40>>1)));
  webpage += String(char(byte(0x64/2)))+String(char(byte(0x60>>1)))+String(char(byte(0x62>>1)))+String(char(0x6e>>1))+"</p>";
  webpage += "</body></html>";
}

void SystemSetup() {
  webpage = ""; // don't delete this command, it ensures the server works reliably!
  append_page_header();
  String IPaddress = WiFi.localIP().toString();
  webpage += "<h3>System Setup, if required enter values then Enter</h3>";
  webpage += "<form action=\"http://"+IPaddress+"\" method=\"POST\">";
  webpage += "City<br><input type='text' name='wu_city' value='"+City+"'><br>";
  webpage += "Country<br><input type='text' name='wu_country' value='"+Country+"'><br>";
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
  httpRequest(Conditions);
}


