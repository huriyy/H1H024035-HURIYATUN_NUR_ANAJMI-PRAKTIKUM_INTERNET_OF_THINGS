
## Pertanyaan Praktikum 5A (Edge Processing & ESP32)

### 1. Diagram Alur (Flowchart) Fungsi `hitungMovingAverage()`
Berikut adalah bagan alir proses perhitungan Moving Average pada fungsi `hitungMovingAverage()` di ESP32:

<img width="198" height="463" alt="Screenshot 2026-11-05 002335" src="https://github.com/user-attachments/assets/85586437-1925-4be4-895d-2859b1835f20" />

---

### 2. Jelaskan fungsi dari operasi `(indeks + 1) % N` pada program!
Operasi `(indeks + 1) % N` berfungsi untuk membentuk **circular buffer** (buffer melingkar). 
* Ketika penulisan data mencapai batas maksimum ukuran array ($N-1$), penggunaan operator modulus (`%`) akan membuat `indeks` otomatis kembali berulang ke nilai `0`.
* Hal ini memungkinkan penyimpanan $N$ sampel data pembacaan sensor terbaru secara kontinu tanpa alokasi memori dinamis tambahan, sehingga data terlama akan langsung tertimpa oleh data yang paling baru secara otomatis.

---

### 3. Dampak perubahan nilai $N$ terhadap data hasil filter
* **Jika $N$ diperbesar (misalnya $N = 20$):** 
  * **Kelebihan:** Grafik data hasil filter menjadi sangat halus (*smooth*) dan sangat tahan terhadap lonjakan derau acak (*random noise*).
  * **Kelemahan:** Respons sistem terhadap perubahan suhu nyata menjadi sangat lambat (*lagging/delay* tinggi).
* **Jika $N$ diperkecil (misalnya $N = 2$):** 
  * **Kelebihan:** Respons sistem sangat cepat mendeteksi pergeseran suhu terbaru.
  * **Kelemahan:** Kemampuan filter meredam *noise* berkurang drastis, sehingga fluktuasi acak kecil masih tampak dominan pada hasil keluaran.

---

### 4. Modifikasi Program ESP32: Perhitungan Simpangan Baku (Standard Deviation)
Berikut adalah penjelasan penambahan perhitungan simpangan baku ($\sigma$) pada buffer ESP32:

```cpp
#include <DHT.h>
#include <math.h> // Memuat pustaka matematika untuk fungsi sqrt() dan pow()

#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const int N = 5;
float buffer[N];
int indeks = 0;
int jumlah = 0;

float suhuMin = 1000.0;
float suhuMax = -1000.0;

// Fungsi untuk menghitung Rata-rata Bergerak
float hitungMovingAverage(float nilaiBaru) {
  buffer[indeks] = nilaiBaru;
  indeks = (indeks + 1) % N;
  if (jumlah < N) jumlah++;

  float total = 0;
  for (int i = 0; i < jumlah; i++) {
    total += buffer[i];
  }
  return total / jumlah;
}

// ============================================================================
// MODIFIKASI TUGAS 4: Fungsi Menghitung Simpangan Baku (Standard Deviation)
// ============================================================================
float hitungStdDev(float mean) {
  // Jika data kurang dari 2, simpangan baku belum dapat dihitung secara valid
  if (jumlah < 2) return 0.0; 

  float jumlahKuadratSelisih = 0.0;
  for (int i = 0; i < jumlah; i++) {
    // Menghitung kuadrat dari selisih tiap elemen buffer terhadap nilai rata-rata
    jumlahKuadratSelisih += pow(buffer[i] - mean, 2);
  }
  
  // Menggunakan rumus varians sampel: S^2 = Sum(x - mean)^2 / (N - 1)
  float varians = jumlahKuadratSelisih / (jumlah - 1);
  
  // Simpangan baku adalah akar kuadrat dari nilai varians
  return sqrt(varians);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  float suhu = dht.readTemperature();

  if (isnan(suhu)) {
    Serial.println("Pembacaan gagal, data diabaikan");
  } else {
    float suhuFilter = hitungMovingAverage(suhu);
    
    // Pemanggilan fungsi hitungStdDev menggunakan mean dari suhuFilter
    float stdDev = hitungStdDev(suhuFilter);

    if (suhu < suhuMin) suhuMin = suhu;
    if (suhu > suhuMax) suhuMax = suhu;

    Serial.print("Mentah: "); Serial.print(suhu);
    Serial.print(" | Filter: "); Serial.print(suhuFilter);
    Serial.print(" | StdDev: "); Serial.print(stdDev); // Menampilkan Nilai StdDev
    Serial.print(" | Min: "); Serial.print(suhuMin);
    Serial.print(" | Max: "); Serial.println(suhuMax);
  }

  delay(2000);
}
```

---

## Pertanyaan Praktikum 5B (Server Processing, Python & MQTT)

