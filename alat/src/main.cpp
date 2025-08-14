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
char *ssid[] = {"Pohon Terbang", "Ayam Jago", "Test"};
char *password[] = {"17081945", "kuda kuda", "ALAMANDA1"};
char *serverIP[] = {"192.168.100.127", "192.168.167.48", "192.168.18.157"};
const int userId = 1;
int connectedIndex = 0;

WiFiClient NodeMCU;

// Global variables
int motorSpeed = 0;
bool fanControlledByApp = false;
bool lastFanStatusWebsite = false;
bool lastFanStatusAndroid = false;

// Function declarations
void resetFanStatus();
void checkFanStatus();
void sendToServer(float temperature, float humidity, int rpm);
int calculateRPM(int pwmValue);
void adjustFanSpeed(float temperature, float humidity);
void displayLCD(float temperature, float humidity);
void connectToWiFi();


float fuzzyLogicRPM(float temperature, float humidity)
{
  // Membership functions for Temperature
  float lowHeat = (temperature <= 25) ? 1 : ((temperature > 25 && temperature <= 28) ? (28 - temperature) / 3.0 : 0);
  float mediumHeat = (temperature > 26 && temperature <= 30) ? (temperature - 26) / 4.0 : ((temperature > 30 && temperature <= 32) ? (32 - temperature) / 2.0 : 0);
  float highHeat = (temperature >= 31) ? 1 : ((temperature > 29 && temperature <= 31) ? (temperature - 29) / 2.0 : 0);

  // Membership functions for Humidity
  float lowHumid = (humidity <= 50) ? 1 : ((humidity > 50 && humidity <= 55) ? (55 - humidity) / 5.0 : 0);
  float mediumHumid = (humidity > 50 && humidity <= 65) ? (humidity - 50) / 15.0 : ((humidity > 65 && humidity <= 75) ? (75 - humidity) / 10.0 : 0);
  float highHumid = (humidity >= 70) ? 1 : ((humidity > 65 && humidity < 70) ? (humidity - 65) / 5.0 : 0);

  // Sugeno RPM values
  float rpmLow = 1000;  // Low RPM
  float rpmMed = 3000;  // Medium RPM
  float rpmHigh = 5000; // High RPM

  // Rules
  float rule1 = fmin(lowHeat, lowHumid);
  float rule2 = fmin(lowHeat, mediumHumid);
  float rule3 = fmin(lowHeat, highHumid);
  float rule4 = fmin(mediumHeat, lowHumid);
  float rule5 = fmin(mediumHeat, mediumHumid);
  float rule6 = fmin(mediumHeat, highHumid);
  float rule7 = fmin(highHeat, lowHumid);
  float rule8 = fmin(highHeat, mediumHumid);
  float rule9 = fmin(highHeat, highHumid);

  // Sugeno Weighted Average
  float numerator = rule1 * rpmLow + rule2 * rpmLow + rule3 * rpmMed +
                    rule4 * rpmMed + rule5 * rpmMed + rule6 * rpmHigh +
                    rule7 * rpmHigh + rule8 * rpmHigh + rule9 * rpmHigh;

  float denominator = rule1 + rule2 + rule3 +
                      rule4 + rule5 + rule6 +
                      rule7 + rule8 + rule9;

  // Scale numerator and ensure denominator >= 1
  numerator *= (5000.0 / 3000.0);
  if (denominator < 1.0)
    denominator = 1.0;

  // Compute RPM and clamp
  float resultRPM = numerator / denominator;
  resultRPM = fmax(1000, fmin(resultRPM, 5000));

  float test = (resultRPM / 5000) * 255;

  // Debug
  Serial.print("Numerator: ");
  Serial.println(numerator);
  Serial.print("Denominator: ");
  Serial.println(denominator);
  Serial.print("Resulting RPM: ");
  Serial.println(resultRPM);
  Serial.print("Resulting RPM test: ");
  Serial.println(test);

  return test;
}

void adjustFanSpeed(float temperature, float humidity)
{
  if (lastFanStatus)
  {
    motorSpeed = fuzzyLogicRPM(temperature, humidity); // Calculate RPM using fuzzy logic

    // motorSpeed = fuzzyLogicRPM(28.8, 55); //middle RPM
    // motorSpeed = fuzzyLogicRPM(32, 70); // high RPM
    // motorSpeed = fuzzyLogicRPM(26, 40); // low RPM

    analogWrite(MOTOR_PIN_ENA, motorSpeed);
    digitalWrite(MOTOR_PIN_IN1, HIGH);
    digitalWrite(MOTOR_PIN_IN2, LOW);

    Serial.print("Fan speed adjusted to (fuzzy RPM): ");
    Serial.println(motorSpeed);
  }
}

