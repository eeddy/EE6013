import libemg 
import serial 
import time 

COLLECT = True 

if __name__ == "__main__":
    p, smm = libemg.streamers.myo_streamer() 
    odh = libemg.data_handler.OnlineDataHandler(smm)
    
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
    train_windows, train_metadata = offline_dh.parse_windows(30, 15)

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

    classifier = libemg.emg_predictor.OnlineEMGClassifier(o_classifier, 30, 15, odh, feature_list)
    classifier.run(block=False)


    # Create while loop to send data 
    import socket 
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('127.0.0.1', 12346))

    ser_conn = serial.Serial('/dev/ttyACM1', 115200)
    time.sleep(1)

    speed = 3.5 

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
