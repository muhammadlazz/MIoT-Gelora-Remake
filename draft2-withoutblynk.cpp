#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// --- LIBRARY SERVO ---
#include <ESP32Servo.h>       

// --- LIBRARY SUHU (DIAKTIFKAN) ---
#include <OneWire.h>          
#include <DallasTemperature.h>

// --- 1. PENGATURAN PIN ---
#define I2C_SDA 21  
#define I2C_SCL 22  

#define trigPin 13 
#define echoPin 12 
#define ldrPin 5 
#define relayPin 23
#define servoPin 14  

// Pin Sensor Suhu (DIAKTIFKAN)
#define oneWireBus 27 

// --- 2. INISIALISASI OBJEK ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo pakanServo; 

// Objek Suhu (DIAKTIFKAN)
OneWire oneWire(oneWireBus); 
DallasTemperature sensors(&oneWire); 

// --- 3. KONFIGURASI RELAY & LDR ---
const int KONDISI_GELAP = HIGH; 
const int KONDISI_TERANG = LOW;
const int RELAY_NYALA = LOW; 
const int RELAY_MATI = HIGH;

// --- 4. PENGATURAN PAKAN OTOMATIS ---
unsigned long previousMillis = 0;
// Atur jadwal makan di sini (contoh: 10 detik untuk tes)
const long intervalPakan = 10000; 

void setup() {
  Serial.begin(115200);

  // --- Init LCD ---
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print("System Starting");
  lcd.setCursor(0, 1);
  lcd.print("Calibrating...");
  delay(1000);
  lcd.clear();

  // --- Init Sensor & Actuator ---
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ldrPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, RELAY_MATI); 

  // --- Init Servo ---
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  pakanServo.setPeriodHertz(50); 
  pakanServo.attach(servoPin, 500, 2400); 
  pakanServo.write(0); // Posisi awal tertutup

  // --- Init Suhu (DIAKTIFKAN) ---
  sensors.begin();
}

void loop() {
  // --- A. BACA SENSOR ---
  float jarak = ukurJarakCm();
  int statusLDR = digitalRead(ldrPin);
  
  // Baca Suhu (DIAKTIFKAN)
  sensors.requestTemperatures(); 
  float suhuAir = sensors.getTempCByIndex(0);

  // --- B. LOGIKA LAMPU (RELAY) ---
  String statusLampuStr = "";
  if (statusLDR == KONDISI_GELAP) {
    digitalWrite(relayPin, RELAY_NYALA);
    statusLampuStr = "ON";
  } else {
    digitalWrite(relayPin, RELAY_MATI);
    statusLampuStr = "OFF";
  }

  // --- C. LOGIKA PAKAN OTOMATIS ---
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalPakan) {
    previousMillis = currentMillis;
    beriPakanIkan(); 
  }

  // --- D. TAMPILAN LCD (LAYOUT BARU) ---
  // Agar muat, kita singkat teksnya.
  
  // Baris 1: Suhu & Lampu
  // Tampilan: "T:28C Lamp:ON "
  lcd.setCursor(0, 0);
  lcd.print("T:"); 
  
  // Cek error sensor suhu (-127 artinya error/kabel lepas)
  if(suhuAir == -127.00) {
    lcd.print("Err"); 
  } else {
    lcd.print(suhuAir, 0); // Tampilkan suhu bulat (tanpa koma) hemat tempat
    lcd.print((char)223);  // Simbol derajat
    lcd.print("C");
  }

  lcd.print(" Lmp:"); 
  lcd.print(statusLampuStr);
  lcd.print(" "); // Spasi pembersih sisa karakter

  // Baris 2: Sisa Pakan (Jarak)
  lcd.setCursor(0, 1);
  lcd.print("Pakan: ");
  if (jarak > 0 && jarak < 100) { // Filter angka aneh
    lcd.print(jarak, 0);
    lcd.print("cm    "); 
  } else {
    lcd.print("Error ");
  }

  // Debugging Serial Monitor
  Serial.print("Suhu: "); Serial.print(suhuAir);
  Serial.print("C | Jarak: "); Serial.print(jarak);
  Serial.print("cm | Lampu: "); Serial.println(statusLampuStr);

  delay(1000); 
}

// --- FUNGSI TAMBAHAN ---

void beriPakanIkan() {
  Serial.println("--- WAKTUNYA MAKAN! ---");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(">> FEEDING TIME <<");
  lcd.setCursor(0, 1);
  lcd.print("Membuka Servo...");

  pakanServo.write(90); 
  delay(1000); 
  
  pakanServo.write(0);  
  delay(500);

  Serial.println("--- SELESAI ---");
  lcd.clear();
}

float ukurJarakCm() {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long durasi = pulseIn(echoPin, HIGH);
  if (durasi == 0) return 0; 
  return durasi * 0.034 / 2.0;
}
