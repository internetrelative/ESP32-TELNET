#include <WiFi.h>

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// set both client and server to end with New Line aka LF , enable local echo on client terminal

WiFiServer server(23);  // Telnet port
IPAddress clientIP;

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long lastTime = millis();
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2, LOW);
    Serial.print('.');
    delay(1000);
    // Restart the ESP32 after 10s  has passed until WiFi is connected
    if (millis() - lastTime >= 10000) {
      ESP.restart();
    }
  }
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

void setup() {
  Serial.begin(115200);
  initWiFi();
  Serial.println("\nConnected to WiFi\n\n");
  server.begin();
  Serial.println("TCP TELNET SERVER STARTED");
}

unsigned long previousMillis = 0;
unsigned long interval = 30000;
void loop() {
  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    previousMillis = currentMillis;
  }

    WiFiClient client = server.available();

    if (client) {
      clientIP = client.remoteIP();
      Serial.printf("New client connected from %s\n\r", clientIP.toString().c_str());

      client.write("Server -> Hello World\n\r"); // Greet user when they connect

      unsigned long lastTime = millis(); 


      while (client.connected()) {
        ///////////////////////////////////////  RECEIVING  DATA /////////////////////////////////////////
        bool firstLine = true;
        while (client.available()) {
          char c = client.read();
          // wait for 500ms to pass before reading from input stream
          if (millis() - lastTime >= 500) {
            //remove ascii that aren't needed
            if (isAscii(c) && (int(c) > 31 && int(c) < 127)) {
              if (firstLine) {
                Serial.print("Client -> ");
                firstLine = false;
              }
              Serial.write(c);
            }
            if (c == '\n') {
              firstLine = true;
              Serial.printf("\n\r");
            }
          }
        }
        ///////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////// SENDING DATA ///////////////////////////////////
        while (Serial.available()) {
          String inputString = Serial.readStringUntil('\n');
          const char* cstr = inputString.c_str();
          if (strcmp(cstr, "restart") == 0) {
            Serial.printf("Client disconnected from serial: %s\n\r", clientIP.toString().c_str());
            client.stop();
            ESP.restart();
          }

          if (strcmp(cstr, "client_disconnect") == 0) {
            Serial.printf("Client %s disconnected from serial\n\r", clientIP.toString().c_str());
            client.stop();
            return;
          }

          //log to serial
          Serial.printf("Sending to client -> %s\n\r", cstr);

          //send to client
          client.write("Server -> ");
          client.write(cstr);
          client.write("\n\r");
        }
      }
      /////////////////////////////////////////////////////////////////////////////////

      client.stop();
      Serial.printf("Client disconnected: %s\n\r", clientIP.toString().c_str());
    }

  // Input from serial when no client is connected
  while (Serial.available()) {
    String inputString = Serial.readStringUntil('\n');
    const char* cstr = inputString.c_str();
    if (strcmp(cstr, "restart") == 0) {
      ESP.restart();
    }
  }
}
