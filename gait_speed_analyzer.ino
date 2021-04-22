#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include <RTCZero.h>
#include "arduino_secrets.h"
#include <Wire.h>
#include <VL53L1X.h>

VL53L1X sensor;
float current_distance;
float previous_distance = 0;
float starting_distance;
float final_time;
float starting_time;
float ending_time;
float distance;

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

char serverAddress[] = "dcrawford41.pythonanywhere.com";

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress);

RTCZero rtc;

const int EST = 20;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000); // use 400 kHz I2C
  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1);
  }
  
  // Use long distance mode and allow up to 50000 us (50 ms) for a measurement.
  // You can change these settings to adjust the performance of the sensor, but
  // the minimum timing budget is 20 ms for short distance mode and 33 ms for
  // medium and long distance modes. See the VL53L1X datasheet for more
  // information on range and timing limits.
  sensor.setDistanceMode(VL53L1X::Long);
  sensor.setMeasurementTimingBudget(50000);

  // Start continuous readings at a rate of one measurement every 50 ms (the
  // inter-measurement period). This period should be at least as long as the
  // timing budget.
  sensor.startContinuous(50);
  randomSeed(analogRead(0));
  
  while (status != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
    status = WiFi.begin(ssid, pass);

    delay(10000);
  }
  
  rtc.begin();
  unsigned long epoch;
  int numberOfTries = 0, maxTries = 6;

  do {

    epoch = WiFi.getTime();

    numberOfTries++;

  }

  while ((epoch == 0) && (numberOfTries < maxTries));

  if (numberOfTries == maxTries) {

    Serial.print("NTP unreachable!!");

    while (1);

  }

  else {

    Serial.print("Epoch received: ");

    Serial.println(epoch);

    rtc.setEpoch(epoch);

    Serial.println();

  }
  
  Serial.println("Connected to the WiFi network");
}

void loop() {
  // check if previous - current distance is less than 
  starting_distance = sensor.read();
  current_distance = starting_distance;
  if (current_distance <= 500) {
    starting_time = millis();
    Serial.println("ST: " + String(starting_time));
    while((current_distance - previous_distance) <= 500) {
      Serial.println("cd: " + String(current_distance - previous_distance));
      if (current_distance >= 3000) {
        ending_time = millis();
        final_time = ending_time - starting_time;
        distance = current_distance - starting_distance;
        Serial.println("Time: " + String(final_time));
        Serial.println("Distance: " + String(distance));
        Serial.println("Speed: " + String(distance/final_time));
        makeRequest(distance/final_time);
        break;
      }        
      previous_distance = current_distance;
      current_distance = sensor.read();
    }
  }
  if (sensor.timeoutOccurred()) { Serial.println(" TIMEOUT"); }
}

void makeRequest(float gaitSpeed) {
  Serial.println("making POST request");
  String contentType = "application/json";
  String currentDate = getDate();
  String postData = "{\"speed\":\"" + String(gaitSpeed) + "\",\"date\":\"" + currentDate + "\"}";
  Serial.println(currentDate);
  client.post("/api/gaitmeasurements/", contentType, postData);

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
}

String getDate() {
   String currentMonth = ((rtc.getMonth() >= 10) ? String(rtc.getMonth()):("0" + String(rtc.getMonth())));
   String currentDay = ((rtc.getDay() >= 10) ? String(rtc.getDay()):("0" + String(rtc.getDay())));
   String currentHour = ((rtc.getHours() + EST >= 10) ? String(rtc.getHours() + EST):("0" + String(rtc.getHours() + EST)));
   if (rtc.getHours() + 22 >= 24) {
    currentDay = ((rtc.getDay() - 1 >= 10) ? String(rtc.getDay() - 1):("0" + String(rtc.getDay() - 1)));
   }
   String currentMinute = ((rtc.getMinutes() >= 10) ? String(rtc.getMinutes()):("0" + String(rtc.getMinutes())));
   String currentSecond = ((rtc.getSeconds() >= 10) ? String(rtc.getSeconds()):("0" + String(rtc.getSeconds())));
   String currentDate = "20" + String(rtc.getYear()) + "-" + currentMonth + "-" + currentDay + "T" + currentHour + ":" + currentMinute + ":" + currentSecond + "Z";
   return currentDate;
}

int addZero(int number) {

  if (number < 10) {

     return 0;

  }
  return number;
}
