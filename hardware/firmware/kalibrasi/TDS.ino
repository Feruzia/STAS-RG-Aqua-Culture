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
ADS1115 ads(0x48); // Inisialisasi Modul ADS_A pada alamat I2C 0x48

// Konstanta pengali Gain 1 (+/- 4.096V) -> 1 bit = 0.125mV
const float multiplier = 0.000125; 

// ==========================================
// 3. SETUP UTAMA
// ==========================================
void setup() {
  Serial.begin(115200);
  
  Wire.begin(I2C_SDA, I2C_SCL);
  
  ads.begin();
  ads.setGain(1); // Konfigurasi Gain 1 (+/- 4.096V) untuk sensor TDS
  
  Serial.println("\n=== PROGRAM KALIBRASI SENSOR TDS ===");
  Serial.println("Pin: A1 | I2C: 0x48");
  Serial.println("Standar Riset - Mode Pembacaan Serial Murni");
  Serial.println("=========================================\n");
  
  delay(1500);
}

// ==========================================
// 4. MAIN LOOP
// ==========================================
void loop() {
  // A. Membaca tegangan analog (rata-rata 10 sampel)
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    // Membaca pin A1 dari ADS_A
    sum += ads.readADC(1); 
    delay(10);
  }
  
  // B. Konversi RAW ADC menjadi satuan Voltase
  float tdsVoltage = (sum / 10.0) * multiplier;
  
  // C. Kompensasi Suhu
  // Catatan: Suhu diset statis pada 25.0 C untuk keperluan kalibrasi independen.
  // Saat digabung ke Master Code, nilai ini menyesuaikan pembacaan sensor suhu aktual.
  float temperature = 25.0; 
  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensationVoltage = tdsVoltage / compensationCoefficient;
  
  // D. Kalkulasi Nilai TDS (Rumus Standar Analog TDS)
  float tdsValue = (133.42 * pow(compensationVoltage, 3) - 255.86 * pow(compensationVoltage, 2) + 857.39 * compensationVoltage) * 0.5;

  // E. Pembatasan nilai minimum agar tidak menghasilkan nilai negatif di udara terbuka
  if (tdsValue < 0) tdsValue = 0;

  // F. Menampilkan hasil ke Serial Monitor (Format Single-Line)
  Serial.printf("TDS: %.0f ppm | Voltase: %.3f V\n", tdsValue, tdsVoltage);
  
  delay(1000);
}#include <Wire.h>
#include "ADS1X15.h" // Library Rob Tillaart

// ==========================================
// 1. KONFIGURASI PIN I2C ESP32
// ==========================================
#define I2C_SDA 41
#define I2C_SCL 42

// ==========================================
// 2. INISIALISASI OBJEK
// ==========================================
ADS1115 ads(0x48); // Inisialisasi Modul ADS_A pada alamat I2C 0x48

// Konstanta pengali Gain 1 (+/- 4.096V) -> 1 bit = 0.125mV
const float multiplier = 0.000125; 

// ==========================================
// 3. SETUP UTAMA
// ==========================================
void setup() {
  Serial.begin(115200);
  
  Wire.begin(I2C_SDA, I2C_SCL);
  
  ads.begin();
  ads.setGain(1); // Konfigurasi Gain 1 (+/- 4.096V) untuk sensor TDS
  
  Serial.println("\n=== PROGRAM KALIBRASI SENSOR TDS ===");
  Serial.println("Pin: A1 | I2C: 0x48");
  Serial.println("Standar Riset - Mode Pembacaan Serial Murni");
  Serial.println("=========================================\n");
  
  delay(1500);
}

// ==========================================
// 4. MAIN LOOP
// ==========================================
void loop() {
  // A. Membaca tegangan analog (rata-rata 10 sampel)
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    // Membaca pin A1 dari ADS_A
    sum += ads.readADC(1); 
    delay(10);
  }
  
  // B. Konversi RAW ADC menjadi satuan Voltase
  float tdsVoltage = (sum / 10.0) * multiplier;
  
  // C. Kompensasi Suhu
  // Catatan: Suhu diset statis pada 25.0 C untuk keperluan kalibrasi independen.
  // Saat digabung ke Master Code, nilai ini menyesuaikan pembacaan sensor suhu aktual.
  float temperature = 25.0; 
  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensationVoltage = tdsVoltage / compensationCoefficient;
  
  // D. Kalkulasi Nilai TDS (Rumus Standar Analog TDS)
  float tdsValue = (133.42 * pow(compensationVoltage, 3) - 255.86 * pow(compensationVoltage, 2) + 857.39 * compensationVoltage) * 0.5;

  // E. Pembatasan nilai minimum agar tidak menghasilkan nilai negatif di udara terbuka
  if (tdsValue < 0) tdsValue = 0;

  // F. Menampilkan hasil ke Serial Monitor (Format Single-Line)
  Serial.printf("TDS: %.0f ppm | Voltase: %.3f V\n", tdsValue, tdsVoltage);
  
  delay(1000);
}