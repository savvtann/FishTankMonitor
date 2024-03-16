#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <GxEPD.h>
#include <GxFont_GFX.h>
#include <Fonts/FreeMonoBold9pt7b.h>  // Change the font size here
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h>

// Define SPI pin configurations
#define SPI_MOSI 23
#define SPI_MISO -1
#define SPI_CLK 18

// Define E-ink display pin configurations
#define ELINK_SS 5
#define ELINK_BUSY 4
#define ELINK_RESET 16
#define ELINK_DC 17

#define TEMPERATURE_BUS 19 // Digital pin where the DS18B20 is connected
#define SDA_PIN 21
#define SCL_PIN 22
#define BUTTON_PIN 15
#define BUTTON_THRESHOLD 3
#define POTENTIOMETER_PIN 13

#define BUILT_IN_BUTTON_PIN 39


// Color Sensor Variables
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

// Thermometer Variables
OneWire Temperature(TEMPERATURE_BUS);
DallasTemperature sensors(&Temperature);
bool tempUnits;

// Initialize SPI and E-ink display objects
GxIO_Class io(SPI, /*CS=5*/ ELINK_SS, /*DC=*/ ELINK_DC, /*RST=*/ ELINK_RESET);
GxEPD_Class display(io, /*RST=*/ ELINK_RESET, /*BUSY=*/ ELINK_BUSY);

// Button and Potentiometer variables
int lastButtonState = HIGH;    // the previous state from the input pin
int currentButtonState;        // the current reading from the input pin
int builtInLastButtonState = HIGH;
int builtInButtonState;
int lastPotValue = 0;          // the previous value from the potentiometer
int currentPotValue;           // the current reading from the potentiometer
volatile bool buttonPressed = false;
volatile bool builtInButtonPressed = false;


int currentPage = 0;

// Parameter Max and Min Variables
float phMin;
float phMax;
float ghMin;
float ghMax;
float khMin;
float khMax;
float no2Min;
float no2Max;
float no3Min;
float no3Max;
float tempMin;
float tempMax;

// Define a global array to hold notifications
#define MAX_NOTIFICATIONS 30
String notifications[MAX_NOTIFICATIONS];
int numNotifications = 0;

// Define a global array to hold inhabitants
#define MAX_INHABITANTS 30
String inhabitants[MAX_INHABITANTS];
int numInhabitants = 0;


// Define a structure to hold information about each species
struct SpeciesInfo {
    String species;
    int quantity;
};

// Define a maximum number of species to support
#define MAX_SPECIES 50
// Declare a list to store species information
SpeciesInfo speciesList[MAX_SPECIES];
int numSpecies = 0; // Track the number of species in the list

//**************************************************************
// PARAMETER COMPARISON FUNCTIONS

