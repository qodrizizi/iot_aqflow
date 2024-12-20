#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Inisialisasi LCD I2C dengan alamat 0x27 (sesuaikan jika berbeda)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin konfigurasi sensor ultrasonik, LED, dan buzzer
#define TRIG_PIN 4
#define ECHO_PIN 5
#define LED_PIN 2
#define BUZZER_PIN 15

void setup() {
  // Serial monitor
  Serial.begin(115200);

  // Inisialisasi pin
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Inisialisasi LCD
  lcd.init();        // Mulai LCD
  lcd.backlight();   // Nyalakan lampu latar
  lcd.setCursor(0, 0);
  lcd.print("AQFlow");   // Tampilkan teks "AQFlow"
  delay(2000);           // Tampilkan selama 2 detik
  lcd.clear();           // Bersihkan layar
}

void loop() {
  // Ukur jarak
  long duration = getUltrasonicDistance();
  float distance = duration * 0.034 / 2; // Konversi ke cm

  // Tampilkan jarak ke Serial Monitor
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Tampilkan jarak ke LCD
  lcd.setCursor(0, 0);                  // Baris pertama
  lcd.print("Distance: ");             
  lcd.print(distance);                 
  lcd.print(" cm");                    

  // Kontrol LED dan buzzer
  if (distance > 0 && distance < 10) { // Jika jarak < 10 cm
    digitalWrite(LED_PIN, HIGH);       // LED menyala
    tone(BUZZER_PIN, 1000, 200);       // Nada buzzer 1kHz selama 200ms
    delay(200);                        // Jeda sebelum bunyi berikutnya
  } else {
    digitalWrite(LED_PIN, LOW);        // LED mati
    noTone(BUZZER_PIN);                // Buzzer mati
  }

  // Tunggu sebelum pengukuran berikutnya
  delay(500);
}

long getUltrasonicDistance() {
  // Kirim pulsa trigger
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Hitung durasi pulsa
  return pulseIn(ECHO_PIN, HIGH);
}
