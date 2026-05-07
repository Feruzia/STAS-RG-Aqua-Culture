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
// 3. VARIABEL KALIBRASI EC / TDS
// ==========================================
// Nilai kompensasi konstanta (K_VALUE).
// Sesuaikan nilai ini berdasarkan pengujian dengan larutan kalibrasi standar.
// - Jika pembacaan lebih rendah dari standar, naikkan nilai K_VALUE.
// - Jika pembacaan lebih tinggi dari standar, turunkan nilai K_VALUE.
float K_VALUE = 1.00; 

// ==========================================
// 4. FUNGSI FILTER DIGITAL (TRIMMED MEAN)
// ==========================================
// Berfungsi untuk meminimalkan lonjakan noise dengan membuang nilai ekstrem (tertinggi dan terendah).
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
  
  adsB.begin();
  // Konfigurasi Gain 1 (+/- 4.096V). 
  // Rentang tegangan sensor EC/TDS analog umumnya berada pada 0.1V hingga 2.5V.
  adsB.setGain(1); 
  
  // Menampilkan informasi inisialisasi ke Serial Monitor
  Serial.println("\n=== PROGRAM KALIBRASI SENSOR EC/TDS ===");
  Serial.println("Pin: A1 (Modul ADS_B) | I2C: 0x49");
  Serial.println("Standar Riset - Mode Pembacaan Serial Murni");
  Serial.println("=============================================\n");
  
  delay(1500);
}

// ==========================================
// 6. MAIN LOOP
// ==========================================
void loop() {
  int ecArray[40];
  
  // A. Pengambilan 40 sampel pembacaan ADC dari ADS_B (Pin A1)
  for (int i = 0; i < 40; i++) {
    ecArray[i] = adsB.readADC(1); 
    delay(10); 
  }
  
  // B. Pemfilteran data menggunakan algoritma Trimmed Mean
  double avgADC = avergearray(ecArray, 40);
  
  // C. Konversi nilai ADC mentah ke dalam satuan Voltase
  // Pengali 0.000125 adalah resolusi tegangan untuk Gain 1 pada ADS1115.
  float ecVoltage = avgADC * 0.000125;
  
  // D. Kalkulasi Nilai TDS dan EC
  // Menggunakan persamaan polinomial standar untuk sensor EC/TDS analog.
  float tdsValue = (133.42 * pow(ecVoltage, 3) - 255.86 * pow(ecVoltage, 2) + 857.39 * ecVoltage) * 0.5 * K_VALUE;
  
  // Pembatasan batas bawah (0) untuk mencegah nilai negatif saat probe berada di udara
  if (tdsValue < 0) tdsValue = 0;
  
  // Konversi pendekatan dari TDS ke Electrical Conductivity (EC) dalam uS/cm
  float ecValue = tdsValue * 2.0;
  
  // E. Menampilkan data ke Serial Monitor (Format Single-Line)
  Serial.printf("Voltase: %.3f V | TDS: %.0f ppm | EC: %.0f uS/cm\n", ecVoltage, tdsValue, ecValue);
}