// Function to input parameter ranges through serial monitor
void parameterRanges() {
  // pH
  Serial.print("pH Min: ");
  while (Serial.available() == 0) {
  }
  phMin = Serial.parseFloat();
  Serial.println(phMin);

  Serial.print("pH Max: ");
  while (Serial.available() == 0) {
  }
  phMax = Serial.parseFloat();
  Serial.println(phMax);

  Serial.print("gH Min: ");
  while (Serial.available() == 0) {
  }
  ghMin = Serial.parseFloat();
  Serial.println(ghMin);

  Serial.print("gH Max: ");
  while (Serial.available() == 0) {
  }
  ghMax = Serial.parseFloat();
  Serial.println(ghMax);

  Serial.print("kH Min: ");
  while (Serial.available() == 0) {
  }
  khMin = Serial.parseFloat();
  Serial.println(khMin);

  Serial.print("kH Max: ");
  while (Serial.available() == 0) {
  }
  khMax = Serial.parseFloat();
  Serial.println(khMax);

  Serial.print("NO2 Min: ");
  while (Serial.available() == 0) {
  }
  no2Min = Serial.parseFloat();
  Serial.println(no2Min);

  Serial.print("NO2 Max: ");
  while (Serial.available() == 0) {
  }
  no2Max = Serial.parseFloat();
  Serial.println(no2Max);

  Serial.print("NO3 Min: ");
  while (Serial.available() == 0) {
  }
  no3Min = Serial.parseFloat();
  Serial.println(no3Min);

  Serial.print("NO3 Max: ");
  while (Serial.available() == 0) {
  }
  no3Max = Serial.parseFloat();
  Serial.println(no3Max);

  Serial.print("temp Min: ");
  while (Serial.available() == 0) {
  }
  tempMin = Serial.parseFloat();
  Serial.println(tempMin);

  Serial.print("temp Max: ");
  while (Serial.available() == 0) {
  }
  tempMax = Serial.parseFloat();
  Serial.println(tempMax);

  Serial.println("Parameter ranges set successfully!");
  Serial.println(phMin);
  Serial.println(phMax);
  Serial.println(ghMin);
  Serial.println(ghMax);
  Serial.println(khMin);
  Serial.println(khMax);
  Serial.println(no2Min);
  Serial.println(no2Max);
  Serial.println(no3Min);
  Serial.println(no3Max);
  Serial.println(tempMin);
  Serial.println(tempMax);
}


// Function to read float values from serial monitor
float readFloatFromSerial() {
  const int bufferSize = 20; // Buffer size
  char buffer[bufferSize];   // Buffer to store the input string
  int i = 0;
  bool isInputComplete = false;

  // Flush any existing data in the serial input buffer
  while (Serial.available() > 0) {
    Serial.read();
  }

  while (!isInputComplete) {
    while (Serial.available() > 0) {
      char c = Serial.read();

      // Check for Enter key press or newline
      if (c == '\r') {
        buffer[i] = '\0'; // Null-terminate the string
        isInputComplete = true;
        break;
      } else {
        // Store the character in the buffer
        buffer[i] = c;
        i++;

        // Check for buffer overflow
        if (i >= bufferSize - 1) {
          Serial.println("Input too long, please try again");
          i = 0; // Reset the buffer index
        }
      }
    }
  }

  float result = atof(buffer); // Convert the string to a float

  // Echo the entered value back to the user
  Serial.print("Entered value: ");
  Serial.println(result);

  return result;
}


// Function to compare RGB values to predefined PH colors
void compareColorToPH(uint16_t r, uint16_t g, uint16_t b) {
  // Predefined pH color values
  uint16_t phColors[][3] = {
    {236, 148, 0},   // pH 6
    {255, 168, 0},   // pH 6.5
    {255, 150, 29},  // pH 7
    {253, 105, 20},  // pH 7.5
    {222, 48, 4},    // pH 8
    {226, 26, 11},   // pH 8.5
    {230, 4, 28}     // pH 9
  };


  // Calculate the Euclidean distance for each pH color
  float minDistance = MAXFLOAT;
  int closestPHIndex = -1;


  for (int i = 0; i < sizeof(phColors) / sizeof(phColors[0]); i++) {
    float distance = sqrt(pow(r - phColors[i][0], 2) + pow(g - phColors[i][1], 2) + pow(b - phColors[i][2], 2));


    if (distance < minDistance) {
      minDistance = distance;
      closestPHIndex = i;
    }
  }

  // Display the closest pH level
  float closestPH = 6.0 + closestPHIndex * 0.5;
  Serial.print("Detected pH level: ");
  Serial.println(closestPH);
  
  // Check if pH level is outside the specified range
  if (closestPH < phMin || closestPH > phMax) {
    // Construct notification message
    String notification = "Warning: pH level is out of range (";
    notification += closestPH;
    notification += ")";
    
    // Add notification to the list
    if (numNotifications < MAX_NOTIFICATIONS) {
      notifications[numNotifications] = notification;
      numNotifications++;
    }
  }
}


