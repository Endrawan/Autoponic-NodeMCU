#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <SoftwareSerial.h>

#define FIREBASE_HOST "autoponic-3090d.firebaseio.com"                 // the project name address from firebase id
#define FIREBASE_AUTH "aDycIUUwcoy1km0eQmNKdPPD8LzQRWntAG1i3QhB"       // the secret key generated from firebase

#define WIFI_SSID "Redmi"
#define WIFI_PASSWORD "hahahaha"

#define TDS_TAG "TDS"
#define SETPOINT_TAG "setpoint"
#define TEMPERATURE_TAG "temperature"

SoftwareSerial s(D6, D5);

const char startMarker = '|';
const char endMarker = '~';

const byte numChars = 960;
char receivedChars[numChars];
boolean newData = false;

float currentSetPoint = 0;

void setup() {
  Serial.begin(115200);
  s.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

void loop() {
  if(WiFi.status() != WL_CONNECTED) connection();
  loadDataFromFirebase(currentSetPoint, startMarker, endMarker);
  receiveDataFromArduino(startMarker, endMarker);
  Serial.println(".");
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

void receiveDataFromArduino(char startMarker, char endMarker) {
  static boolean recvInProgress = false;
  static byte ndx = 0;
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
        s.flush();
      }
    }

    else if (rc == startMarker) {
      recvInProgress = true;
    }
  }

  processNewData();
}

void processNewData() {
  if (newData == true) {
    DynamicJsonBuffer jb;
    JsonObject& obj = jb.parseObject(receivedChars);
    if (!obj.success()) {
      Serial.println("Gagal mengubah json");
      Serial.println(receivedChars);
      newData = false;
      return;
    }
    Serial.println("----------------Receiving Object from Arduino---------------");
    obj.prettyPrintTo(Serial);
    Serial.println("----------------------End of Receiving----------------------");
    updateDataToFirebase(obj);
    newData = false;
  }
}

void loadDataFromFirebase(float& currentSetPoint, char startMarker, char endMarker) {
  float setPointFirebase = Firebase.getFloat(SETPOINT_TAG);
  if(setPointFirebase != currentSetPoint) {
    transmitDataToArduino(setPointFirebase, startMarker, endMarker);
    currentSetPoint = setPointFirebase;
  }
}

void updateDataToFirebase(JsonObject& obj) {
  Firebase.setFloat(TDS_TAG, obj[TDS_TAG]);
  Firebase.setFloat(TEMPERATURE_TAG, obj[TEMPERATURE_TAG]);
//  Firebase.setFloat(SETPOINT_TAG, obj[SETPOINT_TAG]);
}

void transmitDataToArduino(float setPoint, char startMarker, char endMarker) {
  
  String json, transmittedValue;
  
  DynamicJsonBuffer jb;
  JsonObject& obj = jb.createObject();
  obj[SETPOINT_TAG] = setPoint;
  obj.printTo(json);
  
  transmittedValue = startMarker + json + endMarker;
  
  Serial.println("-----------TRANSMITING data to Arduino-----------");
  Serial.println(transmittedValue);
  Serial.println("----------------END OF TRANSMITING---------------");
  
  s.print(transmittedValue);
//  json.remove(0, json.length());
//  transmittedValue.remove(0, transmittedValue.length());
}
