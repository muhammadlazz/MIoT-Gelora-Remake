#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pengaturan LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);  
#define I2C_SDA 21
#define I2C_SCL 22

// Pengaturan Sensor Ultrasonik
#define trigPin 13
#define echoPin 12

// Pengaturan Sensor LDR (DIGITAL)
#define ldrPin 34 // Pin untuk DO (Digital Output)

// PENGATURAN RELAY
#define relayPin 25 // Pin untuk mengontrol relay

void setup() {
  Serial.begin(115200); // Untuk debugging
 
  // Inisialisasi LCD
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
 
  // Inisialisasi Pin Ultrasonik
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
 
  // Inisialisasi Pin LDR
  pinMode(ldrPin, INPUT);
 
  // Inisialisasi Pin Relay 
  pinMode(relayPin, OUTPUT);
  // Atur kondisi awal relay MATI (LOW)
  digitalWrite(relayPin, LOW);
}

void loop() {
  // Panggil fungsi untuk mengukur jarak
  float jarak = ukurJarakCm();

  // Baca LDRDIGITAL
  int statusCahaya = digitalRead(ldrPin);

  // Tampilkan hasil di Serial Monitor
  Serial.print("Jarak: ");
  Serial.print(jarak);
  Serial.print(" cm  |  LDR (DO): ");
  Serial.print(statusCahaya);

  // Tampilkan hasil di LCD
  lcd.setCursor(0, 0);
  lcd.print("Cahaya: ");
 
  // Logika LDR dan Relay
  if (statusCahaya == LOW) {
    lcd.print("Terang  "); // LDR LOW = Terang
   
    // Matikan lampu (Relay OFF)
    digitalWrite(relayPin, HIGH);
    Serial.println(" | Relay: OFF");
   
  } else {
    lcd.print("Gelap   "); // LDR HIGH = Gelap
   
    // Nyalakan lampu (Relay ON)
    digitalWrite(relayPin, LOW);
    Serial.println(" | Relay: ON");
  }

  lcd.setCursor(0, 1);
  lcd.print("Jarak: ");
  lcd.print(jarak, 0);
  lcd.print(" cm  ");

  delay(500);
}

// mengukur jarak
float ukurJarakCm() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long durasi = pulseIn(echoPin, HIGH);
  float jarak = durasi * 0.034 / 2.0;
  return jarak;
}
