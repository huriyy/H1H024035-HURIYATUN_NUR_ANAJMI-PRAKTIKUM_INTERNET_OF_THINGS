#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>


// Konfigurasi WiFi
const char* ssid = "Gejes";
const char* password = "Adalahpokoknya";


// Konfigurasi MQTT
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* topicPerintah = "unsoed/tk245004/kelompok1/iotmodul4a";


// Konfigurasi LED
const int ledPin = 2;


// Membuat objek WiFi dan MQTT
WiFiClient espClient;
PubSubClient client(espClient);


// Fungsi callback dipanggil otomatis ketika ada pesan baru
void callback(char* topic, byte* payload, unsigned int length) {
  String pesan;


  // Mengubah payload menjadi String
  for (unsigned int i = 0; i < length; i++) {
    pesan += (char)payload[i];
  }


  Serial.print("Pesan diterima [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(pesan);


  // Deserialisasi data JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, pesan);


  // Mengecek hasil parsing JSON
  if (error) {
    Serial.print("Gagal parsing JSON: ");
    Serial.println(error.c_str());
    return;
  }


  // Mengambil nilai perintah dari JSON
  const char* perintah = doc["perintah"];


  // Mengontrol LED
  if (String(perintah) == "ON") {
    digitalWrite(ledPin, HIGH);
    Serial.println("Aktuator: ON");


  } else if (String(perintah) == "OFF") {
    digitalWrite(ledPin, LOW);
    Serial.println("Aktuator: OFF");
  }
}


// Fungsi untuk menghubungkan ESP8266 ke WiFi
void hubungkanWiFi() {
  WiFi.begin(ssid, password);


  Serial.print("Menghubungkan ke WiFi");


  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }


  Serial.println("\nWiFi berhasil terhubung!");
}


// Fungsi untuk menghubungkan ESP8266 ke broker MQTT
void hubungkanMQTT() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke broker MQTT...");


    // Membuat ID client secara acak
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);


    if (client.connect(clientId.c_str())) {
      Serial.println("berhasil terhubung!");


      // Subscribe ke topic setelah berhasil terhubung
      client.subscribe(topicPerintah);


      Serial.print("Subscribe ke topic: ");
      Serial.println(topicPerintah);


    } else {
      Serial.print("gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 2 detik");


      delay(2000);
    }
  }
}


// Fungsi setup dijalankan sekali
void setup() {
  Serial.begin(115200);


  // Mengatur LED D4 sebagai output
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);


  // Menghubungkan ESP8266 ke WiFi
  hubungkanWiFi();


  // Mengatur server MQTT
  client.setServer(mqttServer, mqttPort);


  // Mendaftarkan fungsi callback
  client.setCallback(callback);
}


// Fungsi loop dijalankan terus-menerus
void loop() {
  // Mengecek koneksi MQTT
  if (!client.connected()) {
    hubungkanMQTT();
  }


  // Memproses pesan MQTT yang masuk
  client.loop();
}

