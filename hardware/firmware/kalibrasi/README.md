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
| **ADS_B** | `0x49` | `A1` | EC / TDS dari EC | `Gain 1` (+/- 4.096V) |
| **ADS_B** | `0x49` | `A2` | ORP | `Gain 1` (+/- 4.096V) |

---

## 🧪 Daftar Sensor & Referensi (DFRobot Gravity Series)

Seluruh sensor *water quality* pada riset ini menggunakan ekosistem **Gravity** dari DFRobot yang dirancang khusus untuk pengujian laboratorium dan akuakultur.

1. **Suhu (DS18B20)**
   * **Tipe:** DFRobot Waterproof DS18B20 Sensor Kit (SEN0198)
   * **Deskripsi:** Sensor suhu digital murni dengan resolusi hingga 12-bit. Tidak memerlukan kalibrasi tegangan karena langsung mentransmisikan paket data suhu.
   * **Dokumentasi:** [DFRobot SEN0198 Wiki](https://wiki.dfrobot.com/Waterproof_DS18B20_Digital_Temperature_Sensor__SKU_DFR0198_)
2. **Turbidity (Kekeruhan Air)**
   * **Tipe:** DFRobot Analog Turbidity Sensor (SEN0189)
   * **Deskripsi:** Mengukur tingkat hamburan cahaya di dalam air (satuannya NTU). Perlu penyesuaian nilai voltase air jernih absolut (`TURB_REAL_V`) untuk lingkungan lokal.
   * **Dokumentasi:** [DFRobot SEN0189 Wiki](https://wiki.dfrobot.com/Turbidity_sensor_SKU__SEN0189)
3. **pH (Tingkat Keasaman)**
   * **Tipe:** DFRobot Analog pH Sensor Pro / V2 (SEN0161 / SEN0169)
   * **Deskripsi:** Menggunakan elektroda kaca untuk mendeteksi ion Hidrogen. Memerlukan prosedur *3-Point Calibration* (Buffer pH 4.0, 7.0, dan 10.0). Rangkaian penguat bersifat *Inverting* (tegangan naik = semakin asam).
   * **Dokumentasi:** [DFRobot SEN0161 Wiki](https://wiki.dfrobot.com/PH_meter_SKU__SEN0161_)
4. **Dissolved Oxygen (DO)**
   * **Tipe:** DFRobot Analog Dissolved Oxygen Sensor (SEN0237-A)
   * **Deskripsi:** Sensor tipe Galvanic probe untuk mengukur oksigen terlarut. Kalibrasi cukup dilakukan di udara terbuka (100% saturasi) dengan menyesuaikan nilai `CAL1_V_DO`. **Syarat penggunaan:** Air harus bergerak/diaduk saat pengukuran.
   * **Dokumentasi:** [DFRobot SEN0237-A Wiki](https://wiki.dfrobot.com/Gravity__Analog_Dissolved_Oxygen_Sensor_SKU_SEN0237)
5. **Electrical Conductivity (EC)**
   * **Tipe:** DFRobot Analog EC Meter (DFR0300)
   * **Deskripsi:** Mengukur konduktivitas listrik untuk mengetahui salinitas/kandungan mineral. Dilengkapi konstanta sel (`K_VALUE`) yang harus disesuaikan menggunakan larutan kalibrasi standar industri (misal: 1413 µS/cm).
   * **Dokumentasi:** [DFRobot DFR0300 Wiki](https://wiki.dfrobot.com/Analog_EC_Meter_SKU_DFR0300)
6. **TDS Standalone (Total Dissolved Solids)**
   * **Tipe:** DFRobot Analog TDS Sensor (SEN0244)
   * **Deskripsi:** Secara spesifik dirancang untuk membaca total padatan terlarut (ppm). Memiliki konversi internal yang sedikit berbeda dengan modul EC murni.
   * **Dokumentasi:** [DFRobot SEN0244 Wiki](https://wiki.dfrobot.com/Gravity__Analog_TDS_Sensor___Meter_For_Arduino_SKU__SEN0244)
7. **ORP (Oxidation-Reduction Potential)**
   * **Tipe:** DFRobot Analog ORP Meter (SEN0165)
   * **Deskripsi:** Mengukur potensi oksidasi dan reduksi air (dalam mV). **Wajib** melakukan prosedur *Zero Calibration* (men-short/menghubung singkat konektor BNC) untuk mendapatkan nilai `ORP_OFFSET`.
   * **Dokumentasi:** [DFRobot SEN0165 Wiki](https://wiki.dfrobot.com/Analog_ORP_Meter_SKU_SEN0165_)

---

## 📌 Prasyarat & Persiapan (Wajib Dibaca Sebelum Mulai)

### 1. Kebutuhan Library (Arduino IDE)
Pastikan library berikut sudah terinstal via *Library Manager* sebelum melakukan proses *compile/upload*:
* `ADS1X15` oleh Rob Tillaart (Untuk ADC ADS1115)
* `OneWire` oleh Paul Stoffregen (Protokol komunikasi suhu)
* `DallasTemperature` oleh Miles Burton (Kalkulasi suhu DS18B20)

### 2. Standar Operasional Prosedur (SOP) Kalibrasi Sensor
Untuk mendapatkan dataset yang valid untuk *Machine Learning* / Riset Akademik, terapkan langkah berikut:
1. **Pemanasan (Warm-up):** Biarkan sensor tercelup ke dalam air (atau menyala di udara bebas untuk DO) selama minimal **5-10 menit** sebelum mengambil nilai voltase kalibrasi. Elektroda analog membutuhkan waktu agar reaksi ionnya stabil.
2. **Pembersihan Probe:** Sediakan botol *spray* berisi **Aquades (Air Suling/Air Demineral)**. Selalu semprot dan bersihkan kepala *probe* setiap kali memindahkan sensor dari satu larutan kalibrasi ke larutan kalibrasi lainnya (misal dari Buffer pH 4 ke Buffer pH 7) untuk mencegah kontaminasi silang. Jangan mengelap *bulb* kaca pH dengan tisu secara kasar, cukup tepuk-tepuk ringan.
3. **Pengadukan (Agitasi):** Saat melakukan pengujian sampel, goyangkan *probe* secara perlahan di dalam air (terutama wajib untuk sensor DO) untuk mencegah penumpukan area statis di ujung membran sensor.
