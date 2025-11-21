/**
 * UNO R4 WiFi: DHT11 + NTU + WiFi + TCP JSON Client
 *
 * - Reads DHT11 temperature & humidity
 * - Reads turbidity sensor on A0 (NTU)
 * - Connects to WiFi (WiFiS3)
 * - Connects as TCP client to given IP/Port
 * - Sends:
 *    1) Once per TCP connection: {"track_id": track_id}
 *    2) For each data sample:    {"temperature": t, "humidity": h, "turbility": thirdsensor}
 */


#include <DHT11.h>
#include <WiFiS3.h>     // For Arduino UNO R4 WiFi


// ---------- WIFI CREDENTIALS ----------
const char* ssid     = "TP-Link_85B4";
const char* password = "83528804";


// ---------- TCP SERVER CONFIG ----------
IPAddress serverIP(192, 168, 0, 86);  // <-- CHANGE TO YOUR SERVER'S IP
uint16_t  serverPort = 5000;          // <-- CHANGE TO YOUR SERVER'S PORT


// Track ID (you can make this numeric or string as needed)
const char* teamId = "2";    // <-- CHANGE AS NEEDED


WiFiClient client;
bool trackIdSent = false;


// ---------- DHT11 SETUP ----------
const int DHT_PIN = 7;                // DHT11 data pin
DHT11 dht(DHT_PIN);


// ---------- NTU SENSOR SETUP ----------
const int TURBIDITY_PIN = A0;         // Analog input for sensor
const int OUTPUT_PIN    = 8;          // Digital output (LED/relay/etc.)


float sensorValue = 0.0;
float volt        = 0.0;
float ntu         = 0.0;


// Threshold in NTU (adjust as needed)
float threshold = 2135.0;


int wifiStatus = WL_IDLE_STATUS;


// ---------- WIFI CONNECT FUNCTION ----------
void connectWiFi() {
  // Check that the WiFi module is present
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi module not found! Check board.");
    while (true) {
      delay(1000);
    }
  }


  Serial.print("Connecting to SSID: ");
  Serial.println(ssid);


  // Attempt to connect until successful
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);


    // Wait a bit for connection
    delay(5000);


    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("Failed to connect to WiFi. Retrying...");
    }
  }
}


// Ensure WiFi is still connected, reconnect if needed
void ensureWiFiConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, reconnecting...");
    connectWiFi();
  }
}


// ---------- TCP SERVER CONNECT FUNCTION ----------
void connectServer() {
  // (Re)set track ID sent flag whenever we reconnect
  trackIdSent = false;


  Serial.print("Connecting to TCP server ");
  Serial.print(serverIP);
  Serial.print(":");
  Serial.println(serverPort);


  if (client.connect(serverIP, serverPort)) {
    Serial.println("Connected to TCP server.");


    // Send track ID JSON once after connect
    String trackJson = String("{\"team_id\": \"") + teamId + "\"}";
    client.println(trackJson);          // newline-delimited JSON
    client.flush();
    trackIdSent = true;


    Serial.print("Sent track ID JSON: ");
    Serial.println(trackJson);
  } else {
    Serial.println("TCP server connection failed.");
  }
}


// Ensure TCP connection is active, reconnect if needed
void ensureServerConnected() {
  if (!client.connected()) {
    Serial.println("TCP client not connected, reconnecting...");
    client.stop();   // close any stale connection
    connectServer();
  }
}


void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for Serial to be ready (in some environments)
  }


  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);


  // Ensure classic 10-bit ADC behaviour (0–1023) on UNO R4
  analogReadResolution(10);


  // Connect to WiFi
  connectWiFi();


  // Connect to TCP server
  connectServer();


  Serial.println("Setup complete.");
}


void loop() {
  // Make sure WiFi and server connection are up
  ensureWiFiConnected();
  ensureServerConnected();


  // ----------- DHT11 READ -----------
  int temperature = 0;
  int humidity    = 0;


  humidity = dht.readHumidity();
  temperature = dht.readTemperature();


  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT11");
    humidity= 0;
    temperature =  0;
  }


  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");




  // ----------- NTU SENSOR READ -----------
  sensorValue = analogRead(TURBIDITY_PIN);


  // Convert ADC reading (0–1023) to voltage (0–5V)
  volt = sensorValue * (5.0 / 1023.0);


  // NTU calculation
  ntu = -1120.4 * sq(volt) + 5742.3 * volt - 4353.8;


  // Control output pin based on NTU vs threshold
  if (ntu < threshold) {
    digitalWrite(OUTPUT_PIN, HIGH);
  } else {
    digitalWrite(OUTPUT_PIN, LOW);
  }


  // Log locally
  Serial.print("ADC: ");
  Serial.print(sensorValue);
  Serial.print("  Volt: ");
  Serial.print(volt);
  Serial.print(" V  NTU: ");
  Serial.println(ntu);


  // ----------- SEND JSON TO TCP SERVER -----------
  if (client.connected() && trackIdSent) {
    // thirdsensor = ntu
    String dataJson = "{";
    dataJson += "\"temperature\": ";
    dataJson += temperature;
    dataJson += ", \"humidity\": ";
    dataJson += humidity;
    dataJson += ", \"turbility\": ";
    dataJson += ntu;
    dataJson += "}";


    client.println(dataJson);   // newline-delimited JSON
    client.flush();


    Serial.print("Sent data JSON: ");
    Serial.println(dataJson);
  } else {
    Serial.println("Not sending data: TCP client not connected or track ID not sent.");
  }


  delay(2000); // adjust sampling rate as needed
}
