//Libraries
#include <Arduino.h>
#include "MHZ19.h"
#include <SoftwareSerial.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include "bitmap.h"

//Pin definitions
#define RX_PIN 2         //MH-Z19 RX-PIN                                         
#define TX_PIN 0         //MH-Z19 TX-PIN                                   
#define DHTPIN 12        //DHT11 Pin
#define LEDPIN 15        //LED Pin 
#define NUM_LEDS 4        //Number of LEDs
#define BAUDRATE 9600    //Terminal Baudrate

// define LED strip
Adafruit_NeoPixel strip(NUM_LEDS, LEDPIN, NEO_GRB + NEO_KHZ800);

//DHT11 Sensor
#define DHTTYPE    DHT11
DHT dht(DHTPIN, DHTTYPE);
//

//MH-Z19 Sensor
MHZ19 myMHZ19;
SoftwareSerial mySerial(RX_PIN, TX_PIN);
unsigned long getDataTimer = 0;

//Screen
//dimensions
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128

#define SCLK_PIN 14
#define MOSI_PIN 13
#define DC_PIN   5
#define CS_PIN   4
#define RST_PIN  16

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

//Convert color to RGB
uint32_t convertColor(uint16_t color) {
  // Extract the red, green, and blue components from the 16-bit color
  uint8_t r = (color >> 11) & 0x1F;
  uint8_t g = (color >> 5) & 0x3F;
  uint8_t b = color & 0x1F;

  // Scale the components to 8 bits
  r = (r * 255) / 31;
  g = (g * 255) / 63;
  b = (b * 255) / 31;

  // Combine the components into a 24-bit color
  return strip.Color(r, g, b);
}

//Define CO2 limits
#define CO2OK 1000
#define CO2NOK 2000

//Define map limits
#define MinMapValue 10
#define MaxMapValue 65

//Array definitions
#define NOOFARRAY   15
#define NOOFENTRIES 60
#define UPDATETIME  2000

//Create needed objects
Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, CS_PIN, DC_PIN, MOSI_PIN, SCLK_PIN, RST_PIN);

