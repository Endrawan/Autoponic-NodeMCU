// *{"TDS":4.174336,"TDS_setpoint":500,"temperature":25}#

#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <SoftwareSerial.h>

#define FIREBASE_HOST "autoponic-3090d.firebaseio.com"             // the project name address from firebase id
#define FIREBASE_AUTH "aDycIUUwcoy1km0eQmNKdPPD8LzQRWntAG1i3QhB"       // the secret key generated from firebase

#define WIFI_SSID "Redmi"                   // input your home or public wifi name 
#define WIFI_PASSWORD "hahahaha"             //password of wifi ssid

SoftwareSerial s(D6, D5);

const byte numChars = 960;
char receivedChars[numChars];
boolean newData = false;

int tds_value, temperature;

void setup() {
  Serial.begin(115200);
  s.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

void loop() {
  if(WiFi.status() != WL_CONNECTED) {
    connection();
  }
  //loadDataFromFirebase();
  //logValue();
  recvWithStartEndMarkers();
  processNewData();
  delay(1000);
}

void connection() {
  Serial.print("connecting...");
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
}

void logValue() {
  Serial.print("TDS: ");
  Serial.println(tds_value);
  Serial.print("Temperature: ");
  Serial.println(temperature);
}

void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '^';
  char endMarker = '~';
  char rc;

  while (s.available() > 0 && newData == false) {
    rc = s.read();

    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
      else {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }

    else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

void processNewData() {
  if (newData == true) {
      newData = false;
    DynamicJsonBuffer jb;
    JsonObject& obj = jb.parseObject(receivedChars);
    if (!obj.success()) {
      Serial.println("Gagal mengubah json");
      Serial.println(receivedChars);
      return;
    }
    Serial.println("----------------Receiving Object from Arduino---------------");
    obj.prettyPrintTo(Serial);
    Serial.println("----------------------End of Receiving----------------------");
    updateDataToFirebase(obj);
  }
}


void loadDataFromFirebase() {
  tds_value = Firebase.getInt("TDS");
  temperature = Firebase.getInt("temperature");
}

void updateDataToFirebase(JsonObject& obj) {
  Firebase.setFloat("TDS", obj["TDS"]);
  Firebase.setFloat("temperature", obj["temperature"]);
  Firebase.setInt("TDS_setpoint", obj["TDS_setpoint"]);
}
