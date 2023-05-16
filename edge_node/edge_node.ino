/*
    @file edge_node.ino
 
    @brief Kode untuk edge node
 
    Edge node berfungsi untuk berkomunikasi dengan master node.
    Edge node berkomunikasi dengan master node menggunakan protokol MQTT.
    Edge node berperan sebagai publisher dan subscriber.
    Sebagai publisher, edge node akan mengirimkan data berupa suhu, kadar lpg, kadar co, dan kadar asap ke master node.
    Sebagai Subscriber, edge node akan menerima data dari master node.
*/
 
// Libraries
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <MQ2.h>
 
#define LED_PIN D1
#define BUZZER_PIN D2
#define DHT_PIN D3
#define DHT_TYPE DHT11
#define MQ2_ANALOG_PIN A0
 
// Update these with values suitable for your network.
const char* ssid = "Universitas Mulawarman";
const char* password = "";
 
// The mqtt server used
const char* mqtt_server = "broker.hivemq.com";
 
// Instantiate object from classes
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHT_PIN, DHT_TYPE);
MQ2 mq2(MQ2_ANALOG_PIN);
 
// for publishing
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
 
// variables to store data received from master node
int alarmState;
int ledBrightness;
int buzzerLoudness;
 
void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
 
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
 
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
 
    randomSeed(micros());
 
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}
 
// connect to the broker
void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str())) {
            Serial.println("connected");
            client.subscribe("iot_unmul/iot_c_8/#");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}
 
// called when there is a new message from a subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Pesan Diterima [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
 
    String data = "";
 
    for (int i = 0; i < length; i++) {
        data += (char)payload[i];
    }
 
    // check setiap topic yang mungkin
    if (strcmp(topic, "iot_unmul/iot_c_8/alarm_state") == 0) {
        alarmState = data.toInt();
    } else if (strcmp(topic, "iot_unmul/iot_c_8/led_brightness") == 0) {
        ledBrightness = data.toInt();
    } else if (strcmp(topic, "iot_unmul/iot_c_8/buzzer_loudness") == 0) {
        buzzerLoudness = data.toInt();
    }
}
 
void setup() {
    Serial.begin(115200);
 
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
 
    setup_wifi();
 
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
 
    dht.begin();
    mq2.begin();
}
 
void loop() {
    if (!client.connected()) {
        reconnect();
    }
 
    client.loop();
 
    float temperature = dht.readTemperature();
    float lpgLevel = mq2.readLPG();
    float coLevel = mq2.readCO();
    float smokeLevel = mq2.readSmoke();
 
    if (isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }
 
    // Event log
    if (alarmState) {
        if (temperature > 20 || lpgLevel > 30 || coLevel > 30 || smokeLevel > 30) {
            analogWrite(LED_PIN, ledBrightness);
            analogWrite(BUZZER_PIN, buzzerLoudness);
        } else {
            digitalWrite(LED_PIN, LOW);
            digitalWrite(BUZZER_PIN, LOW);      
        }
    } else {
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);    
    }
 
    delay(2000);
 
    // publish data
    snprintf (msg, MSG_BUFFER_SIZE, "%0.2f", temperature);
    client.publish("iot_unmul/iot_c_8/temperature", msg);
    snprintf (msg, MSG_BUFFER_SIZE, "%0.2f", lpgLevel);
    client.publish("iot_unmul/iot_c_8/lpg_level", msg);
    snprintf (msg, MSG_BUFFER_SIZE, "%0.2f", coLevel);
    client.publish("iot_unmul/iot_c_8/co_level", msg);
    snprintf (msg, MSG_BUFFER_SIZE, "%0.2f", smokeLevel);
    client.publish("iot_unmul/iot_c_8/smoke_level", msg);
}