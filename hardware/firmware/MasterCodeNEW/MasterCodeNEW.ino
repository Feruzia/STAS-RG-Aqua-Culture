#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "ADS1X15.h"
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"

// ==========================================
// 1. KONFIGURASI WIFI & SERVER
// ==========================================
const char* ssid = "inode-A002";
const char* password = "stasrg2024";
// const char* ssid = "hotspott";
// const char* password = "password";
const char* serverName = "https://dev-aquaculture.stas-rg.com/api/sensor-data";
const char* device_code = "DEV-9TQZXJVL";

// ==========================================
// 2. DEFINISI PIN & OBJEK
// ==========================================
#define I2C_SDA 41
#define I2C_SCL 42
#define SUHU_PIN 4
#define LED_PIN 20
#define SD_MOSI 5
#define SD_MISO 6
#define SD_SCK 7
#define SD_CS 15

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);
ADS1115 adsA(0x48);  // A0: Turb, A1: TDS Standalone, A2: pH
ADS1115 adsB(0x49);  // A0: DO, A1: EC (inc TDSe), A2: ORP
OneWire oneWire(SUHU_PIN);
DallasTemperature sensors(&oneWire);
//------------------------------
// ==========================================
// 3. DATA KALIBRASI (REFERENSI MASTER TERAKHIR)
// ==========================================
// --- pH ---
const float neutralVoltage = 1.550;
const float acidVoltage = 2.500;
const float baseVoltage = 1.150;
const float phSlope = (abs(neutralVoltage - acidVoltage) / 3.0 + abs(neutralVoltage - baseVoltage) / 3.0) / 2.0;

// --- Turbidity ---
const float TURB_CLEAR_V_REF = 4.336;
const float TURB_REAL_V = 3.80; 
const float TURB_SENSITIVITY = 18.0;

// --- DO, EC, ORP ---
const float CAL1_V_DO = 1.000;
const float K_VALUE_EC = 4.64;
const float ORP_OFFSET = 1.4;  // Offset yang sudah disesuaikan
bool sdStatus = false; // Variabel untuk menyimpan status SD Card

// ==========================================
// 4. VARIABEL GLOBAL (VALUE & VOLT)
// ==========================================
// Semua parameter punya pasangan Volt-nya untuk mempermudah monitoring
float suhuC;
float phVal, phVolt;
float doVal, doVolt;
float ecVal, ecVolt;
float tdsEC, tdsECVolt;       // TDS hasil perhitungan modul EC
float tdsStdVal, tdsStdVolt;  // TDS dari modul standalone
float ntuVal, turbVolt;
float orpVal, orpVolt;

unsigned long lastLCDUpdate = 0;
unsigned long lastWebSend = 0;
unsigned long lastLogTime = 0;
const unsigned long logInterval = 15000; // Interval 15 detik
unsigned long lastSensorRead = 0;
int page = 0;


// ================================================================
// 5. FUNGSI FILTER DIGITAL (TRIMMED MEAN)
// ================================================================
// Fungsi ini mengambil 40 data, membuang nilai paling kacau (Min & Max),
// baru kemudian merata-ratakan sisanya. Ini kunci agar EC/TDS tidak 0.
double getAvgADC_Safe(ADS1115& ads, int pin) {
  int samples = 40;
  int arr[40];
  long amount = 0;

  for (int i = 0; i < samples; i++) {
    arr[i] = ads.readADC(pin);
    delay(2);  // Delay singkat untuk stabilitas pembacaan ADS
  }

  // Cari nilai minimum dan maksimum
  int maxVal = arr[0];
  int minVal = arr[0];
  for (int i = 1; i < samples; i++) {
    if (arr[i] > maxVal) maxVal = arr[i];
    if (arr[i] < minVal) minVal = arr[i];
    amount += arr[i];
  }

  // Buang satu nilai max dan satu nilai min dari total
  amount = amount - maxVal - minVal;

  // Rata-rata dari 38 data sisa
  return (double)amount / (samples - 2);
}

