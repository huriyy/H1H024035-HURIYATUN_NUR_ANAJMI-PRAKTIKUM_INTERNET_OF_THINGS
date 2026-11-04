
## Percobaan 3A: Komunikasi Data Menggunakan HTTP

### 1. Diagram Alur (Flowchart) Pengiriman Data HTTP POST
Berikut adalah diagram alur proses pengiriman data melalui HTTP POST:

<img width="327" height="424" alt="Screenshot 2026-11-05 000212" src="https://github.com/user-attachments/assets/8fdc8004-3d2b-4d3c-b3f7-bf44083257d9" />

### 2. Fungsi Perintah `http.addHeader("Content-Type", "application/json")`
Perintah ini berfungsi untuk menyisipkan informasi metadata pada bagian *header* HTTP Request. Parameter `"Content-Type: application/json"` memberitahu server (`httpbin.org`) bahwa *payload* / *body* data yang dikirimkan oleh ESP32 dikemas dalam format JSON. Hal ini memungkinkan server untuk melakukan *parsing* atau memproses struktur data JSON secara tepat.

### 3. Arti Kode Response HTTP 200 & Contoh Kode Response Lain
* **HTTP 200 (OK):** Menandakan bahwa permintaan (*request*) HTTP POST dari ESP32 telah berhasil diterima, diproses, dan disetujui dengan baik oleh server.
* **Contoh Kode Response Lain:**
  * **404 (Not Found):** Server tidak dapat menemukan alamat URL (*endpoint*) yang dituju.
  * **500 (Internal Server Error):** Terjadi kesalahan atau eror internal pada sisi server saat mengolah data.
  * **400 (Bad Request):** Format data yang dikirimkan oleh klien salah atau tidak sesuai dengan spesifikasi server.

### 4. Modifikasi Program Percobaan 3A (Menambahkan Parameter Waktu `millis()`)

```cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "Gejes gejes";
const char* password = "cihuykandululeee";
const char* serverUrl = "https://httpbin.org/post"; // Endpoint uji HTTP POST

void setup() {
  Serial.begin(115200);             // Membuka serial monitor pada baud rate 115200 bps
  WiFi.begin(ssid, password);       // Memulai koneksi ke jaringan WiFi

  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);                     // Jeda 500 ms saat menunggu koneksi WiFi
    Serial.print(".");
  }
  Serial.println("\nWiFi berhasil terhubung!");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);          // Menginisialisasi koneksi ke target URL server
    http.addHeader("Content-Type", "application/json"); // Menetapkan header format JSON

    // Membuat dokumen JSON menggunakan ArduinoJson
    JsonDocument doc;
    doc["timer"] = millis() / 1000; // MODIFIKASI: Menambahkan waktu (detik) sejak dinyalakan
    doc["suhu"] = 28.5;             // Menambahkan data suhu (°C)
    doc["kelembaban"] = 65.0;       // Menambahkan data kelembaban (%)

    String requestBody;
    serializeJson(doc, requestBody); // Mengonversi objek JSON menjadi teks (string)

    Serial.print("Mengirim data: ");
    Serial.println(requestBody);

    // Mengirimkan request HTTP POST ke server
    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      Serial.print("Kode Response HTTP: ");
      Serial.println(httpResponseCode);       // Menampilkan kode status (misal: 200 OK)
      Serial.println("Isi Response Server:");
      Serial.println(http.getString());       // Menampilkan balasan body dari server
    } else {
      Serial.print("Pengiriman gagal, kode error: ");
      Serial.println(httpResponseCode);
    }

    http.end(); // Menutup sirkuit koneksi HTTP untuk membebaskan resource
  }

  delay(10000); // Mengirimkan data kembali setiap 10 detik
}
```

---

## Percobaan 3B: Komunikasi MQTT

### 1. Fungsi Topic pada Protokol MQTT & Alasan Harus Unik
* **Fungsi Topic:** Topic berfungsi sebagai pengelompok/kategori alamat pesan (*routing address*) dalam pola komunikasi *Publish-Subscribe*. Perangkat pengirim (*publisher*) dan penerima (*subscriber*) menggunakan *topic* ini agar pesan tersalurkan ke target tujuan yang spesifik.
* **Mengapa Harus Unik:** Broker MQTT publik (seperti `broker.hivemq.com`) diakses oleh ribuan pengguna secara bersamaan. Jika *topic* tidak dibuat unik (misal hanya dinamai `sensor`), data yang dikirim akan saling tertimpa atau tercampur dengan data milik pengguna lain.

### 2. Fungsi Perintah `client.loop()`
Perintah `client.loop()` wajib dipanggil secara berkala di dalam fungsi `loop()` karena berfungsi untuk:
1. Menjaga koneksi tetap hidup (*keep-alive signal*) antara ESP32 dan broker MQTT.
2. Memproses antrean pesan masuk (*incoming messages*) apabila ESP32 mendaftar sebagai *subscriber*.
3. Mengelola *buffer* internal dan arus lalu lintas data pustaka `PubSubClient`.

