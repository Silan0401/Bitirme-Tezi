#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <MPU9250.h>

// ——————————————————————————
// 1. SoftAP ayarları
//——————————————————————————
const char* apSsid     = "ESP_MPU_AP";
const char* apPassword = "12345678";

ESP8266WebServer server(80);
MPU9250 mpu;

// Ölçülen açı değerleri (kalibrasyon sonrası)
float pitch = 0, roll = 0, yaw = 0;

// ——————————————————————————
// 2. Kalibrasyon Tablosu
//    (Excel’den alınan “Raw” ve “Coefficient”)
//——————————————————————————
const int CAL_SIZE = 19;
const float rawTable[CAL_SIZE] = {
   0.70,  6.12, 11.13, 15.90, 20.90,
  25.68, 30.70, 35.43, 40.47, 45.55,
  50.38, 55.00, 60.13, 65.00, 70.04,
  74.54, 79.85, 84.19, 87.70
};
const float coeffTable[CAL_SIZE] = {
  1.300000, 1.224000, 1.113000, 1.060000, 1.045000,
  1.027200, 1.023333, 1.012286, 1.011750, 1.012222,
  1.007600, 1.000000, 1.002167, 1.000000, 1.000571,
  0.993867, 0.998125, 0.990471, 0.974444
};

// ——————————————————————————
// 3. Kalibrasyon Fonksiyonu
//    - Katsayıları invert eder (1/c)
//    - Çıkan değeri 90° ile sınırlar
//——————————————————————————
float calibratePitch(float raw) {
  float c, invC, result;

  if (raw <= rawTable[0]) {
    c    = coeffTable[0];
    invC = 1.0f / c;
    result = raw * invC;
  } else {
    bool inRange = false;
    for (int i = 0; i < CAL_SIZE - 1; i++) {
      if (raw >= rawTable[i] && raw <= rawTable[i + 1]) {
        float t = (raw - rawTable[i]) / (rawTable[i + 1] - rawTable[i]);
        c    = coeffTable[i] + t * (coeffTable[i + 1] - coeffTable[i]);
        invC = 1.0f / c;
        result = raw * invC;
        inRange = true;
        break;
      }
    }
    if (!inRange) {
      c    = coeffTable[CAL_SIZE - 1];
      invC = 1.0f / c;
      result = raw * invC;
    }
  }

  if (result > 90.0f) result = 90.0f;
  return result;
}

// ——————————————————————————
// 4. Hareketli Ortalama Filtre Değişkenleri
//——————————————————————————
#define FILTER_SIZE 10
float pitchBuffer[FILTER_SIZE] = {0};
int pitchIndex = 0;

// Fonksiyon prototipleri
void handleRoot();
void handleData();

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!mpu.setup(0x68)) {
    Serial.println("MPU9250 bağlantısı başarısız!");
    while (1);
  }

  WiFi.softAP(apSsid, apPassword);
  Serial.print("AP IP: http://"); Serial.println(WiFi.softAPIP());

  server.on("/",    handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
  if (mpu.update()) {
    float rawPitch = mpu.getPitch();
    float rawRoll  = mpu.getRoll();
    float rawYaw   = mpu.getYaw();

    float calibratedPitch = calibratePitch(rawPitch);

    // Hareketli ortalama filtresi uygula
    pitchBuffer[pitchIndex] = calibratedPitch;
    pitchIndex = (pitchIndex + 1) % FILTER_SIZE;

    float sum = 0;
    for (int i = 0; i < FILTER_SIZE; i++) {
      sum += pitchBuffer[i];
    }
    pitch = sum / FILTER_SIZE;

    roll = rawRoll;
    yaw  = rawYaw;
  }
  delay(10);
}

