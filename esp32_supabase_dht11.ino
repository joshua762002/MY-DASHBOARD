#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ========== CONFIGURATION ==========
// WiFi credentials
const char* ssid = "Joshua-2.4G";
const char* password = "joshua762002";

// Supabase configuration – ITO ANG PINALITAN KO
const char* supabase_url = "https://vszeihhsgzmqadnrwckq.supabase.co";
const char* supabase_anon_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InZzemVpaGhzZ3ptcWFkbnJ3Y2txIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzQ0MzM0OTYsImV4cCI6MjA5MDAwOTQ5Nn0.w3u5N-uRh6YZeLddDYRM1WJ2Ij-ds_Y-tDTBvlzPn40";
const char* supabase_table = "sensor_readings";  // ito ang table name

// DHT11 settings
#define DHTPIN 4
#define DHTTYPE DHT11

// Web server port (optional, for local debugging)
WebServer server(80);

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Variables
float temperature = 0;
float humidity = 0;
bool sensorOK = true;
unsigned long lastReadTime = 0;
const unsigned long readInterval = 10000;  // Send every 10 seconds
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 30000;  // Check WiFi every 30 sec

// ===================================

// Function to send data to Supabase
void sendToSupabase(float temp, float hum, bool ok) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, cannot send data");
    return;
  }

  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();  // For testing only; use proper certificate in production
  http.begin(client, String(supabase_url) + "/rest/v1/" + supabase_table);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", supabase_anon_key);
  http.addHeader("Authorization", "Bearer " + String(supabase_anon_key));

  // Build JSON payload
  String payload = "{";
  payload += "\"temperature\":" + String(temp, 1) + ",";
  payload += "\"humidity\":" + String(hum, 1) + ",";
  payload += "\"sensor_ok\":" + String(ok ? "true" : "false");
  payload += "}";

  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    if (httpCode == 201) {
      Serial.println("Data sent to Supabase successfully");
    } else {
      Serial.printf("HTTP POST failed, code: %d\n", httpCode);
      Serial.println(http.getString());
    }
  } else {
    Serial.printf("HTTP POST error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

// Local web dashboard (optional)
String getDashboardHTML() {
  String html = "<html><body><h1>ESP32 Local Dashboard</h1><p>Data is being sent to Supabase.</p></body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", getDashboardHTML());
}

void handleData() {
  String json = "{";
  json += "\"temperature\":" + String(temperature) + ",";
  json += "\"humidity\":" + String(humidity) + ",";
  json += "\"sensor_ok\":" + String(sensorOK ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\nESP32 DHT11 + Supabase");

  // Initialize DHT
  dht.begin();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Setup local web server (optional)
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  // First reading delay
  delay(2000);
}

void loop() {
  // Handle local web requests
  server.handleClient();

  // Periodic WiFi check (reconnect if needed)
  if (millis() - lastWiFiCheck >= wifiCheckInterval) {
    lastWiFiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi lost, reconnecting...");
      WiFi.reconnect();
    }
  }

  // Read and send sensor data
  unsigned long now = millis();
  if (now - lastReadTime >= readInterval) {
    lastReadTime = now;

    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();

    if (!isnan(newTemp) && !isnan(newHum)) {
      temperature = newTemp;
      humidity = newHum;
      sensorOK = true;
      Serial.printf("Temp: %.1f°C, Hum: %.1f%%\n", temperature, humidity);
    } else {
      sensorOK = false;
      Serial.println("Failed to read DHT11 sensor!");
    }

    // Send to Supabase (even if sensor failed, we report false)
    sendToSupabase(temperature, humidity, sensorOK);
  }
}