//Global vars
bool bFirstRun = true;
double dCurrentTemp = 0.0;
float fCurrentHum = 0.0;
float fcurrentCO2 = 0.0;
int iCO2Array[NOOFARRAY];  // Array for temporal course CO2
unsigned long iLastUpate = 0;
/*
  =================================================================
  Function:     setup
  Returns:      void
  Description:  Setup display and sensors
  =================================================================
*/
void setup() {
  //Serielle Intialisierung
  Serial.begin(BAUDRATE);
  mySerial.begin(BAUDRATE);
  myMHZ19.begin(mySerial);
  dht.begin();
  tft.begin();

  while (!Serial) delay(20);    // Wait until serial communication is ready

  //Start LEDs
  strip.begin();
  strip.show();
  strip.setBrightness(50);
  colorWipe(strip.Color(255, 0, 0), 50); // Red
  colorWipe(strip.Color(255, 213, 0), 50); // Yellow
  colorWipe(strip.Color(0, 255, 0), 50); // Green
  colorWipe(strip.Color(255, 255, 255), 50); // White
  strip.fill(strip.Color(0, 0, 0));
  strip.show();

  //Start with some text in serial monitor and tft
  tft.fillScreen(BLACK);
  tft.setTextSize(2);
  tft.setTextColor(RED);
  tft.setCursor(50, 10);
  tft.print("CO");
  tft.setTextSize(1); 
  tft.setCursor(75, 20);
  tft.print("2");
  tft.setTextSize(2);
  tft.setTextColor(RED);
  tft.setCursor(60, 28);
  tft.print("-");
  tft.setTextColor(GREEN);
  tft.setCursor(8, 40);
  tft.print("Messger\204t");
  tft.setTextSize(1);
  tft.setTextColor(YELLOW);
  tft.setCursor(50, 60);
  tft.print("von");
  tft.setCursor(10, 75);
  tft.print("NAME");
  tft.setTextColor(WHITE);
  tft.setCursor(0, 90);
  delay(5000);
  
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);
  tft.setCursor(25, 30);
  tft.print("Device startup");
  tft.setCursor(0, 50);
  tft.print("Init screen ... ");    
  tft.setCursor(100, 50);
  tft.setTextColor(GREEN);
  tft.print("DONE");
  tft.setTextColor(WHITE);
  tft.setCursor(0, 70);
  tft.print("Init DHT11 ... ");
  tft.setTextColor(GREEN);
  tft.setCursor(100, 70);
  tft.print("DONE");
  delay(5000);
  tft.fillScreen(BLACK);
  
  
  tft.setTextColor(WHITE);
  tft.setCursor(0, 60);
  tft.print("Waiting 10 seconds");
  tft.setCursor(0, 70);
  tft.print("for MH-Z19 sensor ... ");
  delay(10000);
  tft.fillScreen(BLACK);
  
  //Check im MH-Z19 is available
  tft.setTextColor(WHITE);
  tft.setCursor(0, 60);
  tft.print("Init MH-Z19 ... ");
  if (millis() - getDataTimer >= 2000)
  {
    int testCO2;                                        // Buffer for CO2
    testCO2 = myMHZ19.getCO2();                         // Request CO2 (as ppm)

    if (myMHZ19.errorCode == RESULT_OK)            // RESULT_OK is an alis for 1. Either can be used to confirm the response was OK.
    {
      Serial.print("CO2 Value successfully Recieved: ");
      Serial.println(testCO2);
      Serial.print("Response Code: ");
      Serial.println(myMHZ19.errorCode);          // Get the Error Code value
    }

    else
    {
      Serial.println("Failed to recieve CO2 value - Error");
      Serial.print("Response Code: ");
      colorWipe(strip.Color(0, 255, 0), 50); // Green
      Serial.println(myMHZ19.errorCode);     // Get the Error Code value
      tft.setTextColor(RED);
      tft.println("FAIL");
      while (1);                              // Endless loop
    }
    getDataTimer = millis();
  }
  tft.setTextColor(WHITE);
  tft.setCursor(0, 60);  
  tft.print("Init MH-Z19 ... ");
  tft.setCursor(100, 60);
  tft.setTextColor(GREEN);
  tft.print("DONE");
  delay(2000);
  tft.fillScreen(BLACK);  // Clear screen
  
  //autocalibration for MH-z19
  myMHZ19.autoCalibration(false);
  tft.fillScreen(BLACK);  // Clear screen
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(5, 50);
  tft.print("Clearing Array  ... ");
  delay(2000);
  //Clear array from maybe random entries
  for (int iCnt = 0; iCnt < NOOFARRAY; iCnt++)
    iCO2Array[iCnt] = 0;
  tft.fillScreen(BLACK);  // Clear screen
  tft.setTextColor(GREEN);
  tft.setTextSize(1);
  tft.setCursor(25, 50);
  tft.println("Clearing DONE");
  tft.fillScreen(BLACK);  // Clear screen
}

void loop() {
  // Just want data every 60sec or during first start
  if (millis() - iLastUpate > UPDATETIME || bFirstRun)
  {
    UpdateIndoorClimate();  // Receive data from MH-Z19
    {
      Serial.println("New data from  MH-Z19 available...");
      {
        UpdateArray(int(myMHZ19.getCO2()));  // Update array with CO2-data
        if (bFirstRun)
          bFirstRun = false;
        WriteDataToSerialMonitor(); // Write data to serial monitor
        UpdateDisplayInfo();        // Update display
        iLastUpate = millis();      // Update time to get data after 60sec
      }
    }
  }
}

