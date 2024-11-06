#include <LiquidCrystal.h>
#include <DHT.h>

// Pin connections for the LCD (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);  // Updated to match your pin configuration

// DHT22 sensor settings
#define DHTPIN A0      // Pin connected to DHT22 data pin
#define DHTTYPE DHT11  // DHT22 sensor
DHT dht(DHTPIN, DHTTYPE);

// Pin definitions for the buttons (connected to GND)
const int buttonApple = 6;
const int buttonMushroom = 7;
const int buttonBanana = 5;
const int buttonHerbs = 4;
const int buttonIdle = 3;

// Pin for relay control
const int relayPin = A1;

// Variable to store the selected mode
String mode = "IDLE";
String previousMode = "";

// Variables to store previous sensor readings for efficient updates
float previousTemp = 0.0;
float previousHumidity = 0.0;

unsigned long lastLCDUpdate = 0;
const unsigned long updateInterval = 2000; // Update every 2 seconds

void setup() {
  // Initialize the LCD
  lcd.begin(16, 2);
  lcd.print("Initializing...");
  delay(2000);  // Wait 2 seconds to see if the LCD displays anything
  
  // Clear the screen after the test
  lcd.clear();
  lcd.print("Select Mode:");

  // Set up the button pins as inputs
  pinMode(buttonApple, INPUT_PULLUP);
  pinMode(buttonMushroom, INPUT_PULLUP);
  pinMode(buttonBanana, INPUT_PULLUP);
  pinMode(buttonHerbs, INPUT_PULLUP);
  pinMode(buttonIdle, INPUT_PULLUP);

  // Set up the relay pin as an output
  pinMode(relayPin, OUTPUT);

  // Initialize the DHT22 sensor
  dht.begin();

  // Initially turn off the relay
  digitalWrite(relayPin, LOW);
}

void loop() {
  // Check if any button is pressed (LOW when pressed because connected to GND)
  if (digitalRead(buttonApple) == LOW) {
    mode = "APPLE";
  } else if (digitalRead(buttonMushroom) == LOW) {
    mode = "MUSHROOM";
  } else if (digitalRead(buttonBanana) == LOW) {
    mode = "BANANA";
  } else if (digitalRead(buttonHerbs) == LOW) {
    mode = "HERBS";
  } else if (digitalRead(buttonIdle) == LOW) {
    mode = "IDLE";
  }

  // Read temperature and humidity from the DHT22 sensor
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Handle the relay based on the selected mode
  if (mode == "APPLE" || mode == "MUSHROOM" || mode == "BANANA" || mode == "HERBS") {
    // Turn on the relay when a drying mode is selected
    digitalWrite(relayPin, LOW);
  } else {
    // Turn off the relay when in idle mode
    digitalWrite(relayPin, HIGH);
  }

  // Update the LCD only if mode, temperature, or humidity has changed
  if (millis() - lastLCDUpdate >= updateInterval || mode != previousMode || temp != previousTemp || humidity != previousHumidity) {
    // Clear and update the LCD only if necessary
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mode:");
    lcd.print(mode);
    lcd.setCursor(0, 1);
    lcd.print("T:");
    lcd.print(temp);
    lcd.print("C H:");
    lcd.print(humidity);
    lcd.print("%");

    // Update the last known values and timestamp
    previousMode = mode;
    previousTemp = temp;
    previousHumidity = humidity;
    lastLCDUpdate = millis();
  }

  // Add a small delay to debounce the buttons
  delay(200);
}
