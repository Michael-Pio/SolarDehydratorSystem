// TO DO
// Add code to communicate and exchange info
// Add code to control digital pins and read sensor DHT and voltage readings

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <TZ.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>
#include "presetValues.h"
#include <DHT.h>
 

#define pin_LED1 D5 //Indicates WIFI connection
#define pin_LED2 D6 //Indicates MQTT connection
#define pin_LED3 D7 //Indicates ARM State
#define pin_BUZZ D1 //Buzzer
#define pin_LDR A0
#define pin_BTN1 D2 //TEMP
#define pin_BTN2 D3 //TEMP


DHT dhtSensor(2,DHT11);  

// Update these with values suitable for your network.
const char* ssid = "PioAndroid";
const char* password = "12345678";
const char* mqtt_server = "5ba3cc9682464c34888197ac60a3b249.s1.eu.hivemq.cloud";


// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
BearSSL::CertStore certStore;
WiFiClientSecure espClient;
PubSubClient * client;
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (500)
char msg[MSG_BUFFER_SIZE];
// int value = 0;

PresetValues presetValues;

struct DeviceState{
  int Temp = 36,Humid = 60;
  int TresTemp = 100 , TresHumid = 100;
  int Mode = 0;
}deviceState;



//Display Setup + Menu system


// Menu variables
int currentMode = 1;       // 1: Show Mode, 2: Temp/Humidity, 3: Select Veggies
int selectedVeggie = 0;    // Index for veggie selection
unsigned long previousMillis = 0;
const long interval = 3000; // 3 seconds delay to switch screens
int veggieIndex = 0;       // 0: Onion, 1: Potato, 2: Banana, 3: Pulses

// Array of vegetables
String veggies[4] = {"APPLE", "MUSHROOM", "BANANA", "HERBS"};


void update_TemperatureHumidity(){
  Serial.println("Updating the Device Temperature and Humidity");
  deviceState.Temp = dhtSensor.readTemperature();
  deviceState.Humid = dhtSensor.readHumidity();
  Serial.println("Temperature in C:");  
  Serial.println(deviceState.Temp);  
  Serial.println("Humidity in %:");  
  Serial.println(deviceState.Humid);  
  delay(1000); 
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  digitalWrite(pin_LED1,HIGH);  //Inidicate that the device is connected

}

void setDateTime() {
  // You can use your own timezone, but the exact time is not used at all.
  // Only the date is needed for validating the certificates.
  configTime(TZ_Europe_Berlin, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("%s %s", tzname[0], asctime(&timeinfo));
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if(strcmp(topic, "SSDH001/App") == 0){
    //snprintf (msg, MSG_BUFFER_SIZE, "Temperature: %ld ,Humidity: %s ,isArm: %d ,Threat: %d", deviceState.Temp , deviceState.Humid, deviceState.isArm, deviceState.Threat);
    String msg = String(deviceState.Temp) + "," + String(deviceState.Humid) + "," + String(deviceState.Mode) + "," + String(deviceState.TresTemp) + "," + String(deviceState.TresHumid);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client->publish("SSDH001/Device/State", msg.c_str());
  }else if (strcmp(topic, "SSDH001/App/Mode") == 0) {
    // Convert payload to a string
    String payloadStr = "";
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }

    // Check if we have commas in the payload (indicating a custom mode)
    int firstComma = payloadStr.indexOf(',');

    if (firstComma != -1) {
        // Custom Mode (Mode == 0) with additional data (tresTemp, tresHumid)
        int secondComma = payloadStr.indexOf(',', firstComma + 1);
        if (secondComma != -1) {
            deviceState.Mode = payloadStr.substring(0, firstComma).toInt();
            deviceState.TresTemp = payloadStr.substring(firstComma + 1, secondComma).toInt();
            deviceState.TresHumid = payloadStr.substring(secondComma + 1).toInt();

            Serial.println("Custom Mode Set:");
            Serial.print("Threshold Temp: "); Serial.println(deviceState.TresTemp);
            Serial.print("Threshold Humidity: "); Serial.println(deviceState.TresHumid);
        } else {
            Serial.println("Error: Second comma missing in custom mode payload");
        }
    } else {
        // Preset Mode (Mode != 0) with no extra thresholds
        deviceState.Mode = payloadStr.toInt();

        // Use preset values for the selected mode
        PresetValues::ModeValues values = presetValues.getValues(deviceState.Mode);
        deviceState.TresTemp = values.tresTemp;
        deviceState.TresHumid = values.tresHumid;

        Serial.println("Preset Mode Set:");
        Serial.print("Mode: "); Serial.println(deviceState.Mode);
        Serial.print("Preset Threshold Temp: "); Serial.println(deviceState.TresTemp);
        Serial.print("Preset Threshold Humidity: "); Serial.println(deviceState.TresHumid);
    }

    // Optionally, send acknowledgment to the device's state topic
    String msg = "Mode: " + String(deviceState.Mode) + ", tresTemp: " + String(deviceState.TresTemp) + ", tresHumid: " + String(deviceState.TresHumid);
    client->publish("SSDH001/Device/State", msg.c_str());
    
  }
}