/*
  =================================================================
  Function:     UpdateIndoorClimate
  Returns:      void
  Description:  Setup display and sensors
  =================================================================
*/
void UpdateIndoorClimate() {
  dCurrentTemp = dht.readTemperature();
  fCurrentHum = dht.readHumidity();
  fcurrentCO2 = myMHZ19.getCO2();
  
  // Check if DHT11 data is within limits of specs on data sheet
  if (dCurrentTemp < 0 || dCurrentTemp > 50) {
    Serial.println("invalid temperature value: " + String(dCurrentTemp));
    dCurrentTemp = NAN;  // Ignoring invalid values
  }
  if (fCurrentHum < 20 || fCurrentHum > 90) {
    Serial.println("invalid humidity value: " + String(fCurrentHum));
    fCurrentHum = NAN;  // Ignoring invalid values
  }

  // Update data if valid
  if (!isnan(dCurrentTemp)) {
    dCurrentTemp = dCurrentTemp;
  }
  if (!isnan(fCurrentHum)) {
    fCurrentHum = fCurrentHum;
  }
}

/*
  =================================================================
  Function:     WriteDataToSerialMonitor
  Returns:      void
  Description:  Setup display and sensors
  =================================================================
*/
void WriteDataToSerialMonitor() {
  Serial.print("Temperature = ");
  Serial.print(dCurrentTemp);
  Serial.println(" C");

  Serial.print("Humidity = ");
  Serial.print(fCurrentHum);
  Serial.println(" %");

  Serial.print("CO2 = ");
  Serial.print(fcurrentCO2);
  Serial.println(" ppm");
  Serial.println("-------------------------");
}

/*
  =================================================================
  Function:     UpdateArrays
  Returns:      void
  Description:  Update data arrays with needed info for graphic
  =================================================================
*/
void UpdateArray(int iCO2) {
  // Sanity Check for CO2 data
  if (iCO2 < 0) {
    Serial.println("Invalid CO2 Value: " + String(iCO2));
    return;  // ignore invalid values
  }
  // Swap data array for the last minutes
  for (int iCnt = 0; iCnt < (NOOFARRAY - 1); iCnt++)
    iCO2Array[iCnt] = iCO2Array[iCnt + 1];
  iCO2Array[NOOFARRAY - 1] = iCO2;
  Serial.println("Current values in array:");
  for (int iCnt = 0; iCnt < NOOFARRAY; iCnt++) {
    if (iCnt != 0)
      Serial.print(" | ");
    Serial.print(iCO2Array[iCnt]);
  }
  Serial.println("");
}

