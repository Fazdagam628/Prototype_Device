#include <DHT.h>
#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define MQ2pin 0
#define Threshold 100
#define buser 8
#define ledPin 10
float sensorValue;
bool gasTerdeteksi = false;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

unsigned long previousMillis = 0;
const long interval = 200;  // 200ms kedipan
bool ledState = false;
bool buzzerState = false;

const unsigned char termometer_bmp[] PROGMEM = {
  0x18, 0x3C, 0x3C, 0x24, 0x24, 0x24, 0x3C, 0x3C
};

const unsigned char air_bmp[] PROGMEM = {
  0x18, 0x3C, 0x7E, 0x7E, 0x3C, 0x18, 0x00, 0x00
};
const unsigned char gas_bmp[] PROGMEM = {
  0x08,  //    *
  0x1C,  //   ***
  0x3E,  //  *****
  0x7F,  // *******
  0x5B,  // * ** **
  0x3E,  //  *****
  0x1C,  //   ***
  0x08   //    *
};



RTC_DS3231 rtc;

// Konfigurasi DHT
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

void alarmGasNonBlocking() {
  if (!gasTerdeteksi) {
    noTone(buser);
    digitalWrite(ledPin, LOW);
    return;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    ledState = !ledState;
    digitalWrite(ledPin, ledState ? HIGH : LOW);

    buzzerState = !buzzerState;
    tone(buser, buzzerState ? 1000 : 1500);
  }
}


void mq2Setup() {
  Serial.println("MQ2 Warming Up");
  pinMode(ledPin, OUTPUT);
  delay(20000);
}

void dhtSetup() {
  dht.begin();
}

void clockSetup() {
  if (!rtc.begin()) {
    Serial.println("RTC Tidak Terdeteksi!");
    while (1)
      ;
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void displaySetup() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Tidak Terdeteksi!");
    while (1)
      ;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}
void setup() {
  Serial.begin(9600);
  clockSetup();
  displaySetup();
  dhtSetup();
}

void readMQ2() {
  sensorValue = analogRead(MQ2pin);
  Serial.print("Gas Value: ");
  Serial.println(sensorValue);
  gasTerdeteksi = sensorValue > Threshold;
}



void readDHT() {
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("Gagal membaca data dari DHT11!");
    return;
  }

  Serial.print("Suhu: ");
  Serial.print(suhu);
  Serial.println("°C");
  Serial.print("Kelembapan: ");
  Serial.print(kelembapan);
  Serial.println("%");

  // Gambar ikon termometer
  display.drawBitmap(0, 18, termometer_bmp, 8, 8, SSD1306_WHITE);
  display.setCursor(10, 18);
  display.print("Suhu  : ");
  display.print(suhu);
  display.println("C");

  // Gambar ikon tetesan air
  display.drawBitmap(0, 30, air_bmp, 8, 8, SSD1306_WHITE);
  display.setCursor(10, 30);
  display.print("Lembab: ");
  display.print(kelembapan);
  display.println("%");
}

void readClock() {
  DateTime now = rtc.now();

  display.print("Tanggal : ");
  display.print(now.day());
  display.print('/');
  display.print(now.month());
  display.print('/');
  display.println(now.year());

  display.print("Waktu   : ");
  display.print(now.hour());
  display.print(':');
  display.print(now.minute());
  display.print(':');
  display.println(now.second());
}
void showMQ2Gas() {
  // Tampilkan ikon gas di samping nilai gas
  display.drawBitmap(0, 42, gas_bmp, 8, 8, SSD1306_WHITE);  // posisi Y=48 agar tidak bentrok

  display.setCursor(10, 42);
  display.print("Gas   : ");
  display.println(sensorValue);

  if (gasTerdeteksi) {
    display.clearDisplay();
    display.setCursor(20, 10);
    display.setTextSize(2);
    display.println("!!!");
    display.setTextSize(1);
    display.setCursor(10, 32);
    display.println("!!GAS TERDETEKSI!!");
  }
}
void displayData() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);

  readClock();
  readDHT();
  readMQ2();
  showMQ2Gas();



  display.display();
}



void loop() {
  displayData();          // Update OLED
  alarmGasNonBlocking();  // Jalankan alarm real-time
  delay(100);             // Boleh kecil agar tidak boros refresh
}
