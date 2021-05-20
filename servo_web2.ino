#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <Servo.h>

static const int servoPin = 13;

Servo servo1;

const char* ssid = "Ferry";
const char* password = "LennuLennu127";

unsigned long lastTime = 0;
unsigned long timerDelay = 1000;

int upRotation = 90;
int downRotation = 1;

String jsonBuffer;
int lastStamp = 0;
String lastState;
int lastId;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 
  Serial.println("Timer set to 10 seconds (timerDelay variable), it will take 10 seconds before publishing the first reading.");
}

void loop() {
  // Send an HTTP GET request
  if ((millis() - lastTime) > timerDelay) {
    // Check WiFi connection status
    if (WiFi.status()== WL_CONNECTED){
      String serverPath = "http://10.0.1.29:8080/iotstate/getState.php?key=Zqgfd7wN4YFRhppqKHyz";
      
      jsonBuffer = httpGETRequest(serverPath.c_str());
      // Serial.println(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }

      int alreadyFinished = (bool) myObject["finished"];
      int thisStamp = (int) myObject["stamp"];
      if (thisStamp > lastStamp && !alreadyFinished) {
        int thisId = (int) myObject["id"];
        const char* thisState = myObject["state"];
        lastId = thisId;
        lastStamp = thisStamp;
        Serial.print("Sending servo command: ");
        Serial.println(thisState);
        servoAction(thisState);
      }
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

void servoAction(const char* newState) {
  if (strcmp(newState, "up") == 0) {
    Serial.println("Received the UP command");
    servo1.attach(servoPin);
    servo1.write(upRotation);
    delay(5000);
  }
  else if (strcmp(newState, "down") == 0) {
    Serial.println("Received the DOWN command");
    servo1.attach(servoPin);
    servo1.write(downRotation);
    delay(5000);
  }

  Serial.println("Stopping rotation");
  servo1.detach();
  
  lastState = newState;
  String lastIdString = (String) lastId;
  String requestBody = "finished=1&id=" + lastIdString;
  String patchPath = "http://10.0.1.29:8080/iotstate/patchState.php?key=Zqgfd7wN4YFRhppqKHyz";

  jsonBuffer = httpPOSTRequest(patchPath.c_str(), requestBody);
  JSONVar myObject = JSON.parse(jsonBuffer);
  lastStamp = (int) myObject["stamp"];
  return;
}

String httpPOSTRequest(const char* serverName, String requestBody) {
  HTTPClient http;

  http.begin(serverName);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Send HTTP POST request
  int httpResponseCode = http.POST(requestBody);

  String payload = "{}";

  if (httpResponseCode > 0) {
    Serial.print("POST request response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error with POST request to patch endpoint");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

String httpGETRequest(const char* serverName) {
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(serverName);
  
  // Send HTTP GET request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("GET request response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}
