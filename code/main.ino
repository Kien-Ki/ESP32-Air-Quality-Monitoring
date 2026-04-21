#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <DHT.h>
#include <MQ135.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
hd44780_I2Cexp lcd;
// ================= WIFI =================
const char* ssid = "ssid";
const char* password = "pass";
WebServer server(80);
// ================= DHT =================
#define DHTPIN 13
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
// ================= MQ135 =================
#define MQ135_PIN 34
MQ135 gasSensor = MQ135(MQ135_PIN);
// ================= LED =================
#define LED_MQ 33
#define LED_T 32
#define LED_H 27
// ================= BUZZER =================
#define BUZ 26
// ================= RELAY =================
#define RELAY_FAN 16
// ================= NGƯỠNG =================
float TEMP_LIMIT_MAX = 35.0;
float HUM_LIMIT_MAX  = 85.0;
float TEMP_LIMIT_MIN = 15.0;
float HUM_LIMIT_MIN  = 70.0;
float GAS_LIMIT  = 1000.0;
// ================= BIẾN WEB =================
float g_temp = 0;
float g_hum  = 0;
float g_gas  = 0;
// ===== GOOGLE SHEET =====
float sumTemp = 0, sumHum = 0, sumGas = 0;
int countSample = 0;
unsigned long lastSend = 0;
String GAS_ID = "https://script.google.com/macros/s/xxxxxxxxxxxx/exec";
// ================= API JSON =================
void handleData() {
  String json = "{";
  json += "\"temp\":" + String(g_temp) + ",";
  json += "\"hum\":" + String(g_hum) + ",";
  json += "\"gas\":" + String(g_gas) + ",";
  json += "\"t_alert\":" + String(g_temp > TEMP_LIMIT_MAX ? 1 : 0) + ",";
  json += "\"h_alert\":" + String(g_hum > HUM_LIMIT_MAX ? 1 : 0) + ",";
  json += "\"g_alert\":" + String(g_gas > GAS_LIMIT ? 1 : 0);
  json += "}";
  server.send(200, "application/json", json);
}
// ================= WEB UI =================
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Dashboard</title>
  </head>

  <body>

  <style>
  body {
    margin: 0;
    font-family: 'Segoe UI', sans-serif;
    background: linear-gradient(135deg, #cceeff, #99ddff, #66ccff);
    color: white;
    overflow-x: hidden;
    display: flex;
  }
  /* HEADER */
  h1 {
    text-align: center;
    padding: 15px;
    font-size: 26px;
  }
  /* CONTAINER */
  .wrapper {
    width: 100%;
    max-width: 900px;
    display: flex;
    flex-direction: column;
    gap: 20px;
    padding: 15px;
    margin: auto;
  }
  /* CARD */
  .row {
    display: flex;
    align-items: center;
    justify-content: flex-start;
    padding: 15px;
    border-radius: 20px;
    background: rgba(255,255,255,0.6);
    color: #003344;
    backdrop-filter: blur(15px);
    box-shadow: 
      0 4px 20px rgba(0,0,0,0.4),
      inset 0 0 10px rgba(255,255,255,0.05);
    transition: all 0.3s ease;
    gap: 30px;
    padding: 25px;
    margin: 0 auto;
    border: 1px solid rgba(255,255,255,0.08);
    width: 100%;
    max-width: 100%;
    box-sizing: border-box;
  }
  .row.alert {
    box-shadow: 0 0 25px red;
    border: 1px solid red;
    animation: glow 1s infinite alternate;
  }
  @keyframes glow {
    from { box-shadow: 0 0 10px red; }
    to   { box-shadow: 0 0 30px red; }
  }
  .container {
    display: flex;
    width: 100%;
    min-height: 100vh;
  }
  .sidebar {
    width: 25%;
    min-width: 250px;
    display: flex;
    align-items: center;
    justify-content: flex-start;
    padding-left: 20px;
    gap: 4px;
    white-space: nowrap;
    font-size: 30px;
    font-weight: bold;
    background: rgba(255,255,255,0.5);
    color: #003344;
    backdrop-filter: blur(10px);
  }
  .content {
    width: 75%;
    overflow-y: auto;
  }
  /* LEFT */
  .left {
    width: 20%;
    min-width: 120px;

    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 8px;
  }
  .label {
    font-size: 14px;
    opacity: 0.8;
    margin-top: 5px;
  }
  /* LED */
  .middle {
    width: 5%;
    min-width: 40px;
    display: flex;
    justify-content: center;
    align-items: center;
    height: 100%;
  }
  .led {
    width: 22px;
    height: 22px;
    border-radius: 50%;
    background: #333;
    transition: all 0.3s ease;
  }
  .on {
    background: #ff3b3b;
    box-shadow: 0 0 5px #ff3b3b, 0 0 10px #ff3b3b;
  }
  .off {
    background: #007bff;
    box-shadow: none;
  }
  /* CHART */
  .right {
    width: 75%;
    display: flex;
    align-items: center;
  }
  .left canvas {
    width: 120px !important;
    height: 120px !important;
  }
  .right canvas {
    width: 100% !important;
    height: 150px !important;
  }
  canvas {
    width: 100% !important;
    height: auto;
    border-radius: 20px;
  }

  /* 📱 MOBILE */
  @media (max-width: 768px) {
    body {
      display: block;
    }

    .container {
      flex-direction: column;
      height: auto;
    }
    .sidebar {
      width: 100%;
      height: 60px;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 4px;
      font-size: 20px;
      font-weight: 600;
      background: rgba(255,255,255,0.7);
      color: #003344;
      backdrop-filter: blur(10px);
      position: sticky;
      top: 0;
      z-index: 10;
    }
    .content {
      width: 100%;
    }
    .wrapper {
      padding: 10px;
      gap: 15px;
    }
    .row {
      display: flex;
      flex-direction: column; /*  xếp dọc */
      align-items: center;
      padding: 15px;
      border-radius: 20px;
      background: rgba(255,255,255,0.7);
      color: #003344;
      backdrop-filter: blur(10px);
      box-shadow: 0 4px 15px rgba(0,0,0,0.2);
      gap: 10px;
    }
    /* QUAN TRỌNG */
    .left {
      width: 100%;
      align-items: center;
    }
    .middle {
      width: 100%;
      margin: 5px 0;
    }
    .right {
      width: 100%;
    }
    /* Gauge nhỏ lại cho vừa màn */
    .left canvas {
      width: 120px !important;
      height: 120px !important;
      canvas.style.display = "block";
    }
    /* Chart full màn */
    .right canvas {
      width: 100% !important;
      height: 140px !important;
    }
    /* Text nhỏ lại */
    .label {
      font-size: 13px;
    }
    .dark-btn {
      font-size: 18px;
      border: none;
      background: transparent;
      cursor: pointer;
      color: inherit;
      transition: transform 0.2s ease;
    }
    .dark-btn:active {
      transform: scale(0.8); /* hiệu ứng bấm */
    }
  }

  /* ===== DARK MODE ===== */
  body.dark {
    background: #0f172a;
    color: #e2e8f0;
  }
  body.dark .row {
    background: #1e293b;
    color: #e2e8f0;
    box-shadow: 0 0 10px rgba(0,0,0,0.8);
  }
  body.dark .sidebar {
    background: #020617;
    color: #38bdf8;
  }
  body.dark canvas {
    background: #1a1a1a;
  }
  body.dark .label {
    color: #94a3b8;
  }
  .dark-btn {
    font-size: 22px;
    border: none;
    background: transparent; /* không nền */
    cursor: pointer;
    color: inherit; /* cùng màu chữ */
    transition: transform 0.2s ease;
  }
  .dark-btn:hover {
    transform: scale(1.2);
  }
  </style>
  <div class="container">

    <div class="sidebar">
      Air Quality Monitoring
      <button id="darkBtn" onclick="toggleDark()" class="dark-btn">🌙</button>
    </div>

    <div class="content">
    <div class="wrapper">

  <!-- TEMP -->
  <div class="row">
    <div class="left">
      <canvas id="tempGauge" width="150" height="150"></canvas>
      <div class="label">Temperature</div>
    </div>

    <div class="middle">
      <span id="ledT" class="led"></span>
    </div>

    <div class="right">
      <canvas id="tempChart"></canvas>
    </div>
  </div>

  <!-- HUM -->
  <div class="row">
    <div class="left">
      <canvas id="humGauge" width="150" height="150"></canvas>
      <div class="label">Humidity</div>
    </div>

    <div class="middle">
      <span id="ledH" class="led"></span>
    </div>

    <div class="right">
      <canvas id="humChart"></canvas>
    </div>
  </div>

  <!-- GAS -->
  <div class="row">
    <div class="left">
      <canvas id="gasGauge" width="150" height="150"></canvas>
      <div class="label">Air Quality</div>
    </div>

    <div class="middle">
      <span id="ledG" class="led"></span>
    </div>

    <div class="right">
      <canvas id="gasChart"></canvas>
    </div>
  </div>

  <script>
  let tempData = [];
  let humData  = [];
  let gasData  = [];
  let animationProgress = 1;

  function drawGauge(canvasId, value, max, unit, isAlert) {
    let canvas = document.getElementById(canvasId);
    let ctx = canvas.getContext("2d");
    let dpr = window.devicePixelRatio || 1;
    canvas.width = canvas.offsetWidth * dpr;
    canvas.height = canvas.offsetWidth * dpr;
    canvas.style.width = canvas.offsetWidth + "px";
    canvas.style.height = canvas.offsetWidth + "px";
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

    ctx.lineCap = "round";
    ctx.shadowBlur = isAlert ? 25 : 12;

    let size = canvas.width / dpr;   //  kích thước thật
    let centerX = size / 2;
    let centerY = size / 2;
    let radius = size / 2 - 15;

    ctx.clearRect(0,0,canvas.offsetWidth,canvas.offsetWidth);

    ctx.beginPath();
    ctx.arc(centerX, centerY, radius, 0.75*Math.PI, 2.25*Math.PI);
    ctx.strokeStyle = document.body.classList.contains("dark") 
    ? "#334155" 
    : "#111";
    ctx.lineWidth = 10;
    ctx.stroke();

    let angle = 0.75*Math.PI + Math.min(value/max, 1)*(1.5*Math.PI);

    // đổi màu
    let color = document.body.classList.contains("dark")
    ? (isAlert ? "#ff6b6b" : "#38bdf8")
    : (isAlert ? "#ff3b3b" : "#007bff");
    ctx.shadowColor = color; 

    ctx.beginPath();
    ctx.arc(centerX, centerY, radius, 0.75*Math.PI, angle);
    ctx.strokeStyle = color;
    ctx.lineWidth = 10;
    ctx.stroke();

    ctx.fillStyle = document.body.classList.contains("dark") 
    ? "#e2e8f0"   // sáng rõ trong dark mode
    : "#003344";  // giữ nguyên light mode
    ctx.font = "20px Arial";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(value.toFixed(1) + unit, centerX, centerY);
  }

  function drawLineChart(canvasId, data, max, limit, alert, color) {
    let canvas = document.getElementById(canvasId);
    let ctx = canvas.getContext("2d");

    let dpr = window.devicePixelRatio || 1;
    canvas.width = canvas.offsetWidth * dpr;
    canvas.height = 180 * dpr;
    canvas.style.width = canvas.offsetWidth + "px";
    canvas.style.height = "180px";
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    
    ctx.imageSmoothingEnabled = true;
    ctx.lineCap = "round";
    ctx.lineJoin = "round";

    ctx.clearRect(0,0,canvas.width,canvas.height);

    // trục
    ctx.beginPath();
    let w = canvas.offsetWidth;
    let h = 180;

    let paddingTop = 20;
    let paddingBottom = 30;

    // nền đẹp hơn
    let bg = ctx.createLinearGradient(0,0,0,canvas.height);
    if(document.body.classList.contains("dark")){
      bg.addColorStop(0, "#020617");
      bg.addColorStop(1, "#0f172a");
    }else{
      bg.addColorStop(0, "#e6f7ff");
      bg.addColorStop(1, "#cceeff");
    }
    ctx.fillStyle = bg;
    ctx.fillRect(0,0,canvas.width,canvas.height);
    ctx.strokeStyle = "#99bbcc";
    ctx.strokeRect(0,0,canvas.width,canvas.height);

    // trục
    ctx.beginPath();
    ctx.moveTo(40,20);
    ctx.lineTo(40,h-20);
    ctx.lineTo(w-10,h-20);
    ctx.strokeStyle = "#555";
    ctx.stroke();

    if(data.length < 2) return;

    // scale cố định
    let min = 0;
    let maxVal = max;
    
    // ===== GRID =====
    ctx.strokeStyle = document.body.classList.contains("dark") 
    ? "#334155" 
    : "#aacccc";
    ctx.lineWidth = 1;
    ctx.fillStyle = "#003344";
    ctx.font = "12px Arial";
    ctx.textAlign = "right";
    ctx.fillStyle ="#336677";

    for(let i=0;i<=5;i++){
      let y = (h - paddingBottom) - i*((h - paddingTop - paddingBottom)/5);
      ctx.beginPath();
      ctx.moveTo(40,y);
      ctx.lineTo(w-10,y);
      ctx.stroke();
      let value = Math.round(i * (max / 5));
      let unitText = "";
    if(max == 100) unitText = "%";
    if(max == 2000) unitText = "ppm";
      ctx.fillText(value, 35, y + 4);
    }
    ctx.beginPath();

    // ===== VẼ LINE =====
    ctx.beginPath();
    for(let i=0;i<data.length;i++){
      let x = 40 + i * ((w - 60) / (data.length -1));
      let y = (h - paddingBottom) - ((data[i]) / maxVal) * (h - paddingTop - paddingBottom);

      if(i==0) ctx.moveTo(x,y);
      else ctx.lineTo(x,y);
    }
    // màu line
    let lineColor = document.body.classList.contains("dark") 
    ? (alert ? "#ff6b6b" : "#38bdf8") 
    : (alert ? "#ff3b3b" : "#007bff");

    ctx.strokeStyle = lineColor;
    ctx.lineWidth = 2;
    ctx.stroke();

    // CHỈ HIỂN THỊ ĐIỂM CUỐI (đỡ rối)
    // ===== CHẤM CUỐI =====
    let i = data.length - 1;

    let x = 40 + i * ((w - 60) / (data.length -1));
    let y = (h - paddingBottom) - ((data[i]) / maxVal) * (h - paddingTop - paddingBottom);

    // chấm
    ctx.beginPath();
    ctx.arc(x, y, 4, 0, 2*Math.PI);
    ctx.fillStyle = alert ? "#ff0000" : "#0066ff";
    ctx.fill();

    // text nhỏ lại
    ctx.fillStyle = document.body.classList.contains("dark") 
    ? "#e2e8f0" 
    : "#003344";
    ctx.font = "10px Arial";
    ctx.textAlign = "center";

    let text = data[i].toFixed(1);

    if(canvasId.includes("temp")) text += "°C";
    else if(canvasId.includes("hum")) text += "%";
    else text += "ppm";

    ctx.fillText(text, x, y - 8);
  }

  function updateData() {
    fetch("/data")
    .then(res => res.json())
    .then(data => {

      // gauge
      drawGauge("tempGauge", data.temp, 100, "°C", data.t_alert);
      drawGauge("humGauge", data.hum, 100, "%", data.h_alert);
      drawGauge("gasGauge", data.gas, 2000, "ppm", data.g_alert);

      // LED
      document.getElementById("ledT").className = "led " + (data.t_alert ? "on":"off");
      document.getElementById("ledH").className = "led " + (data.h_alert ? "on":"off");
      document.getElementById("ledG").className = "led " + (data.g_alert ? "on":"off");

      // lưu dữ liệu
      tempData.push(data.temp);
      humData.push(data.hum);
      gasData.push(data.gas);

      if(tempData.length > 30) tempData.shift();
      if(humData.length > 30) humData.shift();
      if(gasData.length > 30) gasData.shift();

      // vẽ chart
      animationProgress = 0;

      function animate(){
        animationProgress += 0.08;

        if(animationProgress > 1) animationProgress = 1;

        drawLineChart("tempChart", tempData, 100, 35, data.t_alert, "#3399ff");
        drawLineChart("humChart", humData, 100, 70, data.h_alert, "#3399ff");
        drawLineChart("gasChart", gasData, 2000, 300, data.g_alert, "#3399ff");

        if(animationProgress < 1){
          requestAnimationFrame(animate);
        }
      }
      animate();

      document.querySelectorAll(".row")[0].classList.toggle("alert", data.t_alert);
      document.querySelectorAll(".row")[1].classList.toggle("alert", data.h_alert);
      document.querySelectorAll(".row")[2].classList.toggle("alert", data.g_alert);
    });
  }
  setInterval(updateData, 2500);
  updateData();

  window.onload = function(){
    let btn = document.getElementById("darkBtn");
    if(localStorage.getItem("dark") === "1"){
      document.body.classList.add("dark");
      btn.innerHTML = "☀️";
    }
  }

  function toggleDark(){
    let btn = document.getElementById("darkBtn");
    document.body.classList.toggle("dark");
    if(document.body.classList.contains("dark")){
      btn.innerHTML = "☀️";
      localStorage.setItem("dark","1");
    }else{
      btn.innerHTML = "🌙";
      localStorage.setItem("dark","0");
    }
  }
  </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void sendToGoogleSheet(float t, float h, float g) {
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    String url = GAS_ID + "?temp=" + String(t,1) +
                          "&hum=" + String(h,1) +
                          "&gas=" + String(g,0);

    http.begin(url);
    int httpCode = http.GET();

    Serial.print("Send GGSheet: ");
    Serial.println(httpCode);
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  lcd.begin(16,2);
  lcd.backlight();

  pinMode(LED_MQ, OUTPUT);
  pinMode(LED_T, OUTPUT);
  pinMode(LED_H, OUTPUT);
  pinMode(BUZ, OUTPUT);

  digitalWrite(LED_MQ, LOW);
  digitalWrite(LED_T, LOW);
  digitalWrite(LED_H, LOW);
  digitalWrite(BUZ, LOW);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFI");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.printf(".");
  }
  
  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  pinMode(RELAY_FAN, OUTPUT);
  digitalWrite(RELAY_FAN, LOW); 

  lcd.setCursor(0,0);
  lcd.print("System Start");
  lcd.setCursor(0,1);
  lcd.print("MQ135 Warmup");

  Serial.println("MQ135 warming up...");
  delay(15000);   // warm up 15s
}

