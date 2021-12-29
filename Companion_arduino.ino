

//Firebase Library for arduino/nodeMCU by K Suwatchai
//Code by Indra Wirananta

//Library
//#include <WiFi.h>
#include <WiFiUdp.h>
//#include <ArduinoJson.h>//handle json parsing
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
#define USER_EMAIL "indrawiranantatest@gmail.com"
#define USER_PASSWORD "indrawiranantatest"
#define USER_UID "01ALOpn6oPSIPUdqlT1ZayrSB0y2"
#define MODUL_NAME "sensorAnjing"

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
NTPClient timeClient(ntpUDP, "pool.ntp.org");

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

  if (Firebase.ready() && (millis() - dataMillis > 60000 * 5 || dataMillis == 0)) //timer untuk mengirim data setiap 1 menit (60000 milisekon)
  {
    dataMillis = millis();
    String documentPath = "users/" USER_UID "/sensor/" MODUL_NAME;
    String field = "reading";
    std::vector<struct fb_esp_firestore_document_write_t> writes;
    struct fb_esp_firestore_document_write_t transform_write;
    transform_write.type = fb_esp_firestore_document_write_type_transform;
    transform_write.document_transform.transform_document_path = "users/" USER_UID "/sensor/" MODUL_NAME;
    struct fb_esp_firestore_document_write_field_transforms_t field_transforms;
    field_transforms.fieldPath = "reading";
    field_transforms.transform_type = fb_esp_firestore_transform_type_append_missing_elements;

    FirebaseJson content;
    //    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), field.c_str())) {
    //      FirebaseJson payload;
    //      FirebaseJsonData result;
    //
    //      payload.setJsonData(fbdo.payload().c_str());
    //      Serial.println(fbdo.payload().c_str());
    //      payload.get(result, "fields/reading/arrayValue/values");
    //
    //      if (result.success) {
    //        FirebaseJsonArray arr;
    //        result.get<FirebaseJsonArray>(arr);
    //
    //        FirebaseJson newPayload;
    //        newPayload.set("/mapValue/fields/humidity/integerValue", humidity);
    //        newPayload.set("/mapValue/fields/temperature/integerValue", temperature);
    //        newPayload.set("/mapValue/fields/motionSensor/integerValue", motionSensor);
    //        newPayload.set("/mapValue/fields/timestamp/integerValue", timestamp);
    //        arr.add(newPayload);
    //
    //        arr.toString(Serial, true);
    //        FirebaseJsonData result2;
    //        for (size_t i = 0; i < arr.size(); i++) {
    //          arr.get(result2, i);
    //          Serial.println(result2.to<String>().c_str());
    //        }
    //        content.set("values", arr);
    //      }else{
    //         Serial.println("Failed to query reading fields");
    //        }
    //    }
    //    else
    //      Serial.println(fbdo.errorReason());
    
    FirebaseJson newPayload;
    newPayload.set("/mapValue/fields/humidity/integerValue", humidity);
    newPayload.set("/mapValue/fields/temperature/integerValue", temperature);
    newPayload.set("/mapValue/fields/motionSensor/integerValue", motionSensor);
    newPayload.set("/mapValue/fields/timestamp/integerValue", timestamp);
    
    content.set("values", newPayload);
    
    field_transforms.transform_content = content.raw();
    transform_write.document_transform.field_transforms.push_back(field_transforms);
    writes.push_back(transform_write);


    if (Firebase.Firestore.commitDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, writes /* dynamic array of fb_esp_firestore_document_write_t */, "" /* transaction */))
      Serial.println("data sent succesfully");
    else
      Serial.println(fbdo.errorReason());



  }
}
