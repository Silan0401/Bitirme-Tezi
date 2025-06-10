#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <MPU9250.h>

// ——————————————————————————
// 1. Wi-Fi ayarları
//——————————————————————————
const char* ssid     = "POCO M3";
const char* password = "9876543210";

ESP8266WebServer server(80);
MPU9250 mpu;

// ——————————————————————————
// 2. Hareketli Ortalama (Moving Average)
//——————————————————————————
const int MA_WINDOW = 10;
float pitchBuf[MA_WINDOW] = {0}, rollBuf[MA_WINDOW] = {0}, yawBuf[MA_WINDOW] = {0};
int   bufIndex = 0, bufCount = 0;
float sumPitch = 0, sumRoll = 0, sumYaw = 0;
float pitch = 0, roll = 0, yaw = 0;

// ——————————————————————————
// 3. Kalibrasyon Tablosu (Excel’den alınan “Raw” ve “Coefficient”)
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
// 4. Kalibrasyon Fonksiyonu
//    - Katsayıları invert eder (1/c)
//    - Çıkan değeri 90° ile sınırlar
//——————————————————————————
float calibratePitch(float raw) {
  float c, invC, result;

  // En düşük aralık
  if (raw <= rawTable[0]) {
    c    = coeffTable[0];
    invC = 1.0f / c;
    result = raw * invC;
  } else {
    bool inRange = false;
    // Tablo arasında lineer interpolasyon
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
    // En yüksek aralık
    if (!inRange) {
      c    = coeffTable[CAL_SIZE - 1];
      invC = 1.0f / c;
      result = raw * invC;
    }
  }

  // 90° üstünü kırp
  if (result > 90.0f) result = 90.0f;
  return result;
}

// Fonksiyon prototipleri
void handleRoot();
void handleData();

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // MPU9250 başlatma
  if (!mpu.setup(0x68)) {
    Serial.println("MPU9250 bağlantısı başarısız!");
    while (1);
  }

  // Wi-Fi bağlantısı
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Bağlandı! IP adresi: ");
  Serial.println(WiFi.localIP());

  // HTTP route’ları
  server.on("/",    handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("HTTP sunucusu başladı");
}

void loop() {
  server.handleClient();

  // MPU'dan ham açı al, moving average ve kalibrasyon uygulama
  if (mpu.update()) {
    float rawPitch = mpu.getPitch();
    float rawRoll  = mpu.getRoll();
    float rawYaw   = mpu.getYaw();

    // 1) Eski değeri çıkar, yeniyi ekle
    sumPitch += rawPitch - pitchBuf[bufIndex];
    sumRoll  += rawRoll  - rollBuf[bufIndex];
    sumYaw   += rawYaw   - yawBuf[bufIndex];

    // 2) Buffer güncelle
    pitchBuf[bufIndex] = rawPitch;
    rollBuf[bufIndex]  = rawRoll;
    yawBuf[bufIndex]   = rawYaw;

    // 3) İndeksi döngüsel ilerlet, ilk 10 ölçümde sayacı artır
    bufIndex = (bufIndex + 1) % MA_WINDOW;
    if (bufCount < MA_WINDOW) bufCount++;

    // 4) Ortalama hesapla
    pitch = sumPitch / bufCount;
    roll  = sumRoll  / bufCount;
    yaw   = sumYaw   / bufCount;

    // 5) Ortalama pitch'e kalibrasyon ve 90° sınırla
    pitch = calibratePitch(pitch);
  }

  delay(10);
}

// Ana sayfa: HTML + Chart.js
void handleRoot() {
  String html = R"=====( 
<!DOCTYPE html>
<html>
<head>
  <title>Kalibreli MPU9250 Ölçümleri</title>
  <meta charset="UTF-8">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body { font-family: Arial; margin: 20px; }
    .container { max-width: 800px; margin: 0 auto; text-align: center; }
    .value-box { display: inline-block; width: 30%; margin: 10px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Kalibrasyonlu Açı Ölçümleri</h1>
    <div class="value-box"><h3>Pitch</h3><div id="pitchValue">0.00°</div></div>
    <div class="value-box"><h3>Roll</h3><div id="rollValue">0.00°</div></div>
    <div class="value-box"><h3>Yaw</h3><div id="yawValue">0.00°</div></div>
    <canvas id="angleChart" width="800" height="400"></canvas>
  </div>
  <script>
    const ctx = document.getElementById('angleChart').getContext('2d');
    const chart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [], datasets: [{
          label: 'Pitch',
          borderColor: 'rgb(255,99,132)',
          backgroundColor: 'rgba(255,99,132,0.2)',
          data: [], fill: false, tension: 0.1
        }]
      },
      options: {
        responsive: false,
        scales: {
          x: { grid: { display: true, color: 'rgba(0,0,0,0.1)' } },
          y: { min: -180, max: 180, grid: { display: true, color: 'rgba(0,0,0,0.1)' } }
        }
      }
    });

    function updateData() {
      fetch('/data')
        .then(r => r.json())
        .then(d => {
          document.getElementById('pitchValue').textContent = d.pitch.toFixed(2) + '°';
          document.getElementById('rollValue').textContent  = d.roll.toFixed(2)  + '°';
          document.getElementById('yawValue').textContent   = d.yaw.toFixed(2)   + '°';
          if (chart.data.labels.length > 20) {
            chart.data.labels.shift();
            chart.data.datasets[0].data.shift();
          }
          const t = new Date().toLocaleTimeString();
          chart.data.labels.push(t);
          chart.data.datasets[0].data.push(d.pitch);
          chart.update();
        });
    }
    setInterval(updateData, 500);
  </script>
</body>
</html>
)=====";
  server.send(200, "text/html", html);
}

// JSON endpoint: kalibreli ortalama değerler
void handleData() {
  String json = "{";
  json += "\"pitch\":" + String(pitch) + ",";
  json += "\"roll\":"  + String(roll)  + ",";
  json += "\"yaw\":"   + String(yaw);
  json += "}";
  server.send(200, "application/json",json);
}