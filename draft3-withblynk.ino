#define BLYNK_TEMPLATE_ID "TMPL6nTtgmM5k"
#define BLYNK_TEMPLATE_NAME "GELORA"
#define BLYNK_AUTH_TOKEN "vqzGjgY19NVbxDzoExlqNfVJnRV8BHJl"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>       
#include <OneWire.h>          
#include <DallasTemperature.h>

char ssid[] = "lazz";
char pass[] = "dollar$$";

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

float suhuAir = 0;
float jarak = 0;
String statusLampuStr = "OFF";

bool modeAutoLampu = true; 
int manualLampuState = 0;  

unsigned long previousMillis = 0;
const long intervalPakan = 10000; 

BLYNK_WRITE(V3) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    beriPakanIkan(); 
  }
}

BLYNK_WRITE(V4) {
  manualLampuState = param.asInt(); 
  if (!modeAutoLampu) {
    if (manualLampuState == 1) digitalWrite(relayPin, RELAY_NYALA);
    else digitalWrite(relayPin, RELAY_MATI);
  }
}

BLYNK_WRITE(V5) {
  int pinValue = param.asInt();
  modeAutoLampu = (pinValue == 1);
  
  if (!modeAutoLampu) {
    Blynk.virtualWrite(V4, manualLampuState); 
  }
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V4);
  Blynk.syncVirtual(V5);
}

void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");

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

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  lcd.clear();
  lcd.print("System Online");
  delay(1000);
  lcd.clear();

  timer.setInterval(1000L, timerEvent);
}

void loop() {
  Blynk.run();
  timer.run();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= intervalPakan) {
    previousMillis = currentMillis;
    beriPakanIkan(); 
  }
}

void timerEvent() {
  jarak = ukurJarakCm();
  sensors.requestTemperatures(); 
  suhuAir = sensors.getTempCByIndex(0);
  int statusLDR = digitalRead(ldrPin);

  if (modeAutoLampu) {
    if (statusLDR == KONDISI_GELAP) {
      digitalWrite(relayPin, RELAY_NYALA);
      statusLampuStr = "AUTO:ON";
      Blynk.virtualWrite(V4, 1); 
    } else {
      digitalWrite(relayPin, RELAY_MATI);
      statusLampuStr = "AUTO:OFF";
      Blynk.virtualWrite(V4, 0);
    }
  } else {
    if (manualLampuState == 1) {
      digitalWrite(relayPin, RELAY_NYALA);
      statusLampuStr = "MAN:ON";
    } else {
      digitalWrite(relayPin, RELAY_MATI);
      statusLampuStr = "MAN:OFF";
    }
  }

  updateLCD();

  Blynk.virtualWrite(V0, suhuAir);       
  Blynk.virtualWrite(V1, jarak);         
  Blynk.virtualWrite(V2, statusLampuStr); 
  
  Serial.print("Suhu: "); Serial.print(suhuAir);
  Serial.print(" | Mode: "); Serial.println(modeAutoLampu ? "Auto" : "Manual");
}

void updateLCD() {
  lcd.setCursor(0, 0);
  lcd.print("T:"); 
  if(suhuAir == -127.00) lcd.print("Er"); 
  else lcd.print(suhuAir, 0);
  lcd.print((char)223); lcd.print("C ");
  
  lcd.print(statusLampuStr); 

  lcd.setCursor(0, 1);
  lcd.print("Pakan: ");
  if (jarak > 0 && jarak < 400) { 
    lcd.print(jarak, 0); lcd.print("cm    "); 
  } else {
    if (jarak == 0) lcd.print("Putus "); 
    else lcd.print("Jauh  ");
  }
}

void beriPakanIkan() {
  Serial.println("--- FEEDING TIME ---");
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(">> FEEDING TIME");
  
  pakanServo.write(90); 
  delay(1000); 
  pakanServo.write(0);  
  delay(500);
  
  lcd.clear();
}

float ukurJarakCm() {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long durasi = pulseIn(echoPin, HIGH, 30000); 
  if (durasi == 0) return 0; 
  return durasi * 0.034 / 2.0;
}
