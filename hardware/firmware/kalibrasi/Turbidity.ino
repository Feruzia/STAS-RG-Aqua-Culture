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
ADS1115 adsA(0x48); // Inisialisasi Modul ADS_A pada alamat I2C 0x48

// ==========================================
// 3. VARIABEL KALIBRASI TURBIDITY
// ==========================================
// Nilai referensi teoritis untuk air jernih absolut
const float TURB_CLEAR_V_REF = 4.336; 

// Voltase aktual yang terbaca saat sensor dicelupkan ke sampel air jernih
const float TURB_REAL_V = 3.90; 

// Parameter sensitivitas untuk menentukan kurva kemiringan (slope) pembacaan NTU
const float TURB_SENSITIVITY = 25.0; 

// ==========================================
// 4. SETUP UTAMA
// ==========================================
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  
  adsA.begin();
  // Konfigurasi Gain 0 (+/- 6.144V) khusus untuk sensor Turbidity 
  // karena tegangan referensinya bisa mendekati 4.5V.
  adsA.setGain(0); 
  
  // Menampilkan informasi inisialisasi ke Serial Monitor
  Serial.println("\n=== PROGRAM KALIBRASI SENSOR TURBIDITY ===");
  Serial.println("Pin: A0 (Modul ADS_A) | I2C: 0x48");
  Serial.println("Standar Riset - Mode Pembacaan Serial Murni");
  Serial.println("============================================\n");
  
  delay(1500);
}

// ==========================================
// 5. MAIN LOOP
// ==========================================
void loop() {
  // A. Membaca tegangan analog (rata-rata dari 30 sampel)
  long sum = 0;
  for (int i = 0; i < 30; i++) {
    sum += adsA.readADC(0); 
    delay(5); 
  }
  
  // B. Konversi RAW ADC menjadi satuan Voltase
  // Pengali 0.0001875 adalah resolusi tegangan mutlak untuk Gain 0 pada ADS1115.
  float turbVolt = (sum / 30.0) * 0.0001875;
  
  // C. Kalkulasi kompensasi pergeseran tegangan (Offset)
  float vCalc = turbVolt + (TURB_CLEAR_V_REF - TURB_REAL_V);
  
  float ntuVal;
  
  // D. Algoritma Filter Logika dan Kalkulasi NTU
  // Pagar pembatas 1: Filter untuk kondisi air dengan tingkat kejernihan maksimal
  if (vCalc >= 4.30) {
    ntuVal = 0.0;
  } 
  // Pagar pembatas 2: Filter identifikasi kondisi sensor kering di udara terbuka
  // (Karakteristik sensor menghasilkan drop voltase di kisaran 3.2V - 3.4V)
  else if (turbVolt >= 3.20 && turbVolt <= 3.40) { 
    ntuVal = 0.0; 
  } 
  // Kalkulasi definitif nilai NTU berdasarkan tingkat sensitivitas
  else {
    ntuVal = (TURB_CLEAR_V_REF - vCalc) * TURB_SENSITIVITY; 
  }
  
  // Memastikan tidak ada perolehan nilai negatif pada hasil akhir
  if (ntuVal < 0) {
    ntuVal = 0.0;
  }
  
  // E. Menampilkan hasil komputasi ke Serial Monitor (Format Single-Line)
  Serial.printf("Turbidity: %.1f NTU | Voltase: %.3f V\n", ntuVal, turbVolt);
  
  delay(1000);
}