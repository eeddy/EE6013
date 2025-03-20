import asyncio
from bleak import BleakClient, BleakError
import struct


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
                            voltage = struct.unpack('f', value)[0] 
                            print(f"Received Voltage: {voltage:.3f} V")
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