// Function to compare RGB values to predefined GH colors
void compareColorToGH(uint16_t r, uint16_t g, uint16_t b) {
  uint16_t ghColors[][3] = {
    {171, 187, 168},  // GH 0
    {118, 165, 162},  // GH 30
    {122, 146, 158},  // GH 60
    {101, 111, 136},  // GH 120
    {147, 130, 177}   // GH 180
  };


  float minDistance = MAXFLOAT;
  int closestGHIndex = -1;


  for (int i = 0; i < sizeof(ghColors) / sizeof(ghColors[0]); i++) {
    float distance = sqrt(pow(r - ghColors[i][0], 2) + pow(g - ghColors[i][1], 2) + pow(b - ghColors[i][2], 2));


    if (distance < minDistance) {
      minDistance = distance;
      closestGHIndex = i;
    }
  }


  float closestGH = closestGHIndex * 30.0;
  Serial.print("Detected GH value: ");
  Serial.println(closestGH);

  // Check if GH value is outside the specified range
  if (closestGH < ghMin || closestGH > ghMax) {
    // Construct notification message
    String notification = "Warning: GH value is out of range (";
    notification += closestGH;
    notification += ")";
    
    // Add notification to the list
    if (numNotifications < MAX_NOTIFICATIONS) {
      notifications[numNotifications] = notification;
      numNotifications++;
    }
  }
}


// Function to compare RGB values to predefined KH colors
void compareColorToKH(uint16_t r, uint16_t g, uint16_t b) {
  uint16_t khColors[][3] = {
    {239, 180, 66},   // KH 0
    {194, 156, 60},   // KH 40
    {181, 173, 112},  // KH 80
    {142, 148, 102},  // KH 120
    {158, 172, 139}   // KH 180
  };


  float minDistance = MAXFLOAT;
  int closestKHIndex = -1;


  for (int i = 0; i < sizeof(khColors) / sizeof(khColors[0]); i++) {
    float distance = sqrt(pow(r - khColors[i][0], 2) + pow(g - khColors[i][1], 2) + pow(b - khColors[i][2], 2));


    if (distance < minDistance) {
      minDistance = distance;
      closestKHIndex = i;
    }
  }


  float closestKH = closestKHIndex * 40.0;
  Serial.print("Detected KH value: ");
  Serial.println(closestKH);

  // Check if KH value is outside the specified range
  if (closestKH < khMin || closestKH > khMax) {
    // Construct notification message
    String notification = "Warning: KH value is out of range (";
    notification += closestKH;
    notification += ")";
    
    // Add notification to the list
    if (numNotifications < MAX_NOTIFICATIONS) {
      notifications[numNotifications] = notification;
      numNotifications++;
    }
  }
}


// Function to compare RGB values to predefined NO2 colors
void compareColorToNO2(uint16_t r, uint16_t g, uint16_t b) {
  uint16_t no2Colors[][3] = {
    {239, 177, 142},  // NO2 0
    {250, 182, 150},  // NO2 0.5
    {232, 158, 133},  // NO2 1
    {227, 127, 116},  // NO2 3
    {250, 126, 114}   // NO2 5
  };


  float minDistance = MAXFLOAT;
  int closestNO2Index = -1;


  for (int i = 0; i < sizeof(no2Colors) / sizeof(no2Colors[0]); i++) {
    float distance = sqrt(pow(r - no2Colors[i][0], 2) + pow(g - no2Colors[i][1], 2) + pow(b - no2Colors[i][2], 2));


    if (distance < minDistance) {
      minDistance = distance;
      closestNO2Index = i;
    }
  }


  float closestNO2 = closestNO2Index * 0.5;
  Serial.print("Detected NO2 value: ");
  Serial.println(closestNO2);

  // Check if NO2 value is outside the specified range
  if (closestNO2 < no2Min || closestNO2 > no2Max) {
    // Construct notification message
    String notification = "Warning: NO2 value is out of range (";
    notification += closestNO2;
    notification += ")";
    
    // Add notification to the list
    if (numNotifications < MAX_NOTIFICATIONS) {
      notifications[numNotifications] = notification;
      numNotifications++;
    }
  }
}