### 1. Fungsi blok `try-except` pada fungsi `on_message()`
* **Fungsi:** Untuk menangani potensi kesalahan pembacaan atau kegagalan struktur pesan (*exception handling*). Kasus ini mencakup kegagalan pemformatan data JSON (`ValueError`) dan tidak tersedianya kunci `"suhu"` pada objek JSON payload (`KeyError`).
* **Akibat jika dihilangkan:** Jika ESP32 atau perangkat pengirim mengirimkan pesan yang korup, terputus, atau tidak berformat JSON secara tidak sengaja, program Python akan langsung mengalami kekacauan (*crash*) dan berhenti berjalan secara mendadak.

---

### 2. Arti nilai Z-Score dan Penggunaan Batas $|z| > 2$
* **Arti Z-score:** Z-score mengukur seberapa jauh suatu nilai sampel menyimpang dari rata-rata populasi/kelompok dalam satuan simpangan baku ($\sigma$).
$$\text{Z-score} = \frac{x - \bar{x}}{\sigma}$$
* **Alasan Batas $|z| > 2$:** Berdasarkan prinsip Distribusi Normal (Teorema Limiting Central), sekitar **95.4%** dari total populasi data alami terletak di dalam interval rentang $\pm 2\sigma$ dari rata-rata. Oleh sebab itu, data yang memiliki nilai $|z| > 2$ dianggap memiliki probabilitas kemunculan sangat kecil (hanya $\approx 4.6\%$) sehingga tergolong sebagai **anomali** atau outlier.

---

### 3. Kelemahan metode Z-Score pada data tren tajam
Metode Z-score standar mengasumsikan bahwa distribusi data bersifat stasioner (rata-rata dan standar deviasi bernilai konstan sepanjang waktu). 
* Apabila data memiliki tren naik atau turun secara signifikan/tajam (non-stasioner), rata-rata global $\bar{x}$ dan $\sigma$ dari keseluruhan rentang waktu akan bergeser drastis.
* Akibatnya, data normal pada awal atau akhir tren akan keliru teridentifikasi sebagai anomali (*false positive*), sedangkan titik pencilan yang sebenarnya di tengah-tengah tren justru bisa luput dari pendeteksian.

---

### 4. Modifikasi Program Python: Moving Average Sebelum Z-Score
Modifikasi program Python untuk menerapkan *Moving Average* (jendela 5 data) pada file CSV sebelum penghitungan Z-Score:

```python
import json
import time
import paho.mqtt.client as mqtt
import pandas as pd
import matplotlib.pyplot as plt

BROKER = "broker.hivemq.com"
PORT = 1883
TOPIC = "unsoed/tk245004/kelompok1/data"
JUMLAH_SAMPEL = 30

data = []

def on_connect(client, userdata, flags, reason_code, properties):
    print("Terhubung ke broker, kode:", reason_code)
    client.subscribe(TOPIC)

def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode())
        suhu = float(payload["suhu"])
    except (ValueError, KeyError):
        print("Data tidak valid, diabaikan")
        return

    data.append({"waktu": time.strftime("%Y-%m-%d %H:%M:%S"), "suhu": suhu})
    print(f"[{len(data)}/{JUMLAH_SAMPEL}] suhu = {suhu}")

    if len(data) >= JUMLAH_SAMPEL:
        client.disconnect()

# Tahap 1: Akuisisi Data melalui MQTT
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, PORT)
client.loop_forever()

# Tahap 2: Pengolahan Dataframe
df = pd.DataFrame(data)

# ============================================================================
# MODIFIKASI TUGAS 4: Penerapan Moving Average Jendela 5 Sebelum Z-Score
# ============================================================================
# Menggunakan fungsi rolling(window=5, min_periods=1) dari Pandas 
# untuk meredam noise acak pada pembacaan suhu mentah.
df["suhu_ma"] = df["suhu"].rolling(window=5, min_periods=1).mean()

# Tahap 3: Hitung Z-Score berbasis Data Hasil Filter Moving Average
rata2 = df["suhu_ma"].mean()
std = df["suhu_ma"].std()

if std == 0:
    df["zscore"] = 0.0
else:
    # Memakai kolom suhu_ma (bukan suhu mentah) untuk kalkulasi Z-Score
    df["zscore"] = ((df["suhu_ma"] - rata2) / std).round(4)

# Penentuan Anomali dengan ambang batas |Z| > 2
df["anomali"] = df["zscore"].abs() > 2
df["status_anomali"] = df["anomali"].apply(lambda x: "Anomali" if x else "Normal")

# Simpan Ke File CSV
df.to_csv("data_suhu_ma_zscore.csv", index=False)
print("Data berhasil disimpan ke data_suhu_ma_zscore.csv")

# Ringkasan Statistik
print(f"\n--- Ringkasan Statistik ---")
print(f"Rata-rata (MA)  : {rata2:.2f} °C")
print(f"Std Deviasi (MA): {std:.2f}")
print(f"Jumlah Anomali  : {df['anomali'].sum()}\n")

# Tahap 4: Visualisasi Data
plt.figure(figsize=(10, 5))
plt.plot(df.index, df["suhu"], marker="o", color="lightgray", linestyle=":", label="Suhu Mentah")
plt.plot(df.index, df["suhu_ma"], marker="s", color="blue", label="Suhu MA (Window=5)")

anom = df[df["anomali"]]
plt.scatter(anom.index, anom["suhu_ma"], color="red", s=100, label="Anomali (pada MA)", zorder=5)
plt.axhline(rata2, color="gray", linestyle="--", label="Rata-rata MA")

plt.xlabel("Urutan Data")
plt.ylabel("Suhu (°C)")
plt.title("Analisis Suhu dengan Filter Moving Average & Deteksi Anomali Z-Score")
plt.legend()
plt.tight_layout()
plt.savefig("grafik_suhu_ma_zscore.png")
plt.show()
```