void resetFanStatus()
{
  Serial.print("------- reseting fan status...");
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String url = "http://" + String(serverIP[connectedIndex]) + "/Alpha%20Chill/LandingPage/dist/php/data-fan.php";
    url += "?fan_status=0";
    url += "&id_user=" + String(userId);
    url += "&status=submit";

    http.begin(url);
    int httpCode = http.GET();

    Serial.println(" (" + String(httpCode) + ")");

    if (httpCode > 0)
    {
      Serial.println("Fan status reset to OFF");
    }
    http.end();
  }
}

int calculateRPM(int pwmValue)
{
  int maxRPM = 5000;
  return map(pwmValue, 0, 255, 0, maxRPM);
}

void sendToServer(float temperature, float humidity, int rpm)
{

  Serial.println("------- sending fan data...");
  const int maxRetries = 3;
  int retryCount = 0;

  while (retryCount < maxRetries)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      HTTPClient http;
      http.setTimeout(15000);

      String url = "http://" + String(serverIP[connectedIndex]) + "/Alpha%20Chill/LandingPage/dist/php/data-fan.php";
      url += "?temperature=" + String(temperature, 2);
      url += "&humidity=" + String(humidity, 2);
      url += "&rpm=" + String(rpm);
      url += "&id_user=" + String(userId);
      url += "&status=submit";

      http.begin(url);
      http.addHeader("Accept", "application/json");

      int httpResponseCode = http.GET();
      if (httpResponseCode > 0)
      {
        Serial.println("Data sent successfully. Response code: " + String(httpResponseCode));
        Serial.println("URL: " + url);
        http.end();
        return;
      }
      else
      {
        Serial.print("Error sending data. Code: ");
        Serial.println(httpResponseCode);
        Serial.println("Error description: " + http.errorToString(httpResponseCode));
      }

      http.end();
    }
    else
    {
      Serial.println("WiFi not connected. Attempting to reconnect...");
      WiFi.disconnect();
      delay(1000);
      connectToWiFi();
    }

    retryCount++;
    if (retryCount < maxRetries)
    {
      Serial.println("Retrying in 5 seconds...");
      delay(5000);
    }
  }

  if (retryCount == maxRetries)
  {
    Serial.println("Failed to send data after " + String(maxRetries) + " attempts.");
  }
}

void checkFanStatus()
{
  Serial.print("------- checking fan status...");
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String url = "http://" + String(serverIP[connectedIndex]) + "/Alpha%20Chill/LandingPage/dist/php/data-fan.php";
    url += "?id_user=" + String(userId);
    url += "&status=check";

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);

      Serial.println(" (" + String(httpCode) + ")");
      Serial.println("response: " + String(payload));

      if (!error && doc["status"] == "success")
      {
        lastFanStatusWebsite = doc["data"]["fan_status"].as<bool>();

        Serial.println("Fan status (Website): " + String(lastFanStatusWebsite));
        Serial.println("Statement.... " + String(lastFanStatusWebsite != lastFanStatus));
        if (lastFanStatusWebsite != lastFanStatus)
        {
          Serial.println("CHECKING.....");
          lastFanStatus = lastFanStatusWebsite;

          if (lastFanStatusWebsite == false)
          {
            Serial.println("Fan status (FALSE)");
            motorSpeed = 0;
            analogWrite(MOTOR_PIN_ENA, motorSpeed);
            digitalWrite(MOTOR_PIN_IN1, LOW);
            digitalWrite(MOTOR_PIN_IN2, LOW);
            Serial.println("Fan turned OFF by app");
          }
          else
          {
            Serial.println("Fan status (TRUE)");
            Serial.println("Fan turned ON by app");
            adjustFanSpeed(dht.readTemperature(), dht.readHumidity());
          }
        }
      }
    }
    http.end();
  }
}