// ================================================================
// 6. FUNGSI KONEKSI WIFI
// ================================================================
void connectWiFi() {
  Serial.println("\n[SYSTEM] Menghubungkan ke WiFi...");
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connecting ");

  WiFi.begin(ssid, password);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));  // Blink LED saat mencari sinyal
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[SYSTEM] WiFi Terhubung!");
    Serial.print("[SYSTEM] IP Address: ");
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.print("STAS-RG ONLINE");
  } else {
    Serial.println("\n[SYSTEM] WiFi Gagal Konek (Timeout)");
    lcd.clear();
    lcd.print("WiFi Offline    ");
  }
}

// ================================================================
// 7. SETUP UTAMA
// ================================================================
void setup() {
  // 1. Inisialisasi Dasar
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // 2. Inisialisasi I2C (Harus paling awal sebelum modul I2C lain dipanggil)
  Wire.begin(I2C_SDA, I2C_SCL);

  // 3. Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Starting");
  delay(500);

  // 4. Inisialisasi Sensor & ADC
  sensors.begin(); // Sensor Suhu DS18B20
  adsA.begin();    // ADS1115 Pertama
  adsB.begin();    // ADS1115 Kedua

  // 5. Inisialisasi RTC
  if (!rtc.begin(&Wire)) {
    Serial.println("[ERROR] RTC Tidak Terdeteksi!");
    lcd.setCursor(0, 1); 
    lcd.print("RTC Error!      ");
  }

  // 6. Inisialisasi SD Card & File Dataset
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS); 
  if (!SD.begin(SD_CS)) {
    Serial.println("[ERROR] SD Card Tidak Terdeteksi!");
    lcd.setCursor(0, 1); 
    lcd.print("SD Card Error!  ");
    sdStatus = false; // <--- TAMBAHKAN INI JIKA GAGAL
  } else {
    Serial.println("[OK] SD Card Siap.");
    sdStatus = true; // <--- TAMBAHKAN INI JIKA BERHASIL
    // CEK FILE: Jika BELUM ada, buat baru + Header. Jika SUDAH ada, biarkan saja.
    if (!SD.exists("/lastest_dataset.csv")) {
      File dataFile = SD.open("/lastest_dataset.csv", FILE_WRITE); 
      if (dataFile) {
        dataFile.println("Tanggal,Waktu,Tipe,Suhu_C,pH,DO_mgL,Turb_NTU,EC_uS,TDSe_ppm,TDSs_ppm,ORP_mV");
        dataFile.close();
        Serial.println("[OK] File baru dibuat dengan header.");
      }
    } else {
      Serial.println("[OK] File sudah ada, data akan diteruskan.");
    }
  }

  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi.");

  // 7. Jalankan Koneksi WiFi
  //connectWiFi();

  delay(1500);
  lcd.clear();
}

// ================================================================
// 8. FUNGSI PEMBACAAN 7 SENSOR (ONE SENSOR, ONE FUNCTION)
// ================================================================

// [1] SENSOR SUHU (DS18B20)
void readSuhu() {
  sensors.requestTemperatures();
  suhuC = sensors.getTempCByIndex(0);
  if (suhuC < -50) suhuC = 25.0;  // Fail-safe jika kabel lepas
}

// [2] SENSOR TURBIDITY (ADS_A Pin A0)
void readTurbidity() {
  adsA.setGain(0); 
  delay(10);
  long sum = 0;
  for(int i = 0; i < 30; i++) { sum += adsA.readADC(0); delay(5); }
  turbVolt = (sum / 30.0) * 0.0001875;
  
  float vCalc = turbVolt + (TURB_CLEAR_V_REF - TURB_REAL_V);
  
  // PAGAR DIPERLEBAR: Dari 4.30 diturunkan ke 4.20
  // Agar sedikit debu/keruh halus tidak langsung dihitung tinggi
  if (vCalc >= 4.20) {
    ntuVal = 0.0;
  } 
  else if (turbVolt >= 3.20 && turbVolt <= 3.40) { 
    ntuVal = 0.0; 
  } 
  else {
    ntuVal = (TURB_CLEAR_V_REF - vCalc) * TURB_SENSITIVITY; 
  }
  
  if (ntuVal < 0) ntuVal = 0;
}

