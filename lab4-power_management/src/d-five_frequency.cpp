#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>

#define WIFI_SSID "myLab"
#define WIFI_PASSWORD "1swh01am"
#define DATABASE_SECRET "AIzaSyDGLaaNI3OfBxN2q3JLWpm2Mcs0sY-LRyQ"
#define DATABASE_URL "https://esp32-firebase-demo-6789f-default-rtdb.firebaseio.com/"

#define MAX_WIFI_RETRIES 5 // Maximum number of WiFi connection retries

WiFiClientSecure ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));

FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
LegacyToken dbSecret(DATABASE_SECRET);

unsigned long previousMillis = 0;
int uploadIntervals[] = {500, 1000, 2000, 3000, 4000};
int stageIndex = 0;

// HC-SR04 Pins
const int trigPin = D0;
const int echoPin = D1;
const float soundSpeed = 0.034;

float measureDistance();
void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance);

void setup(){
  Serial.begin(115200);
  delay(500);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  connectToWiFi();
  initFirebase();
}

void loop(){
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= uploadIntervals[stageIndex]) {
    previousMillis = currentMillis;
    float currentDistance = measureDistance();
    sendDataToFirebase(currentDistance);
  }
  
  if (millis() > (stageIndex + 1) * 10000) { // 30 seconds per stage
    stageIndex++;
    if (stageIndex >= 5) {
      Serial.println("All stages complete. Going to deep sleep.");
      WiFi.disconnect();
      esp_sleep_enable_timer_wakeup(30000000);
      esp_deep_sleep_start();
    }
  }
}

float measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * soundSpeed / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  return distance;
}

void connectToWiFi() {
  Serial.println(WiFi.macAddress());
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi");
  int wifiCnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifiCnt++;
    if (wifiCnt > MAX_WIFI_RETRIES) {
      Serial.println("WiFi connection failed");
      ESP.restart();
    }
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void initFirebase() {
  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
  ssl.setInsecure();
  Serial.println("Initializing Firebase...");
  initializeApp(client, app, getAuth(dbSecret));
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  client.setAsyncResult(result);
}

void sendDataToFirebase(float distance) {
  Serial.print("Pushing the float value... ");
  String name = Database.push<number_t>(client, "/test/distance", number_t(distance));
  if (client.lastError().code() == 0) {
    Firebase.printf("ok, name: %s\n", name.c_str());
  }
}