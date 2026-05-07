#include <Wire.h>
#include "ADS1X15.h" // Library Rob Tillaart

// ==========================================
// 1. KONFIGURASI PIN I2C ESP32
// ==========================================
#define I2C_SDA 41
#define I2C_SCL 42

ADS1115 adsA(0x48); // Inisialisasi Modul ADS_A pada alamat I2C 0x48

// ==========================================
// 2. VARIABEL KALIBRASI pH (3-POINT CALIBRATION)
// ==========================================
// PENTING UNTUK PENERUS RISET: 
// Sesuaikan ketiga nilai tegangan di bawah ini berdasarkan hasil 
// pengujian aktual menggunakan larutan buffer standar.

// 1. Tegangan Netral (Referensi larutan buffer pH 7.0)
float neutralVoltage = 1.550;  

// 2. Tegangan Asam (Referensi larutan asam, representasi setara pH 4.0)
// Catatan: Nilai ini merepresentasikan rentang 3 step dari pH netral (7.0 - 3.0 = 4.0 step).
float acidVoltage = 2.500;  

// 3. Tegangan Basa (Referensi larutan basa, representasi setara pH 10.0)
// Catatan: Nilai asumsi awal berdasarkan perhitungan selisih ~0.19V per step pH. 
// Wajib dikalibrasi ulang menggunakan larutan buffer basa standar.
float baseVoltage = 1.150;  

// ==========================================
// 3. MENGHITUNG SLOPE OTOMATIS
// ==========================================
// Menghitung rata-rata kemiringan (slope) dari sisi asam dan sisi basa.
// Pembagi 3.0 digunakan karena rentang interval pengujian adalah 3 step 
// (dari pH 7 ke pH 4, dan dari pH 7 ke pH 10).
float slope = (abs(neutralVoltage - acidVoltage) / 3.0 + abs(neutralVoltage - baseVoltage) / 3.0) / 2.0;

// ==========================================
// 4. FUNGSI FILTER DIGITAL (TRIMMED MEAN)
// ==========================================
// Fungsi ini bertujuan untuk mengeliminasi gangguan noise ekstrem pada pembacaan ADC.
double avergearray(int* arr, int number) {
  int i, max, min;
  double avg;
  long amount = 0;
  
  if (number <= 0) return 0;
  if (number < 5) { 
    for (i = 0; i < number; i++) amount += arr[i];
    return amount / number;
  } else {
    // Menentukan nilai awal untuk pencarian min dan max
    if (arr[0] < arr[1]) {
      min = arr[0]; max = arr[1];
    } else {
      min = arr[1]; max = arr[0];
    }
    // Proses pencarian nilai ekstrem dan akumulasi total
    for (i = 2; i < number; i++) {
      if (arr[i] < min) { amount += min; min = arr[i]; } 
      else if (arr[i] > max) { amount += max; max = arr[i]; } 
      else { amount += arr[i]; }
    }
    // Rata-rata dihitung setelah membuang 1 nilai tertinggi dan 1 nilai terendah
    avg = (double)amount / (number - 2);
  }
  return avg;
}

// ==========================================
// 5. SETUP UTAMA
// ==========================================
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  
  adsA.begin();
  adsA.setGain(1); // Konfigurasi Gain 1 (+/- 4.096V) spesifik untuk sensor pH
  
  // Menampilkan informasi inisialisasi ke Serial Monitor
  Serial.println("\n=== PROGRAM KALIBRASI SENSOR pH ===");
  Serial.println("Metode: 3-Point Calibration");
  Serial.print("Nilai Slope Aktual: ");
  Serial.print(slope, 4);
  Serial.println(" V/pH");
  Serial.println("Standar Riset - Mode Pembacaan Serial Murni");
  Serial.println("=========================================\n");
  
  delay(2000);
}

// ==========================================
// 6. MAIN LOOP
// ==========================================
void loop() {
  int phSamples[40];
  
  // A. Pengambilan 40 sampel pembacaan dari ADS_A (Pin A2)
  for (int i = 0; i < 40; i++) {
    phSamples[i] = adsA.readADC(2);
    delay(10); 
  }
  
  // B. Pemfilteran data menggunakan fungsi Trimmed Mean
  double avgADC = avergearray(phSamples, 40);
  
  // C. Konversi nilai ADC mentah menjadi satuan Voltase
  float phVoltage = avgADC * 0.000125;
  
  // D. Kalkulasi Nilai pH berdasarkan kemiringan (Slope)
  // Catatan: Modul pH bersifat Inverting (Tegangan naik = Semakin Asam)
  float phValue;
  if (phVoltage > neutralVoltage) {
    // Perhitungan untuk kondisi Asam (Tegangan lebih besar dari Neutral)
    phValue = 7.0 - ((phVoltage - neutralVoltage) / slope);
  } else {
    // Perhitungan untuk kondisi Basa (Tegangan lebih kecil dari Neutral)
    phValue = 7.0 + ((neutralVoltage - phVoltage) / slope);
  }
  
  // E. Pembatasan rentang pengukuran absolut untuk menghindari nilai tidak logis
  if (phValue < 0.0) phValue = 0.0;
  if (phValue > 14.0) phValue = 14.0;
  
  // F. Kategorisasi Status Kualitas Air
  String keterangan;
  if (phValue >= 6.5 && phValue <= 7.5) {
    keterangan = "Normal (Aman)";
  } else if (phValue < 6.5) {
    keterangan = "Asam";
  } else {
    keterangan = "Basa";
  }
  
  // G. Menampilkan hasil ke Serial Monitor (Format Single-Line)
  Serial.printf("pH: %.2f | Voltase: %.3f V | Status: %s\n", phValue, phVoltage, keterangan.c_str());
}