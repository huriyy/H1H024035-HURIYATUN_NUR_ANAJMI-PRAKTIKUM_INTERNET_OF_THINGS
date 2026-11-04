## Percobaan 2A: Konfigurasi Mode Station (STA)

### 1. Diagram Alur (Flowchart) Koneksi ESP32 Mode Station (STA)

<img width="269" height="236" alt="Screenshot 2026-11-04 235118" src="https://github.com/user-attachments/assets/43d248b0-d982-49cd-abd0-3a4f3331851e" />

### 2. Fungsi Perintah `WiFi.mode(WIFI_STA)`

Perintah `WiFi.mode(WIFI_STA)` berfungsi mengonfigurasi modul WiFi ESP32 agar beroperasi secara khusus sebagai **Station (klien)**. Dalam mode ini, ESP32 bertindak layaknya laptop atau HP yang mencari dan terhubung ke jaringan nirkabel eksternal (router/hotspot) guna mendapatkan akses data atau internet.

### 3. Dampak Jika SSID atau Password yang Dimasukkan Salah

Apabila SSID atau *password* yang dimasukkan salah, ESP32 akan terus tertahan di dalam perulangan pengujian koneksi `while (WiFi.status() != WL_CONNECTED)`. Akibatnya:

* Serial Monitor akan mencetak tanda titik (`.`) secara terus-menerus tanpa henti.

* ESP32 tidak akan pernah berhasil mendapatkan alokasi alamat IP (*local IP*).

* Eksekusi program tidak dapat berlanjut ke baris perintah berikutnya, sehingga LED indikator status koneksi tetap mati (`LOW`).

### 4. Modifikasi Program Percobaan 2A Modul 2 (Reconnection Otomatis)

```
#include <WiFi.h>

const char* ssid = "Gejes gejes";
const char* password = "cihuykandululeee";

const int ledPin = 2; // Pin LED indikator status koneksi (GPIO 2)

void setup() {
  Serial.begin(115200);        // Membuka komunikasi serial pada 115200 bps
  pinMode(ledPin, OUTPUT);     // Mengatur GPIO 2 sebagai pin keluaran (OUTPUT)
  digitalWrite(ledPin, LOW);   // Memastikan LED mati saat awal booting

  WiFi.mode(WIFI_STA);         // Menetapkan mode WiFi ESP32 sebagai Station (STA)
  WiFi.begin(ssid, password);  // Memulai proses otentikasi koneksi ke router/hotspot

  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);               // Jeda 500 ms antar percobaan pemeriksaan status
    Serial.print(".");        // Menampilkan indikator proses koneksi di Serial Monitor
  }

  Serial.println("\nWiFi berhasil terhubung!");
  Serial.print("IP Address : ");
  Serial.println(WiFi.localIP());     // Mencetak IP address lokal yang didapat dari router
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());  // Mencetak alamat fisik (MAC Address) ESP32
  Serial.print("RSSI (dBm) : ");
  Serial.println(WiFi.RSSI());        // Mencetak indikator kekuatan sinyal WiFi
  
  digitalWrite(ledPin, HIGH);        // Menyalakan LED sebagai indikator koneksi sukses
}

void loop() {
  // Mengecek apakah status jaringan WiFi terputus dari router
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Koneksi WiFi terputus! Memulai proses reconnect otomatis...");
    digitalWrite(ledPin, LOW);       // Mematikan LED indikator karena koneksi terputus
    
    WiFi.disconnect();               // Memutus sisa sesi sirkuit jaringan yang terganggu
    WiFi.reconnect();                // Menginstruksikan ESP32 untuk mencoba menghubungkan ulang
    
    // Menunggu hingga ESP32 berhasil terhubung kembali
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    
    Serial.println("\nTerhubung kembali ke WiFi!");
    Serial.print("IP Address Baru: ");
    Serial.println(WiFi.localIP());
    digitalWrite(ledPin, HIGH);      // Menyalakan kembali LED setelah koneksi pulih
  } else {
    Serial.println("Status: Terhubung (Koneksi Stabil)");
  }

  delay(5000); // Pemeriksaan berkala kestabilan sinyal dilakukan setiap 5 detik
}

```

