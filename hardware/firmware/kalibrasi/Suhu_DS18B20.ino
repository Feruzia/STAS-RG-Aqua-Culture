#include <OneWire.h>
#include <DallasTemperature.h>

// ==========================================
// 1. DEFINISI PIN & OBJEK SENSOR
// ==========================================
#define SUHU_PIN 4  // Pin data untuk DS18B20

OneWire oneWire(SUHU_PIN);
DallasTemperature sensors(&oneWire);

// ==========================================
// 2. VARIABEL & NILAI KALIBRASI
// ==========================================
float suhuC;
float suhuVolt = 0.00; // Sensor digital tidak menggunakan pembacaan voltase ADC

// [PENTING UNTUK PENERUS RISET]
// Ubah nilai OFFSET_SUHU ini jika hasil pembacaan sensor berbeda 
// dengan termometer standar/laboratorium.
// Contoh: Jika sensor terbaca 26.0 tapi suhu asli 25.5, ubah menjadi 0.5
const float OFFSET_SUHU = 0.0; 

// ==========================================
// 3. SETUP UTAMA
// ==========================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== PROGRAM KALIBRASI SENSOR SUHU (DS18B20) ===");
  Serial.println("Standar Riset - Mode Pembacaan Serial Murni");
  Serial.println("===============================================\n");

  // Inisialisasi Sensor
  sensors.begin();

  // UPGRADE: Mengunci resolusi sensor ke 12-bit untuk ketelitian maksimal (0.0625°C)
  // Catatan: Waktu konversi menjadi sedikit lebih lama (~750ms) tapi data sangat presisi.
  sensors.setResolution(12); 
  
  delay(1000);
}

// ==========================================
// 4. MAIN LOOP
// ==========================================
void loop() {
  // --- A. Meminta Data dari Sensor ---
  sensors.requestTemperatures(); 
  suhuC = sensors.getTempCByIndex(0);

  // --- B. Sistem Pengaman (Fail-Safe) ---
  // Jika kabel terputus atau sensor rusak, DS18B20 akan mereturn nilai -127.00.
  // Kita kunci di angka 25.0 C agar perhitungan kompensasi sensor lain (pH, DO, EC)
  // tidak hancur berantakan saat digabungkan di Master Code.
  if (suhuC < -50.0) {
    suhuC = 25.0; 
    Serial.println("[WARNING] Sensor tidak terdeteksi! Menggunakan suhu default 25.0 C");
  } else {
    // --- C. Menerapkan Nilai Kalibrasi (Offset) ---
    suhuC = suhuC - OFFSET_SUHU; 
  }

  // --- D. Menampilkan ke Serial Monitor ---
  // Format disamakan dengan struktur Master Code (Value dan Voltase)
  Serial.printf("Suhu: %.2f C | Voltase: %.2f V (Digital)\n", suhuC, suhuVolt);

  // Delay 1 detik untuk kestabilan pembacaan di Serial Monitor
  delay(1000); 
}