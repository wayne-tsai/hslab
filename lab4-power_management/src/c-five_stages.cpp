#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>

#define WIFI_SSID "myLab"
#define WIFI_PASSWORD "1swh01am"
#define DATABASE_SECRET "AIzaSyDGLaaNI3OfBxN2q3JLWpm2Mcs0sY-LRyQ"
#define DATABASE_URL "https://esp32-firebase-demo-6789f-default-rtdb.firebaseio.com/"

#define STAGE_INTERVAL 12000 // 12 seconds each stage
#define MAX_WIFI_RETRIES 5 // Maximum number of WiFi connection retries

WiFiClientSecure ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));

FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
LegacyToken dbSecret(DATABASE_SECRET);

int uploadInterval = 1000; // 1 seconds each upload
unsigned long sendDataPrevMillis = 0;
int count = 0;
// bool signupOK = false;

// HC-SR04 Pins
const int trigPin = D0;
const int echoPin = D1;

// Define sound speed in cm/usec
const float soundSpeed = 0.034;

//functions
float measureDistance();
void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance);

void printError(int code, const String &msg)
{
    Firebase.printf("Error, msg: %s, code: %d\n", msg.c_str(), code);
}

void setup(){
  Serial.begin(115200);
  delay(500);
  // while(!Serial){
  //   yield();
  // }

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // 1. Idle ESP32 (not running WiFi or ultrasonic sensor)
  Serial.println("Running for 12 seconds without doing anything...");
  unsigned long startTime = millis();
  while (millis() - startTime < STAGE_INTERVAL)
  {
    delay(100); // Delay between measurements
  }

  // 2. Only ultrasonic sensor working
  Serial.println("Measuring distance for 12 seconds...");
  startTime = millis();
  while (millis() - startTime < STAGE_INTERVAL)
  {
    measureDistance();
    delay(100); // Delay between measurements
  }

  // 3. Ultrasonic + Wifi working
  Serial.println("Turning on WiFi and measuring for 12 seconds...");
  connectToWiFi();
  startTime = millis();
  while (millis() - startTime < STAGE_INTERVAL)
  {
    measureDistance();
    delay(100); // Delay between measurements
  }

  // 4. Ultrasonic + Wifi + Sending data to Firebase
  Serial.println("Turning on Firebase and sending data every 1 second...");
  initFirebase();
  startTime = millis();
  while (millis() - startTime < STAGE_INTERVAL)
  {
    float currentDistance = measureDistance();
    sendDataToFirebase(currentDistance);
    delay(100); // Delay between measurements
  }

  // 5. Deep Sleep mode
  Serial.println("Going to deep sleep for 12 seconds...");
  WiFi.disconnect();
  esp_sleep_enable_timer_wakeup(STAGE_INTERVAL * 1000); // in microseconds
  esp_deep_sleep_start();
  
}

void loop(){
  // This is not used
}

float measureDistance()
{
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

void connectToWiFi()
{
  // Print the device's MAC address.
  Serial.println(WiFi.macAddress());
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi");
  int wifiCnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifiCnt++;
    if (wifiCnt > MAX_WIFI_RETRIES){
      Serial.println("WiFi connection failed");
      ESP.restart();
    }
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void initFirebase(){
  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    ssl.setInsecure();
#if defined(ESP8266)
    ssl.setBufferSizes(1024, 1024);
#endif

    // Initialize the authentication handler.
    Serial.println("Initializing the app...");
    initializeApp(client, app, getAuth(dbSecret));

    // Binding the authentication handler with your Database class object.
    app.getApp<RealtimeDatabase>(Database);

    // Set your database URL (requires only for Realtime Database)
    Database.url(DATABASE_URL);

    // In sync functions, we have to set the operating result for the client that works with the function.
    client.setAsyncResult(result);
}

void sendDataToFirebase(float distance){
  if (millis() - sendDataPrevMillis > uploadInterval || sendDataPrevMillis == 0){
    sendDataPrevMillis = millis();

    Serial.print("Pushing the float value... ");
    String name = Database.push<number_t>(client, "/test/distance", number_t(distance));
    if (client.lastError().code() == 0){
        Firebase.printf("ok, name: %s\n", name.c_str());
        count ++;
    }
  }
}