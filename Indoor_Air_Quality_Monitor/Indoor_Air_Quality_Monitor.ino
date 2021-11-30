#include "SparkFun_SGP30_Arduino_Library.h" 
#include <Wire.h>
SGP30 mySensor;

#include <QubitroMqttClient.h>
#include <WiFi.h>

// WiFi Client
WiFiClient wifiClient;

// Qubitro Client
QubitroMqttClient mqttClient(wifiClient);

// Device Parameters
char deviceID[] = "Your Device ID here";
char deviceToken[] = "Your Device Token Here";

// WiFi Parameters
const char* ssid = "WiFi SSID here";
const char* password = "WiFi Password here";

float co2;
float tvoc;

#define uS_TO_S_FACTOR 1000000  
#define TIME_TO_SLEEP  300

void print_wakeup_reason()
{
esp_sleep_wakeup_cause_t wakeup_reason;

wakeup_reason = esp_sleep_get_wakeup_cause();
Serial.println();
Serial.println();
Serial.println();
switch(wakeup_reason)
{
case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
}
}

void setup() 
{

serial_init();

print_wakeup_reason();
esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds"); 
  
  // Initialize wireless connectivity
  wifi_init();

  // Initialize Qubitro
  qubitro_init();
  Wire.begin();
  //Initialize sensor
  if (mySensor.begin() == false) {
    Serial.println("No SGP30 Detected. Check connections.");
    while (1);
  }
  //Initializes sensor for air quality readings
  //measureAirQuality should be called in one second increments after a call to initAirQuality
  mySensor.initAirQuality();  
}

void loop() 
{

for (int i = 0; i <31; i++)
{
  delay(1000); //Wait 1 second
  //measure CO2 and TVOC levels
  mySensor.measureAirQuality();
  Serial.print("CO2: ");
  Serial.print(mySensor.CO2);
  Serial.print(" ppm\tTVOC: ");
  Serial.print(mySensor.TVOC);
  Serial.println(" ppb");  
}


  //measure CO2 and TVOC levels
  mySensor.measureAirQuality();
  Serial.print("CO2: ");
  Serial.print(mySensor.CO2);
  Serial.print(" ppm\tTVOC: ");
  Serial.print(mySensor.TVOC);
  Serial.println(" ppb");
  tvoc = mySensor.TVOC;
  co2 = mySensor.CO2; 
  
  // Send telemetry
  String payload = "{\"CO2\":" + String(co2)
    + ",\"TVOC\":" + String(tvoc) + "}";
  mqttClient.poll();
  mqttClient.beginMessage(deviceID);
  mqttClient.print(payload);
  mqttClient.endMessage();

  delay(1000);

  esp_deep_sleep_start();
  

}

// Initialization code

void serial_init() 
{

Serial.begin(115200);
delay(200);
}

void wifi_init() {
  // Set WiFi mode
  WiFi.mode(WIFI_STA);

  // Disconnect WiFi
  WiFi.disconnect();
  delay(100);

  // Initiate WiFi connection
  WiFi.begin(ssid, password);

  // Print connectivity status to the terminal
  Serial.print("Connecting to WiFi...");
  while(true)
  {
    delay(1000);
    Serial.print(".");
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("");
      Serial.println("WiFi Connected.");
      Serial.print("Local IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("RSSI: ");
      Serial.println(WiFi.RSSI());
      break;
    }
  }
}

void qubitro_init() {
  char host[] = "broker.qubitro.com";
  int port = 1883;
  mqttClient.setId(deviceID);
  mqttClient.setDeviceIdToken(deviceID, deviceToken);
  Serial.println("Connecting to Qubitro...");
  if (!mqttClient.connect(host, port))
  {
    Serial.print("Connection failed. Error code: ");
    Serial.println(mqttClient.connectError());
    Serial.println("Visit docs.qubitro.com or create a new issue on github.com/qubitro");
  }
  Serial.println("Connected to Qubitro.");
  mqttClient.subscribe(deviceID);
}