// Function to compare RGB values to predefined NO3 colors
void compareColorToNO3(uint16_t r, uint16_t g, uint16_t b) {
  uint16_t no3Colors[][3] = {
    {244, 184, 149},  // NO3 0
    {254, 184, 153},  // NO3 20
    {248, 165, 146},  // NO3 40
    {230, 123, 103},  // NO3 80
    {230, 93, 87}     // NO3 160
  };


  float minDistance = MAXFLOAT;
  int closestNO3Index = -1;


  for (int i = 0; i < sizeof(no3Colors) / sizeof(no3Colors[0]); i++) {
    float distance = sqrt(pow(r - no3Colors[i][0], 2) + pow(g - no3Colors[i][1], 2) + pow(b - no3Colors[i][2], 2));


    if (distance < minDistance) {
      minDistance = distance;
      closestNO3Index = i;
    }
  }


  float closestNO3 = closestNO3Index * 20.0;
  Serial.print("Detected NO3 value: ");
  Serial.println(closestNO3);

  // Check if NO3 value is outside the specified range
  if (closestNO3 < no3Min || closestNO3 > no3Max) {
    // Construct notification message
    String notification = "Warning: NO3 value is out of range (";
    notification += closestNO3;
    notification += ")";
    
    // Add notification to the list
    if (numNotifications < MAX_NOTIFICATIONS) {
      notifications[numNotifications] = notification;
      numNotifications++;
    }
  }
}

//**************************************************************
// TEMPERATURE FUNCTIONS
void temperatureSensor() {
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);


  if (temperatureC != DEVICE_DISCONNECTED_C) {
    if (tempUnits == 0){
      // Celsius
      Serial.print("Temperature: ");
      Serial.print(temperatureC);
      Serial.println("�C");
      // Check if temperature is outside the specified range
      if (temperatureC < tempMin || temperatureC > tempMax) {
        // Construct notification message
        String notification = "Warning: Temperature is out of range (";
        notification += temperatureC;
        notification += ")";
        
        // Add notification to the list
        if (numNotifications < MAX_NOTIFICATIONS) {
          notifications[numNotifications] = notification;
          numNotifications++;
        }
      }
    }

    if (tempUnits == 1){
      // Fahrenheit
      Serial.print("Temperature: ");
      Serial.print((temperatureC * 9/5) + 32);
      Serial.println("�F");
      // Check if temperature is outside the specified range
      if (((temperatureC * 9/5) + 32) < tempMin || ((temperatureC * 9/5) + 32) > tempMax) {
        // Construct notification message
        String notification = "Warning: Temperature is out of range (";
        notification += ((temperatureC * 9/5) + 32);
        notification += ")";
        
        // Add notification to the list
        if (numNotifications < MAX_NOTIFICATIONS) {
          notifications[numNotifications] = notification;
          numNotifications++;
        }
      }
    }
    Serial.println();
  } 
  else {
    Serial.println("Error reading temperature!");
  }
}


void chooseFahrenheitCelsius(){
  Serial.println("Fahrenheit (F) or Celsius (C): ");
  while (!Serial.available()) {
    // Wait for user input
  }
  char choice = Serial.read();

  if (choice == 'F'){
    tempUnits = 1;
  }
  else if (choice == 'C'){
    tempUnits = 0;
  }
  else{
    Serial.println("Invalid choice. Please enter F for Fahrenheit or C for Celsius.");
    chooseFahrenheitCelsius(); // Ask again if the choice is invalid
  }
}

