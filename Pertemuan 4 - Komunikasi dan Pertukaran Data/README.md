
## Percobaan 4A: Subscribe dan Deserialisasi Data JSON untuk Kendali Aktuator

### 1. Diagram Alur (Flowchart) Penerimaan dan Pemrosesan Pesan pada Callback
Berikut adalah diagram alur logika pemrosesan pesan masuk pada fungsi `callback`:

<img width="626" height="423" alt="Screenshot 2026-11-05 001044" src="https://github.com/user-attachments/assets/52120445-695c-4afa-89df-ce056d2671ee" />
---

### 2. Dampak Jika Pesan yang Dipublikasikan Bukan Format JSON Valid
Apabila data yang dikirimkan oleh klien MQTT tidak berformat JSON valid (misalnya teks biasa `ON` tanpa tanda kurung kurawal atau petik JSON), maka:
* Fungsi `deserializeJson(doc, pesan)` akan mengembalikan status kesalahan (*deserialization error*).
* Kondisi `if (error)` bernilai `true`, sehingga pesan kesalahan `"Gagal parsing JSON:"` dicetak ke Serial Monitor.
* Eksekusi fungsi `callback` langsung dihentikan oleh perintah `return;`. Hal ini melindungi sistem dari *crash* atau *unhandled exception*, dan kondisi LED tetap aman pada status terakhirnya.

---

### 3. Alasan `client.subscribe()` Dipanggil di Dalam `hubungkanMQTT()`, Bukan `setup()`
Instruksi `client.subscribe()` dipanggil di dalam `hubungkanMQTT()` karena:
* Fungsi `setup()` hanya dieksekusi satu kali saat mikrokontroler pertama kali dinyalakan. Jika *subscribe* ditaruh di `setup()`, subskripsi topic hanya didaftarkan sekali.
* Apabila jaringan WiFi atau koneksi broker terputus di tengah jalan (*disconnect*), ESP32 akan melakukan proses penyambungan ulang (*reconnect*). Setelah sambungan pulih, broker MQTT menganggapnya sebagai sesi baru sehingga daftar *subscribe* sebelumnya hilang.
* Menempatkan `client.subscribe()` di dalam `hubungkanMQTT()` menjamin ESP32 **otomatis memulihkan subskripsi topic** setiap kali koneksi ke broker MQTT berhasil disambungkan kembali.

---

### 4. Kode Modifikasi Percobaan 4A (Pengaturan Kecerahan LED via PWM)

```cpp
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* ssid = "Gejes gejes";
const char* password = "cihuykanduluee";
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* topicPerintah = "unsoed/tk245004/kelompok1/iotmodul4a";

const int ledPin = 2; // Pin LED (GPIO 2 / D4)

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  String pesan;
  for (unsigned int i = 0; i < length; i++) {
    pesan += (char)payload[i];
  }

  Serial.print("Pesan diterima [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(pesan);

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, pesan);

  if (error) {
    Serial.print("Gagal parsing JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // MODIFIKASI: Mengambil parameter "perintah" dan "intensitas" dari JSON
  const char* perintah = doc["perintah"];
  int intensitas = doc["intensitas"] | 0; // Mengambil nilai intensitas (default 0 jika tidak ada)

  if (String(perintah) == "ON") {
    // Jika ada parameter intensitas (1-255), atur kecerahan menggunakan PWM analogWrite
    if (intensitas > 0) {
      analogWrite(ledPin, intensitas); 
      Serial.print("Aktuator: ON (PWM Kecerahan: ");
      Serial.print(intensitas);
      Serial.println(")");
    } else {
      digitalWrite(ledPin, HIGH); // Nyala penuh jika intensitas tidak ditentukan
      Serial.println("Aktuator: ON (Nyala Penuh)");
    }
  } else if (String(perintah) == "OFF") {
    digitalWrite(ledPin, LOW); // Mematikan LED
    analogWrite(ledPin, 0);
    Serial.println("Aktuator: OFF");
  }
}

void hubungkanWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi berhasil terhubung!");
}

void hubungkanMQTT() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke broker MQTT...");
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("berhasil terhubung!");
      client.subscribe(topicPerintah); // Mendaftarkan subskripsi topic
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

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  hubungkanWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback); // Mendaftarkan penangan pesan callback
}

void loop() {
  if (!client.connected()) {
    hubungkanMQTT();
  }
  client.loop(); // Memproses lalu lintas pesan MQTT secara berkelanjutan
}
```

---

