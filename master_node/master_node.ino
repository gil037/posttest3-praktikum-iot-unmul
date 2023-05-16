/*
    @file master_node.ino
 
    @brief Kode untuk master node
 
    Master node berfungsi untuk berkomunikasi dengan Blynk.
    Master node berkomunikasi dengan edge node menggunakan protokol MQTT.
    Master node berperan sebagai publisher dan subscriber.
    Sebagai publisher, master node akan mengirimkan data berupa Button State Alarm dan Intensitas cahaya dan buzzer dari Blynk ke edge node.
    Sebagai Subscriber, master node akan menerima data dari edge node.
*/
 
// Libraries
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <BlynkSimpleEsp8266.h>
 
// BLYNK_AUTH_TOKEN
const char* auth = "Y_mnlCkmKFhOIpLHfyyUsZw9nt0zyEa9";
 
// Update these with values suitable for your network.
const char* ssid = "Universitas Mulawarman";
const char* password = "";
 
// The mqtt server used
const char* mqttServer = "broker.hivemq.com";
 
// Instantiate object from classes
WiFiClient espClient;
PubSubClient client(espClient);
BlynkTimer timer;
 
// for publishing
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
 
// variables to store data received from sensors placed on edge node
float temperature;
float lpgLevel;
float coLevel;
float smokeLevel;
 
/*
    data on this variable will be sent to the edge node
    data on this variable can be changed from Blynk.
*/
int alarmState = 1;
int ledBrightness = 0;
int buzzerLoudness = 31;
 
void setupWifi() {
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
    if (strcmp(topic, "iot_unmul/iot_c_8/temperature") == 0) {
        temperature = data.toFloat();
    } else if (strcmp(topic, "iot_unmul/iot_c_8/lpg") == 0) {
        lpgLevel = data.toFloat();
    } else if (strcmp(topic, "iot_unmul/iot_c_8/co") == 0) {
        coLevel = data.toFloat();
    } else if (strcmp(topic, "iot_unmul/iot_c_8/smoke") == 0) {
        smokeLevel = data.toFloat();
    }
}
 
// read data from virtual pin 0 blynk
BLYNK_WRITE(0){
    alarmState = param.asInt();
}
 
// read data from virtual pin 5 blynk
BLYNK_WRITE(5){
    ledBrightness = param.asInt();
}
 
// read data from virtual pin 6 blynk
BLYNK_WRITE(6){
    buzzerLoudness = param.asInt();
}
 
// send data to blynk
void sendData() {
    Blynk.virtualWrite(V1, temperature);
    Blynk.virtualWrite(V2, lpgLevel);
    Blynk.virtualWrite(V3, coLevel);
    Blynk.virtualWrite(V4, smokeLevel);
}
 
void setup() {
    Serial.begin(115200);
    setupWifi();
    client.setServer(mqttServer, 1883);
    client.setCallback(callback);
    Blynk.begin(auth, ssid, password, "blynk.cloud", 80);
    timer.setInterval(500L, sendData);
}
 
void loop() {
    if (!client.connected()) {
        reconnect();
    }
 
    client.loop();
 
    Blynk.run();
    timer.run();
 
    delay(2000);
 
    // publish data
    snprintf (msg, MSG_BUFFER_SIZE, "%d", alarmState);
    client.publish("iot_unmul/iot_c_8/alarm_state", msg);
    snprintf (msg, MSG_BUFFER_SIZE, "%d", ledBrightness);
    client.publish("iot_unmul/iot_c_8/led_brightness", msg);
    snprintf (msg, MSG_BUFFER_SIZE, "%d", buzzerLoudness);
    client.publish("iot_unmul/iot_c_8/buzzer_loudness", msg);
}