#include <Wire.h>
#include "ADS1X15.h" // Library Rob Tillaart

// ==========================================
// 1. KONFIGURASI PIN I2C ESP32
// ==========================================
#define I2C_SDA 41
#define I2C_SCL 42

// ==========================================
// 2. INISIALISASI OBJEK
// ==========================================
ADS1115 adsB(0x49); // Inisialisasi Modul ADS_B pada alamat I2C 0x49

// ==========================================
// 3. VARIABEL KALIBRASI DO
// ==========================================
// Nilai tegangan referensi saat probe DO berada di udara terbuka (100% saturasi oksigen).
const float CAL1_V_DO = 1.000; 

// ================================================================
// 4. FUNGSI FILTER DIGITAL (TRIMMED MEAN) DARI MASTER CODE
// ================================================================
// Mengambil 40 data, membuang nilai paling ekstrem (Min & Max),
// lalu merata-ratakan sisanya untuk menghindari noise.
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

// ==========================================
// 5. SETUP UTAMA
// ==========================================
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  
  adsB.begin();
  adsB.setGain(1); // Konfigurasi Gain 1 (+/- 4.096V) untuk sensor DO
  
  Serial.println("\n=== PROGRAM KALIBRASI SENSOR DO ===");
  Serial.println("Pin: A0 (Modul ADS_B) | I2C: 0x49");
  Serial.println("Standar Riset - Mode Pembacaan Serial Murni");
  Serial.println("===========================================\n");
  
  delay(1500);
}

// ==========================================
// 6. MAIN LOOP
// ==========================================
void loop() {
  // A. Membaca voltase menggunakan fungsi aman dari Master Code
  float doVolt = getAvgADC_Safe(adsB, 0) * 0.000125;
  
  // B. Kompensasi Suhu Statis
  float suhuC = 25.0; 
  
  // C. Tabel sederhana saturasi oksigen berdasarkan suhu (Sesuai Master Code)
  float doSaturation;
  if (suhuC < 22) doSaturation = 8.7;
  else if (suhuC < 27) doSaturation = 8.2;
  else if (suhuC < 31) doSaturation = 7.6;
  else doSaturation = 7.1;

  // D. Kalkulasi Nilai DO
  float doVal = (doVolt / CAL1_V_DO) * doSaturation;
  
  // Pembatasan batas bawah (0) untuk mencegah nilai negatif
  if (doVal < 0) doVal = 0;
  
  // E. Menampilkan data ke Serial Monitor (Format Single-Line)
  Serial.printf("DO: %.2f mg/L | Voltase: %.3f V\n", doVal, doVolt);
  
  delay(1000);
}