// [3] SENSOR PH (ADS_A Pin A2)
void readPH() {
  adsA.setGain(1);  // FIX: 1 = Gain +/- 4.096V
  delay(20);
  phVolt = getAvgADC_Safe(adsA, 2) * 0.000125;

  // Kompensasi Suhu: Slope pH berubah 0.003 unit per derajat dari 25C
  float compensationFactor = 0.03 * (suhuC - 25.0);

  if (phVolt > neutralVoltage) {
    phVal = 7.0 - ((phVolt - neutralVoltage) / phSlope);
  } else {
    phVal = 7.0 + ((neutralVoltage - phVolt) / phSlope);
  }

  phVal += compensationFactor;
  if (phVal < 0) phVal = 0;
  if (phVal > 14) phVal = 14;
}

// [4] SENSOR TDS STANDALONE (ADS_A Pin A1)
void readTDS_Standalone() {
  adsA.setGain(1);  // FIX: 1 = Gain +/- 4.096V
  delay(10);
  tdsStdVolt = getAvgADC_Safe(adsA, 1) * 0.000125;

  float comp = 1.0 + 0.02 * (suhuC - 25.0);
  float vFinal = tdsStdVolt / comp;
  tdsStdVal = (133.42 * pow(vFinal, 3) - 255.86 * pow(vFinal, 2) + 857.39 * vFinal) * 0.5;
}

// [5] SENSOR DO (ADS_B Pin A0)
void readDO() {
  adsB.setGain(1);  // FIX: 1 = Gain +/- 4.096V
  delay(20);
  doVolt = getAvgADC_Safe(adsB, 0) * 0.000125;

  // Tabel sederhana saturasi oksigen berdasarkan suhu (mg/L)
  float doSaturation;
  if (suhuC < 22) doSaturation = 8.7;
  else if (suhuC < 27) doSaturation = 8.2;
  else if (suhuC < 31) doSaturation = 7.6;
  else doSaturation = 7.1;

  doVal = (doVolt / CAL1_V_DO) * doSaturation;
}

// [6] SENSOR EC & TDS DARI EC (ADS_B Pin A1)
void readEC() {
  adsB.setGain(1);  // FIX: 1 = Gain +/- 4.096V
  delay(10);
  ecVolt = getAvgADC_Safe(adsB, 1) * 0.000125;
  tdsECVolt = ecVolt;  // Voltase sama karena satu jalur

  float comp = 1.0 + 0.02 * (suhuC - 25.0);
  float vFinal = ecVolt / comp;

  // Rumus TDS dari modul EC
  tdsEC = (133.42 * pow(vFinal, 3) - 255.86 * pow(vFinal, 2) + 857.39 * vFinal) * 0.5 * K_VALUE_EC;
  ecVal = tdsEC * 2.0;  // Konversi TDS ke EC
}

// [7] SENSOR ORP (ADS_B Pin A2 - MENGGUNAKAN MODUL PH)
void readORP() {
  adsB.setGain(1);  // FIX: 1 = Gain +/- 4.096V
  delay(10);
  orpVolt = getAvgADC_Safe(adsB, 2) * 0.000125;

  // FIXED: Rumus dibalik karena modul pH bersifat Inverting untuk ORP
  // (ORP_OFFSET - orpVolt) membuat nilai naik saat tegangan turun
  orpVal = (orpVolt - ORP_OFFSET) * 1000.0;
}

// ================================================================
// 9. SERIAL MONITORING (ONE FUNCTION)
// ================================================================
void serialMonitoring() {
  // Menampilkan semua parameter dalam satu baris, ditambah voltase khusus Turbidity
  Serial.printf("Suhu:%.1f | pH:%.2f | DO:%.2f | Turb:%.1f(%.2fV) | EC:%.0f | TDSe:%.0f | TDSs:%.0f | ORP:%.0f\n", 
                suhuC, phVal, doVal, ntuVal, turbVolt, ecVal, tdsEC, tdsStdVal, orpVal); 
}

