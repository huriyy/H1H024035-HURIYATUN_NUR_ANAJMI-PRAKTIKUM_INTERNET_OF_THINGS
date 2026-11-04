#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>


// =======================
// WiFi
// =======================
const char* ssid = "Gejes";
const char* password = "Adalahpokoknya";


// =======================
// MQTT
// =======================
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;


const char* topicData =
  "unsoed/tk245004/kelompok1/datasensor";


const char* topicPerintah =
  "unsoed/tk245004/kelompok1/iotmodul4b";


// =======================
// DHT11
// =======================
#define DHTPIN D2       // D2 = GPIO4
#define DHTTYPE DHT11


DHT dht(DHTPIN, DHTTYPE);


// =======================
// LED
// =======================
const int ledPin = D4;  // D4 = GPIO2


// =======================
// MQTT Client
// =======================
WiFiClient espClient;
PubSubClient client(espClient);


// Waktu publish
unsigned long waktuTerakhirPublish = 0;
const long intervalPublish = 5000;




// =======================
// Callback MQTT
// =======================
void callback(char* topic, byte* payload, unsigned int length) {


  String pesan;


  for (unsigned int i = 0; i < length; i++) {
    pesan += (char)payload[i];
  }


  // Membaca JSON
  JsonDocument doc;


  if (deserializeJson(doc, pesan)) {
    Serial.println("Gagal membaca JSON!");
    return;
  }


  const char* perintah = doc["perintah"];


  // Mengontrol LED
  if (String(perintah) == "ON") {


    digitalWrite(ledPin, HIGH);
    Serial.println("Perintah diterima -> Aktuator: ON");


  } else if (String(perintah) == "OFF") {


    digitalWrite(ledPin, LOW);
    Serial.println("Perintah diterima -> Aktuator: OFF");
  }
}




// =======================
// Menghubungkan WiFi
// =======================
void hubungkanWiFi() {


  WiFi.begin(ssid, password);


  Serial.print("Menghubungkan ke WiFi");


  while (WiFi.status() != WL_CONNECTED) {


    delay(500);
    Serial.print(".");
  }


  Serial.println();
  Serial.println("WiFi berhasil terhubung!");


  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}




// =======================
// Menghubungkan MQTT
// =======================
void hubungkanMQTT() {


  while (!client.connected()) {


    Serial.print("Menghubungkan ke MQTT...");


    String clientId =
      "ESP8266Client-" + String(random(0xffff), HEX);


    if (client.connect(clientId.c_str())) {


      Serial.println("berhasil!");


      // Subscribe topic perintah
      client.subscribe(topicPerintah);


      Serial.print("Subscribe ke: ");
      Serial.println(topicPerintah);


    } else {


      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" - mencoba lagi...");


      delay(2000);
    }
  }
}




// =======================
// Setup
// =======================
void setup() {


  Serial.begin(115200);


  // LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);


  // DHT11
  dht.begin();


  // WiFi
  hubungkanWiFi();


  // MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}




// =======================
// Loop
// =======================
void loop() {


  // Cek koneksi MQTT
  if (!client.connected()) {
    hubungkanMQTT();
  }


  // Memproses pesan MQTT
  client.loop();




  // =======================
  // Publish data DHT11
  // setiap 5 detik
  // =======================
  if (millis() - waktuTerakhirPublish > intervalPublish) {


    waktuTerakhirPublish = millis();


    float suhu = dht.readTemperature();


    // Cek apakah sensor berhasil dibaca
    if (!isnan(suhu)) {


      JsonDocument doc;


      doc["suhu"] = suhu;


      char buffer[128];


      serializeJson(doc, buffer);


      // Kirim data ke MQTT
      client.publish(topicData, buffer);


      Serial.print("Data terkirim: ");
      Serial.println(buffer);


    } else {


      Serial.println("Gagal membaca DHT11!");
    }
  }
}

