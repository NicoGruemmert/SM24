#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // Include ArduinoJson library
#include <omegaPlant.h>

#include "Adafruit_VEML6075.h"

Adafruit_VEML6075 uv = Adafruit_VEML6075();
PlantProfile profile;
omegaPlant myPlant = omegaPlant(profile);

#define ID 1
uint8_t hi = (uint8_t)'P';
uint8_t counter = 1;
uint16_t id = (hi << 8) | (counter & 0xFF);

#define LED_PIN 15
#define MQTT_PASSWORD "49937025" // Define the MQTT password here

const char *ssid = "OMEGA";  // Replace with your Wi-Fi SSID
const char *password = "49937025"; // Replace with your Wi-Fi password
const char *mqtt_server = "192.168.2.5";
const char *mqtt_user = "containership";
const char *mqtt_pass = MQTT_PASSWORD;

const char *sensor_topic = "plantpal/sensor";
const char *config_topic = "plantpal/config";
const char *alive_topic = "plantpal/alive";

const char * friendly_name = "PotPal8A";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Start connecting to Wi-Fi
  WiFi.begin(ssid, password);

  uint16_t counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter < 5000) {
    counter++;
    if (counter % 10 == 0) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    delay(1);
  }

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect to WiFi");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(friendly_name, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // Once connected, publish an "alive" message
      client.publish(alive_topic, friendly_name);
      // Subscribe to the configuration topic
      //client.subscribe(config_topic);
      client.subscribe(sensor_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  delay(1000);
  Wire.begin(17,16);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  setup_wifi();
  client.setKeepAlive(60);
  client.setServer(mqtt_server, 1883);

  uv.begin();
  uv.setHighDynamic(true);
    
  // Set the calibration coefficients
  uv.setCoefficients(2.22, 1.33,  // UVA_A and UVA_B coefficients
                     2.95, 1.74,  // UVB_C and UVB_D coefficients
                     0.001461, 0.002591); // UVA and UVB responses

    
}


void publishPlantState() {
  StaticJsonDocument<256> doc;
                    
  
    doc["name"] = profile.name;
    doc["tempc"] = profile.tempc;
    doc["hum"] = profile.hum;
    doc["soil_moisture"] = profile.soil_moisture;
    doc["light"] = profile.light;
    doc["range_temp"] = profile.range_temp;
    doc["range_hum"] = profile.range_hum;
    doc["range_light"] = profile.range_light;
    doc["range_soil_moisture"] = profile.range_soil_moisture;

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  client.publish(config_topic, jsonBuffer);
}


void publishSensorData() {
  StaticJsonDocument<256> doc;
  // Assuming you have functions to get the actual sensor values
  float temperature = 25.6 + random(-50, 50) / 10.0;  // Rauschen zwischen -5.0 und 5.0
  float humidity = 60.2 + random(-50, 50) / 10.0;     // Rauschen zwischen -5.0 und 5.0
  int soilMoisture = 70 + random(-5, 5);              // Rauschen zwischen -5 und 5
  int light = 100 + random(-10, 10);                  // Rauschen zwischen -10 und 10
  int mood = 80 + random(-5, 5);                      // Rauschen zwischen -5 und 5
  static int xp = 1;                         
  
  doc["id"] = id;
  doc["tempc"] = round(temperature);
  doc["hum"] = round(humidity);
  doc["soilm"] = soilMoisture;
  doc["light"] = light;
  doc["mood"] = mood;
  doc["xp"] = xp;
  
  
  doc["freeHeap"] = esp_get_free_heap_size();
  
  xp++;
  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  client.publish(sensor_topic, jsonBuffer);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  // Handle the configuration message if needed
}

sensorData getMeasureMents()
{
  sensorData newData;
  newData.lightIntensity = uv.readUVA();

  return newData;

}


void loop() {


  if (!client.connected()) {
    delay(2000);
    Serial.println("Reconnect");
    reconnect();
  }
  client.loop();

  // Publish sensor data every 10 seconds
  static unsigned long lastPublishTime = 0;
  static unsigned long lastConfigTime = 0;
  if (millis() - lastPublishTime > 2000) {
    myPlant.getMeasurement(getMeasureMents());
    publishSensorData();
    lastPublishTime = millis();
  }

   if (millis() - lastConfigTime > 20000) {
    publishPlantState();
    lastConfigTime = millis();
  }
  delay(100);
}