### Analisis Perbandingan Jumlah Anomali:
Penerapan filter *Moving Average* sebelum kalkulasi Z-Score secara dramatis **mengurangi jumlah anomali yang terdeteksi**. Hal ini terjadi karena fluktuasi acak yang sifatnya sementara (*transient noise*) dihaluskan terlebih dahulu oleh *windowing*, sehingga Z-score fokus pada anomali yang bernilai ekstrem dan berlangsung secara konsisten.

---

## Pertanyaan Analisis

### 1. Uraian Hasil Setiap Percobaan
1. **Percobaan 5A (Edge Processing):** ESP32 berhasil membaca nilai suhu dari sensor DHT22/Mock Sensor, menerapkan algoritma *Moving Average* berbasis *circular buffer*, serta menghitung parameter batas minimum dan maksimum suhu secara internal (*on-device*).
2. **Percobaan 5B (Server Processing & Analysis):** Skrip Python berhasil menerima aliran data real-time via MQTT broker, memparsing format JSON, menyimpan rekaman ke berkas `data_suhu.csv`, melakukan analisis deskriptif statistik, mengkalkulasi deteksi anomali berbasis *Z-Score*, serta menyajikan grafik visualisasi (*matplotlib*).

---

### 2. Perbandingan Edge Processing vs Server Processing

| Parameter Evaluasi | Edge Processing (Percobaan 5A) | Server Processing (Percobaan 5B) |
| :--- | :--- | :--- |
| **Kecepatan Respons** | Sangat cepat / *Real-time* (tidak bergantung latensi jaringan). | Relatif lebih lambat (terdapat latensi pengiriman MQTT). |
| **Kompleksitas Analisis** | Rendah (terbatas pada aritmatika dasar & filter sederhana). | Sangat Tinggi (statistik kompleks, machine learning, plotting). |
| **Kebutuhan Sumber Daya** | Sangat kecil (RAM/CPU mikrokontroler terbatas). | Besar (RAM/CPU komputer/server, kapasitas storage besar). |
| **Penggunaan Bandwidth** | Hemat (dapat menyaring data sebelum dikirim). | Tinggi (seluruh deret data mentah dikirim ke broker). |

---

### 3. Pengaruh Ukuran Jendela Moving Average terhadap Aktuator
Pemilihan ukuran jendela ($N$) sangat krusial jika keluaran data digunakan langsung sebagai masukan sistem kontrol otomatis (seperti *heater* atau *exhaust fan*):
* **Jendela Terlalu Besar ($N$ besar):** Meredam noise dengan sempurna, namun memberikan efek keterlambatan (*lagging*). Aktuator akan terlambat menyala saat terjadi lonjakan suhu yang ekstrem.
* **Jendela Terlalu Kecil ($N$ kecil):** Respons cepat, namun sinyal kurang stabil. Derau kecil acak dapat menyebabkan aktuator mengalami *chattering* (sakelar relay menyala-mati terlalu sering secara mendadak), yang memperpendek umur komponen fisik aktuator.

---

### 4. Pemanfaatan Analisis & Deteksi Anomali pada Sistem IoT Real-World
* **Smart Home:** Deteksi Z-score pada lonjakan suhu abnormal yang terisolasi di suatu ruangan dapat menjadi penanda awal terjadinya korsleting listrik atau bahaya kebakaran sebelum timbul api besar, sehingga sistem dapat otomatis mematikan daya utama (*circuit breaker*).
* **Smart Farming:** Pada rumah kaca (*greenhouse*), pembacaan penurunan suhu atau kelembapan yang tidak lazim dapat menandakan rusaknya sistem irigasi atau kebocoran ventilasi. Sistem dapat memberikan alarm peringatan (*early warning system*) secara otomatis ke pengelola.
