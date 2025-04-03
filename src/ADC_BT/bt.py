import asyncio
from bleak import BleakClient, BleakError
import struct
import time
import os

# Linux
os.system("sudo systemctl stop bluetooth")
time.sleep(0.55)
os.system("sudo systemctl start bluetooth")
time.sleep(2)

# # Windows
# os.system("netsh interface set interface \"Bluetooth Network Connection\" admin=disable")
# time.sleep(0.55)
# os.system("netsh interface set interface \"Bluetooth Network Connection\" admin=enable")
# time.sleep(2)

esp32_address = "7c:df:a1:fb:26:55" # ESP #1
characteristic_uuid = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

async def read_data():
    retries = 5  
    for attempt in range(retries):
        try:
            print(f"Attempting to connect to {esp32_address}...")
            async with BleakClient(esp32_address) as client:
                if client.is_connected:
                    print(f"Connected to {esp32_address}")
                    while True:
                        try:
                            value = await client.read_gatt_char(characteristic_uuid)

                            # Unpack two floats (8 bytes total)
                            voltage1, voltage2 = struct.unpack('ff', value)
                            print(f"Received Voltage 1: {voltage1:.3f} V ; Received Voltage 2: {voltage2:.3f} V")
                        except Exception as e:
                            print(f"Error reading data: {e}")
                            break
                        # await asyncio.sleep(1)
                else:
                    print("Failed to connect.")
                    break
        except BleakError as e:
            print(f"Failed to connect or read data: {e}")
            await asyncio.sleep(1) 
    else:
        print("Unable to establish a stable connection after several retries.")

loop = asyncio.get_event_loop()
loop.run_until_complete(read_data())