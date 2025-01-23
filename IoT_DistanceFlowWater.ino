#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <vector>
#include <ArduinoJson.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
#define TRIG_PIN 4
#define ECHO_PIN 5
#define LED_PIN 2
#define BUZZER_PIN 15
#define FLOW_SENSOR_PIN 23

#define BOT_TOKEN "input your API"
const char* ssid = "";
const char* password = "";

volatile int flowPulseCount = 0;
float flowRate = 0.0;
float totalVolume = 0.0;
unsigned long previousMillis = 0;
const unsigned long interval = 1000;
String status = "Normal";
String lastStatus = "Normal";

std::vector<String> userChatIDs; // Menyimpan daftar chat ID pengguna

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseCounter, FALLING);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Menghubungkan...");
  delay(2000);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wi-Fi Terkoneksi");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
  lcd.clear();
}

void loop() {
  long duration = getUltrasonicDistance();
  float distance = duration * 0.034 / 2;
  unsigned long currentMillis = millis();

  calculateFlowRate();
  totalVolume += flowRate / 60.0;

  String newStatus = checkStatus(distance);
  if (newStatus != lastStatus) {
    status = newStatus;
    lastStatus = status;
    sendTelegramNotificationToAll();
  }

  displayOnLCD(distance);

  // Periksa pesan masuk untuk mendaftarkan pengguna baru
  handleTelegramMessages();
}

long getUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  return pulseIn(ECHO_PIN, HIGH);
}

void flowPulseCounter() {
  flowPulseCount++;
}

void calculateFlowRate() {
  const float calibrationFactor = 4.5;
  flowRate = (flowPulseCount / calibrationFactor) / (interval / 1000.0);
  flowPulseCount = 0;
}

String checkStatus(float distance) {
  if (distance < 10 || flowRate > 5 || totalVolume > 20) {
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 2000, 500);
    return "Darurat";
  } else if ((distance >= 10 && distance <= 15) || (flowRate >= 3 && flowRate <= 5) || (totalVolume >= 10 && totalVolume <= 20)) {
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 1000, 500);
    return "Siaga";
  } else {
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
    return "Normal";
  }
}

void displayOnLCD(float distance) {
  lcd.setCursor(0, 0);
  lcd.print("Dist: ");
  lcd.print(distance);
  lcd.print(" cm ");
  lcd.setCursor(0, 1);
  lcd.print("Flow: ");
  lcd.print(flowRate);
  lcd.print("L ");
  lcd.print(status);
}

void sendTelegramNotificationToAll() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi tidak terhubung.");
    return;
  }

  HTTPClient http;
  for (String chatID : userChatIDs) {
    String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/sendMessage?chat_id=" + chatID + "&text=Status+sistem%3A%0A" +
                 "Jarak%3A+" + String(getUltrasonicDistance()) + "+cm%0A" +
                 "Aliran%3A+" + String(flowRate) + "+L%2Fmin%0A" +
                 "Volume+Total%3A+" + String(totalVolume) + "+L%0A" +
                 "Status%3A+" + status;
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.println("Notifikasi terkirim ke " + chatID + ". Kode respon: " + String(httpResponseCode));
    } else {
      Serial.println("Gagal mengirim notifikasi ke " + chatID + ". Kode respon: " + String(httpResponseCode));
    }
    http.end();
  }
}

void handleTelegramMessages() {
  HTTPClient http;

  String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/getUpdates";
  http.begin(url);
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, payload);

    JsonArray updates = doc["result"].as<JsonArray>();
    for (JsonObject update : updates) {
      String chatID = update["message"]["chat"]["id"].as<String>();

      // Jika chat ID belum ada, tambahkan ke dalam daftar
      if (std::find(userChatIDs.begin(), userChatIDs.end(), chatID) == userChatIDs.end()) {
        userChatIDs.push_back(chatID);
        Serial.println("Pengguna baru terdaftar dengan chat ID: " + chatID);
      }
    }
  } else {
    Serial.println("Gagal mendapatkan pembaruan dari Telegram. Kode respon: " + String(httpResponseCode));
  }

  http.end();
}
