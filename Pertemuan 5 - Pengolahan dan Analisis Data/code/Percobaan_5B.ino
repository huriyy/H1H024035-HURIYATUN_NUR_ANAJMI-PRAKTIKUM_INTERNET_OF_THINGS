import json
import time
import paho.mqtt.client as mqtt
import pandas as pd
import matplotlib.pyplot as plt


BROKER = "broker.hivemq.com"
PORT = 1883
TOPIC = "unsoed/tk245004/kelompok1/data"  # Sesuaikan dengan topicData ESP32
JUMLAH_SAMPEL = 20                        # Jumlah data yang dikumpulkan


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


    data.append({
        "waktu": time.strftime("%Y-%m-%d %H:%M:%S"),
        "suhu": suhu
    })
    print(f"[{len(data)}/{JUMLAH_SAMPEL}] suhu = {suhu}")


    if len(data) >= JUMLAH_SAMPEL:
        client.disconnect()


# --- Tahap 1: Akuisisi data dari MQTT ---
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, PORT)
client.loop_forever()


# --- Tahap 2: Pengolahan DataFrame & Analisis Z-Score ---
df = pd.DataFrame(data)


rata2 = df["suhu"].mean()
std = df["suhu"].std()


# Kondisional perhitungan Z-Score
if std == 0:
    df["zscore"] = 0.0
else:
    df["zscore"] = ((df["suhu"] - rata2) / std).round(4)


# Kondisional Status Anomali (ambang batas z-score > 2 atau < -2)
df["anomali"] = df["zscore"].abs() > 2
df["status_anomali"] = df["anomali"].apply(lambda x: "Anomali" if x else "Normal")


# --- Tahap 3: Simpan ke File CSV & Status Penyimpanan ---
try:
    # Kondisional status penyimpanan CSV
    df["status_penyimpanan_csv"] = "Tersimpan"
    kolom_tabel = ["waktu", "suhu", "zscore", "status_anomali", "status_penyimpanan_csv"]
    df.to_csv("data_suhu.csv", columns=kolom_tabel, index=False)
    simpan_sukses = True
except Exception as e:
    df["status_penyimpanan_csv"] = f"Gagal ({e})"
    simpan_sukses = False


if simpan_sukses:
    print("\n✓ Status: Seluruh data pengamatan (20 data) berhasil disimpan ke 'data_suhu.csv'")
else:
    print("\n✗ Status: Terjadi kesalahan saat menyimpan ke CSV")


# Ringkasan Statistik
print(f"\n--- Ringkasan Statistik ---")
print(f"Rata-rata     : {rata2:.2f} °C")
print(f"Std deviasi   : {std:.2f}")
print(f"Minimum       : {df['suhu'].min():.2f} °C")
print(f"Maksimum      : {df['suhu'].max():.2f} °C")
print(f"Total Sampel  : {len(df)}")
print(f"Jumlah Anomali: {df['anomali'].sum()}\n")


# --- Tahap 4: Visualisasi data ---
plt.figure(figsize=(10, 4))
plt.plot(df.index, df["suhu"], marker="o", label="Suhu")


anom = df[df["anomali"]]
plt.scatter(anom.index, anom["suhu"], color="red", s=100, label="Anomali", zorder=3)


plt.axhline(rata2, color="gray", linestyle="--", label="Rata-rata")
plt.xlabel("Urutan data")
plt.ylabel("Suhu (°C)")
plt.title("Data Suhu dan Deteksi Anomali")
plt.legend()
plt.tight_layout()
plt.savefig("grafik_suhu.png")
plt.show()

