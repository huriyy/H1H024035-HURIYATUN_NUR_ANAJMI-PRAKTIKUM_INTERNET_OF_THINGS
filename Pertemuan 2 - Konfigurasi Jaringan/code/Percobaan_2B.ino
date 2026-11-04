#include <ESP8266WiFi.h>
const char* ap_ssid = "ESP32_AccessPoint";
const char* ap_password = "12345678"; // minimal 8 karakter


void setup() {
 Serial.begin(115200);
// Timestamp dalam detik
  unsigned long timestamp = millis() / 1000;


  Serial.print("Timestamp : ");
  Serial.print(timestamp);
  Serial.println(" s");


 // Set mode WiFi menjadi Access Point
 WiFi.mode(WIFI_AP);
 WiFi.softAP(ap_ssid, ap_password);
 IPAddress apIP = WiFi.softAPIP();
 Serial.println("Access Point aktif!");
 Serial.print("SSID : ");
 Serial.println(ap_ssid);
 Serial.print("IP Address : ");
 Serial.println(apIP);


 
}
void loop() {
 // Menampilkan jumlah perangkat yang terhubung setiap 5 detik
 int jumlahClient = WiFi.softAPgetStationNum();
 Serial.print("Jumlah perangkat terhubung: ");
 Serial.println(jumlahClient);
 delay(5000);
}

