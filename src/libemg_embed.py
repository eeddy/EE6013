import libemg 
import serial 
import time 
from bleak import BleakClient, BleakError
import struct
from libemg.shared_memory_manager import SharedMemoryManager
from multiprocessing import Process
import numpy as np
from multiprocessing import Process, Event, Lock
import os
import asyncio

##########################################################################
class LibEMGEmbed(Process):
    def __init__(self, shared_memory_items: list = []):
        super().__init__(daemon=True)
        self.shared_memory_items = shared_memory_items
        self.num_samples = 16
        self.num_channels = 8
        self.total_floats = self.num_samples * self.num_channels

    def run(self):
        os.system("sudo systemctl stop bluetooth")
        time.sleep(0.55)
        os.system("sudo systemctl start bluetooth")
        time.sleep(2)

        asyncio.run(self.ble_main())

    async def ble_main(self):
        #esp32_address = "7c:df:a1:fb:26:29"
        esp32_address = "7c:df:a1:fb:26:55"  # Power module
        characteristic_uuid = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

        self.smm = SharedMemoryManager()
        for item in self.shared_memory_items:
            self.smm.create_variable(*item)

        def notification_handler(sender, data):
            try:
                readings = struct.unpack('f' * self.total_floats, data)
                emg = np.array(readings, dtype=np.float32).reshape((self.num_samples, self.num_channels))
                
                indx = [True, True, True, False, False, False, False, True]
                for i in range(16):
                    self.smm.modify_variable("emg", lambda x: np.vstack((emg[i, indx], x))[:x.shape[0], :])
                    self.smm.modify_variable("emg_count", lambda x: x + 1)
            except Exception as e:
                print(f"[Notification Error] {e}")

        for attempt in range(5):
            try:
                print(f"Connecting to {esp32_address} (attempt {attempt + 1})...")
                async with BleakClient(esp32_address) as client:
                    if client.is_connected:
                        print("BLE Connected")
                        await client.start_notify(characteristic_uuid, notification_handler)

                        while True:
                            await asyncio.sleep(1)
            except BleakError as e:
                print(f"BLE error: {e}")
                await asyncio.sleep(1)
                
def libemgembed_streamer(shared_memory_items=None):
    if shared_memory_items is None:
        shared_memory_items = []
        shared_memory_items.append(["emg",       (5000, 4), np.float32])
        shared_memory_items.append(["emg_count", (1, 1),  np.int32])

    for item in shared_memory_items:
        item.append(Lock())

    ns = LibEMGEmbed(shared_memory_items)
    ns.start()
    return ns, shared_memory_items

####################################################################
COLLECT = True 

if __name__ == "__main__":
    p, smm = libemgembed_streamer() 
    odh = libemg.data_handler.OnlineDataHandler(smm)
    odh.visualize(num_samples=1000)
    
    if COLLECT:
        args = {'num_reps': 5, 'rep_time': 3, 'rest_time': 1, 'media_folder': 'images/'}
        ui = libemg.gui.GUI(odh, args=args, width=700, height=700)
        ui.download_gestures([1,2,3,4,5], "images/")
        ui.start_gui()

    dataset_folder = 'data/'
    filters = [
            libemg.data_handler.RegexFilter(left_bound="C_", right_bound="_R", values=["0","1","2","3","4"], description='classes'),
            libemg.data_handler.RegexFilter(left_bound="R_", right_bound="_emg.csv", values=["0", "1", "2", "3", "4"], description='reps'), 
    ]
    offline_dh = libemg.data_handler.OfflineDataHandler()
    offline_dh.get_data(folder_location=dataset_folder, regex_filters=filters, delimiter=',')
    train_windows, train_metadata = offline_dh.parse_windows(300, 100)

    fe = libemg.feature_extractor.FeatureExtractor()
    feature_list = ['WENG']
    training_feats = fe.extract_features(feature_list, train_windows)

    dataset = {
            'training_features': training_feats,
            'training_labels': train_metadata['classes'],
    }

    o_classifier = libemg.emg_predictor.EMGClassifier("LDA")
    o_classifier.fit(feature_dictionary=dataset)
    o_classifier.add_rejection(0.9)
    o_classifier.add_velocity(train_windows, train_metadata['classes'])

    classifier = libemg.emg_predictor.OnlineEMGClassifier(o_classifier, 300, 100, odh, feature_list)
    classifier.run(block=False)


    # Create while loop to send data 
    import socket 
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('127.0.0.1', 12346))

    ser_conn = serial.Serial('/dev/ttyACM0', 115200)
    time.sleep(1)

    speed = 4.5

    while True:
        data,_ = sock.recvfrom(1024)
        data = str(data.decode("utf-8"))

        if data:
            gesture = float(data.split(' ')[0])
            velocity = float(data.split(' ')[1])
            
            message = ''
            if gesture == 0:
                message = str(0.0) + ';' + str(-velocity*speed)
            elif gesture == 1:
                message = str(0.0) + ';' + str(velocity*speed)
            elif gesture == 3:
                message = str(velocity*speed) + ';' + str(0.0)
            elif gesture == 4:
                message = str(-velocity*speed) + ';' + str(0.0)
            else:
                message = str(0.0) + ';' + str(0.0)

            if message != '':
                # Send this message
                message += '\n'
                print(message)
                ser_conn.write(message.encode('utf-8'))
