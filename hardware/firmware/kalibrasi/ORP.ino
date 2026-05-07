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
// 3. VARIABEL KALIBRASI ORP
// ==========================================
// Lakukan "Zero Calibration" dengan menghubungkan singkat (short) konektor BNC.
// Masukkan nilai tegangan yang terbaca ke dalam variabel ini.
const float ORP_OFFSET = 1.493; 

// ================================================================
// 4. FUNGSI FILTER DIGITAL (TRIMMED MEAN) DARI MASTER CODE
// ================================================================
// Mengambil 40 data, membuang nilai paling ekstrem (Min & Max),
// lalu merata-ratakan sisanya untuk meminimalkan noise pembacaan.
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

  // Rata-rata dari 38 data sisa setelah nilai ekstrem dibuang
  amount = amount - maxVal - minVal;
  return (double)amount / (samples - 2);
}

// ==========================================
// 5. SETUP UTAMA
// ==========================================
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  
  adsB.begin();
  // Konfigurasi Gain 1 (+/- 4.096V) untuk sensor ORP
  adsB.setGain(1); 
  
  // Menampilkan informasi inisialisasi ke Serial Monitor
  Serial.println("\n=== PROGRAM KALIBRASI SENSOR ORP ===");
  Serial.println("Pin: A2 (Modul ADS_B) | I2C: 0x49");
  Serial.println("Standar Riset - Mode Pembacaan Serial Murni");
  Serial.println("============================================\n");
  
  delay(1500);
}

// ==========================================
// 6. MAIN LOOP
// ==========================================
void loop() {
  // A. Membaca voltase menggunakan fungsi aman dari Master Code (Pin A2)
  float orpVoltage = getAvgADC_Safe(adsB, 2) * 0.000125;
  
  // B. Kalkulasi Nilai ORP
  // Mengonversi selisih voltase pembacaan dengan voltase kalibrasi (offset) menjadi milivolt (mV)
  float orpValue = (orpVoltage - ORP_OFFSET) * 1000.0;
  
  // C. Menampilkan data ke Serial Monitor (Format Single-Line)
  Serial.printf("ORP: %.0f mV | Voltase: %.3f V\n", orpValue, orpVoltage);
  
  delay(1000);
}