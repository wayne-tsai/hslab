#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>

// Network credentials - should be stored in secure config
#define WIFI_SSID "myLab"
#define WIFI_PASSWORD "1swh01am"
#define DATABASE_SECRET "AIzaSyDGLaaNI3OfBxN2q3JLWpm2Mcs0sY-LRyQ"
#define DATABASE_URL "https://esp32-firebase-demo-6789f-default-rtdb.firebaseio.com/"

// Power management constants
#define DISTANCE_THRESHOLD_CM 50        // Distance threshold for sleep mode
#define MOTION_CHECK_PERIOD_MS 30000    // Check for motion every 30 seconds
#define SLEEP_DURATION_US 30000000      // 30 second deep sleep
#define READINGS_BEFORE_SLEEP 3         // Number of consistent readings before sleep
#define MEASUREMENT_INTERVAL_MS 1000    // Take measurements every 1 second
#define MAX_WIFI_RETRIES 3             // Reduced from 5 to save power on connection failures

// Sensor pins
const int trigPin = D0;
const int echoPin = D1;
const float soundSpeed = 0.034;

// Global variables
WiFiClientSecure ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
LegacyToken dbSecret(DATABASE_SECRET);

unsigned long previousMeasureMillis = 0;
unsigned long motionCheckStartMillis = 0;
int consistentReadings = 0;
float lastDistance = 0;
bool motionDetected = false;

// Function declarations
float measureDistance();
void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance, bool motion);
void goToSleep();
bool isSignificantChange(float newDistance, float oldDistance);

void setup() {
  // Initialize with minimal delay
  Serial.begin(115200);
  
  // Configure pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // Get initial distance reading
  lastDistance = measureDistance();
  motionCheckStartMillis = millis();
  
  // Only connect to WiFi if motion is detected
  if (lastDistance < DISTANCE_THRESHOLD_CM) {
    connectToWiFi();
    initFirebase();
    sendDataToFirebase(lastDistance, true);
  }
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Take measurements at regular intervals
  if (currentMillis - previousMeasureMillis >= MEASUREMENT_INTERVAL_MS) {
    previousMeasureMillis = currentMillis;
    float currentDistance = measureDistance();
    
    // Check for significant motion
    if (isSignificantChange(currentDistance, lastDistance)) {
      motionDetected = true;
      consistentReadings = 0;
      
      // Connect to WiFi if not connected
      if (WiFi.status() != WL_CONNECTED) {
        connectToWiFi();
        initFirebase();
      }
      
      // Send data to Firebase
      sendDataToFirebase(currentDistance, true);
    } else {
      consistentReadings++;
    }
    
    lastDistance = currentDistance;
  }
  
  // Check if it's time to consider sleep mode
  if (currentMillis - motionCheckStartMillis >= MOTION_CHECK_PERIOD_MS) {
    if (consistentReadings >= READINGS_BEFORE_SLEEP && lastDistance >= DISTANCE_THRESHOLD_CM) {
      // No motion detected for specified period
      if (WiFi.status() == WL_CONNECTED) {
        sendDataToFirebase(lastDistance, false);
        WiFi.disconnect(true);
      }
      goToSleep();
    }
    motionCheckStartMillis = currentMillis;
    consistentReadings = 0;
  }
}

float measureDistance() {
  // Optimize sensor reading time
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * soundSpeed / 2;
  
  Serial.printf("Distance: %.2f cm\n", distance);
  return distance;
}

void connectToWiFi() {
  Serial.println(WiFi.macAddress());
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi");
  
  int wifiCnt = 0;
  while (WiFi.status() != WL_CONNECTED && wifiCnt < MAX_WIFI_RETRIES) {
    delay(500);
    Serial.print(".");
    wifiCnt++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    goToSleep(); // Sleep if can't connect to save power
  }
  
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
}

void initFirebase() {
  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
  ssl.setInsecure();
  initializeApp(client, app, getAuth(dbSecret));
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  client.setAsyncResult(result);
}

void sendDataToFirebase(float distance, bool motion) {
  // Create a structured data object
  String dataPath = "/sensor_data/" + String(millis());
  Database.set(client, dataPath + "/distance", number_t(distance));
  Database.set(client, dataPath + "/motion", motion);
  Database.set(client, dataPath + "/timestamp", number_t(millis()));
}

void goToSleep() {
  Serial.println("Going to deep sleep...");
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
  esp_deep_sleep_start();
}

bool isSignificantChange(float newDistance, float oldDistance) {
  // Consider a 5% change as significant
  const float changeThreshold = 0.05;
  return abs(newDistance - oldDistance) / oldDistance > changeThreshold;
}