//Libraries
#include <Arduino.h>
#include "MHZ19.h"
#include <SoftwareSerial.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include "bitmap.h"

//Pin definitions
#define RX_PIN D4         //MH-Z19 RX-PIN                                         
#define TX_PIN D3         //MH-Z19 TX-PIN                                   
#define DHTPIN D6         //DHT11 Pin
#define BAUDRATE 9600    //Terminal Baudrate

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

#define SCLK_PIN D5
#define MOSI_PIN D7
#define DC_PIN   D1
#define CS_PIN   D2
#define RST_PIN  D0

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF
//
//Define CO2
#define CO2OK 1000
#define CO2NOK 2000
//Define map limits
#define MinMapValue 10
#define MaxMapValue 65

//Some definitions
#define NOOFARRAY   15
#define NOOFENTRIES 60
#define UPDATETIME  60000

//Create needed objects
Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, CS_PIN, DC_PIN, MOSI_PIN, SCLK_PIN, RST_PIN);
//

//Some global vars
bool bFirstRun = true;
double dCurrentTemp = 0.0;
float fCurrentHum = 0.0;
float fcurrentCO2 = 0.0;
int iCO2Array[NOOFARRAY];  //Array for temporal course CO2
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

  //Start with some text in serial monitor and tft
  tft.fillScreen(BLACK);  // Clear screen
  tft.setTextSize(2);
  tft.setTextColor(RED);
  tft.setCursor(50, 10);
  tft.print("CO2");
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
  
  tft.fillScreen(BLACK);  // Clear screen
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
  
  tft.fillScreen(BLACK);  // Clear screen
  tft.setTextColor(WHITE);
  tft.setCursor(0, 60);
  tft.print("Waiting 10 seconds");
  tft.setCursor(0, 70);
  tft.print("for MH-Z19 sensor ... ");
  delay(10000);
  tft.setTextColor(WHITE);
  tft.setCursor(0, 60);
  tft.print("Continuing ... ");
  tft.fillScreen(BLACK);  // Clear screen
  
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
      Serial.println(myMHZ19.errorCode);          // Get the Error Code value
      tft.setTextColor(RED);
      tft.println("FAIL");
      while (1); //Endless loop
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
  myMHZ19.autoCalibration();
  tft.setTextColor(YELLOW);
  tft.setCursor(40, 50);
  tft.print("MH-Z19");
  tft.setCursor(0, 60);
  tft.print("Autocalibration ON");
  delay(2000);
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
}

