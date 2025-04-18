import asyncio
from bleak import BleakClient, BleakError
import struct
import time
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import threading
import os
from scipy.signal import butter, lfilter
import pandas as pd
from datetime import datetime

# # Linux
# os.system("sudo systemctl stop bluetooth")
# time.sleep(0.55)
# os.system("sudo systemctl start bluetooth")
# time.sleep(2)

# Windows
os.system("netsh interface set interface \"Bluetooth Network Connection\" admin=disable")
time.sleep(0.55)
os.system("netsh interface set interface \"Bluetooth Network Connection\" admin=enable")
time.sleep(2)

def butter_bandpass(lowcut, highcut, fs, order=5):
    return butter(order, [lowcut, highcut], fs=fs, btype='band')

def butter_bandpass_filter(data, lowcut=58, highcut=62, fs=2000, order=5):
    b, a = butter_bandpass(lowcut, highcut, fs, order=order)
    y = lfilter(b, a, data)
    return y

esp32_address = "7c:df:a1:fb:26:55"  # Power module
# esp32_address = "7c:df:a1:fb:26:29"
characteristic_uuid = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

num_samples = 16
num_channels = 8
total_floats = num_samples * num_channels
plot_len = 4000

fig, axs = plt.subplots(num_channels, 2, figsize=(12, 16))
ydata = [[] for _ in range(num_channels)] 
lines = [[None, None] for _ in range(num_channels)]  

for i in range(num_channels):
    for j in range(2):
        ax = axs[i][j] if num_channels > 1 else axs[j]
        line, = ax.plot([], [], lw=1.5)
        ax.set_xlim(0, plot_len)
        ax.set_ylim(-1, 4)
        ax.set_title(f'Channel {i+1} {"Raw" if j == 0 else "Filtered"}')
        lines[i][j] = line

last_receive_time = None
def notification_handler(sender, data):
    global last_receive_time
    try:
        now = time.perf_counter()
        if last_receive_time is not None:
            delta_ms = (now - last_receive_time) * 1000
            print(f"Time since last packet: {delta_ms:.2f} ms")
        last_receive_time = now

        readings = struct.unpack('f' * total_floats, data)
        for i in range(num_samples):
            for ch in range(num_channels):
                value = readings[i * num_channels + ch]
                ydata[ch].append(value)
    except Exception as e:
        print(f"Unpack error: {e}")

async def read_data():
    for attempt in range(5):
        try:
            print(f"Connecting to {esp32_address}...")
            async with BleakClient(esp32_address) as client:
                if client.is_connected:
                    print("Connected!")
                    await client.start_notify(characteristic_uuid, notification_handler)
                    while True:
                        await asyncio.sleep(1)
        except BleakError as e:
            print(f"BLE error: {e}")
            await asyncio.sleep(1)

def update(frame):
    for ch in range(num_channels):
        raw = ydata[ch][-plot_len:]
        filt = butter_bandpass_filter(ydata[ch][-plot_len - 1000:])[-plot_len:]
        x = list(range(len(raw)))
        lines[ch][0].set_data(x, raw)
        lines[ch][1].set_data(x, filt)
    return [line for pair in lines for line in pair]

def run_bt():
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(read_data())

bt = threading.Thread(target=run_bt)
bt.start()

try:
    ani = animation.FuncAnimation(fig, update, interval=10, blit=True)
    plt.tight_layout()
    plt.show()
except KeyboardInterrupt:
    print("\nSaving data to CSV...")

    min_len = min(len(ch_data) for ch_data in ydata)
    trimmed_data = [ch[:min_len] for ch in ydata]

    df = pd.DataFrame({f"Channel {i+1}": trimmed_data[i] for i in range(num_channels)})
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"adc_data_{timestamp}.csv"
    df.to_csv(filename, index=False)

    print(f"Data saved to {filename}")