// ================================================================
// 10. LCD PAGING SYSTEM (VALUE & VOLT)
// ================================================================
void updateLCD() {
  lcd.clear();
  switch (page) {
    case 0:  // Page 1: Suhu & pH
      lcd.print("S:");
      lcd.print(suhuC, 1);
      lcd.print(" pH:");
      lcd.print(phVal, 1);
      lcd.setCursor(0, 1);
      lcd.print("VpH:");
      lcd.print(phVolt, 3);
      page = 1;
      break;

    case 1:  // Page 2: DO & Turbidity
      lcd.print("DO:");
      lcd.print(doVal, 1);
      lcd.print(" T:");
      lcd.print(ntuVal, 0);
      lcd.setCursor(0, 1);
      lcd.print("VDO:");
      lcd.print(doVolt, 2);
      lcd.print(" VT:");
      lcd.print(turbVolt, 2);
      page = 2;
      break;

    case 2:  // Page 3: EC & TDS dari EC (TDSe)
      lcd.print("EC:");
      lcd.print(ecVal, 0);
      lcd.print(" uS");
      lcd.setCursor(0, 1);
      lcd.print("TDSe:");
      lcd.print(tdsEC, 0);
      lcd.print(" V:");
      lcd.print(ecVolt, 2);
      page = 3;
      break;

    case 3:  // Page 4: TDS Standalone (TDSs) & ORP
      lcd.print("TDSs:");
      lcd.print(tdsStdVal, 0);
      lcd.setCursor(0, 1);
      lcd.print("ORP:");
      lcd.print(orpVal, 0);
      lcd.print(" V:");
      lcd.print(orpVolt, 2);
      // Di sini kita tampilkan Voltase TDS Standalone (tdsStdVolt)
      lcd.setCursor(11, 0);
      lcd.print("V:");
      lcd.print(tdsStdVolt, 1);
      page = 4;
      break;

case 4:  // Page 5: Status WiFi & SD Card
      lcd.print("WiFi: ");
      lcd.print(WiFi.status() == WL_CONNECTED ? "ON" : "OFF");
      lcd.setCursor(0, 1);
      lcd.print("SD Card: ");
      lcd.print(sdStatus ? "OK" : "ERR"); // <--- Manggil variabel di sini
      page = 0; 
      break;
  }
}

// ===========================================================
// 11. FUNGSI KIRIM DATA KE WEB LARAVEL
// ===========================================================oke
void sendToLaravel() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.begin(client, serverName);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "ESP32-STAS-RG");

    String payload = "{\"device_code\":\"" + String(device_code) + "\",";
    payload += "\"suhuDHT\":0.00,";
    payload += "\"suhu\":" + String(suhuC, 2) + ",";
    payload += "\"ph\":" + String(phVal, 2) + ",";
    payload += "\"do\":" + String(doVal, 2) + ",";
    payload += "\"turbidity_ntu\":" + String(ntuVal, 2) + ",";
    payload += "\"ec_s_m\":" + String(ecVal, 0) + ",";

    // SEKARANG: Mengirim TDS dari modul Standalone ke Laravel
    payload += "\"tds_ppm\":" + String(tdsStdVal, 0) + ",";

    payload += "\"orp_mv\":" + String(orpVal, 2) + ",";
    payload += "\"risiko\":0.00}";

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      Serial.printf("[WEB] Berhasil! Respon: %d\n", httpResponseCode);
    } else {
      Serial.printf("[WEB] Error! Kode: %d\n", httpResponseCode);
    }
    http.end();
  }
}

