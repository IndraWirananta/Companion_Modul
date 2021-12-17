//Firebase Library for arduino/nodeMCU by K Suwatchai
//Code by Indra Wirananta

//Library
//#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>//go get time from NTP
#include <Firebase_ESP_Client.h> //For firebase related stuff
#include <U8g2lib.h>// for i2c display used
#include <Arduino.h>// for arduino
#include <SPI.h> //Serial Peripheral Interface (SPI) for peripheral stuff
#include <Wire.h>// communication with i21 display
#include <dht.h> // commnunication with dht sensor

//Provide the token generation process info.
#include <addons/TokenHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "Ari online"
#define WIFI_PASSWORD "indrasattvika"

/* 2. Define the API Key */
#define API_KEY "AIzaSyB4sz3w6d7sWBWlqMMHWUGJ6Zjn7ULnuD0"

/* 3. Define the project ID */
#define FIREBASE_PROJECT_ID "companion-d01bb"

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "indra@indra.com"
#define USER_PASSWORD "indraindra"
#define USER_UID "01ALOpn6oPSIPUdqlT1ZayrSB0y2"
#define MODUL_NAME "sensor1"
#define MODUL_GROUP "kandang"



#define dht_apin D7
#define pirSensor D8

dht DHT;

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long dataMillis = 0;
int count = 0;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600 * 8);

void setup()
{

  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  u8g2.begin();
  timeClient.begin();
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  delay(1500);

}

void loop()
{
  timeClient.update();

  DHT.read11(dht_apin);

  double humidity = DHT.humidity;
  double temperature = DHT.temperature;
  int timestamp = timeClient.getEpochTime();
  int motionSensor = digitalRead(pirSensor);

  String humd = "Humd : " + String(humidity, 0);
  String temp = "Temp : " + String(temperature, 1) + " C";

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_9x18B_tf);
  u8g2.drawStr(8, 29, humd.c_str());
  u8g2.sendBuffer();
  delay(800);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_9x18B_tf);
  u8g2.drawStr(8, 29, temp.c_str());
  u8g2.sendBuffer();
  delay(800);

  if (Firebase.ready() && (millis() - dataMillis > 60000 || dataMillis == 0))//timer untuk mengirim data setiap 1 menit (60000 milisekon)
  {
    dataMillis = millis();
    FirebaseJson content;
    std::vector<struct fb_esp_firestore_document_write_t> writes;
    struct fb_esp_firestore_document_write_t update_write;
    update_write.type = fb_esp_firestore_document_write_type_update;

    String documentPath = "users/" USER_UID "/sensor/" MODUL_GROUP "/"  MODUL_NAME  "/" + String(count);

    //Timestamp
    content.set("fields/timestamp/integerValue", timestamp);

    //Temperature Sensor
    content.set("fields/temperature/doubleValue", temperature);

    //Humidity Sensor
    content.set("fields/humidity/doubleValue", humidity);

    //Motion Sensor
    content.set("fields/motionSensor/integerValue", motionSensor);

    update_write.update_document_content = content.raw();
    update_write.update_document_path = documentPath.c_str();
    writes.push_back(update_write);

    if (Firebase.Firestore.commitDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, writes /* dynamic array of fb_esp_firestore_document_write_t */, "" /* transaction */))
      //    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw()))
      Serial.println("data sent succesfully");
    else
      Serial.println(fbdo.errorReason());

    count++;
    if (count > 2880) {
      count = 0;
    }
  }
}