void reconnect() {
  // Loop until we’re reconnected
  while (!client->connected()) {
    Serial.print("Attempting MQTT connection…");
    String clientId = "ESP8266Client - MyClient";
    // Attempt to connect
    // Insert your password
    if (client->connect(clientId.c_str(), "Michael", "Pio@2004")) {
      Serial.println("connected");
      // Once connected, publish an announcement…
      client->publish("SSDH001/Device/State", "hello ,Device is BackOnline");
      // … and resubscribe
      client->subscribe("SSDH001/App");
      client->subscribe("SSDH001/App/Mode");
      digitalWrite(pin_LED2,HIGH);
      
    } else { 
      Serial.print("failed, rc = ");
      Serial.print(client->state());
      Serial.println(" try again in 5 seconds");
      digitalWrite(pin_LED2,LOW);
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



//Display function

void displayIdleMode(int mode) {
  lcd.clear();
  if (mode == 1) {
    // Display Current Mode with selected veggie
    lcd.setCursor(0, 0);
    lcd.print("CURRENT MODE");
    lcd.setCursor(0, 1);
    lcd.print(veggies[selectedVeggie]);
  } else if (mode == 2) {
    // Display Temperature and Humidity
    lcd.setCursor(0, 0);
    lcd.print("T: ");
    lcd.print(deviceState.Temp);
    lcd.print("C");

    lcd.setCursor(10, 0);
    lcd.print("H: ");
    lcd.print(deviceState.Humid);
    lcd.print("%");
  }
}

void displayVeggieSelection() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SELECT");
  lcd.setCursor(0, 1);
  lcd.print(veggies[veggieIndex]);
  lcd.setCursor(0, 1);  // Left arrow
  lcd.write(0x7F);      // Character for left arrow (or a custom character)
  lcd.setCursor(15, 1); // Right arrow
  lcd.write(0x7E);      // Character for right arrow (or a custom character)
}


void handleDisplayMenu() {
  // Check if buttons are pressed to enter the veggie selection menu
  if (digitalRead(pin_BTN1) == LOW || digitalRead(pin_BTN2) == LOW) {
    currentMode = 3;  // Set mode to veggie selection
    displayVeggieSelection();

    delay(500);  // Debouncing delay

    // Wait for selection and update index based on button presses
    while (digitalRead(pin_BTN1) == LOW || digitalRead(pin_BTN2) == LOW) {
      if (digitalRead(pin_BTN1) == LOW) {
        veggieIndex = (veggieIndex - 1 + 4) % 4;  // Move left
        displayVeggieSelection();
        delay(300);  // Debouncing delay
      }
      if (digitalRead(pin_BTN2) == LOW) {
        veggieIndex = (veggieIndex + 1) % 4;      // Move right
        displayVeggieSelection();
        delay(300);  // Debouncing delay
      }
    }

    selectedVeggie = veggieIndex;  // Update selected veggie
    delay(2000);  // Wait for 2 seconds before returning to idle
    currentMode = 1;  // Go back to idle mode
  }
}

void handleIdleMode() {
  unsigned long currentMillis = millis();

  // Switch between idle screens (Current mode and Temp/Humidity)
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    currentMode = (currentMode == 1) ? 2 : 1;
    displayIdleMode(currentMode);
  }
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT); 
  pinMode(pin_LED1,OUTPUT);
  pinMode(pin_LED2,OUTPUT);
  pinMode(pin_LED3,OUTPUT);
  pinMode(pin_BUZZ,OUTPUT);
  pinMode(pin_LDR,INPUT);

  // Button pins setup
  pinMode(pin_BTN1, INPUT_PULLUP);
  pinMode(pin_BTN2, INPUT_PULLUP);

  // Initial display
  displayIdleMode(currentMode);
  delay(100);

  // When opening the Serial Monitor, select 9600 Baud
  Serial.begin(9600);
  delay(500);

  LittleFS.begin();
  setup_wifi();
  setDateTime();


  // you can use the insecure mode, when you want to avoid the certificates
  //espclient->setInsecure();

  int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  Serial.printf("Number of CA certs read: %d\n", numCerts);
  if (numCerts == 0) {
    Serial.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
    return; // Can't connect to anything w/o certs!
  }

  BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();
  // Integrate the cert store with this connection
  bear->setCertStore(&certStore);

  client = new PubSubClient(*bear);

  client->setServer(mqtt_server, 8883);
  client->setCallback(callback);
}

void loop() {
  if (!client->connected()) {

    reconnect();
  }
  digitalWrite(pin_LED2,HIGH);
  client->loop();
  update_TemperatureHumidity();

  handleDisplayMenu();  // Handle button press and display menu if necessary
  handleIdleMode();     // Switch between idle modes (screen 1 and screen 2)
  
  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    snprintf (msg, MSG_BUFFER_SIZE, "Alive");
    Serial.print("Publish message: ");
    Serial.println(msg);
    client->publish("SSDH001/Device/State", msg);
  }

}


