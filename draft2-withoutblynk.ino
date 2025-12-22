#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>       
#include <OneWire.h>          
#include <DallasTemperature.h>

#define I2C_SDA 21  
#define I2C_SCL 22  

#define trigPin 13 
#define echoPin 12 
#define ldrPin 5 
#define relayPin 23
#define servoPin 14  

#define oneWireBus 27 

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo pakanServo; 

OneWire oneWire(oneWireBus); 
DallasTemperature sensors(&oneWire); 

const int KONDISI_GELAP = HIGH; 
const int KONDISI_TERANG = LOW;
const int RELAY_NYALA = LOW; 
const int RELAY_MATI = HIGH;

unsigned long previousMillis = 0;
const long intervalPakan = 10000; 

void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print("System Starting");
  lcd.setCursor(0, 1);
  lcd.print("Calibrating...");
  delay(1000);
  lcd.clear();

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ldrPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, RELAY_MATI); 

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  pakanServo.setPeriodHertz(50); 
  pakanServo.attach(servoPin, 500, 2400); 
  pakanServo.write(0); 

  sensors.begin();
}

void loop() {
  float jarak = ukurJarakCm();
  int statusLDR = digitalRead(ldrPin);
  
  sensors.requestTemperatures(); 
  float suhuAir = sensors.getTempCByIndex(0);

  String statusLampuStr = "";
  if (statusLDR == KONDISI_GELAP) {
    digitalWrite(relayPin, RELAY_NYALA);
    statusLampuStr = "ON";
  } else {
    digitalWrite(relayPin, RELAY_MATI);
    statusLampuStr = "OFF";
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalPakan) {
    previousMillis = currentMillis;
    beriPakanIkan(); 
  }

  lcd.setCursor(0, 0);
  lcd.print("T:"); 
  
  if(suhuAir == -127.00) {
    lcd.print("Err"); 
  } else {
    lcd.print(suhuAir, 0); 
    lcd.print((char)223);  
    lcd.print("C");
  }

  lcd.print(" Lmp:"); 
  lcd.print(statusLampuStr);
  lcd.print(" "); 

  lcd.setCursor(0, 1);
  lcd.print("Pakan: ");
  if (jarak > 0 && jarak < 100) { 
    lcd.print(jarak, 0);
    lcd.print("cm    "); 
  } else {
    lcd.print("Error ");
  }

  Serial.print("Suhu: "); Serial.print(suhuAir);
  Serial.print("C | Jarak: "); Serial.print(jarak);
  Serial.print("cm | Lampu: "); Serial.println(statusLampuStr);

  delay(1000); 
}

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
