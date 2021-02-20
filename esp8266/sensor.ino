#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <DHT.h>

// WIFI credentials
const char ssid1[] = "xxx";
const char psk1[] = "xxx";
//const char ssid2[] = "xxx";
//const char psk2[] = "xxx";

// MQTT Broker address
const char mqttBroker[] = "xxx"; // mqtt broker runs as service container on RaspberryPi, i.e. "192.168.0.10"
#define MSG_BUFFER_SIZE  (255)
char msg[MSG_BUFFER_SIZE];

// Influxdb label fields
const char influxOpts[] = "plant=gummibaum,location=arbeitszimmer"; // comma separated, NO ending comma

// OTA credentials
const char OTAName[] = "ESP8266";      
const char OTAPassword[] = "xxx";

// Samplerate in samples per second for read sensor values and send to MQTT Broker
#define SAMPLE_RATE 1 // Samples per second
const unsigned long samplePeriod = 1000 / (SAMPLE_RATE);
unsigned long lastMillis = 0;
 
WiFiClient espClient;
ESP8266WiFiMulti wifiMulti;    // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'

PubSubClient client(espClient);

// Sensor settings
const int analogInPin = A0;  // ESP8266 Analog Pin ADC0 = A0
#define DHTPIN 5     // Digital pin D1 connected to the DHT sensor
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);
 
void setup() {
    Serial.begin(115200);        // Start the Serial communication to send messages to the computer
    delay(10);
    Serial.println("\n");
    startWiFi();                 // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
    startOTA();                  // Start the OTA service
    startMQTT();                 // Start a connection to a given MQTT Broker
    startSensors();              // Start sensor specific stuff
    Serial.println("Setup complete. Start loop().");
}

void loop() {
  unsigned long currentMillis = millis();
  if (!client.connected()) {
    Serial.println("MQTT Broker connection lost. Reconnecting...");
    while (!client.connected()) {
      client.connect("ESP8266Client");
      delay(100);
    }
    Serial.println("MQTT Broker reconnected.");
  }
  client.loop();

  if (currentMillis - lastMillis > samplePeriod) {
    lastMillis = currentMillis;
    // Read sensor values
    int sensorValue = analogRead(analogInPin);
    float temp = dht.readTemperature();
    float hum = dht.readHumidity(); 
    // Build MQTT message
    msg[0] = '\0';
    sprintf(msg, "sensors,%s", influxOpts);
    sprintf(msg, "%s light=%d", msg, sensorValue);
    if (!isnan(temp) && !isnan(hum)) {
      sprintf(msg, "%s,humidity=%.1f,temperature=%.1f", msg, hum, temp);
    }
    // Publish message to mqtt broker
    client.publish("sensors/nodemcu", msg);
  }
  
  ArduinoOTA.handle();                        // listen for OTA events
}

void startWiFi() {
  wifiMulti.addAP(ssid1, psk1);
  //wifiMulti.addAP(ssid12, psk2);

  Serial.print("Connecting to WIFI");
  while (wifiMulti.run() != WL_CONNECTED) {  // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }
  Serial.println(" ");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());             // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  Serial.println("\r\n");
}

void startMQTT() {
  Serial.print("Try to connect to MQTT Broker... ");
  client.setServer(mqttBroker, 1883);
  Serial.println("OK");
}

void startSensors() {
  dht.begin();
}

void startOTA() { // Start the OTA service
  ArduinoOTA.setHostname(OTAName);
  ArduinoOTA.setPassword(OTAPassword);

  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nOTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready\r\n");
}