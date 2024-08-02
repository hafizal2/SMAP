#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "dlinkap1360";
const char* password = "abcd1234";

const char* mqtt_server = "174.138.28.115";
const int mqtt_port = 1883;
const char* mqtt_user = ""; // Add user if authentication is required
const char* mqtt_password = ""; // Add password if authentication is required

// MQTT Topics
const char* distance_topic = "sensor/distance/007";
const char* led_control_topic = "switch/led/007";

// Ultrasonic Sensor Pins
const int trigPin = D7;
const int echoPin = D8;
float preDistance = 0;

// External LEDs
const int redLed = D2;
const int greenLed = D4;

WiFiClient espClient;
PubSubClient client(espClient);
bool ledControl = false;

void setupWifi() {
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Control LEDs based on the message
  if (String(topic) == led_control_topic) {
    if ((char)payload[0] == 't') { // 'true' starts with 't'
      digitalWrite(redLed, HIGH);
      digitalWrite(greenLed, HIGH);
      ledControl = true;
    } 
    if ((char)payload[0] == 'f') { // 'false' starts with 'f'
      digitalWrite(redLed, LOW);
      digitalWrite(greenLed, LOW);
      delay(1000); // delay 1 second
      ledControl = false;
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(led_control_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void ensureConnections() {
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
  }
  if (!client.connected()) {
    reconnect();
  }
}

void setup() {
  Serial.begin(9600);
  setupWifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);

  digitalWrite(redLed, LOW);
  digitalWrite(greenLed, LOW);
}

void loop() {
  ensureConnections();
  client.loop();

  if (!ledControl) {
    // Read distance from ultrasonic sensor
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    unsigned long duration = pulseIn(echoPin, HIGH);
    float distance = duration * 0.034 / 2;

    if (distance < 10) {
      digitalWrite(redLed, HIGH);
      digitalWrite(greenLed, LOW);
      Serial.print(distance);
      Serial.println(": Red LED ON, Green LED OFF");
      preDistance = distance;
    } else {
      digitalWrite(redLed, LOW);
      digitalWrite(greenLed, HIGH);
      Serial.print(distance);
      Serial.println(": Red LED OFF, Green LED ON");
      preDistance = distance;
    }

    delay(400);

    // Check if any reads failed and exit early (to try again).
    if (isnan(distance)) {
      Serial.println("Failed to read from sensors!");
      return;
    }

    // Publish Sensor Data
    client.publish(distance_topic, String(distance).c_str(), true);

    delay(2000); // Delay between measurements
  }
}