//**************************************************************
// COLOR FUNCTIONS
void printHexColor(uint16_t r, uint16_t g, uint16_t b) {
  unsigned long hexColor = ((unsigned long)r << 16) | ((unsigned long)g << 8) | b;
  Serial.print("HEX Color: 0x");
  Serial.println(hexColor, HEX);
}


void html_rgb(uint16_t r, uint16_t g, uint16_t b, uint16_t c, float (&rgb)[3]) {
  // Calculate normalization factor
  float maxRGB = max(max(r, g), b);
  float factor = 255.0 / maxRGB;

  // Normalize RGB values
  rgb[0] = r * factor;
  rgb[1] = g * factor;
  rgb[2] = b * factor;

  Serial.print("R: "); Serial.print(rgb[0]);
  Serial.print(", G: "); Serial.print(rgb[1]);
  Serial.print(", B: "); Serial.print(rgb[2]);
  Serial.println();
}


String html_hex(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
  float rgb[3];
  html_rgb(r, g, b, c, rgb);
  return String(String((int)rgb[0], HEX) + String((int)rgb[1], HEX) + String((int)rgb[2], HEX));
}


void colorSensor() {
  uint16_t r, g, b, c, colorTemp, lux;
  tcs.getRawData(&r, &g, &b, &c);
  colorTemp = tcs.calculateColorTemperature(r, g, b);
  lux = tcs.calculateLux(r, g, b);

  Serial.print("R: "); Serial.print(r);
  Serial.print(", G: "); Serial.print(g);
  Serial.print(", B: "); Serial.print(b);
  Serial.println();

  String hexColor = html_hex(r, g, b, c);
  float rgb[3];
  html_rgb(r, g, b, c, rgb);
  Serial.print("HEX Color: 0x"); Serial.println(hexColor);

  compareColorToPH(r, g, b);  // call the PH function with corrected values
  compareColorToGH(r, g, b);  // Call the GH function with corrected values
  compareColorToKH(r, g, b);  // Call the KH function with corrected values
  compareColorToNO2(r, g, b); // Call the NO2 function with corrected values
  compareColorToNO3(r, g, b); // Call the NO3 function with corrected values
}

//**************************************************************
// INTERRUPT FOR BUTTON PRESS
void buttonISR() {
  buttonPressed = true;
}

// ISR (Interrupt Service Routine) for the built-in button
void builtInButtonISR() {
  builtInButtonPressed = true;
}

//**************************************************************
// DISPLAY FUNCS

bool leftRightFunc(int currentPotValue){
  bool leftRight;
    int threshold = 4095 / 2;

    if (currentPotValue > threshold) {
        leftRight = true;  // 1 represents true
    } else {
        leftRight = false; // 0 represents false
    }
    return leftRight;
}


void switchPages(bool leftRight){
  if (leftRight) {
    currentPage += 1;
    if (currentPage >= 4){
      currentPage = 0;
    }
  } else {
    currentPage -= 1;
    if (currentPage <= -1){
      currentPage = 3;
    }
  }
  Serial.println(currentPage);
  displayPages(currentPage);
}


