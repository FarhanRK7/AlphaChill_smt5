#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Pin definitions
#define DHTPIN 4         
#define DHTTYPE DHT11    
#define MOTOR_PIN_ENA 13  
#define MOTOR_PIN_IN1 12  
#define MOTOR_PIN_IN2 14  
#define TEMPERATURE_THRESHOLD 29 
#define TEMPERATURE_THRESHOLD1 32 

// Object instantiation
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Network configuration
const char* ssid = "Pohon Terbang";
const char* password = "17081945";
const char* serverIP = "192.168.100.27";
const int userId = 1; 

WiFiClient NodeMCU;

// Global variables
int motorSpeed = 0;
bool fanControlledByApp = false;
bool lastFanStatus = false;

// Function declarations
void resetFanStatus();
void checkFanStatus();
void sendToServer(float temperature, float humidity, int rpm);
int calculateRPM(int pwmValue);
void adjustFanSpeed(float temperature);
void displayLCD(float temperature, float humidity);
void connectToWiFi();

// Function implementations
void adjustFanSpeed(float temperature) {
  if (lastFanStatus) {
    if (temperature > TEMPERATURE_THRESHOLD1) {
      motorSpeed = 255;
    } else if (temperature > TEMPERATURE_THRESHOLD) {
      motorSpeed = 150;
    } else {
      motorSpeed = 75;
    }
    
    analogWrite(MOTOR_PIN_ENA, motorSpeed);
    digitalWrite(MOTOR_PIN_IN1, HIGH);
    digitalWrite(MOTOR_PIN_IN2, LOW);
    
    Serial.print("Fan speed adjusted to: ");
    Serial.println(motorSpeed);
  }
}

void resetFanStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://" + String(serverIP) + "/alphachill/android/submit_data.php";
    url += "?fan_status=0";
    url += "&id_user=" + String(userId);
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      Serial.println("Fan status reset to OFF");
    }
    http.end();
  }
}

int calculateRPM(int pwmValue) {
  int maxRPM = 3000;
  return map(pwmValue, 0, 255, 0, maxRPM);
}

void sendToServer(float temperature, float humidity, int rpm) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setTimeout(10000);

    String url = "http://" + String(serverIP) + "/alphachill/android/submit_data.php";
    url += "?temperature=" + String(temperature, 2);
    url += "&humidity=" + String(humidity, 2);
    url += "&rpm=" + String(rpm);
    url += "&id_user=" + String(userId);

    http.begin(url);
    http.addHeader("Accept", "application/json");

    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.println("Data sent successfully");
    } else {
      Serial.print("Error sending data. Code: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }
}

void checkFanStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://" + String(serverIP) + "/alphachill/android/get_sensor_data.php";
    url += "?id_user=" + String(userId);
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error && doc["status"] == "success") {
        bool newFanStatus = doc["data"]["fan_status"].as<bool>();
        
        if (newFanStatus != lastFanStatus) {
          lastFanStatus = newFanStatus;
          
          if (!newFanStatus) {
            motorSpeed = 0;
            analogWrite(MOTOR_PIN_ENA, motorSpeed);
            digitalWrite(MOTOR_PIN_IN1, LOW);
            digitalWrite(MOTOR_PIN_IN2, LOW);
            Serial.println("Fan turned OFF by app");
          } else {
            Serial.println("Fan turned ON by app");
            adjustFanSpeed(dht.readTemperature());
          }
        }
      }
    }
    http.end();
  }
}

void displayLCD(float temperature, float humidity) {
  lcd.setCursor(12, 0);
  lcd.print(temperature);
  lcd.print("   ");

  lcd.setCursor(11, 1);
  lcd.print(humidity);
  lcd.print("   ");
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected.");
}

void setup() {
  Serial.begin(9600);
  dht.begin();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Temperature:");
  lcd.setCursor(0, 1);
  lcd.print("Humidity:");

  connectToWiFi();

  pinMode(MOTOR_PIN_ENA, OUTPUT);
  pinMode(MOTOR_PIN_IN1, OUTPUT);
  pinMode(MOTOR_PIN_IN2, OUTPUT);

  analogWrite(MOTOR_PIN_ENA, 0);
  digitalWrite(MOTOR_PIN_IN1, LOW);
  digitalWrite(MOTOR_PIN_IN2, LOW);
  
  resetFanStatus();
}

void loop() {
  delay(1000);

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  checkFanStatus();

  if (lastFanStatus) {
    adjustFanSpeed(temperature);
  }

  int rpm = calculateRPM(motorSpeed);
  Serial.print("Motor Speed: ");
  Serial.print(motorSpeed);
  Serial.print(" (PWM), RPM: ");
  Serial.println(rpm);

  displayLCD(temperature, humidity);

  sendToServer(temperature, humidity, rpm);

  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
}
