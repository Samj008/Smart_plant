#include <dht11.h>

dht11 DHT;
#define MOISTURE_PIN A2  //soil Moisture sensor//
#define DHT11_PIN    9   //DHT11

int airHumidity;   //environment humidity
int airTemperature;  // environment temperature
int soilHumidity;   //soil moisture
int setHumidity = 50;      //Set the pump trigger threshold

void setup(){
  Serial.begin(9600);

  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
}

void loop(){
  int chk;
  chk = DHT.read(DHT11_PIN);   //Read Data
  switch (chk){
    case DHTLIB_OK:
                Serial.print("OK,\t");
                break;
    case DHTLIB_ERROR_TIMEOUT:
                Serial.print("Time out error,\t");
                break;
    default:
                Serial.print("OK,\t");
                break;
  }
  airHumidity=DHT.humidity;
  airTemperature=DHT.temperature;
  soilHumidity=analogRead(MOISTURE_PIN);

  Serial.print("airHumidity:");
  Serial.print(airHumidity);
  Serial.print(",\t");
  Serial.print("airTemperature:");
  Serial.print(airTemperature);
  Serial.print(",\t");
  Serial.print("soilHumidity:");
  Serial.println(soilHumidity);

  soilHumidity = map(analogRead(MOISTURE_PIN), 0, 1023, 0, 100);    //Map analog value to 0~100% soil moisture value
  if (soilHumidity < setHumidity) {
    pumpOn();
  } else {
    pumpOff();
  }
delay(5000);
}

//open pump
void pumpOn() {
  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);
}
//close pump
void pumpOff() {
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
}