void displayPages(int currentPage){
  int xValues[] = {0, 1, 2, 3, 4, 5};
  int yValues[] = {10, 20, 15, 30, 25, 35};
  int numPoints = sizeof(xValues) / sizeof(xValues[0]);

  int myIntList[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
  int listLength = sizeof(myIntList) / sizeof(myIntList[0]);
  switch (currentPage){
    case 0:
      // Check if there are any notifications to display
      if (numNotifications > 0) {
        // Construct a string to display all notifications
        String notificationText = "Notifications:\n";
        for (int i = 0; i < numNotifications; ++i) {
          notificationText += notifications[i] + "\n";
        }
        displayText(notificationText.c_str()); // Convert String to const char* before passing to displayText
      } else {
        displayText("No notifications");
      }
      break;
    case 1:
      displayText("Page 1: Example Nums: \n 1 1 2 ");
      break;
    case 2:
      displaySpeciesList();
      break;
    case 3:
      displayGraph(xValues, yValues, numPoints);
      break;
    default:
      displayText("default");
      break;
  }
  return;
}


void displayText(const char* text) {
  Serial.println("displaytext");
  display.fillScreen(GxEPD_WHITE); // Clear the screen
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(10, 20);
  display.print(text);
  display.update();
  delay(1000);

}

void displayIntList(int list[], int length) {
  Serial.println("Displaying Integer List");
  display.fillScreen(GxEPD_WHITE); // Clear the screen
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  
  // Set initial cursor position
  int x = 20;
  int y = 50;

  // Display each element of the list
  for (int i = 0; i < length; i++) {
    display.setCursor(x, y);
    display.print(list[i]);

    // Move the cursor to the next position
    y += 20;  // You can adjust the vertical spacing based on your preference

    // Check if the next position goes beyond the display height
    if (y > display.height() - 20) {
      // If it does, reset y and move to the next column
      y = 50;
      x += 60;  // You can adjust the horizontal spacing based on your preference
    }
  }

  display.update();
  delay(1000);
}

void displayGraph(int xValues[], int yValues[], int numPoints) {
  Serial.println("Displaying Graph");
  display.fillScreen(GxEPD_WHITE); // Clear the screen
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);

  // Define graph properties
  int xStart = 20;  // Starting X-coordinate of the graph
  int yStart = 20;  // Starting Y-coordinate of the graph
  int graphWidth = 200;  // Width of the graph
  int graphHeight = 100; // Height of the graph

  // Calculate the X and Y scale factors
  float xScale = graphWidth / (float)(numPoints - 1);
  float yScale = graphHeight / (float)(*std::max_element(yValues, yValues + numPoints) - *std::min_element(yValues, yValues + numPoints));

  // Draw the X and Y axes
  display.drawLine(xStart, yStart + graphHeight, xStart + graphWidth, yStart + graphHeight, GxEPD_BLACK); // X-axis
  display.drawLine(xStart, yStart, xStart, yStart + graphHeight, GxEPD_BLACK); // Y-axis

  // Draw the graph points and connecting lines
  for (int i = 0; i < numPoints - 1; i++) {
    int x1 = xStart + i * xScale;
    int y1 = yStart + graphHeight - ((yValues[i] - *std::min_element(yValues, yValues + numPoints)) * yScale);
    int x2 = xStart + (i + 1) * xScale;
    int y2 = yStart + graphHeight - ((yValues[i + 1] - *std::min_element(yValues, yValues + numPoints)) * yScale);

    // Draw connecting lines
    display.drawLine(x1, y1, x2, y2, GxEPD_BLACK);

    // Draw circles at each data point
    display.fillCircle(x1, y1, 2, GxEPD_BLACK);
    display.fillCircle(x2, y2, 2, GxEPD_BLACK);
  }

  display.update();
  delay(1000);
}

//**************************************************************
// INHABITANTS SCREEN FUNCTIONS

// Function to add inhabitants to the list
void addInhabitants() {
    // Check if the list is full
    if (numSpecies >= MAX_SPECIES) {
        Serial.println("Cannot add more species. List is full.");
        return;
    }

    // Prompt the user to input species
    Serial.println("Enter species:");
    while (!Serial.available()) {
        // Wait for user input
    }
    String species = Serial.readStringUntil('\n');

    // Prompt the user to input quantity
    Serial.println("Enter quantity:");
    while (!Serial.available()) {
        // Wait for user input
    }
    int quantity = Serial.parseInt();

    // Add the species to the list
    speciesList[numSpecies].species = species;
    speciesList[numSpecies].quantity = quantity;
    numSpecies++;

    Serial.println("Species added successfully.");
}

