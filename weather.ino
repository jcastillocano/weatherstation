// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFiClientSecure.h> 
#include <UniversalTelegramBot.h>
#include <SFE_BMP180.h>
#include <Wire.h>

// Replace with your network credentials
static const char* ssid     = "MIWIFI_6149";         // The SSID (name) of the Wi-Fi network you want to connect to
static const char* password = "5UNCPFHH";     // The password of the Wi-Fi network
static const char* telegram_token = "918886039:AAGCF6zktpxS6jYIXdp7vpreeDZxgdGgTW0"; // Telegram bot
static const char* chatId = "56377557";
// Constants
#define DHTPIN 12 // Change this pin by yours
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

SFE_BMP180 pressure;
#define ALTITUDE 500.9 // Altitude in La Laguna
char status;
double T,P,p0,a;

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;
const float maxTemp = 28.0; // Max temperature for alerting system
const float minTemp = 15.0; // Min temperature for alerting system
const float maxHum = 90.0;  // Max humidity for alerting system
const float minHum = 50.0;  // Min humidity for alerting system

WiFiClientSecure net_ssl; 
UniversalTelegramBot tbot(telegram_token, net_ssl);
int Bot_mtbs = 1000; //mean time between scan messages
long Bot_lasttime;
bool ack = false; 

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 10 seconds
const long interval = 10000;  

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>Mateo's room</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units"></sup>
  </p>
  <p>
    <i class="fas fa-sun" style="color:#f0d050;"></i> 
    <span class="dht-labels">Pressure</span>
    <span id="pressure">%PRESSURE%</span>
    <sup class="units">mb</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;


setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("pressure").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/pressure", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(t);
  }
  else if(var == "HUMIDITY"){
    return String(h);
  }
  else if(var == "PRESSURE"){
    return String(p0);
  }
  return String();
}

void processMessage(String chat_id, String text) { 
  if (text == "ack") {   
    ack = true;   
    tbot.sendMessage(chat_id, "Ack received. No more alerts", "");   
  }   
  else if (text == "ack_off") {   
    ack = false;
    tbot.sendMessage(chat_id, "Ack disabled. Ready to receive alerts", "");   
  }  
  else if (text == "ping") {   
    tbot.sendMessage(chat_id, "pong!", "");   
  } 
  else if (text == "help") {
    String help = "List of commands:\n";
    help = help + " * ack: ignore future alerts\n";
    help = help + " * ack_off: enable alerting\n";
    help = help + " * ping: classic ping<->pong\n";
    help = help + " * temperature: show current temperature\n";
    help = help + " * humidity: show current humidity\n";
    help = help + " * pressure: show current pressure\n";
    help = help + " * help: show this message";
    tbot.sendMessage(chat_id, help, "");   
  }
  else if (text == "temperature") {
    String msg = "Temperature is: ";
    msg = msg + t;
    tbot.sendMessage(chat_id, msg, "");
  }
  else if (text == "humidity") {
    String msg = "Humidity is: ";
    msg = msg + h;
    tbot.sendMessage(chat_id, msg), "";    
  }
  else if (text == "pressure") {
    String msg = "Pressure is: ";
    msg = msg + p0;
    tbot.sendMessage(chat_id, msg), "";    
  }
}

void checkSensorValues(const float metric_value, const float max_metric,
                       const float min_metric, const String metric_name) {
  if (metric_value == 0.0) {
    Serial.println("Still initializaing sensor...");
  } else {
    if (((metric_value > max_metric) or (metric_value < minTemp)) and (not ack)) {
      String msg = "Alert!!! " + metric_name + " is ";
      msg = msg + metric_value;
      tbot.sendMessage(chatId, msg, "");
    }
  }
}

void checkTemp() {
  checkSensorValues(t, maxTemp, minTemp, "Temperature (Celsius)");
}

void checkHum() {
  checkSensorValues(h, maxHum, minHum, "Humidity (%)");
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(9600);
  dht.begin();
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi\n");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  if (pressure.begin())
    Serial.println("BMP180 init success\n");
  else
  {
    Serial.println("BMP180 init fail\n\n");
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(t).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(h).c_str());
  });
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(p0).c_str());
  });

  // Start server
  server.begin();

  // Configure SSL for telegram
  net_ssl.setInsecure();
}
 
void loop(){
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;
    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      t = newT;
      Serial.print("Temperature: ");
      Serial.print(t);
      Serial.println("C"); 
      checkTemp();
    }
    // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value 
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      h = newH;
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.println("%"); 
      checkHum();
    }
    // Read pressure
    status = pressure.startPressure(3);
    if (status != 0) {
      delay(status);
      status = pressure.getPressure(P,T);
      if (status != 0) {
        Serial.print("Absolute pressure: ");
        Serial.print(P,2);
        Serial.println(" mb, ");
        p0 = pressure.sealevel(P,ALTITUDE);
        Serial.print("Relative (sea-level) pressure: ");
        Serial.print(p0,2);
        Serial.println(" mb, ");
      }
      else Serial.println("error retrieving pressure measurement\n");
    }
    else Serial.println("error starting pressure measurement\n");
  }
  if (millis() > Bot_lasttime + Bot_mtbs)  {
    int numNewMessages = tbot.getUpdates(tbot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      for (int i=0; i<numNewMessages; i++) {
        String text = tbot.messages[i].text;
        text.toLowerCase();
        processMessage(tbot.messages[i].chat_id, text);
      }
      numNewMessages = tbot.getUpdates(tbot.last_message_received + 1);
    }

    Bot_lasttime = millis();
  }
}
