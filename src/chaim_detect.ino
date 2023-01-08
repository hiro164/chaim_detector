#include <WiFi.h>
#include <WiFiMulti.h>
#include <ssl_client.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "arduinoFFT.h"

#define SWITCH1 12
#define SWITCH2 14
#define SAMPLES 128
#define SAMPLING_FREQUENCY 5000 

arduinoFFT FFT = arduinoFFT();

unsigned int sampling_period_us;
unsigned long microseconds;
double vReal[SAMPLES];
double vImag[SAMPLES];
const char* ssid     = "XXXX";
const char* password = "XXXX";
int count = 0;
int timer = 0;
const int red = 22;
const int green = 23;

void wifiConnect() {
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);  // Wi-Fi接続
  while (WiFi.status() != WL_CONNECTED) {  // Wi-Fi 接続待ち
    digitalWrite(red, HIGH);
    delay(100);
    Serial.printf(".");
  }
  digitalWrite(red, LOW);
  digitalWrite(green, HIGH);
  delay(1000);
  digitalWrite(green, LOW);

  Serial.println("\nwifi connect ok");
}

void send(String message) {
  const char* host = "notify-api.line.me";
  const char* token = "XXXX";
  WiFiClientSecure client;
  Serial.println("Try");
  //LineのAPIサーバに接続
  if (!client.connect(host, 443)) {
    Serial.println("Connection failed");
    digitalWrite(red, HIGH);
    return;
  }
  Serial.println("Connected");
  digitalWrite(red, LOW);
  digitalWrite(green, HIGH);
  delay(1000);
  digitalWrite(green, LOW);
  //リクエストを送信
  String query = String("message=") + message;
  String request = String("") +
                   "POST /api/notify HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Authorization: Bearer " + token + "\r\n" +
                   "Content-Length: " + String(query.length()) +  "\r\n" +
                   "Content-Type: application/x-www-form-urlencoded\r\n\r\n" +
                   query + "\r\n";
  client.print(request);

  //受信終了まで待つ
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
    if (line == "\r") {
      break;
    }
  }

  String line = client.readStringUntil('\n');
  Serial.println(line);
}

void setup() {
  Serial.begin(115200);
  Serial.println("setup start");
  pinMode(SWITCH1, INPUT_PULLUP);
  pinMode(SWITCH2, INPUT_PULLUP);
  // ピンを二つとも出力ピンに割り当て
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  // ２つのピンをオフ（LEDをオフ）
  digitalWrite(red, LOW);
  digitalWrite(green, LOW);
  wifiConnect();
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
}

void loop() {
  timer++;
  if (digitalRead(SWITCH1) == LOW) {
    send("ご飯の時間ですよ");
  } else if (digitalRead(SWITCH2) == LOW) {
    send("風呂の時間ですよ");
  }
  for (int i = 0; i < SAMPLES; i++) {
    microseconds = micros();    

    vReal[i] = analogRead(33);
    vImag[i] = 0;

    while (micros() < (microseconds + sampling_period_us)) {
    }
  }
  // FFT解析
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
  double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);

  if (peak > 2400) { // 2400Hzより大きい周波数を検出
    count++; 
  }
  if (count == 15) { // 2400Hzを15回検出したときメッセージ送信
    send("誰か来たよ");
    delay(1000);
    count = 0;
  }
  if (timer == 3000) { // 3秒ごとにカウンタークリア
    count = 0;
    timer = 0;
  }
  delay(1);
}
