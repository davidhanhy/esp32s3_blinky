#ifndef WIFICONFIG_H
#define WIFICONFIG_H

#include <Arduino.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Ticker.h>

// --- CẤU HÌNH PIN CHO WIFI MANAGER (N16R8 Safe) ---
#define LED_STATUS_PIN 48 // Đèn RGB trên board báo trạng thái
#define BTN_RESET_PIN 0   // Nút BOOT dùng để Reset cài đặt
#define PUSHTIME 5000     // Giữ 5s để xóa Wifi

// --- CÁC ĐỐI TƯỢNG TOÀN CỤC ---
WebServer webServer(80);
Ticker blinker;

// Biến quản lý trạng thái
String ssid;
String password;
unsigned long lastTimePress = 0;
int wifiMode = 0; // 0: Config (AP), 1: Connected (STA), 2: Lost
unsigned long blinkTime = 0;

// --- HTML GIAO DIỆN ---
const char html[] PROGMEM = R"html( 
  <!DOCTYPE html>
  <html>
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>ESP32 WIFI CONFIG</title>
        <style>
          body{font-family: sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; background: #f0f0f0; margin: 0;}
          .container{background: white; padding: 20px; border-radius: 8px; box-shadow: 0 0 10px rgba(0,0,0,0.1); text-align: center; width: 90%; max-width: 400px;}
          h3 { color: #333; }
          button{width: 100%; height: 40px; margin-top: 10px; border-radius: 5px; border: none; cursor: pointer; font-weight: bold; font-size: 16px;}
          input, select{width: 100%; height: 40px; margin-bottom: 10px; box-sizing: border-box; padding: 5px; border: 1px solid #ccc; border-radius: 4px;}
          .btn-save{background-color: #007bff; color: white;}
          .btn-restart{background-color: #dc3545; color: white;}
          label { display: block; text-align: left; margin-bottom: 5px; font-weight: bold; }
        </style>
    </head>
    <body>
      <div class="container">
        <h3>CAU HINH WIFI</h3>
        <p id="info" style="color: gray;">Dang quet wifi...</p>
        <label>Ten Wifi:</label>
        <select id="ssid"></select>
        <label>Mat khau:</label>
        <input id="password" type="password" placeholder="Nhap mat khau...">
        <button class="btn-save" onclick="saveWifi()">LUU CAI DAT</button>
        <button class="btn-restart" onclick="reStart()">KHOI DONG LAI</button>
      </div>
      <script>
        window.onload=scanWifi;
        function scanWifi(){
          fetch("/scanWifi").then(r=>r.json()).then(data=>{
             document.getElementById("info").innerHTML = "Da quet xong!";
             let sel = document.getElementById("ssid");
             sel.innerHTML = "";
             data.forEach(net => {
               let opt = document.createElement("option");
               opt.text = net; opt.value = net; sel.add(opt);
             });
          });
        }
        function saveWifi(){
          let s = document.getElementById("ssid").value;
          let p = document.getElementById("password").value;
          if(!s) { alert("Chua chon Wifi!"); return; }
          document.getElementById("info").innerHTML = "Dang luu...";
          fetch("/saveWifi?ssid="+encodeURIComponent(s)+"&pass="+encodeURIComponent(p))
            .then(r=>r.text()).then(res => { alert(res); });
        }
        function reStart(){
          if(confirm("Ban muon khoi dong lai?")) {
            fetch("/reStart");
          }
        }
      </script>
    </body>
  </html>
)html";

// --- HÀM HỖ TRỢ ---
void blinkLed(uint32_t t)
{
  if (millis() - blinkTime > t)
  {
    digitalWrite(LED_STATUS_PIN, !digitalRead(LED_STATUS_PIN));
    blinkTime = millis();
  }
}

// Hàm này được Ticker gọi mỗi 50ms để xử lý LED trạng thái
void ledControl()
{
  // 1. Xử lý nút nhấn Reset Wifi
  if (digitalRead(BTN_RESET_PIN) == LOW)
  {
    if (lastTimePress == 0)
      lastTimePress = millis(); // Bắt đầu bấm

    // Nếu giữ lâu -> Nháy cực nhanh báo hiệu sắp reset
    if (millis() - lastTimePress > 3000)
      blinkLed(50);
    else
      blinkLed(200);
  }
  else
  {
    lastTimePress = 0; // Nhả nút
    // 2. Xử lý hiển thị trạng thái Wifi
    if (wifiMode == 0)
      blinkLed(100); // Mode AP (Chưa có wifi): Nháy nhanh
    else if (wifiMode == 1)
    {
      // digitalWrite(LED_STATUS_PIN, LOW); // Đã kết nối: Tắt LED (hoặc HIGH tùy board)
    }
    else if (wifiMode == 2)
      blinkLed(1000); // Mất kết nối: Nháy chậm
  }
}

// Xử lý sự kiện Wifi (Connect/Disconnect)
// Xử lý sự kiện Wifi (Connect/Disconnect)
void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
  // SỬA Ở ĐÂY: Xóa "Arduino_event_t::" đi, chỉ để lại tên sự kiện
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    Serial.println("WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    wifiMode = 1;
    break;

  // SỬA Ở ĐÂY LUÔN:
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    Serial.println("WiFi Lost. Reconnecting...");
    wifiMode = 2;
    WiFi.begin(ssid.c_str(), password.c_str());
    break;

  default:
    break;
  }
}

// --- CLASS CONFIG CHÍNH ---
class Config
{
public:
  void begin()
  {
    pinMode(LED_STATUS_PIN, OUTPUT);
    pinMode(BTN_RESET_PIN, INPUT_PULLUP);

    // Timer 50ms kiểm tra nút bấm và nháy đèn
    blinker.attach_ms(50, ledControl);

    // Khởi tạo EEPROM
    EEPROM.begin(100);
    ssid = EEPROM.readString(0);
    password = EEPROM.readString(32);

    if (ssid.length() > 0)
    {
      Serial.println("Connecting to saved Wifi: " + ssid);
      setupWifi(true); // Mode Station
    }
    else
    {
      Serial.println("No Wifi saved. Starting AP Mode.");
      setupWifi(false); // Mode AP
    }

    // Chỉ bật Web Server khi ở chế độ AP (Cấu hình)
    if (wifiMode == 0)
      setupWebServer();
  }

  void setupWifi(bool isStation)
  {
    if (isStation)
    {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid.c_str(), password.c_str());
      WiFi.onEvent(WiFiEvent);
    }
    else
    {
      WiFi.mode(WIFI_AP);
      // Tạo tên Wifi riêng biệt theo MAC Address
      String apName = "ESP32-CONFIG-" + String((uint32_t)ESP.getEfuseMac(), HEX);
      WiFi.softAP(apName.c_str());
      Serial.println("AP Created: " + apName);
      Serial.println("Config IP: " + WiFi.softAPIP().toString());
      wifiMode = 0;
    }
  }

  void setupWebServer()
  {
    webServer.on("/", []()
                 { webServer.send(200, "text/html", html); });

    webServer.on("/scanWifi", []()
                 {
      int n = WiFi.scanNetworks();
      DynamicJsonDocument doc(1024);
      for (int i = 0; i < n; ++i) doc.add(WiFi.SSID(i));
      String s; serializeJson(doc, s);
      webServer.send(200, "application/json", s); });

    webServer.on("/saveWifi", []()
                 {
      String s = webServer.arg("ssid");
      String p = webServer.arg("pass");
      Serial.println("Saving: " + s + " / " + p);
      EEPROM.writeString(0, s);
      EEPROM.writeString(32, p);
      EEPROM.commit();
      webServer.send(200, "text/plain", "Saved! Device will restart...");
      delay(1000); ESP.restart(); });

    webServer.on("/reStart", []()
                 { 
      webServer.send(200, "text/plain", "Restarting...");
      delay(1000); ESP.restart(); });

    webServer.begin();
  }

  void run()
  {
    // Logic Reset EEPROM khi giữ nút BOOT 5s
    if (digitalRead(BTN_RESET_PIN) == LOW)
    {
      if (lastTimePress > 0 && (millis() - lastTimePress > PUSHTIME))
      {
        Serial.println("Factory Reset...");
        for (int i = 0; i < 100; i++)
          EEPROM.write(i, 0);
        EEPROM.commit();
        delay(500); // Chờ ghi xong
        ESP.restart();
      }
    }

    // Xử lý Client khi ở chế độ cấu hình
    if (wifiMode == 0)
      webServer.handleClient();
  }
};

#endif