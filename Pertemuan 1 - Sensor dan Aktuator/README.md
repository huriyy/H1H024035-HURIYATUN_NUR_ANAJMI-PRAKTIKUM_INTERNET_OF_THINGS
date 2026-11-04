## Percobaan 1A: Akuisisi Data Sensor DHT22

### 1. Diagram Alur (Flowchart) Akuisisi Data Sensor DHT22
Berikut adalah gambaran alur proses pembacaan data sensor:

<img width="408" height="245" alt="Screenshot 2026-11-04 233333" src="https://github.com/user-attachments/assets/c475b593-06f8-403a-9c34-7f64e27280d1" />


---

### 2. Fungsi Perintah `isnan()` pada Program
Fungsi `isnan()` (*Is Not a Number*) digunakan untuk memeriksa apakah variabel mengembalikan nilai berupa bukan angka (`NaN`). On sensor DHT22, pembacaan yang gagal (misalnya akibat koneksi kabel data yang longgar, gangguan *timing* komunikasi, atau ketidakstabilan daya) akan mengembalikan nilai `NaN`. Dengan mengecek kondisi ini menggunakan `if (isnan(...))`, mikrokontroler dapat memberikan penanganan error secara aman tanpa menyebabkan gangguan sistem atau kalkulasi yang salah.

---

### 3. Alasan Diperlukannya Jeda (Delay) Minimal 2 Detik
Sensor seri DHT (khususnya DHT22) memiliki batas *sampling rate* yang relatif lambat, yaitu sekitar **0.5 Hz** (hanya dapat memperbarui data pembacaan fisik setiap 2 detik sekali). Jika mikrokontroler meminta atau membaca data lebih cepat dari rentang waktu tersebut (misalnya setiap 100 ms), sensor belum selesai melakukan konversi fisik dan respons komunikasi, sehingga menghasilkan data yang *stuck* (sama terus), eror, atau mengembalikan nilai `NaN`.

---

### 4. Modifikasi Program Percobaan 1A (Rata-Rata 5 Kali Pembacaan)

```cpp
#include <DHT.h>

#define DHTPIN 4       // Pin data DHT22 terhubung ke GPIO 4 ESP32
#define DHTTYPE DHT22  // Tipe sensor yang digunakan (DHT22)

DHT dht(DHTPIN, DHTTYPE); // Inisialisasi objek sensor DHT

void setup() {
  Serial.begin(115200);   // Membuka komunikasi serial dengan baud rate 115200
  dht.begin();            // Menginisialisasi komunikasi dengan sensor DHT22
  Serial.println("Memulai akuisisi data sensor DHT22 (Modifikasi Rata-rata 5x)...");
}

void loop() {
  float totalSuhu = 0;       // Akumulator nilai suhu
  float totalKelembaban = 0; // Akumulator nilai kelembaban
  int pembacaanValid = 0;    // Counter jumlah sampel yang valid

  // Iterasi pembacaan data sebanyak 5 kali
  for (int i = 0; i < 5; i++) {
    float tempSuhu = dht.readTemperature();  // Membaca nilai suhu instan
    float tempHum = dht.readHumidity();      // Membaca nilai kelembaban instan

    // Validasi apakah sampel bukan berupa NaN
    if (!isnan(tempSuhu) && !isnan(tempHum)) {
      totalSuhu += tempSuhu;       // Menambahkan nilai suhu ke akumulator
      totalKelembaban += tempHum;  // Menambahkan nilai kelembaban ke akumulator
      pembacaanValid++;            // Menambah hitungan sampel valid
    } else {
      Serial.println("Peringatan: Sampel pembacaan gagal (NaN), dilewati.");
    }
    
    delay(2000); // Jeda 2 detik antar sampel sesuai batas sampling rate sensor
  }

  // Perhitungan rata-rata jika sampel valid terdeteksi
  if (pembacaanValid > 0) {
    float rataSuhu = totalSuhu / pembacaanValid;           // Menghitung rata-rata suhu
    float rataKelembaban = totalKelembaban / pembacaanValid; // Menghitung rata-rata kelembaban

    // Menampilkan hasil kalkulasi rata-rata ke Serial Monitor
    Serial.print("[RATA-RATA 5 SAMPEL] Suhu: ");
    Serial.print(rataSuhu);
    Serial.print(" °C | Kelembaban: ");
    Serial.print(rataKelembaban);
    Serial.println(" %");
  } else {
    Serial.println("Gagal membaca seluruh sampel data dari sensor!");
  }
}
```