// ================================================================
// 11.5 FUNGSI DATA LOGGER SD CARD (ML DATASET FORMAT)
// ================================================================
void logToSD_Full() {
  // PASTIKAN NAMA FILE SAMA DENGAN DI VOID SETUP!
  File dataFile = SD.open("/dataset_toksisitas.csv", FILE_APPEND);

  if (dataFile) {
    DateTime now = rtc.now();

    // Siapkan String Tanggal & Waktu
    char dateStr[12];
    sprintf(dateStr, "%04d/%02d/%02d", now.year(), now.month(), now.day());

    char timeStr[10];
    sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    // ---------------------------------------------------------
    // BARIS 1: Menyimpan Nilai Aktual (VALUE)
    // ---------------------------------------------------------
    dataFile.printf("%s,%s,VALUE,", dateStr, timeStr);
    dataFile.print(suhuC, 2);     dataFile.print(","); // Suhu (2 desimal)
    dataFile.print(phVal, 2);     dataFile.print(","); // pH (2 desimal)
    dataFile.print(doVal, 2);     dataFile.print(","); // DO (2 desimal)
    dataFile.print(ntuVal, 2);    dataFile.print(","); // Turbidity (2 desimal)
    dataFile.print(ecVal, 0);     dataFile.print(","); // EC (tanpa desimal)
    dataFile.print(tdsEC, 0);     dataFile.print(","); // TDS dari EC (tanpa desimal)
    dataFile.print(tdsStdVal, 0); dataFile.print(","); // TDS Standalone (tanpa desimal)
    dataFile.println(orpVal, 0);                       // ORP (Diakhiri println untuk enter)

    // ---------------------------------------------------------
    // BARIS 2: Menyimpan Nilai Tegangan ADC (VOLT)
    // ---------------------------------------------------------
    dataFile.printf("%s,%s,VOLT,", dateStr, timeStr);
    dataFile.print("0.00,");                            // Suhu DS18B20 digital (tanpa voltase ADC)
    dataFile.print(phVolt, 3);     dataFile.print(","); // Voltase pH (3 desimal)
    dataFile.print(doVolt, 3);     dataFile.print(","); // Voltase DO (3 desimal)
    dataFile.print(turbVolt, 3);   dataFile.print(","); // Voltase Turbidity (3 desimal)
    dataFile.print(ecVolt, 3);     dataFile.print(","); // Voltase EC (3 desimal)
    dataFile.print(ecVolt, 3);     dataFile.print(","); // Voltase TDS dari EC disamakan
    dataFile.print(tdsStdVolt, 3); dataFile.print(","); // Voltase TDS Standalone (3 desimal)
    dataFile.println(orpVolt, 3);                       // Voltase ORP (Diakhiri println)

    dataFile.close();
    Serial.println("[SD CARD] 2 Baris (Value & Volt) berhasil masuk ke dataset_toksisitas.csv!");
  } else {
    Serial.println("[SD CARD] Error: Gagal membuka dataset_toksisitas.csv");
  }
}

// ======================================================
// 12. MAIN LOOP
// ======================================================
void loop() {
  // 1. Baca Sensor (Tiap 1 Detik biar pembacaan ADC stabil & tidak loncat)
  if (millis() - lastSensorRead >= 1000) {
    readSuhu();
    readTurbidity();
    readPH();
    readTDS_Standalone();
    readDO();
    readEC();
    readORP();
    lastSensorRead = millis();
  }

  // 2. Update LCD & Serial Monitor (Tiap 2.5 Detik)
  if (millis() - lastLCDUpdate >= 2500) {
    serialMonitoring();
    updateLCD();
    lastLCDUpdate = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));  // Indikator kedip
  }

  // // 3. Kirim ke Web Laravel (Tiap 15 Detik)
  // if (millis() - lastWebSend >= 15000) {
  //   // Cek koneksi WiFi sebelum maksa ngirim data
  //   if (WiFi.status() == WL_CONNECTED) {
  //     sendToLaravel();
  //   } else {
  //     Serial.println("[WIFI] Koneksi terputus! Mencoba nyambung ulang...");
  //     WiFi.reconnect(); // Auto reconnect tanpa bikin sistem macet
  //     lcd.setCursor(15, 0); // Opsional: Kasih tanda 'X' di pojok LCD kalau WiFi putus
  //     lcd.print("X");
  //   }
  //   lastWebSend = millis();
  // }

  // 4. Log ke SD Card (Tiap 60 Detik / 1 Menit)
// Log ke SD Card setiap 15 detik
unsigned long currentMillis = millis();
  if (currentMillis - lastLogTime >= logInterval) {
    lastLogTime = currentMillis;
    logToSD_Full();
  }


}