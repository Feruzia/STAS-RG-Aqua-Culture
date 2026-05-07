# 🌊 Firmware Kalibrasi Sensor - STAS-RG Aqua-Culture

Folder ini berisi *source code* murni (`.ino`) yang dikhususkan untuk proses **kalibrasi independen** masing-masing sensor kualitas air. Kodingan di dalam folder ini berfokus pada pembacaan sensor mentah (RAW ADC), konversi voltase, dan kalkulasi parameter tanpa adanya integrasi ke LCD, modul SD Card, maupun pengiriman data ke Web/Server. 

Seluruh keluaran (*output*) pembacaan akan ditampilkan melalui **Serial Monitor** (Baudrate: 115200) dalam format *single-line* yang rapi untuk mempermudah ekspor data ke format *spreadsheet*.

---

## 🛠️ Spesifikasi Perangkat Keras Utama

Sistem ini dirancang menggunakan arsitektur mikrokontroler dan ADC eksternal dengan spesifikasi sebagai berikut:

* **Microcontroller:** Heltec WiFi LoRa 32 (V3) - *Non-OLED Version* (Berbasis ESP32-S3, Logika 3.3V).
* **ADC Eksternal:** 2x Texas Instruments ADS1115 (16-bit ADC, Antarmuka I2C).
* **Logic Level Converter:** Bidirectional I2C Level Shifter (3.3V <-> 5V).

### ❓ Mengapa Menggunakan ADS1115 dan Level Shifter? (vs. Internal ADC ESP32)

1. **Akurasi ADC Internal ESP32 (Non-Linear):** ADC internal pada ESP32 (meskipun 12-bit) memiliki kelemahan bawaan berupa kurva pembacaan yang tidak linier (melengkung di ujung bawah dan atas) dan sangat rentan terhadap *noise* frekuensi radio (terutama saat WiFi/LoRa aktif). Pembacaan tegangan analog sensor akan melompat-lompat (*fluctuating*).
2. **Presisi Tinggi ADS1115 (16-bit):** Modul ADS1115 menawarkan resolusi 16-bit yang jauh lebih presisi dan stabil. Dengan menggunakan *Programmable Gain Amplifier* (PGA) bawaan ADS1115, kita dapat mengunci rentang tegangan pembacaan (misal `Gain 1` untuk +/- 4.096V), sehingga setiap perubahan mikrovolt dari sensor dapat terbaca dengan sangat akurat.
3. **Fungsi Logic Level Shifter:** Mikrokontroler Heltec V3 beroperasi murni pada tegangan logika **3.3V**. Di sisi lain, sensor analog DFRobot dan modul ADS1115 disuplai menggunakan tegangan **5V** untuk memaksimalkan rentang pembacaan sensor. Level Shifter wajib digunakan pada jalur I2C (SDA & SCL) untuk menjembatani komunikasi data 5V dari ADS1115 agar turun menjadi 3.3V, sehingga tidak merusak pin ESP32.

---

## 🔌 Konfigurasi Pinout & I2C Address

Sistem menggunakan pin I2C kustom pada ESP32-S3 (SDA: 41, SCL: 42) yang dihubungkan ke dua buah modul ADS1115 dan satu pin OneWire.

* **Sensor Suhu (Digital OneWire):** Pin `4` (Langsung ke ESP32)

| Modul ADC | Alamat I2C | Pin Analog | Digunakan Untuk Sensor | Gain Konfigurasi |
| :--- | :--- | :--- | :--- | :--- |
| **ADS_A** | `0x48` | `A0` | Turbidity | `Gain 0` (+/- 6.144V) |
| **ADS_A** | `0x48` | `A1` | TDS Standalone | `Gain 1` (+/- 4.096V) |
| **ADS_A** | `0x48` | `A2` | pH | `Gain 1` (+/- 4.096V) |
| **ADS_B** | `0x49` | `A0` | DO (Dissolved Oxygen) | `Gain 1` (+/- 4.096V) |
| **ADS_B** | `0x49