void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP8266 MPU9250 Ölçümleri</title>
  <style>
    body { font-family: Arial; text-align:center; margin:20px; }
    .value-box { display:inline-block; width:30%; margin:10px; }
    canvas { border:1px solid #ccc; }
  </style>
</head>
<body>
  <h1>MPU9250 Açı Ölçümleri </h1>
  <div class="value-box"><h3>Pitch</h3><div id="pitchValue">0.00°</div></div>
  <div class="value-box"><h3>Roll</h3><div id="rollValue">0.00°</div></div>
  <div class="value-box"><h3>Yaw</h3><div id="yawValue">0.00°</div></div>
  <canvas id="angleChart" width="800" height="400"></canvas>

  <script>
    const canvas       = document.getElementById('angleChart');
    const ctx          = canvas.getContext('2d');
    const MAX_POINTS   = 20;
    const degMin       = -90, degMax = 120, degStep = 30;
    const leftPadding  = 60, rightPadding = 40;
    const topPadding   = 20, bottomPadding = 80;
    const chartWidth   = canvas.width  - leftPadding - rightPadding;
    const chartHeight  = canvas.height - topPadding - bottomPadding;
    let dataPoints = [], timeLabels = [];

    function drawAxes() {
      ctx.clearRect(0,0,canvas.width,canvas.height);
      ctx.strokeStyle = '#000'; ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(leftPadding, topPadding);
      ctx.lineTo(leftPadding, topPadding + chartHeight);
      ctx.lineTo(leftPadding + chartWidth, topPadding + chartHeight);
      ctx.stroke();
      ctx.strokeStyle = 'rgba(0,0,0,0.1)';
      ctx.fillStyle   = '#000';
      ctx.textAlign   = 'right';
      ctx.textBaseline= 'middle';
      ctx.font        = '12px Arial';
      for (let d = degMin; d <= degMax; d += degStep) {
        let y = topPadding + chartHeight - ((d - degMin)/(degMax-degMin))*chartHeight;
        ctx.beginPath();
        ctx.moveTo(leftPadding, y);
        ctx.lineTo(leftPadding + chartWidth, y);
        ctx.stroke();
        ctx.fillText(d + '°', leftPadding - 10, y);
      }
    }

    function drawData() {
      if (dataPoints.length < 2) return;
      ctx.strokeStyle = 'red'; ctx.lineWidth = 2;
      ctx.beginPath();
      dataPoints.forEach((val,i) => {
        let x = leftPadding + (i/(MAX_POINTS-1))*chartWidth;
        let y = topPadding + chartHeight - ((val - degMin)/(degMax-degMin))*chartHeight;
        i===0 ? ctx.moveTo(x,y) : ctx.lineTo(x,y);
      });
      ctx.stroke();
      ctx.fillStyle = 'red';
      dataPoints.forEach((val,i) => {
        let x = leftPadding + (i/(MAX_POINTS-1))*chartWidth;
        let y = topPadding + chartHeight - ((val - degMin)/(degMax-degMin))*chartHeight;
        ctx.beginPath();
        ctx.arc(x, y, 4, 0, 2*Math.PI);
        ctx.fill();
      });
      ctx.fillStyle    = '#000';
      ctx.font         = '12px Arial';
      timeLabels.forEach((t,i) => {
        let x = leftPadding + (i/(MAX_POINTS-1))*chartWidth;
        let y = topPadding + chartHeight + 10;
        ctx.save();
        ctx.translate(x, y);
        ctx.rotate(-Math.PI / 4); // 45 derece
        ctx.textAlign = 'right';
        ctx.textBaseline = 'top';
        ctx.fillText(t, 0, 0);
        ctx.restore();

      });
    }

    function updateData() {
      fetch('/data').then(r=>r.json()).then(d=>{
        document.getElementById('pitchValue').textContent = d.pitch.toFixed(2)+'°';
        document.getElementById('rollValue') .textContent = d.roll.toFixed(2) +'°';
        document.getElementById('yawValue')  .textContent = d.yaw.toFixed(2)  +'°';
        let now = new Date().toLocaleTimeString();
        dataPoints.push(d.pitch);
        timeLabels.push(now);
        if(dataPoints.length>MAX_POINTS){
          dataPoints.shift();
          timeLabels.shift();
        }
        drawAxes();
        drawData();
      });
    }

    drawAxes();
    setInterval(updateData,200);
  </script>
</body>
</html>
)rawliteral");
}

void handleData() {
  String json = "{";
       json += "\"pitch\":" + String(pitch) + ",";
       json += "\"roll\":"  + String(roll)  + ",";
       json += "\"yaw\":"   + String(yaw);
       json += "}";
  server.send(200, "application/json",json);
}
