#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define PIR_PIN D5     // PIR Sensor on GPIO14 (D5)
#define RELAY_PIN D1    // Relay on GPIO5 (D1)

// WiFi credentials
const char* ssid = "Pakiss muna";
const char* password = "lambingmo";

// Variables
bool motionDetected = false;
unsigned long lastMotionTime = 0;
const unsigned long LIGHT_DELAY = 5000; // 5 seconds delay after motion
unsigned int entryCount = 0;            // Counter for room entries
String motionHistory = "";              // Store motion events history
ESP8266WebServer server(80);            // Web server on port 80

void setup() {
  Serial.begin(9600);
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Start with relay OFF

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected");
  Serial.print("IP address:");
  Serial.println(WiFi.localIP());

  // PIR sensor calibration
  Serial.println("Calibrating PIR (10 sec)...");
  for (int i = 0; i < 10; i++) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nPIR Ready!");

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient(); // Handle web requests
  
  if (digitalRead(PIR_PIN) == HIGH) { // Motion detected
    lastMotionTime = millis();
    if (!motionDetected) {
      motionDetected = true;
      digitalWrite(RELAY_PIN, HIGH); // Turn ON relay
      entryCount++; // Increment entry counter
      
      // Add timestamp to history
      String timestamp = getTimestamp();
      motionHistory += timestamp + " - Motion detected<br>";
      if (motionHistory.length() > 1000) { // Prevent memory overflow
        motionHistory = motionHistory.substring(motionHistory.length() - 1000);
      }
      
      Serial.println("Motion! Light ON");
    }
  }
  
  // Turn OFF if no motion for delay time
  if (motionDetected && (millis() - lastMotionTime > LIGHT_DELAY)) {
    motionDetected = false;
    digitalWrite(RELAY_PIN, LOW); // Turn OFF relay
    Serial.println("No motion. Light OFF");
  }
  
  delay(100); // Small delay for stability
}

String getTimestamp() {
  unsigned long seconds = millis() / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  seconds %= 60;
  minutes %= 60;
  return String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>Smart Lightning Monitoring</title>";
  html += "<meta http-equiv='refresh' content='5'>"; // Auto-refresh every 5 seconds
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; }";
  html += ".container { max-width: 800px; margin: 0 auto; }";
  html += ".card { background: #f9f9f9; padding: 20px; margin-bottom: 20px; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }";
  html += "h1 { color: #444; }";
  html += ".status { font-weight: bold; }";
  html += ".on { color: green; }";
  html += ".off { color: red; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Room Entry Monitor</h1>";
  
  html += "<div class='card'>";
  html += "<h2>Current Status</h2>";
  html += "<p>Motion: <span class='status " + String(motionDetected ? "on" : "off") + "'>";
  html += motionDetected ? "DETECTED" : "NO MOTION";
  html += "</span></p>";
  html += "<p>Light: <span class='status " + String(motionDetected ? "on" : "off") + "'>";
  html += motionDetected ? "ON" : "OFF";
  html += "</span></p>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h2>Statistics</h2>";
  html += "<p>Total entries detected: <strong>" + String(entryCount) + "</strong></p>";
  html += "<p>Uptime: " + getTimestamp() + "</p>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h2>Recent Activity</h2>";
  html += "<div style='height: 200px; overflow-y: scroll; border: 1px solid #ddd; padding: 10px;'>";
  html += motionHistory;
  html += "</div>";
  html += "</div>";
  
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleData() {
  // JSON endpoint for AJAX requests
  String json = "{";
  json += "\"motion\":" + String(motionDetected ? "true" : "false") + ",";
  json += "\"light\":" + String(motionDetected ? "true" : "false") + ",";
  json += "\"entries\":" + String(entryCount) + ",";
  json += "\"uptime\":\"" + getTimestamp() + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}
