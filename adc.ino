const int adcPin = 1; 
int adcValue = 0;      

void setup() {
    Serial.begin(115200);  
    
}

void loop() {
    adcValue = analogRead(adcPin); 
    float voltage = adcValue * (3.3 / 4095.0); 

    Serial.print("ADC Value: ");
    Serial.print(adcValue);
    Serial.print(" | Voltage: ");
    Serial.println(voltage, 3); 

    delay(100); 
}