void loop() {
  static unsigned long buzzerTimer = 0;
  static bool buzzerState = false;
  static unsigned long lastRead = 0;
  if(millis() - lastRead >= 2500){
    lastRead = millis();
    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();

      // kiểm tra lỗi DHT
    if (isnan(temp) || isnan(hum)) {
      Serial.println("DHT Error");
      return;
    }
      
    // đọc MQ135 trung bình
    float sum = 0;
    for(int i=0;i<10;i++){
      sum += gasSensor.getPPM();
      delay(10);
      server.handleClient();
    }

    float ppm = sum / 10;
    // giới hạn ppm
    if(ppm > 2000) ppm = 2000;

    // Cập nhật web
    g_temp = temp;
    g_hum = hum;
    g_gas = ppm;
    // ===== CỘNG DỒN =====
    sumTemp += temp;
    sumHum  += hum;
    sumGas  += ppm;
    countSample++;
    // serial monitor
    Serial.print("Temp: ");
    Serial.print(temp);
    Serial.print(" C  ");
    Serial.print("Hum: ");
    Serial.print(hum);
    Serial.print(" %  ");
    Serial.print("ARI: ");
    Serial.print(ppm);
    Serial.println(" ppm");
    // LCD
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("T:");
    lcd.print(temp,1);
    lcd.print("C ");
    lcd.print("H:");
    lcd.print(hum,0);
    lcd.print("%");
    lcd.setCursor(0,1);
    lcd.print("ARI:");
    lcd.print(ppm,0);
    lcd.print("ppm");

    // cảnh báo nhiệt độ
    if(temp > TEMP_LIMIT_MAX || temp < TEMP_LIMIT_MIN){  digitalWrite(LED_T, HIGH);  }
      else{ digitalWrite(LED_T, LOW); }
    // cảnh báo độ ẩm
    if(hum > HUM_LIMIT_MAX || hum < HUM_LIMIT_MIN){  digitalWrite(LED_H, HIGH);  }
      else{ digitalWrite(LED_H, LOW); }
    // cảnh báo khí
    if(ppm > GAS_LIMIT){  digitalWrite(LED_MQ, HIGH); }
      else{ digitalWrite(LED_MQ, LOW);  }
    // buzzer cảnh báo
    if(temp < TEMP_LIMIT_MIN || temp > TEMP_LIMIT_MAX || hum < HUM_LIMIT_MIN || hum > HUM_LIMIT_MAX || ppm > GAS_LIMIT){
      if(buzzerState && millis() - buzzerTimer >= 1000){
        buzzerState = false;        // tắt sau 1s
        buzzerTimer = millis();
        digitalWrite(BUZ, LOW);
      }
      if(!buzzerState && millis() - buzzerTimer >= 1500){
        buzzerState = true;         // bật lại sau 4s
        buzzerTimer = millis();
        digitalWrite(BUZ, HIGH);
      }
    }else{
      digitalWrite(BUZ, LOW);
      buzzerState = false;
    }
    // relay
    if(ppm > GAS_LIMIT || hum > HUM_LIMIT_MAX || temp > TEMP_LIMIT_MAX){ 
      digitalWrite(RELAY_FAN, HIGH);  
      } 
      else if(ppm < GAS_LIMIT && hum < HUM_LIMIT_MAX && temp < TEMP_LIMIT_MAX){
        digitalWrite(RELAY_FAN, LOW); 
        }
  }
  // ===== GỬI GOOGLE SHEET MỖI 60s =====
  if(millis() - lastSend > 60000 && countSample > 0){
    lastSend += 60000;
    float avgTemp = sumTemp / countSample;
    float avgHum  = sumHum / countSample;
    float avgGas  = sumGas / countSample;
    sendToGoogleSheet(avgTemp, avgHum, avgGas);
    // reset
    sumTemp = 0;
    sumHum  = 0;
    sumGas  = 0;
    countSample = 0;
    Serial.println("Sent average to Google Sheet");
  }
  server.handleClient();
}