## Percobaan 2B: Konfigurasi Mode Access Point (AP)

### 1. Alasan Alamat IP Default Access Point ESP32 Bernilai `192.168.4.1`

Alamat IP `192.168.4.1` merupakan konfigurasi standar (*default subnet IP*) yang ditanamkan pada *firmware core* ESP-IDF/Arduino ESP32. Pemilihan segmen subnet ini bertujuan untuk **mencegah terjadinya konflik alamat IP (*IP Address Conflict*)** saat perangkat lain terhubung, karena sebagian besar router lokal maupun hotspot seluler menggunakan alokasi alamat `192.168.1.x` atau `192.168.0.x`.

### 2. Perbedaan Mendasar Mode Station (STA) dan Mode Access Point (AP)

* **Mode Station (STA):** ESP32 bertindak sebagai **klien** yang mencari dan terhubung ke pemancar nirkabel luar (seperti router atau hotspot HP) untuk memperoleh IP dan akses data jaringan eksternal.

* **Mode Access Point (AP):** ESP32 bertindak sebagai **pemancar / penyedia jaringan mandiri (hotspot)** yang memancarkan SSID sendiri, sehingga perangkat lain (seperti HP atau laptop) dapat terhubung langsung ke ESP32 tanpa memerlukan bantuan router eksternal.

### 3. Risiko Keamanan Tanpa Password / Password Terlalu Sederhana pada AP

* **Akses Tanpa Izin (*Unauthorized Access*):** Perangkat luar atau pengguna tak dikenal di sekitar jangkauan sinyal dapat secara bebas masuk ke dalam jaringan lokal ESP32.

* **Penyalahgunaan Sistem & Data:** Pihak luar yang berhasil terhubung dapat menyusupkan perintah berbahaya, memanipulasi parameter konfigurasi perangkat IoT, atau mengambil alih kendali aktuator karena lalu lintas komunikasi tidak terenkripsi dengan aman.

### 4. Modifikasi Program Percobaan 2B Modul 2 (Mode Gabungan AP + STA)

```
#include <WiFi.h>

// Kredensial untuk terhubung ke router luar (Mode STA)
const char* sta_ssid     = "ESP32_AccessPointH";
const char* sta_password = "12345678";

// Kredensial untuk memancarkan Access Point mandiri (Mode AP)
const char* ap_ssid      = "ESP32_Config_AP";
const char* ap_password  = "12345678"; // Password AP minimal 8 karakter

void setup() {
  Serial.begin(115200);

  // Mengaktifkan mode gabungan AP + STA secara bersamaan
  WiFi.mode(WIFI_AP_STA);

  // 1. Konfigurasi Pemancar Access Point (SoftAP)
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress apIP = WiFi.softAPIP();  // Mengambil IP bawaan AP (biasanya 192.168.4.1)
  Serial.println("=== Mode Access Point (AP) Aktif ===");
  Serial.print("SSID AP    : ");
  Serial.println(ap_ssid);
  Serial.print("IP Address : ");
  Serial.println(apIP);

  // 2. Konfigurasi Hubungan ke Jaringan Router (Station)
  WiFi.begin(sta_ssid, sta_password);
  Serial.println("\n=== Menghubungkan ke Jaringan Router (STA) ===");
  
  int timeoutCounter = 0;
  // Mencoba terhubung ke router dengan batas waktu 10 detik (20 x 500ms)
  while (WiFi.status() != WL_CONNECTED && timeoutCounter < 20) {
    delay(500);
    Serial.print(".");
    timeoutCounter++;
  }

  // Evaluasi keberhasilan koneksi Mode Station
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nMode Station Berhasil Terhubung!");
    Serial.print("IP Address STA : ");
    Serial.println(WiFi.localIP());     // Menginformasikan IP dari DHCP router
  } else {
    Serial.println("\nGagal terhubung ke Router STA (Access Point tetap aktif untuk konfigurasi).");
  }
}

void loop() {
  // Memantau status perangkat lain yang terhubung ke AP ESP32
  int jumlahClient = WiFi.softAPgetStationNum();
  Serial.print("[AP MONITOR] Perangkat terhubung ke AP: ");
  Serial.print(jumlahClient);

  // Memantau kestabilan koneksi ke router luar
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(" | [STA MONITOR] Status Router: Terhubung (IP: ");
    Serial.print(WiFi.localIP());
    Serial.println(")");
  } else {
    Serial.println(" | [STA MONITOR] Status Router: Terputus!");
  }

  delay(5000); // Pemantauan rutin dilakukan setiap 5 detik
}

```