## Percobaan 4B: Pertukaran Data Dua Arah (Publish dan Subscribe Simultan)

### 1. Bahaya Penggunaan `delay()` pada Komunikasi Dua Arah
Fungsi `delay()` bersifat *blocking* (menghentikan seluruh instruksi mikrokontroler secara total selama durasi jeda). 
* Jika program menggunakan `delay(5000)` untuk mengatur interval *publish*, maka selama 5 detik mikrokontroler mati suri dan tidak dapat mengeksekusi `client.loop()`.
* Dampaknya: Pesan *subscribe* yang dikirim oleh pengguna menjadi tertunda (*delay/latency* tinggi), penumpukan *buffer* paket data di broker, hingga terjadi pemutusan koneksi (*timeout/disconnected*) akibat ESP32 gagal mengirimkan sinyal *keep-alive ping* tepat waktu.

---

### 2. Cara Kerja Mekanisme *Non-Blocking* Menggunakan `millis()`
Fungsi `millis()` mengembalikan jumlah milidetik sejak perangkat pertama kali booting:
1. Variabel `waktuTerakhirPublish` mencatat nilai `millis()` terakhir saat data sensor dikirim.
2. Di dalam perulangan `loop()`, program terus menghitung selisih `millis() - waktuTerakhirPublish`.
3. Selama selisih belum melampaui `intervalPublish` (5000 ms), alur program langsung melompati blok pengiriman data dan tetap konsisten menjalankan `client.loop()`.
4. Begitu selisih mencapai atau melebihi 5000 ms, data sensor terbaru dibaca, dipublikasikan via `client.publish()`, dan variabel `waktuTerakhirPublish` diperbarui dengan nilai `millis()` terkini.

---

### 3. Akibat Jika `client.loop()` Jarang Dipanggil
Jika `client.loop()` jarang dipanggil (misalnya disisipi delay panjang):
* ESP32 menjadi tidak responsif (*unresponsive*) terhadap perintah kendali yang masuk.
* Perubahan status aktuator (LED/Buzzer) akan sangat lambat atau tidak tereksekusi sama sekali.
* Sesi koneksi TCP ke broker MQTT akan diputus secara sepihak oleh server broker karena dianggap mati (*ping timeout*).

---

### 4. Kode Modifikasi Percobaan 4B (Dua Topic Subskripsi: Kendali LED & Buzzer)

```cpp
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

const char* ssid = "Gejes gejes";
const char* password = "Cihuykanduluee";
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;

// Topic Publish Data Sensor
const char* topicData = "unsoed/tk245004/kelompok1/datasensor";

// MODIFIKASI: Dua Topic Subscribe untuk Dua Aktuator Berbeda
const char* topicLED = "unsoed/tk245004/kelompok1/perintah/led";
const char* topicBuzzer = "unsoed/tk245004/kelompok1/perintah/buzzer";

#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const int ledPin = D4;     // Pin LED
const int buzzerPin = D5;  // MODIFIKASI: Pin Buzzer (GPIO 14 / D5)

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long waktuTerakhirPublish = 0;
const long intervalPublish = 5000;

// Callback universal yang dapat membedakan asal Topic
void callback(char* topic, byte* payload, unsigned int length) {
  String pesan;
  for (unsigned int i = 0; i < length; i++) {
    pesan += (char)payload[i];
  }

  JsonDocument doc;
  if (deserializeJson(doc, pesan)) return; // Abaikan jika format data corrupt

  const char* perintah = doc["perintah"];

  // Evaluasi asal Topic pesan masuk
  if (String(topic) == topicLED) {
    if (String(perintah) == "ON") {
      digitalWrite(ledPin, HIGH);
      Serial.println("[TOPIC LED] Perintah diterima -> LED ON");
    } else if (String(perintah) == "OFF") {
      digitalWrite(ledPin, LOW);
      Serial.println("[TOPIC LED] Perintah diterima -> LED OFF");
    }
  } 
  else if (String(topic) == topicBuzzer) { // MODIFIKASI: Penanganan khusus Topic Buzzer
    if (String(perintah) == "ON") {
      digitalWrite(buzzerPin, HIGH);
      Serial.println("[TOPIC BUZZER] Perintah diterima -> BUZZER BUNYI");
    } else if (String(perintah) == "OFF") {
      digitalWrite(buzzerPin, LOW);
      Serial.println("[TOPIC BUZZER] Perintah diterima -> BUZZER MATI");
    }
  }
}

void hubungkanWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("WiFi berhasil terhubung!");
}

void hubungkanMQTT() {
  while (!client.connected()) {
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      // MODIFIKASI: Melakukan subskripsi ke dua topic sekaligus
      client.subscribe(topicLED);
      client.subscribe(topicBuzzer);
      Serial.println("Terhubung ke broker & berhasil subscribe topic LED & Buzzer");
    } else {
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT); // Mengatur pin buzzer sebagai OUTPUT
  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, LOW);

  dht.begin();
  hubungkanWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) hubungkanMQTT();
  client.loop(); // Memproses pesan masuk secara instant (non-blocking)

  // Publish Data Sensor DHT11 secara berkala menggunakan millis()
  if (millis() - waktuTerakhirPublish > intervalPublish) {
    waktuTerakhirPublish = millis();
    float suhu = dht.readTemperature();
    
    if (!isnan(suhu)) {
      JsonDocument doc;
      doc["suhu"] = suhu;
      char buffer[128];
      serializeJson(doc, buffer);
      client.publish(topicData, buffer);
      Serial.print("Data Telemetri Terkirim: ");
      Serial.println(buffer);
    }
  }
}
```