void loop() {
  //Just want data every 60sec or during first start
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
  //Swap data array for the last minutes
  for (int iCnt = 0; iCnt < (NOOFARRAY - 1); iCnt++)
    iCO2Array[iCnt] = iCO2Array[iCnt + 1];
  iCO2Array[NOOFARRAY - 1] = iCO2;
  Serial.println("Current values in array:");
  for (int iCnt = 0; iCnt < NOOFARRAY; iCnt++)
  {
    if ( iCnt != 0)
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
  tft.fillScreen(BLACK);
  tft.drawLine(5, 115, 5, 50, WHITE);  // Y-axis
  tft.drawLine(5, 115, 128, 115, WHITE); // X-Axis
  
  //x-axis label
  tft.setTextColor(WHITE);
  tft.setCursor(8, 120);
  tft.print("0 Min.");
  tft.setCursor(85, 120);
  tft.print("12 Min.");
  
  //Arrows Y-Axis
  tft.drawLine(5, 50, 10, 55, WHITE);
  tft.drawLine(5, 50, 0, 55, WHITE);
  
  //Arrows X-Axis
  tft.drawLine(128, 115, 123, 110, WHITE);
  tft.drawLine(128, 115, 123, 120, WHITE);

  //Get highest PPM to calculate max for graph
  int iHighestPPM = 0;
  int iHighestMap = 0;
  for (int iCnt = 0; iCnt < NOOFARRAY; iCnt++)
  {
    if (iHighestPPM < iCO2Array[iCnt])
    {
      iHighestPPM = iCO2Array[iCnt];
      Serial.println("Highest PPM: " + String(iCO2Array[iCnt]) + " ppm");
      iHighestMap = int(iHighestPPM / 100) * 100 + 200;
      Serial.println("Calc highest map: " + String(iHighestMap));
    }
  }

  //Go through array, starting at the end
  int iNoValues = 0;
  int iSumArray = 0;
  for (int iCnt = NOOFARRAY - 1; iCnt >= 0; iCnt--)
  {
    if (iCO2Array[iCnt] != 0)
    {
      iNoValues++;
      iSumArray += iCO2Array[iCnt];
      //Correction and mapping value for graph
      int iMappedValue = map(iCO2Array[iCnt], 400, iHighestMap, MinMapValue, MaxMapValue);
      //Find correct graph-color
      uint32_t color;
      if (iCO2Array[iCnt] < CO2OK)
        color = GREEN;
      else if (iCO2Array[iCnt] >= CO2OK && iCO2Array[iCnt] < CO2NOK)
        color = YELLOW;
      else
        color = RED;
      //Draw the graph-segments for each item from array
      tft.drawLine(5 + (NOOFARRAY - 1 - iCnt) * 10 + 2, 114 - iMappedValue, 5 + (NOOFARRAY - 1 - iCnt) * 10 + 2, 114, color);
      tft.drawLine(5 + (NOOFARRAY - 1 - iCnt) * 10 + 3, 114 - iMappedValue, 5 + (NOOFARRAY - 1 - iCnt) * 10 + 3, 114, color);
      tft.drawLine(5 + (NOOFARRAY - 1 - iCnt) * 10 + 4, 114 - iMappedValue, 5 + (NOOFARRAY - 1 - iCnt) * 10 + 4, 114, color);
      tft.drawLine(5 + (NOOFARRAY - 1 - iCnt) * 10 + 5, 114 - iMappedValue, 5 + (NOOFARRAY - 1 - iCnt) * 10 + 5, 114, color);
      tft.drawLine(5 + (NOOFARRAY - 1 - iCnt) * 10 + 6, 114 - iMappedValue, 5 + (NOOFARRAY - 1 - iCnt) * 10 + 6, 114, color);
      tft.drawLine(5 + (NOOFARRAY - 1 - iCnt) * 10 + 7, 114 - iMappedValue, 5 + (NOOFARRAY - 1 - iCnt) * 10 + 7, 114, color);
      tft.drawLine(5 + (NOOFARRAY - 1 - iCnt) * 10 + 8, 114 - iMappedValue, 5 + (NOOFARRAY - 1 - iCnt) * 10 + 8, 114, color);
      tft.drawLine(5 + (NOOFARRAY - 1 - iCnt) * 10 + 9, 114 - iMappedValue, 5 + (NOOFARRAY - 1 - iCnt) * 10 + 9, 114, color);
      tft.drawLine(5 + (NOOFARRAY - 1 - iCnt) * 10 + 10, 114 - iMappedValue, 5 + (NOOFARRAY - 1 - iCnt) * 10 + 10, 114, color);
    }
  }

  //Draw Average
  int iAverage = int((iSumArray / iNoValues) / 100) * 100;
  if ((iAverage % 100) >= 50)
    iAverage += 50;
  int iMappedValue = map(iAverage, 400, iHighestMap, MinMapValue, MaxMapValue);
  tft.drawLine(6, 114 - iMappedValue, 120, 114 - iMappedValue, BLUE);
  tft.setTextSize(1);
  tft.setTextColor(BLUE);
  tft.setCursor(100, 114 - iMappedValue - 10);
  tft.print(iAverage);

  //Write latest data to display
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
  if (iCO2Array[NOOFARRAY - 1] < CO2OK)
  {
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
