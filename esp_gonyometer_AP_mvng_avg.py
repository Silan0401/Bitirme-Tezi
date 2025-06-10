import requests
import time
import matplotlib.pyplot as plt
import matplotlib.animation as animation

ESP_IP = '192.168.4.1'

rawTable = [
   0.70,  6.12, 11.13, 15.90, 20.90,
  25.68, 30.70, 35.43, 40.47, 45.55,
  50.38, 55.00, 60.13, 65.00, 70.04,
  74.54, 79.85, 84.19, 87.70
]
coeffTable = [
  1.300000, 1.224000, 1.113000, 1.060000, 1.045000,
  1.027200, 1.023333, 1.012286, 1.011750, 1.012222,
  1.007600, 1.000000, 1.002167, 1.000000, 1.000571,
  0.993867, 0.998125, 0.990471, 0.974444
]

def calibrate_pitch(raw):
    if raw <= rawTable[0]:
        inv_c = 1.0 / coeffTable[0]
        result = raw * inv_c
    else:
        in_range = False
        for i in range(len(rawTable) - 1):
            if rawTable[i] <= raw <= rawTable[i + 1]:
                t = (raw - rawTable[i]) / (rawTable[i + 1] - rawTable[i])
                c = coeffTable[i] + t * (coeffTable[i + 1] - coeffTable[i])
                inv_c = 1.0 / c
                result = raw * inv_c
                in_range = True
                break
        if not in_range:
            inv_c = 1.0 / coeffTable[-1]
            result = raw * inv_c

    return min(result, 90.0)

# Hareketli ortalama fonksiyonu
def moving_average(data, window_size=10):
    if len(data) < window_size:
        return sum(data) / len(data)
    else:
        return sum(data[-window_size:]) / window_size

raw_data = []
calibrated_data = []
timestamps = []
MAX_POINTS = 20

fig, ax = plt.subplots()
line_raw, = ax.plot([], [], 'o-', label='Raw Pitch')
line_cal, = ax.plot([], [], 'o-', label='Filtered & Calibrated Pitch')

# Bilgi kutusu
info_text = ax.text(0.97, 0.95, '', transform=ax.transAxes,
                    verticalalignment='top', horizontalalignment='right',
                    bbox=dict(facecolor='white', edgecolor='gray', boxstyle='round,pad=0.3'),
                    fontsize=9)

ax.set_ylim(-10, 100)
ax.set_ylabel("Derece")
ax.set_xlabel("Zaman")
ax.set_title("Gerçek Zamanlı Pitch Değeri (Filtreli)")
ax.legend()
ax.grid(True)
plt.xticks(rotation=45)

def update(frame):
    try:
        response = requests.get(f"http://{ESP_IP}/data", timeout=1)
        data = response.json()
        raw_pitch = data['pitch']
        roll = data['roll']
        yaw = data['yaw']

        raw_data.append(raw_pitch)
        if len(raw_data) > MAX_POINTS:
            raw_data.pop(0)

        filtered_pitch = moving_average(raw_data)
        calibrated_pitch = calibrate_pitch(filtered_pitch)

        now = time.strftime("%H:%M:%S")
        calibrated_data.append(calibrated_pitch)
        timestamps.append(now)

        if len(calibrated_data) > MAX_POINTS:
            calibrated_data.pop(0)
            timestamps.pop(0)

        # Konsola yaz
        print(f"Roll: {roll:.2f}°, Pitch: {calibrated_pitch:.2f}° (Filtreli), Yaw: {yaw:.2f}°")

        line_raw.set_data(range(len(raw_data)), raw_data)
        line_cal.set_data(range(len(calibrated_data)), calibrated_data)
        ax.set_xlim(0, MAX_POINTS - 1)
        ax.set_xticks(range(len(timestamps)))
        ax.set_xticklabels(timestamps)

        info_text.set_text(f"Pitch: {calibrated_pitch:.2f}°\nRoll: {roll:.2f}°\nYaw: {yaw:.2f}°")

    except Exception as e:
        print("Bağlantı hatası:", e)

    return line_raw, line_cal, info_text

ani = animation.FuncAnimation(fig, update, interval=200)
plt.tight_layout()
plt.show()
