#define BLYNK_TEMPLATE_ID "TMPL6oxrONg8P"
#define BLYNK_TEMPLATE_NAME "GELORA"
#define BLYNK_AUTH_TOKEN "spufwRYysTI5ci4M27X3dTn9P17mlT_4"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>

char ssid[] = "lazz";
char pass[] = "hashtag#";

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
BlynkTimer timer;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

const int KONDISI_GELAP = HIGH; 
const int RELAY_NYALA = LOW; 
const int RELAY_MATI = HIGH;   

const long ldrDelayDuration = 5000; 
unsigned long ldrTimerStart = 0;
bool isWaitingToTurnOff = false;

const float TINGGI_BOTOL = 10.0;
const float BATAS_PAKAN_HABIS = 8.0;


float suhuAir = 0;
float jarak = 0;          
float levelPakan = 0;     
String labelLampu = "OFF";
String labelPakan = "OFF";

bool pakanHabisNotifSent = false; 

bool modeAutoLampu = true; 
bool modeAutoPakan = true; 
int manualLampuState = 0;  

unsigned long previousMillis = 0;
const long intervalPakan = 43200000; // 12 Jam

BLYNK_WRITE(V3) { if (param.asInt() == 1) beriPakanIkan(); }

BLYNK_WRITE(V4) {
  manualLampuState = param.asInt();
  if (!modeAutoLampu) digitalWrite(relayPin, (manualLampuState == 1) ? RELAY_NYALA : RELAY_MATI);
}

BLYNK_WRITE(V5) {
  modeAutoLampu = (param.asInt() == 1);
  if (!modeAutoLampu) Blynk.virtualWrite(V4, manualLampuState);
}

BLYNK_WRITE(V6) { modeAutoPakan = (param.asInt() == 1); }

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V4); Blynk.syncVirtual(V5); Blynk.syncVirtual(V6);
}

void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init(); lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("GELORA Starting");

  pinMode(trigPin, OUTPUT); pinMode(echoPin, INPUT);
  pinMode(ldrPin, INPUT); pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, RELAY_MATI); 

  ESP32PWM::allocateTimer(0); ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2); ESP32PWM::allocateTimer(3);
  pakanServo.setPeriodHertz(50); 
  pakanServo.attach(servoPin, 500, 2400); 
  pakanServo.write(0); 

  sensors.begin();
  sensors.setWaitForConversion(false); 
  sensors.requestTemperatures();       

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  lcd.clear(); lcd.print("Connected!"); delay(1000); lcd.clear();

  timer.setInterval(1000L, timerEvent);
}

void loop() {
  Blynk.run();
  timer.run();

  if (modeAutoPakan) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= intervalPakan) {
      previousMillis = currentMillis;
      beriPakanIkan(); 
    }
  }
}

// --- FUNGSI UTAMA (Setiap 1 Detik) ---
void timerEvent() {
  jarak = ukurJarakCm();
  levelPakan = TINGGI_BOTOL - jarak;

  if (levelPakan < 0) levelPakan = 0;
  if (levelPakan > TINGGI_BOTOL) levelPakan = TINGGI_BOTOL;

  if (jarak >= BATAS_PAKAN_HABIS) {
    if (!pakanHabisNotifSent) {
      Serial.println("Warning: Isi Pakan Abis!");
      Blynk.logEvent("pakan_low", "Isi pakan ikannya brosis!"); 
      pakanHabisNotifSent = true; 
    }
  } else {
    if (jarak < (BATAS_PAKAN_HABIS - 0.5) && jarak > 0) { 
       pakanHabisNotifSent = false;
    }
  }

  float tempReading = sensors.getTempCByIndex(0);
  if (tempReading != -127.00) { suhuAir = tempReading; }
  sensors.requestTemperatures(); 

  checkLampuLogic();

  labelPakan = modeAutoPakan ? "Auto" : "Man.";
  updateLCD();
  
  Blynk.virtualWrite(V0, suhuAir);       
  
  Blynk.virtualWrite(V1, levelPakan);         
  
  Blynk.virtualWrite(V2, labelLampu);
}

void checkLampuLogic() {
  int statusLDR = digitalRead(ldrPin);

  if (modeAutoLampu) {
    if (statusLDR == KONDISI_GELAP) {
      digitalWrite(relayPin, RELAY_NYALA);
      labelLampu = "A-ON";
      Blynk.virtualWrite(V4, 1);
      isWaitingToTurnOff = false; 
    } else {
      if (!isWaitingToTurnOff) {
        ldrTimerStart = millis();
        isWaitingToTurnOff = true;
      }
      if (isWaitingToTurnOff && (millis() - ldrTimerStart >= ldrDelayDuration)) {
        digitalWrite(relayPin, RELAY_MATI);
        labelLampu = "A-OFF";
        Blynk.virtualWrite(V4, 0);
      } else {
        if(digitalRead(relayPin) == RELAY_NYALA) labelLampu = "A-ON*"; 
      }
    }
  } else {
    if (manualLampuState == 1) {
      digitalWrite(relayPin, RELAY_NYALA);
      labelLampu = "M-ON";
    } else {
      digitalWrite(relayPin, RELAY_MATI);
      labelLampu = "M-OFF";
    }
    isWaitingToTurnOff = false;
  }
}

void updateLCD() {
  lcd.setCursor(0, 0);
  lcd.print("T:"); 
  if(suhuAir == 0 || suhuAir == -127) lcd.print("Er "); 
  else {
    lcd.print(suhuAir, 0);
    lcd.print((char)223); lcd.print("C ");
  }
  
  lcd.print("L:"); lcd.print(labelLampu);
  lcd.print("  "); 

  lcd.setCursor(0, 1);
  
  if (jarak >= BATAS_PAKAN_HABIS) {
    lcd.print("! ISI PAKAN !   "); 
  } 
  else {
    lcd.print("P:"); lcd.print(labelPakan);
    lcd.print(" "); 

    if (jarak > 0 && jarak < 400) { 
      lcd.print(jarak, 0); lcd.print("cm   "); 
    } else {
      if (jarak == 0) lcd.print("Err "); 
      else lcd.print("High");
    }
  }
}

void beriPakanIkan() {
  Serial.println("--- FEEDING ---");
  
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(">> FEEDING TIME");
  lcd.setCursor(0, 1); lcd.print("  Nyam Nyam...  ");

  if (!pakanServo.attached()) {
    pakanServo.attach(servoPin);
  }

  
  for (int i = 0; i < 4; i++) {
    
    pakanServo.write(160); 
    delay(700);
    Blynk.run();

    pakanServo.write(0);  
    delay(700); 
    Blynk.run();
  }

  delay(500);
  pakanServo.detach(); 
  
  lcd.clear();
}

float ukurJarakCm() {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long durasi = pulseIn(echoPin, HIGH, 25000);
  
  if (durasi == 0) return 0; 
  return durasi * 0.034 / 2.0;
}
