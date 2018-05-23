#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

const char* MQTT_ID = "plant_sensor_2";
const char* MQTT_SERVER = "10.42.11.15";
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "home/plant/2";

//change to the correct settings after network changes
//const char* MY_SSID = "olympos_iot";
//const char* MY_PWD = "Rq9q82YZX3LqfjF&8Zg7";
const char* MY_SSID = "Olympos 24";
const char* MY_PWD = "b4574rd!";

const int PIN_CLK   = D5;
const int PIN_SOIL  = A0;
const int PIN_LED   = D7;
const int PIN_SWITCH = D8;

// I2C address for temperature sensor
const int TMP_ADDR  = 0x48;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

#define SLEEP_TIME 1200 * 1000 * 1000


float readTemperature() {
  float temp;

  // Begin transmission
  Wire.beginTransmission(TMP_ADDR);
  // Select Data Registers
  Wire.write(0X00);
  // End Transmission
  Wire.endTransmission();

  delay(500);

  // Request 2 bytes , Msb first
  Wire.requestFrom(TMP_ADDR, 2 );
  // Read temperature as Celsius (the default)
  while (Wire.available()) {
    int msb = Wire.read();
    int lsb = Wire.read();
    Wire.endTransmission();

    int rawtmp = msb << 8 | lsb;
    int value = rawtmp >> 4;
    temp = value * 0.0625;

    return temp;
  }

}

// Get soil sensor value
float readSoilSensor() {
  float tmp = 0;
  float total = 0;
  float rawVal = 0;
  int sampleCount = 3;

  for (int i = 0; i < sampleCount; i++) {
    rawVal = analogRead(PIN_SOIL);
    total += rawVal;
  }

  tmp = total / sampleCount;
  return tmp;
}

// Get voltage
float readVoltage() {
  float tmp = 0;
  float total = 0;
  float rawVal = 0;
  int sampleCount = 3;

  for (int i = 0; i < sampleCount; i++) {
    rawVal = analogRead(PIN_SOIL);
    total += rawVal;
  }

  tmp = total / sampleCount;
  return tmp;
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_SOIL, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SWITCH, OUTPUT);

  digitalWrite(PIN_LED, HIGH);

  // device address is specified in datasheet
  Wire.beginTransmission(TMP_ADDR); // transmit to device #44 (0x2c)
  Wire.write(byte(0x01));            // sends instruction byte
  Wire.write(0x60);             // sends potentiometer value byte
  Wire.endTransmission();     // stop transmitting

  analogWriteFreq(40000);
  analogWrite(PIN_CLK, 400);
  delay(500);
  connectWifi();
  
  client.setServer(MQTT_SERVER, MQTT_PORT);
  Serial.print("Connectig MQTT: ");
  Serial.println(client.connect(MQTT_ID));
}

void loop() {
  // Turn on LED to indicate we are working
  digitalWrite(PIN_LED, HIGH);
  float temp = 0, soil_hum = 0, battery = 0;

  // Change to reading Soil Humidity
  digitalWrite(PIN_SWITCH, HIGH);
  // Read soil humidity
  soil_hum = readSoilSensor();
  delay(100);
  
  // Read temperature;
  temp = readTemperature();
  
  // Change to reading voltage then wait 100ms
  digitalWrite(PIN_SWITCH, LOW);
  delay(100);
  // Read voltage
  battery = readVoltage();
  delay(100);

  sendData(temp, soil_hum, battery);
  // Turn off LED to save battery
  digitalWrite(PIN_LED, LOW);
  ESP.deepSleep(SLEEP_TIME, WAKE_RF_DEFAULT);
}

void connectWifi() {
  Serial.print("Connecting to " + *MY_SSID);
  WiFi.begin(MY_SSID, MY_PWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected");
  Serial.println("");
}//end connect

void sendData(float temp, float soil_hum, float battery) {
  const size_t bufferSize = JSON_OBJECT_SIZE(3);
  
  Serial.print("MQTT still connected: ");
  Serial.println(client.loop());

  // Create json object to send.
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.createObject();
  root["temperature"] = temp;
  root["soil_humidity"] = soil_hum;
  root["battery"] = battery;

  const char jsonChar[106] = "";
  root.printTo((char*)jsonChar, root.measureLength() + 1);
  Serial.print("Built json object: ");
  Serial.println(jsonChar);
  Serial.print("Publishing:");
  Serial.println(client.publish(MQTT_TOPIC, jsonChar));
  Serial.print("MQTT State: ");
  Serial.println(client.state());
  client.loop();
  client.disconnect();
  // TEST IF WE NEED TO DO THIS
  wifiClient.stop();
}
