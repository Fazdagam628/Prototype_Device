// Libraries used
#include <DHT.h>                  // For DHT11 temperature and humidity sensor
#include <Wire.h>                 // For I2C communication
#include <RTClib.h>               // For RTC DS3231 module
#include <Adafruit_GFX.h>        // Core graphics library for OLED
#include <Adafruit_SSD1306.h>    // Driver for SSD1306 OLED display

// OLED display size
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// MQ2 gas sensor pin and threshold
#define MQ2pin 0
#define Threshold 100  // Gas detection threshold

// Buzzer and LED pins
#define buser 8
#define ledPin 10

// Global variables
float sensorValue;
bool gasDetected = false;

// Initialize OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Timing variables for non-blocking alarm
unsigned long previousMillis = 0;
const long interval = 200;  // 200ms blink interval
bool ledState = false;
bool buzzerState = false;

// Bitmap icon: thermometer
const unsigned char termometer_bmp[] PROGMEM = {
  0x18, 0x3C, 0x3C, 0x24, 0x24, 0x24, 0x3C, 0x3C
};

// Bitmap icon: water drop (humidity)
const unsigned char air_bmp[] PROGMEM = {
  0x18, 0x3C, 0x7E, 0x7E, 0x3C, 0x18, 0x00, 0x00
};

// Bitmap icon: gas
const unsigned char gas_bmp[] PROGMEM = {
  0x08, 0x1C, 0x3E, 0x7F, 0x5B, 0x3E, 0x1C, 0x08
};

// RTC object
RTC_DS3231 rtc;

// DHT11 sensor configuration
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Non-blocking gas alarm function
void alarmGasNonBlocking() {
  if (!gasDetected) {
    noTone(buser);
    digitalWrite(ledPin, LOW);
    return;
  }

  // Blink LED and alternate buzzer tone every interval
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    ledState = !ledState;
    digitalWrite(ledPin, ledState ? HIGH : LOW);

    buzzerState = !buzzerState;
    tone(buser, buzzerState ? 1000 : 1500);  // Alternate tones
  }
}

// Initialize MQ2 sensor
void mq2Setup() {
  Serial.println("MQ2 Warming Up");
  pinMode(ledPin, OUTPUT);
  delay(20000);  // Warm-up time for MQ2 sensor (20 seconds)
}

// Initialize DHT sensor
void dhtSetup() {
  dht.begin();
}

// Initialize and synchronize RTC
void clockSetup() {
  if (!rtc.begin()) {
    Serial.println("RTC Not Detected!");
    while (1);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Set RTC time from compile time
}

// Initialize OLED display
void displaySetup() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Not Detected!");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}

// Arduino setup function
void setup() {
  Serial.begin(9600);     // Start serial communication
  clockSetup();           // Initialize RTC
  displaySetup();         // Initialize OLED
  dhtSetup();             // Initialize DHT sensor
}

// Read value from MQ2 gas sensor
void readMQ2() {
  sensorValue = analogRead(MQ2pin);
  Serial.print("Gas Value: ");
  Serial.println(sensorValue);
  gasDetected = sensorValue > Threshold;
}

// Read temperature and humidity from DHT11
void readDHT() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT11 sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");

  // Display temperature on OLED
  display.drawBitmap(0, 18, termometer_bmp, 8, 8, SSD1306_WHITE);
  display.setCursor(10, 18);
  display.print("TEMP  : ");
  display.print(temperature);
  display.println("C");

  // Display humidity on OLED
  display.drawBitmap(0, 30, air_bmp, 8, 8, SSD1306_WHITE);
  display.setCursor(10, 30);
  display.print("HUMID : ");
  display.print(humidity);
  display.println("%");
}

// Display current date and time from RTC
void readClock() {
  DateTime now = rtc.now();

  display.print("DATE   : ");
  display.print(now.day());
  display.print('/');
  display.print(now.month());
  display.print('/');
  display.println(now.year());

  display.print("TIME   : ");
  display.print(now.hour());
  display.print(':');
  display.print(now.minute());
  display.print(':');
  display.println(now.second());
}

// Show gas value and warning icon on OLED
void showMQ2Gas() {
  display.drawBitmap(0, 42, gas_bmp, 8, 8, SSD1306_WHITE);
  display.setCursor(10, 42);
  display.print("GAS   : ");
  display.println(sensorValue);

  if (gasDetected) {
    // Clear and show alert message if gas is detected
    display.clearDisplay();
    display.setCursor(20, 10);
    display.setTextSize(2);
    display.println("!!!");
    display.setTextSize(1);
    display.setCursor(10, 32);
    display.println("!!GAS DETECTED!!");
  }
}

// Display all sensor data to OLED
void displayData() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);

  readClock();
  readDHT();
  readMQ2();
  showMQ2Gas();

  display.display();  // Refresh OLED display
}

// Arduino main loop
void loop() {
  displayData();           // Update OLED display
  alarmGasNonBlocking();   // Check and trigger gas alarm
  delay(100);              // Small delay to reduce flickering
}
