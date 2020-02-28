#include <ESP8266WiFi.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>


/************ TEMP SETTINGS (CHANGE THIS FOR YOUR SETUP) *******************************/
#define IsFahrenheit false //to use celsius change to false

/************ WIFI and MQTT INFORMATION (CHANGE THESE FOR YOUR SETUP) ******************/
#define wifi_ssid "Interconnected Shed" //type your WIFI information inside the quotes
#define wifi_password "uZKxLSzMhB9APkhY"
#define mqtt_server ""
#define mqtt_user ""
#define mqtt_password ""
#define mqtt_port 1883



/************* MQTT TOPICS (change these topics as you wish)  **************************/
#define temp_topic "stat/plant_temps"

/**************************** FOR OTA **************************************************/
#define SENSORNAME "smart_plant"
#define OTApassword "smartplant" // change this to whatever password you want to use when you upload OTA
int OTAport = 8266;

#define MOISTURE_PIN  A0  //soil Moisture sensor//
#define DHTTYPE       DHT22
#define AirTempPIN    D2
#define pumpPin       D6
#define heightTrig    D7
#define heightEcho    D8

/**************************** SENSOR DEFINITIONS *******************************************/
float airtempValue;
float airhumValue;

int soilHumidity;   //soil moisture
int setHumidity = 50;      //Set the pump trigger threshold

char message_buff[100];

int calibrationTime = 2;
int testTime = 60;
float height;
float duration;

const int BUFFER_SIZE = 300;

#define MQTT_MAX_PACKET_SIZE 512

WiFiClient espClient;
PubSubClient client(espClient);
DHT AirTemp(AirTempPIN, DHTTYPE);


/********************************** START SETUP*****************************************/
void setup() {

  Serial.begin(115200);

  pinMode(AirTempPIN, INPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(heightTrig, OUTPUT);
  pinMode(heightEcho, INPUT);

  Serial.begin(115200);
  delay(10);

  ArduinoOTA.setPort(OTAport);

  ArduinoOTA.setHostname(SENSORNAME);

  ArduinoOTA.setPassword((const char *)OTApassword);

  Serial.print("calibrating sensor ");
  for (int i = 0; i < calibrationTime; i++) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("Starting Node named " + String(SENSORNAME));


  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);

  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
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
  Serial.println("Ready");
  Serial.print("IPess: ");
  Serial.println(WiFi.localIP());
  reconnect();
}




/********************************** START SETUP WIFI*****************************************/
void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


/********************************** START SEND STATE*****************************************/
void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["air_humidity"] = (String)airhumValue;
  root["air_temperature"] = (String)airtempValue;
  root["air_heatIndex"] = (String)AirTemp.computeHeatIndex(airtempValue, airhumValue, IsFahrenheit);
  root["soil_moisture"] = (String)soilHumidity;
  root["height"] = (String)height;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  Serial.println(buffer);
  client.publish(temp_topic, buffer, false);
}

/********************************** START RECONNECT*****************************************/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



/********************************** START CHECK SENSOR **********************************/
bool checkBoundSensor(float newValue) {
  return !isnan(newValue) && (newValue != 0);
}


/********************************** START MAIN LOOP***************************************/
void loop() {

  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }

  client.loop();

    float newAirTempValue = AirTemp.readTemperature(IsFahrenheit);
    float newAirHumValue = AirTemp.readHumidity();

    if (checkBoundSensor(newAirTempValue)) airtempValue = newAirTempValue;
    if (checkBoundSensor(newAirHumValue)) airhumValue = newAirHumValue;

    soilHumidity = map(analogRead(MOISTURE_PIN), 0, 1023, 0, 100);    //Map analog value to 0~100% soil moisture value
      if (soilHumidity < setHumidity) {
        pumpOn();
      } else {
        pumpOff();
      }

      digitalWrite(heightTrig, LOW);   // Makes trigPin low
      delayMicroseconds(4);       // 2 micro second delay

      digitalWrite(heightTrig, HIGH);  // tigPin high
      delayMicroseconds(20);      // trigPin high for 20 micro seconds
      digitalWrite(heightTrig, LOW);   // trigPin low

      duration = pulseIn(heightEcho, HIGH);   //Read echo pin, time in microseconds
      height = duration*0.034/2;        //Calculating actual/real distance

    sendState();

    delay(5000);
  }

//open pump
void pumpOn() {
  digitalWrite(pumpPin, HIGH);
  Serial.println("pump_on");
}
//close pump
void pumpOff() {
  digitalWrite(pumpPin, LOW);
  Serial.println("pump_off");
}
