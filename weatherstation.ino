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

// Replace with your network credentials
static const char* ssid     = "SSID";         // The SSID (name) of the Wi-Fi network you want to connect to
static const char* password = "PASSWORD";     // The password of the Wi-Fi network
static const char* telegram_token = "XXXXXX:YYYYYYYYYYY"; // Telegram bot
static const char* chat_id = "YOUR_CHAT_ID";

// Constants
#define DHTPIN 12 // Change this pin by yours
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;
const float maxTemp = 26.0; // Max temperature for alerting system
const float minTemp = 18.0; // Min temperature for alerting system
const float maxHum = 85.0;  // Max humidity for alerting system
const float minHum = 50.0;  // Min humidity for alerting system

int bot_mtbs = 1000; //mean time between scan messages
long bot_lasttime;
bool ack = false; // Alert acknowledge flag

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previous_millis = 0;    // will store last time DHT was updated

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
  <h2>Weather station</h2>
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
    <sup class="units">%</sup>
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
}

// Verify sensor values against max/min boundaries
void checkSensorValues(const float metric_value, const float max_metric,
                       const float min_metric, const String metric_name) {
  if (metric_value == 0.0) {
    Serial.println("Still initializaing sensor...");
  } else {
    if (((metric_value > max_metric) or (metric_value < minTemp)) and (not ack)) {
      String msg = "Alert!!! " + metric_name + " is ";
      msg = msg + metric_value;
      tbot.sendMessage(chat_id, msg, "");
    }
  }
}

// Monitor temperature
void checkTemp() {
  checkSensorValues(t, maxTemp, minTemp, "Temperature (Celsius)");
}

// Monitor humidity
void checkHum() {
  checkSensorValues(h, maxHum, minHum, "Humidity (%)");
}

// Initialize Telegram bot
WiFiClientSecure net_ssl; 
UniversalTelegramBot tbot(telegram_token, net_ssl);

// Start AsyncWebServer on port 80
AsyncWebServer server(80);

void setup(){
  // Serial port for debugging purposes
  Serial.begin(9600);

  // Start sensor
  dht.begin();
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

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

  // Start server
  server.begin();

  // Configure SSL for telegram
  net_ssl.setInsecure();
}
 
void loop(){
  unsigned long current_millis = millis();
  if (current_millis - previous_millis >= interval) {
    // save the last time you updated the DHT values
    previous_millis = current_millis;
    // Read temperature as Celsius (the default)
    float new_temp = dht.readTemperature();
    // if temperature read failed, don't change t value
    if (isnan(new_temp)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      t = new_temp;
      Serial.println(t);
      checkTemp();
    }
    // Read Humidity
    float new_hum = dht.readHumidity();
    // if humidity read failed, don't change h value 
    if (isnan(new_hum)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      h = new_hum;
      Serial.println(h);
      checkHum();
    }
  }

  if (millis() > bot_lasttime + bot_mtbs)  {
    int num_new_messages = tbot.getUpdates(tbot.last_message_received + 1);

    while(num_new_messages) {
      Serial.println("got response");
      for (int i=0; i<num_new_messages; i++) {
        String text = tbot.messages[i].text;
        // Support commands with upper chars (telegram uppercase autocorrector is a pain in the ass)
        text.toLowerCase();
        processMessage(tbot.messages[i].chat_id, text);
      }
      num_new_messages = tbot.getUpdates(tbot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }
}
