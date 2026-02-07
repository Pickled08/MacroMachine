#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <Keyboard.h>

// ---------- OLED ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

#define OLED_SDA 6
#define OLED_SCL 7

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------- NEOPIXELS ----------
#define PIXEL_PIN 26
#define NUM_PIXELS 4

Adafruit_NeoPixel pixels(NUM_PIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// ---------- MACRO BUTTONS ----------
const int buttonPins[] = {29, 0, 1, 2, 3, 4};
const int numButtons = 6;
bool buttonStates[numButtons];
bool lastButtonStates[numButtons];

// ---------- Time variables ----------
int hours = 0;
int minutes = 0;
int seconds = 0;
bool timeReceived = false;

// ---------- Rainbow helper ----------
uint32_t wheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return pixels.Color(255 - pos * 3, 0, pos * 3);
  }
  if (pos < 170) {
    pos -= 85;
    return pixels.Color(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return pixels.Color(pos * 3, 255 - pos * 3, 0);
}

void parseTime(String timeStr) {
  // Expected format: "HH:MM:SS"
  if (timeStr.length() >= 8) {
    hours = timeStr.substring(0, 2).toInt();
    minutes = timeStr.substring(3, 5).toInt();
    seconds = timeStr.substring(6, 8).toInt();
    timeReceived = true;
  }
}

void updateTime() {
  // Increment seconds
  static unsigned long lastMillis = 0;
  unsigned long currentMillis = millis();
  
  if (timeReceived && (currentMillis - lastMillis >= 1000)) {
    lastMillis = currentMillis;
    seconds++;
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        if (hours >= 24) {
          hours = 0;
        }
      }
    }
  }
}

void drawTime() {
  display.setTextSize(2);
  display.setCursor(0, 32);

  if (timeReceived) {
    // Display system time
    if (hours < 10) display.print("0");
    display.print(hours);
    display.print(":");
    if (minutes < 10) display.print("0");
    display.print(minutes);
    display.print(":");
    if (seconds < 10) display.print("0");
    display.print(seconds);
  } else {
    // Show waiting message
    display.setTextSize(1);
    display.print("Waiting...");
  }
}

// ---------- MACRO FUNCTIONS ----------
void executeMacro(int buttonIndex) {
  switch(buttonIndex) {
    case 0:  // GPIO 29 SW6 Bottom Right
      // Example: Copy (Ctrl+C)
      Keyboard.press(KEY_LEFT_CTRL);
      delay(100);
      Keyboard.releaseAll();
      Serial.println("MACRO: Copy");
      break;
      
    case 1:  // GPIO 0 SW5 top right
      // Example: Paste (Ctrl+V)
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press('v');
      delay(100);
      Keyboard.releaseAll();
      Serial.println("MACRO: Paste");
      break;
      
    case 2:  // GPIO 1 sw1 TL
      // Example: Undo (Ctrl+Z)
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press('z');
      delay(100);
      Keyboard.releaseAll();
      Serial.println("MACRO: Undo");
      break;
      
    case 3:  // GPIO 2 sw2 BL
      // Example: Save (Ctrl+S)
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press('s');
      delay(100);
      Keyboard.releaseAll();
      Serial.println("MACRO: Save");
      break;
      
    case 4:  // GPIO 3 sw4 bm
      // Example: Type a phrase
      Keyboard.print("Hello from macro pad!");
      Serial.println("MACRO: Hello");
      break;
      
    case 5:  // GPIO 4 sw3 tm
      // Example: Media play/pause
      Keyboard.press(KEY_PLAY_PAUSE);
      delay(100);
      Keyboard.release(KEY_PLAY_PAUSE);
      Serial.println("MACRO: Play/Pause");
      break;
  }
  
  // Flash NeoPixels on macro activation
  for (int i = 0; i < NUM_PIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(255, 255, 255));
  }
  pixels.show();
  delay(50);
}

void checkButtons() {
  for (int i = 0; i < numButtons; i++) {
    buttonStates[i] = digitalRead(buttonPins[i]);
    
    // Button pressed (assuming active LOW with INPUT_PULLUP)
    if (buttonStates[i] == LOW && lastButtonStates[i] == HIGH) {
      executeMacro(i);
    }
    
    lastButtonStates[i] = buttonStates[i];
  }
}

void setup() {
  // Serial for communication
  Serial.begin(115120);
  
  // Initialize Keyboard
  Keyboard.begin();
  
  // Setup button pins
  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastButtonStates[i] = HIGH;
  }
  
  // I2C pins
  Wire.setSDA(OLED_SDA);
  Wire.setSCL(OLED_SCL);
  Wire.begin();

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    while (true);
  }

  display.setRotation(2);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Macro");
  display.println("Machine");
  display.display();

  // NeoPixels
  pixels.begin();
  pixels.setBrightness(50);
  pixels.clear();
  pixels.show();
  
  delay(2000);
}

void loop() {
  // Check for incoming serial data
  if (Serial.available() > 0) {
    String incoming = Serial.readStringUntil('\n');
    incoming.trim();
    parseTime(incoming);
  }

  // Update time counter
  updateTime();
  
  // Check macro buttons
  checkButtons();

  // ---- OLED ----
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("MACRO");
  display.println("PAD");

  drawTime();
  display.display();

  // ---- NeoPixel rainbow flow ----
  static uint8_t hue = 0;
  uint32_t c = wheel(hue);

  for (int i = 0; i < NUM_PIXELS; i++) {
    pixels.setPixelColor(i, c);
  }

  pixels.show();
  hue++;
  delay(20);
}