### 3. Dampak Jika Koneksi Broker MQTT Terputus
* ESP32 tidak dapat mempublikasikan data sensor baru ke broker.
* Pada kode program Percobaan 3B, logika `if (!client.connected())` dalam fungsi `loop()` akan terdeteksi bernilai `true`.
* Sistem secara otomatis memanggil fungsi `hubungkanMQTT()` untuk melakukan upaya penyambungan ulang (*reconnect*) secara berulang hingga koneksi ke broker MQTT pulih kembali.

---

## 📊 Pertanyaan Analisis Modul 3

### 1. Uraian Hasil Tugas Percobaan Praktikum Modul 3
* **Percobaan 3A:** ESP32 berhasil terhubung ke jaringan Wi-Fi dan mengirimkan paket data sensor (suhu 28.5 °C dan kelembaban 65.0%) berformat JSON ke endpoint `https://httpbin.org/post` menggunakan metode HTTP POST. Pengujian pada Serial Monitor menunjukkan kembalian kode response **200 OK** beserta *response body* berisi salinan (*echo*) data JSON yang membuktikan payload diterima dengan tepat oleh server.
* **Percobaan 3B:** ESP32 berhasil terhubung ke jaringan Wi-Fi serta melakukan koneksi TCP persisten ke broker publik `broker.hivemq.com` pada port 1883. Menggunakan pustaka `PubSubClient`, ESP32 berhasil mempublikasikan (*publish*) data sensor berformat JSON ke *topic* khusus. Verifikasi pada aplikasi client MQTT menunjukkan data muncul secara *real-time* sesuai interval tanpa perlu mekanisme *polling*.

### 2. Perbandingan Overhead Data & Pola Komunikasi (HTTP vs MQTT)
* **Pola Komunikasi:**
  * **HTTP:** Menggunakan model *Request-Response* yang bersifat *stateless*. ESP32 (klien) harus menginisialisasi koneksi baru ke server setiap kali ingin mengirimkan data.
  * **MQTT:** Menggunakan model *Publish-Subscribe* dengan perantara broker (*persistent connection*). Perangkat pengirim (*publisher*) cukup mengirimkan data ke *topic* tanpa perlu mengetahui siapa penerimanya.
* **Besar Overhead Data:**
  * **HTTP:** Memiliki *overhead header* yang jauh lebih besar. Setiap data dikirim, HTTP menyertakan *header* teks panjang (seperti *User-Agent*, *Content-Type*, *Host*, dll.) yang ukurannya mencapai puluhan hingga ratusan byte.
  * **MQTT:** Memiliki *overhead header* yang sangat kecil dan ringan (hanya sekitar **2 byte** pada header kontrolnya), sehingga jauh lebih efisien dalam penggunaan bandwidth.

### 3. Protokol Terbaik untuk Pengiriman Data Kontinu (HTTP vs MQTT)
Untuk skenario pengiriman data sensor secara terus-menerus setiap beberapa detik dalam jangka waktu lama, **protokol MQTT jauh lebih sesuai**.
* **Alasan Utama:**
  1. **Efisiensi Bandwidth & Resource:** Header MQTT yang tipis menghemat bandwidth nirkabel dan mengurangi beban pemrosesan pada ESP32.
  2. **Koneksi Persisten (*Persistent Connection*):** MQTT tidak perlu melakukan proses *handshake* TCP/TLS berulang kali setiap kali mengirimkan data seperti pada HTTP.
  3. **Hemat Daya:** Waktu transmisi singkat dan koneksi yang terjaga mengurangi konsumsi daya listrik mikrokontroler secara signifikan.

### 4. Peran Format JSON dalam Interoperabilitas Data IoT
Format JSON (*JavaScript Object Notation*) berperan sebagai **standard data interchange format** yang bersifat *text-based* dan *language-independent*.
* **Standardisasi Struktur:** Data direpresentasikan dalam pasangan *key-value* yang terstruktur (seperti `{"suhu": 28.5}`), sehingga mudah dipahami dan di-*parse* oleh berbagai sistem.
* **Kompatibilitas Lintas Platform:** ESP32 yang menggunakan C/C++ dapat menyusun data ke string JSON, kemudian data tersebut dapat langsung dibaca oleh sistem *backend* berbasis Python, Node.js, PHP, maupun basis data seperti MongoDB/MySQL.
* **Kemudahan Pengembangan:** Sifatnya yang *human-readable* memudahkan proses *debugging* dan pengujian data via aplikasi seperti MQTT Explorer, Postman, maupun browser.

---