void checkFanStatusAndroid()
{
  Serial.print("------- checking fan status (Android)...");
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String url = "http://" + String(serverIP[connectedIndex]) + "/Alpha%20Chill/android/get_sensor_data.php";
    url += "?id_user=" + String(userId);

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);

      Serial.println(" (" + String(httpCode) + ")");
      Serial.println("response (Android): " + String(payload));

      if (!error && doc["status"] == "success")
      {
        lastFanStatusAndroid = doc["data"]["fan_status"].as<bool>();
        Serial.println("Fan status (Android): " + String(lastFanStatusAndroid));
      }
    }
    http.end();
  }
}

void displayLCD(float temperature, float humidity)
{
  lcd.setCursor(12, 0);
  lcd.print(temperature);
  lcd.print("   ");

  lcd.setCursor(11, 1);
  lcd.print(humidity);
  lcd.print("   ");
}

void connectToWiFi()
{
  const unsigned long timeout = 15000; // 15 seconds timeout for each attempt
  const int numNetworks = sizeof(ssid) / sizeof(ssid[0]);

  for (int i = 0; i < numNetworks; i++)
  {
    Serial.print("Connecting to WiFi (");
    Serial.print(ssid[i]);
    Serial.println(")");

    lcd.setCursor(0, 0);
    lcd.print("connecting: ");
    lcd.setCursor(0, 1);
    lcd.print(ssid[i]);

    WiFi.begin(ssid[i], password[i]);

    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout)
    {
      delay(500);
      Serial.print(".");

      lcd.setCursor(14, 1);
      lcd.print(millis() - startAttemptTime);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      connectedIndex = i;
      Serial.println("\nWiFi Connected.");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WiFi connected");
      delay(2000);
      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("Temperature:");
      lcd.setCursor(0, 1);
      lcd.print("Humidity:");

      return; // Exit the function if connected successfully
    }
    else
    {
      Serial.println("\nFailed to connect. Trying next network...");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Failed to");
      lcd.setCursor(0, 1);
      lcd.print("connect");
      delay(2000);
      lcd.clear();

      WiFi.disconnect();
    }
  }

  Serial.println("Failed to connect to any WiFi network.");
}

void sendToAndroid(float temperature, float humidity, int rpm)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String url = "http://" + String(serverIP[connectedIndex]) + "/Alpha%20Chill/android/submit_data.php";
    url += "?temperature=" + String(temperature, 2);
    url += "&humidity=" + String(humidity, 2);
    url += "&rpm=" + String(rpm);
    url += "&id_user=" + String(userId);

    http.begin(url);
    http.addHeader("Accept", "application/json");
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0)
    {
      Serial.println("[Android] Data sent successfully");
    }
    else
    {
      Serial.print("[Android] Error sending data. Code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

void setup()
{
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

float angle = 0;
void loop()
{
  delay(1000);

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(temperature) || isnan(humidity))
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  checkFanStatus();         // Website
  checkFanStatusAndroid();  // Android

  // Gabungkan status: kipas ON jika salah satu ON
  lastFanStatus = lastFanStatusWebsite || lastFanStatusAndroid;

  if (lastFanStatus)
  {
    adjustFanSpeed(temperature, humidity);
  }
  else
  {
    motorSpeed = 0;
    analogWrite(MOTOR_PIN_ENA, motorSpeed);
    digitalWrite(MOTOR_PIN_IN1, LOW);
    digitalWrite(MOTOR_PIN_IN2, LOW);
    Serial.println("Fan turned OFF (sync)");
  }

  int rpm = calculateRPM(motorSpeed);
  Serial.print("Motor Speed: ");
  Serial.print(motorSpeed);
  Serial.print(" (PWM), RPM: ");
  Serial.println(rpm);

  displayLCD(temperature, humidity);

  sendToServer(temperature, humidity, rpm);
  sendToAndroid(temperature, humidity, rpm);

  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
  }
  Serial.print(">");

  Serial.print("temparature:");
  Serial.print(temperature);
  Serial.print(",");

  Serial.print("humidity:");
  Serial.print(humidity);
  Serial.print(",");

  Serial.print("MOTORSPEED:");
  Serial.print(motorSpeed);
  Serial.print(",");

  Serial.print("rpm:");
  Serial.print(rpm);
  Serial.print(",");

  float calculated = (motorSpeed == 0) || (rpm == 0) ? 0 : rpm / motorSpeed;

  Serial.print("R4M:");
  Serial.print(String(calculated));

  Serial.println(); // Writes \r\n
}