---

## Percobaan 1B: Kendali Aktuator Relay Berdasarkan Data Sensor

### 1. Peran Nilai Ambang Batas (Threshold) dalam Sistem Kendali
Nilai ambang batas (*threshold*) diperlukan sebagai acuan atau batasan logika keputusan bagi mikrokontroler untuk membedakan antara kondisi fisik normal dan kondisi fisik yang membutuhkan respons. Tanpa adanya *threshold*, mikrokontroler tidak memiliki patokan pasti kapan harus mengaktifkan (*turn ON*) atau mematikan (*turn OFF*) aktuator (seperti relay atau lampu) berdasarkan perubahan data dari sensor.

---

### 2. Dampak Menurunkan Suhu Threshold Menjadi Sangat Rendah (Misal 20.0 °C)
Aktuator (relay/LED) akan **hampir selalu berada dalam kondisi ON (menyala)**. Hal ini disebabkan oleh suhu ruangan tempat percobaan secara umum berkisar antara $25.0^{\circ}\text{C}$ hingga $30.0^{\circ}\text{C}$, yang mana nilainya sudah melampaui $20.0^{\circ}\text{C}$. Akibatnya, sistem kehilangan fungsinya sebagai pengontrol otomatis berbasis perubahan kondisi lingkungan karena aktuator terus-menerus aktif.

---

### 3. Perbedaan Kendali Kondisi Tunggal vs Kendali Histerisis (Dua Ambang Batas)
* **Kendali Kondisi Tunggal (*Single Threshold*):** Hanya memakai satu titik acuan. Jika suhu berfluktuasi tipis di sekitar titik acuan tersebut, aktuator dapat menyala dan mati secara berulang-ulang dalam waktu singkat (*chattering*) yang berisiko merusak komponen mekanis relay.
* **Kendali Histerisis (*Dual Threshold*):** Memakai dua titik acuan (misalnya: ON di atas $30.0^{\circ}\text{C}$ dan OFF di bawah $28.0^{\circ}\text{C}$) untuk memberikan rentang toleransi (*deadband*). Pendekatan ini menjaga kerja aktuator jauh lebih stabil, responsif, dan bertahan lama.

---

### 4. Modifikasi Program Percobaan 2A (Logika Histerisis / Dua Ambang Batas)

```cpp
#include <DHT.h>

#define DHTPIN 4        // Pin data DHT22 terhubung ke GPIO 4 ESP32
#define DHTTYPE DHT22   // Jenis sensor yang digunakan (DHT22)
#define RELAYPIN 14     // Pin kendali relay/LED terhubung ke GPIO 14 (atau GPIO 26)

DHT dht(DHTPIN, DHTTYPE); // Inisialisasi objek sensor DHT

// Definisi ambang batas histerisis
const float thresholdHIGH = 30.0; // Batas atas: Aktuator ON jika suhu > 30.0 °C
const float thresholdLOW  = 28.0; // Batas bawah: Aktuator OFF jika suhu < 28.0 °C

bool statusAktuator = false;     // Variabel penanda status aktuator (false = OFF, true = ON)

void setup() {
  Serial.begin(115200);           // Inisialisasi komunikasi serial pada 115200 bps
  dht.begin();                    // Mengaktifkan sensor DHT22
  pinMode(RELAYPIN, OUTPUT);      // Mengatur pin relay sebagai output digital
  digitalWrite(RELAYPIN, LOW);    // Menjamin relay dalam keadaan mati saat booting
}

void loop() {
  float suhu = dht.readTemperature(); // Membaca data suhu lingkungan

  // Evaluasi kevalidan sampel
  if (isnan(suhu)) {
    Serial.println("Gagal membaca data dari sensor!");
  } else {
    Serial.print("Suhu Terbaca: ");
    Serial.print(suhu);
    Serial.print(" °C -> ");

    // Logika Keputusan Histerisis
    if (suhu > thresholdHIGH) {
      statusAktuator = true;  // Suhu melampaui batas atas, ubah status ke ON
    } else if (suhu < thresholdLOW) {
      statusAktuator = false; // Suhu turun di bawah batas bawah, ubah status ke OFF
    }
    // Catatan: Apabila suhu berada di antara 28.0 °C dan 30.0 °C, statusAktuator mempertahankan posisi terakhirnya

    // Aktuasi Perangkat Fisik
    if (statusAktuator) {
      digitalWrite(RELAYPIN, HIGH); // Mengirim sinyal HIGH untuk menyalakan relay/LED
      Serial.println("Status Aktuator: ON");
    } else {
      digitalWrite(RELAYPIN, LOW);  // Mengirim sinyal LOW untuk mematikan relay/LED
      Serial.println("Status Aktuator: OFF");
    }
  }

  delay(2000); // Jeda pembacaan 2 detik
}
```

