#include <Arduino.h>

#include <DHT.h>
#include <OneWire.h>
#include <Adafruit_BMP280.h> //CHECK #define BMP280_ADDRESS mine works with (0x76)
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <BH1750.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DHTPIN 1 // GPIO1 (Tx)
#define DHTTYPE DHT11
#define ONE_WIRE_BUS 3 // GPIO3=Rx

const char *ssid = "";         // YOUR WIFI SSID
const char *password = "";  // YOUR WIFIPASS
const char *host = ""; // Site iot project
const String sensor = "home";       // Sensor name!

const int httpsPort = 443;                                                                        //Адрес порта для HTTPS= 443 или HTTP = 80
const char fingerprint[] PROGMEM = "64:3C:48:E2:04:12:47:13:51:72:AA:EA:F8:5A:A2:D4:10:02:20:6A"; //ключ для шифрования

// DHT11 stuff
float temperature_buiten;
float temperature_buiten2;
DHT dht(DHTPIN, DHTTYPE, 15);

// BMP280
Adafruit_BMP280 bmp;
// BH1750
BH1750 lightMeter;

bool sendDataSSL(String url);

void setup()
{
  // I2C stuff
  Wire.begin(0, 2);
  // DHT1
  dht.begin();
  // BMP280
  if (!bmp.begin())
  {
    //   Serial.println("No BMP280");
  }
  if (!lightMeter.begin())
  {
    //   Serial.println("No BH1750");
  }
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
  lcd.init();
  lcd.backlight();
}

void loop()
{
  // DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature))
  {
    // sendDataSSL("/rest/meteo/" + sensor + "/nodht");
    delay(1000);
    return;
  }
  // BMP280
  String t = String(bmp.readTemperature());
  String p = String(bmp.readPressure());
  // BH1750
  float lux = lightMeter.readLightLevel();

  String url = "/rest/meteo/" + sensor + "/?field1=";
  url += String(temperature); // DHT11 CELSIUS
  url += "&field2=";
  url += String(humidity); // DHT11 RELATIVE HUMIDITY
  url += "&field3=";
  url += String(bmp.readTemperature()); // BMP280 CELSIUS
  url += "&field4=";
  url += String(bmp.readPressure() / 100); // BMP280 MILLIBAR
  url += "&field5=";
  url += String(bmp.readAltitude(1013.25)); // BMP280 METER
  url += "&field6=";
  url += String((temperature + bmp.readTemperature()) / 2); // DHT11 + BMP280 AVERAGE CELSIUS
  url += "&field7=";
  url += String(lux); // BH1750 lx

  sendDataSSL(url);

  lcd.setCursor(0, 0);
  lcd.print("t " + String((temperature + bmp.readTemperature()) / 2) + "c ");
  lcd.print(String(humidity) + "%");
  lcd.setCursor(0, 1);
  lcd.print(String(bmp.readPressure() / 100) + " ");
  lcd.print(String(lux) + "lx  ");

  delay(1000);
}

bool sendDataSSL(String url)
{
  WiFiClientSecure httpsClient;            //Обьявляем обьект класса WiFiClient
  httpsClient.setFingerprint(fingerprint); //Присваиваем значения ключа для шифрования
  httpsClient.setTimeout(15000);           //Присваиваем значение паузы (15 секунд)
  delay(1000);                             //Ждем
  int r = 0;                               //Обьявляем переменную счетчика попыток подключения
  while ((!httpsClient.connect(host, httpsPort)) && (r < 30))
  {
    delay(100);
    r++;
  }
  httpsClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
                    "Host: " + host + "\r\n" +
                    "Connection: close\r\n\r\n"); //Отправляем GET запрос через ESP
  while (httpsClient.connected())                 //Ловим ответ веб сервера
  {
    String line = httpsClient.readStringUntil('\n');
    if (line == "\r")
    {
      break;
    }
  }
  String line; //Формируем строку для ответа веб сервера
  while (httpsClient.available())
  {
    // line = httpsClient.readStringUntil('\n');
    // Serial.println(line);
  } //Пишем в UART строку от веб сервера
  return true;
}