## Pertanyaan Analisis

### 1. Uraian Hasil Tugas Percobaan Praktikum Modul 2

* **Percobaan 2A (Mode Station/STA):** ESP8266 berhasil terhubung ke jaringan WiFi eksternal (router/hotspot) menggunakan pustaka `WiFi.h`. Setelah terhubung, Serial Monitor berhasil menampilkan status koneksi, IP address lokal, MAC address, serta kekuatan sinyal (RSSI), dan LED indikator pada GPIO 2 menyala sebagai tanda koneksi sukses.

* **Percobaan 2B (Mode Access Point/AP):** ESP8266 berhasil berfungsi sebagai pemancar jaringan (hotspot) mandiri dengan SSID dan kata sandi yang telah ditentukan. Serial Monitor menampilkan IP default AP (`192.168.4.1`) serta mendeteksi dan memperbarui jumlah perangkat (*client*) yang terhubung secara *real-time*.

### 2. Pengaruh Kekuatan Sinyal (RSSI) terhadap Kestabilan Koneksi WiFi

Kekuatan sinyal yang diukur dalam satuan dBm (RSSI) berbanding lurus dengan stabilitas transmisi data nirkabel pada perangkat IoT:

* Semakin mendekati nol nilai RSSI (misalnya **-30 dBm hingga -60 dBm**), semakin kuat sinyal yang diterima, sehingga pengiriman data berjalan lancar, cepat, dan stabil.

* Sebaliknya, sinyal yang lemah (di bawah **-80 dBm**) dapat menyebabkan tingginya tingkat paket data yang hilang (*packet loss*), *latency/delay* transmisi meningkat, hingga koneksi terputus (*disconnection*), yang berisiko membuat pengiriman data sensor atau perintah kontrol pada sistem IoT gagal.

### 3. Cara Kerja ESP32 Membedakan Peran sebagai Klien (STA) dan Pemancar (AP)

ESP32 membedakan kedua peran tersebut pada tingkat *stack* jaringan LwIP (*Lightweight IP*) dan konfigurasi *driver* WiFi bawaan:

* **Sebagai Station (STA):** ESP32 mengaktifkan modul **DHCP Client**, yang bertugas meminta dan menerima alokasi alamat IP dari DHCP Server luar (seperti router).

* **Sebagai Access Point (AP):** ESP32 mengaktifkan modul **DHCP Server internal**, yang bertugas memancarkan SSID, mengelola proses otentikasi nirkabel, dan membagikan alamat IP secara otomatis kepada perangkat lain yang terhubung kepadanya.

### 4. Pemanfaatan Kombinasi Mode AP+STA dalam Skenario Nyata (*Provisioning*)

Kombinasi mode AP+STA sangat bermanfaat pada proses **provisioning (konfigurasi awal)** perangkat IoT komersial tanpa perlu mengubah kode program (*hardcoding*):

1. Saat pertama kali dinyalakan di lingkungan baru, ESP32 aktif dalam **mode AP** untuk memancarkan jaringan hotspot lokal.

2. Pengguna cukup menghubungkan smartphone ke hotspot tersebut untuk mengakses halaman web konfigurasi internal ESP32 dan memasukkan nama serta kata sandi WiFi rumah/kantor.

3. Setelah kredensial disimpan ke memori internal (NVS/EEPROM), ESP32 akan menggunakan data tersebut untuk terhubung ke router utama menggunakan **mode STA** tanpa kehilangan kemampuannya untuk dikonfigurasi ulang secara lokal.