---

## Pertanyaan Analisis 

### 1. Uraian Hasil Tugas Praktikum Modul 4
* **Percobaan 4A:** ESP8266 berhasil terhubung ke broker MQTT `broker.hivemq.com` dan mendaftarkan subskripsi topic perintah. Saat pesan JSON seperti `{"perintah": "ON"}` dikirimkan melalui aplikasi client MQTT, fungsi `callback` memproses deserialisasi data dan berhasil menyalakan/mematikan LED secara *real-time*.
* **Percobaan 4B:** ESP8266 berhasil menjalankan komunikasi dua arah secara *full duplex*. Perangkat mempublikasikan data suhu dari sensor DHT11 setiap 5 detik (menggunakan `millis()`), sekaligus secara simultan mampu merespons perintah kontrol LED tanpa mengalami kemacetan atau pemutusan sambungan dari broker.

---

### 2. Perbandingan Komunikasi Satu Arah vs Komunikasi Dua Arah
* **Komunikasi Satu Arah (*Publish* Saja - Modul 3):** Perangkat IoT hanya berfungsi sebagai penyedia data telemetry (*passive monitoring node*). Perangkat tidak memiliki kemampuan untuk menerima masukan, koreksi, atau instruksi interaktif dari pengguna secara *real-time*.
* **Komunikasi Dua Arah (*Publish & Subscribe* - Modul 4):** Perangkat IoT bertindak sebagai node cerdas (*interactive control node*). Sistem dapat mengirimkan informasi telemetry lingkungan sekaligus mendengarkan dan mengeksekusi instruksi kontrol aktuator dari jarak jauh secara simultan.

---

### 3. Keunggulan Pendekatan *Non-Blocking* (`millis()`) Dibanding *Blocking* (`delay()`)
Pada sistem komunikasi dua arah *real-time*, pendekatan *non-blocking* wajib digunakan karena:
* **Responsivitas Maksimal:** CPU mikrokontroler tidak dipaksa berhenti (*idle*), sehingga fungsi `client.loop()` dapat mengeksekusi penanganan paket *subscribe* secara cepat tanpa menunda perintah dari pengguna.
* **Stabilitas Jaringan:** Menjaga koneksi socket TCP dengan broker MQTT tetap hangat melalui transmisi paket *keep-alive ping* berkesinambungan.

---

### 4. Contoh Penerapan Komunikasi Dua Arah pada Skenario Real-World
* **Skenario:** **Sistem Smart Agriculture & Irigasi Presisi.**
* **Mekanisme Kerja:**
  * **Arah Publish (Telemetry):** Perangkat mempublikasikan data pembacaan sensor kelembaban tanah, suhu udara, dan tingkat curah hujan ke *dashboard cloud* setiap 10 detik.
  * **Arah Subscribe (Control):** Petani dapat melihat grafik kelembaban di smartphone. Jika tanah terlalu kering, petani menekan tombol "Siram" pada aplikasi seluler, mengirimkan perintah `{"pompa": "ON"}` via MQTT untuk langsung menyalakan katup solenoid/pompa air tanpa harus menunggu jadwal pembacaan sensor berikutnya.
* **Manfaat Dibandingkan Satu Arah:** Memberikan kontrol penuh (*remote override*) secara instan kepada pengguna saat terjadi situasi darurat, dibandingkan sistem satu arah yang hanya pasrah menampilkan data tanpa bisa diintervensi.

---
