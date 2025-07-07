#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiManager.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2591.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "guille_meteorologia"
#define AIO_KEY         "aio_EZZJ59bzhwxVknuCXFIcP8na3Piz"
#define FEED_UV         "monitor-uv-niederhaus-cmmc-sat"
#define FEED_SOLAR      "irradiancia-solar-niederhaus"

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish uvIndexFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/" FEED_UV);
Adafruit_MQTT_Publish solarFeed   = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/" FEED_SOLAR);

WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);
const int UV_PIN = 34;
float uvIndex = 0.0;
float irradiancia = 0.0;
bool wifiConnected = false;
unsigned long lastLCDUpdate = 0;

const int MAX_READINGS = 150;
float uvReadings[MAX_READINGS];
float solarReadings[MAX_READINGS];
int readingIndex = 0;
int totalReadings = 0;

Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);

const char index_html[] PROGMEM = R"rawliteral(
// CONTENIDO HTML OMITIDO PARA BREVIDAD
)rawliteral";

void setupWiFi() {
  Serial.println("Conectando WiFi...");
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  lcd.clear();
  lcd.print("Conectando WiFi");

  if (!wm.autoConnect("ESP32_UV_Monitor", "12345678")) {
    Serial.println("❌ WiFi FAIL");
    lcd.clear(); lcd.print("Modo AP activo");
  } else {
    wifiConnected = true;
    Serial.println("✅ WiFi OK: " + WiFi.localIP().toString());
    lcd.clear(); lcd.print("WiFi OK");
    lcd.setCursor(0, 1); lcd.print(WiFi.localIP());
    delay(2000);
  }
}

void MQTT_connect() {
  if (mqtt.connected()) return;
  Serial.print("Conectando MQTT... ");
  int8_t ret;
  uint8_t retries = 5;
  while ((ret = mqtt.connect()) != 0) {
    Serial.println("MQTT Error: " + String(mqtt.connectErrorString(ret)));
    mqtt.disconnect();
    delay(5000);
    if (--retries == 0) return;
  }
  Serial.println("✅ MQTT conectado");
}

void setupTSL2591() {
  if (!tsl.begin()) {
    Serial.println("❌ No se encontró TSL2591");
  } else {
    tsl.setGain(TSL2591_GAIN_LOW);
    tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);
    Serial.println("✅ TSL2591 iniciado");
  }
}

float calculateAverage(float readings[], int count) {
  float sum = 0.0;
  for (int i = 0; i < count; i++) sum += readings[i];
  return sum / count;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  lcd.init(); lcd.backlight(); lcd.clear(); lcd.print("Iniciando...");
  delay(1000);

  for (int i = 0; i < MAX_READINGS; i++) {
    uvReadings[i] = 0.0;
    solarReadings[i] = 0.0;
  }

  setupWiFi();
  if (wifiConnected) {
    server.begin();
    server.on("/", HTTP_GET, []() {
      server.send_P(200, "text/html; charset=utf-8", index_html);
    });
    server.on("/datos", HTTP_GET, []() {
      String json = "{";
      json += "\"uv\":" + String(uvIndex, 2) + ",";
      json += "\"irradiancia\":" + String(irradiancia, 1) + ",";
      json += "\"mqtt\":" + String(mqtt.connected() ? "true" : "false");
      json += "}";
      server.send(200, "application/json", json);
    });
  }

  analogReadResolution(12);
  setupTSL2591();
}

void loop() {
  if (wifiConnected) {
    server.handleClient();
    MQTT_connect();
  }

  int uvRaw = analogRead(UV_PIN);
  float voltage = (uvRaw / 4095.0) * 3.3;
  uvIndex = (voltage - 1.0) * (11.0 / (2.8 - 1.0));
  if (uvIndex < 0) uvIndex = 0;

  uint32_t luxRaw = tsl.getFullLuminosity();
  uint16_t ir = luxRaw >> 16;
  uint16_t full = luxRaw & 0xFFFF;

  if (full == 65535 || ir == 65535) {
    Serial.println("⚠️ Sensor saturado");
    irradiancia = 0;
  } else {
    float visibleLux = tsl.calculateLux(full, ir);
    if (visibleLux < 0 || visibleLux > 150000) {
      irradiancia = 0;
    } else {
      irradiancia = visibleLux / 120.0;
    }
  }

  uvReadings[readingIndex] = uvIndex;
  solarReadings[readingIndex] = irradiancia;
  readingIndex = (readingIndex + 1) % MAX_READINGS;
  if (totalReadings < MAX_READINGS) totalReadings++;

  static unsigned long lastPublish = 0;
  if (millis() - lastPublish > 300000) {
    lastPublish = millis();

    if (wifiConnected && mqtt.connected()) {
      float avgUV = calculateAverage(uvReadings, totalReadings);
      float avgSolar = calculateAverage(solarReadings, totalReadings);

      if (uvIndexFeed.publish(avgUV * 2)) {
        Serial.print("✅ UV promedio publicado (x2): ");
        Serial.println(avgUV * 2);
      }
      if (solarFeed.publish(avgSolar)) {
        Serial.print("✅ Irradiancia promedio publicada: ");
        Serial.println(avgSolar);
      }

      readingIndex = 0;
      totalReadings = 0;
    }
  }

  if (millis() - lastLCDUpdate > 2000) {
    lastLCDUpdate = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("UV:");
    lcd.print(uvIndex, 1);
    lcd.print(" Sol:");
    lcd.print(irradiancia, 0);
    lcd.setCursor(0, 1);
    lcd.print(wifiConnected && mqtt.connected() ? "MQTT OK" : "MQTT FAIL");
  }

  delay(2000);
}