void displaySpeciesList() {
  display.fillScreen(GxEPD_WHITE); // Clear the screen
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  
  // Set initial cursor position
  int x = 20;
  int y = 20;
  display.setCursor(x, y);
  display.print("Species, Quantity ");
  y += 20;

  // Display each species in the list
  for (int i = 0; i < numSpecies; i++) {
    Serial.println(speciesList[i].species);
    Serial.println(speciesList[i].quantity);

    display.setCursor(x, y);
    display.print(speciesList[i].species);
    display.print(", ");
    display.print(speciesList[i].quantity);

    // Move the cursor to the next position for the next species
    y += 20;  // You can adjust the vertical spacing based on your preference
  }

  display.update();
  delay(1000);
}



//**************************************************************
// BLOCK 2 VERIFICATION FUNCTION

void block2() {
  // Continuous loop for color sensor readings and temperature readings until 'X' is entered
  while (true) {
    // Display color sensor readings
    // Continuous temperature readings
    while (true) {
      colorSensor();
      Serial.println("Type X to exit color loop");

      // Check if the user entered 'X' in the serial monitor
      if (Serial.available() > 0) {
        char userInput = Serial.read();
        if (userInput == 'X' || userInput == 'x') {
          Serial.println("Exiting color loop");
          break;
        }
      }
      
      delay(5000); // Adjust delay based on how frequently you want to take temperature readings
    }

    // Continuous temperature readings
    while (true) {
      temperatureSensor();
      Serial.println("Type X to exit temperature loop");

      // Check if the user entered 'X' in the serial monitor
      if (Serial.available() > 0) {
        char userInput = Serial.read();
        if (userInput == 'X' || userInput == 'x') {
          Serial.println("Exiting temperature loop");
          return; // Exit the function
        }
      }
    }
  }
}


//**************************************************************
// GENERAL FUNCTIONS

void setup() {
  // set up serial monitor
  Serial.begin(9600);

  // set up color sensor
  tcs.begin();
  Wire.begin(SDA_PIN, SCL_PIN);

  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1);
  }

  // set up temperature sensor
  sensors.begin();

  // set up buttons
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  pinMode(BUILT_IN_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUILT_IN_BUTTON_PIN), builtInButtonISR, FALLING);
  // set up screen
  display.init();
  display.setRotation(1);

  // decide on units of temperature
  chooseFahrenheitCelsius();
  parameterRanges();

  // temperatureSensor();
  // colorSensor();

  // char userInput;
  // do {
  //   addInhabitants();
  //   Serial.println("Enter 'X' to stop adding inhabitants or press any key to continue.");

  //   while (!Serial.available()) {
  //     // Wait for user input
  //   }

  //   userInput = Serial.read();
  // } while (userInput != 'X' && userInput != 'x');

  // run first round of tests
  // block2();

}


void loop() {
//*********************************
// POTENTIOMETER STUFF
  //Read the potentiometer value
  currentPotValue = analogRead(POTENTIOMETER_PIN);
  // Check if the potentiometer value has changed significantly
  if (abs(currentPotValue - lastPotValue) > BUTTON_THRESHOLD) {
    Serial.print("Potentiometer value: ");
    Serial.println(currentPotValue);
    // Add code here to handle potentiometer value change (e.g., adjust brightness)
  }
  lastPotValue = currentPotValue;
  bool leftRight = leftRightFunc(currentPotValue);

//*********************************
// BUTTON STUFF
  currentButtonState = digitalRead(BUTTON_PIN);
  if (lastButtonState == LOW && currentButtonState == HIGH) {
    Serial.println("Button pressed");
    switchPages(leftRight);
    // Add code here to handle button press (e.g., switch pages)
  }
  lastButtonState = currentButtonState;

  builtInButtonState = digitalRead(BUILT_IN_BUTTON_PIN);
  if (builtInLastButtonState == LOW && builtInButtonState == HIGH) {
    Serial.println("Built In Button pressed");
    //switchPages(leftRight);
    // Add code here to handle button press (e.g., switch pages)
  }
  builtInLastButtonState = builtInButtonState;

  temperatureSensor();

}