---

## Pertanyaan Analisis

### 1. Uraian Hasil Tugas Percobaan Praktikum
* **Percobaan 1A:** ESP32 berhasil melakukan akuisisi data suhu dan kelembaban udara secara *real-time* dari sensor DHT22 via GPIO 4 dengan interval pembacaan setiap 2 detik. Pemrosesan fungsi `isnan()` terbukti efektif dalam mengantisipasi data *invalid* agar sistem tidak menampilkan kesalahan pada Serial Monitor.
* **Percobaan 1B:** ESP32 berhasil mengeksekusi logika kendali otomatis pada relay/LED (GPIO 26) berdasarkan *threshold* suhu $>30.0^{\circ}\text{C}$. Ketika suhu lingkungan terbaca $>30.0^{\circ}\text{C}$, status aktuator berubah menjadi ON (Menyala). Saat suhu berada $\le30.0^{\circ}\text{C}$, status aktuator kembali ke OFF (Mati).

---

### 2. Pengaruh Akurasi dan Waktu Tanggap (*Response Time*) Sensor terhadap Reaksi Aktuator
Akurasi sensor memastikan aktuator bereaksi pada momen yang tepat tanpa pemicuan yang keliru (*false trigger*), sedangkan *response time* menentukan seberapa cepat perubahan kondisi fisik terdeteksi hingga aktuator mengeksekusi tindakan. Semakin tinggi tingkat akurasi dan semakin cepat waktu tanggap sensor, semakin presisi dan instan pula reaksi aktuator dalam menjaga stabilitas sistem.

---

### 3. Cara Kerja Sistem dalam Mengubah Data Sensor Menjadi Keputusan Kendali (Akuisisi hingga Aktuasi)
1. **Akuisisi Data:** Sensor DHT22 mendeteksi kondisi fisik (suhu & kelembaban) dan mengonversinya menjadi sinyal digital yang dikirimkan ke GPIO 4 ESP32.
2. **Pengolahan & Evaluasi Logika:** Mikrokontroler mengolah sinyal digital tersebut menggunakan *library* `DHT.h`, melakukan pengecekan data *invalid* via `isnan()`, dan mengevaluasi nilainya terhadap instruksi percabangan (`if-else`) berbasis *threshold*.
3. **Eksekusi Aktuasi:** Keputusan dari logika program menentukan tegangan pin keluaran ESP32 (`HIGH` atau `LOW`), yang selanjutnya mengendalikan *switch* fisik modul relay untuk mengaktifkan atau mematikan perangkat.

---

### 4. Penerapan Kombinasi Akuisisi Data dan Kendali Aktuator pada Sistem IoT (Smart Farming / Smart Home)
Pada aplikasi *smart farming*, sensor dapat membaca tingkat kelembaban tanah dan suhu udara secara berkala. Ketika kelembaban tanah terdeteksi turun di bawah batas ambang bawah, ESP32 memicu relay untuk mengaktifkan pompa irigasi secara otomatis. Setelah kadar air tanah mencapai batas kecukupan (ambang atas), sistem membaca nilai terkini tersebut dan segera mematikan pompa irigasi, menciptakan sistem pengairan otomatis yang hemat energi dan air.

---