/*
  =================================================================
  Function:     UpdateDisplayInfo
  Returns:      void
  Description:  Update display content with latest data
  =================================================================
*/
void UpdateDisplayInfo() {
  const int barWidth = 1;  // Width of the bars in the graph
  const int barSpacing = 1;  // Spacing between the bars in the graph
  const int graphBaseX = 5;  // Base-X-Position of graph
  const int graphBaseY = 115;  // base-Y-Position of graph

  tft.fillScreen(BLACK);
  tft.drawLine(graphBaseX, graphBaseY, graphBaseX, 50, WHITE);  // Y-axis 
  tft.drawLine(graphBaseX, graphBaseY, 128, graphBaseY, WHITE); // X-axis
  
  // X-Achsen-Beschriftung
  tft.setTextColor(WHITE);
  tft.setCursor(8, 120);
  tft.print("0 Min.");
  tft.setCursor(85, 120);
  tft.print("60 Min.");
  
  // Pfeile Y-Achse
  tft.drawLine(graphBaseX, 50, graphBaseX + 5, 55, WHITE);
  tft.drawLine(graphBaseX, 50, graphBaseX - 5, 55, WHITE);
  
  // Pfeile X-Achse
  tft.drawLine(128, graphBaseY, 123, graphBaseY - 5, WHITE);
  tft.drawLine(128, graphBaseY, 123, graphBaseY + 5, WHITE);

  // Höchsten PPM-Wert finden, um die maximale Skalierung für den Graphen zu berechnen
  int iHighestPPM = 0;
  int iHighestMap = 0;
  for (int iCnt = 0; iCnt < NOOFARRAY; iCnt++) {
    if (iHighestPPM < iCO2Array[iCnt]) {
      iHighestPPM = iCO2Array[iCnt];
      Serial.println("Highest PPM: " + String(iCO2Array[iCnt]) + " ppm");
      iHighestMap = int(iHighestPPM / 100) * 100 + 200;
      Serial.println("Calc highest map: " + String(iHighestMap));
    }
  }

  // Durch das Array iterieren und die CO2-Werte grafisch darstellen
  int iNoValues = 0;
  int iSumArray = 0;
  uint16_t currentColor = WHITE;  // Default color
  for (int iCnt = NOOFARRAY - 1; iCnt >= 0; iCnt--) {
    if (iCO2Array[iCnt] != 0) {
      iNoValues++;
      iSumArray += iCO2Array[iCnt];
      // Korrektur und Mapping-Wert für den Graphen
      int iMappedValue = map(iCO2Array[iCnt], 400, iHighestMap, MinMapValue, MaxMapValue);
      // Richtige Farbe für den Graphen finden
      uint16_t color;
      if (iCO2Array[iCnt] < CO2OK)
        color = GREEN;
      else if (iCO2Array[iCnt] >= CO2OK && iCO2Array[iCnt] < CO2NOK)
        color = YELLOW;
      else
        color = RED;
      // Graph-Segmente für jedes Element im Array zeichnen
      int x = graphBaseX + (NOOFARRAY - 1 - iCnt) * (barWidth + barSpacing);
      tft.drawLine(x, graphBaseY - iMappedValue, x, graphBaseY, color);
      
      // Update the current color for the LED
      if (iCnt == NOOFARRAY - 1) {
        currentColor = color;
      }
    }
  }

  // Convert currentColor to RGB values and set the LED color for all LEDs
  uint32_t rgbColor = convertColor(currentColor);
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, rgbColor);
  }
  strip.show();

  // Durchschnitt zeichnen
  int iAverage = int((iSumArray / iNoValues) / 100) * 100;
  if ((iAverage % 100) >= 50)
    iAverage += 50;
  int iMappedValue = map(iAverage, 400, iHighestMap, MinMapValue, MaxMapValue);
  iMappedValue = constrain(iMappedValue, MinMapValue, MaxMapValue);
  tft.drawLine(graphBaseX + 1, graphBaseY - iMappedValue, 120, graphBaseY - iMappedValue, BLUE);
  tft.setTextSize(1);
  tft.setTextColor(BLUE);
  tft.setCursor(100, graphBaseY - iMappedValue - 10);
  tft.print(iAverage);

  // Neueste Daten auf dem Display anzeigen
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(0, 0);
  tft.print("Temperatur: " + String(int(dCurrentTemp)) + (char)247 + "C");
  tft.setCursor(0, 10);
  tft.print("Luftfeuchtigkeit: " + String(int(fCurrentHum)) + "%");
  tft.setCursor(0, 20);
  tft.print("CO2: ");
  if (iCO2Array[NOOFARRAY - 1] < CO2OK)
    tft.setTextColor(GREEN);
  else if (iCO2Array[NOOFARRAY - 1] >= CO2OK && iCO2Array[NOOFARRAY - 1] < CO2NOK)
    tft.setTextColor(YELLOW);
  else
    tft.setTextColor(RED);
  tft.print(String(iCO2Array[NOOFARRAY - 1]));
  tft.setTextColor(WHITE);
  tft.println(" ppm");
  tft.setCursor(0, 30);
  tft.print("Frischluft:");
  tft.setCursor(0, 40);
  if (iCO2Array[NOOFARRAY - 1] < CO2OK) {
    tft.setTextColor(GREEN);
    tft.print("nicht notwendig!");
    tft.drawBitmap(105, 25, okay, 20, 20, GREEN);
  }
  else if (iCO2Array[NOOFARRAY - 1] >= CO2OK && iCO2Array[NOOFARRAY - 1] < CO2NOK)
  {
    tft.setTextColor(YELLOW);
    tft.print("ist sinnvoll!");
    tft.drawBitmap(105, 25, warning, 20, 20, YELLOW);
  }
  else
  {
    tft.setTextColor(RED);
    tft.print("sofort notwendig!");
    tft.drawBitmap(105, 25, error, 20, 20, RED);
  }
}

void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { 
    strip.setPixelColor(i, color); 
    strip.show();
    delay(wait);
  }
}
