

#include <SPI.h>
#include <Wire.h>
#include <TFT_eSPI.h> // Modified library for ST7789 Display
#include "MAX30105.h" // SparkFun librarry for MAX30102 sensor
#include "heartRate.h" // Heartrate measurement algorithm
#include "hr1.h"
#include "hr2.h"
#include "fonts.h" // Font file
#include "NotoSansBold15.h"
#include "NotoSansBold36.h"
// The font names are arrays references, thus must NOT be in quotes ""
#define FONT_SMALL NotoSansBold15
#define FONT_LARGE NotoSansBold36
#define TFT_GREY 0x5AEB
#define TFT_SKYBLUE 0x067D
#define color1 TFT_WHITE
#define color2  0x8410//0x8410
#define color3 0x5ACB
#define color4 0x15B3
#define color5 0x00A3
#define background 0x65B8
TFT_eSPI tft = TFT_eSPI();       // TFT_eSPI Instance
TFT_eSprite img = TFT_eSprite(&tft);   //Create sprite for faster updation
MAX30105 particleSensor;//MAX30102 instance
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;
bool beat = false;


void setup(void) {
  Serial.begin(115200);
  Serial.println("Initializing...");
  Wire.begin();//Init I2C
  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  // Initialize sensor
  particleSensor.begin(Wire, I2C_SPEED_FAST);

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED

  //  tft.pushImage(0, 0, 240, 280, Lux_meter_vector);
  img.setSwapBytes(true);
  img.setColorDepth(8);
  img.createSprite(240, 280);
  img.setTextDatum(4);
  img.setTextColor(0x0081, background);
  img.setFreeFont(&DSEG7_Classic_Bold_30);
}

void loop() {
  long irValue = particleSensor.getIR();    //Reading the IR value it will permit us to know if there's a finger on the sensor or not
  //Also detecting a heartbeat
  if (checkForBeat(irValue) == true)                        //If a heart beat is detected
  {
    img.fillSprite(TFT_BLACK);                                         //Clear the display
    if (beat == true)
    {
      img.pushImage(0, 0, 240, 280, hr1);
    }
    else
    {
      img.pushImage(0, 0, 240, 280, hr2);
    }
    beat = !beat;
    img.setTextColor(TFT_CYAN, TFT_BLACK);
    img.loadFont(FONT_LARGE);
    img.setCursor(100, 130);
    img.print(beatAvg);
    img.setCursor(140, 145);
    img.loadFont(FONT_SMALL);
    img.print("BPM ");
    img.pushSprite(0, 0);
    //We sensed a beat!
    long delta = millis() - lastBeat;                   //Measure duration between two beats
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);           //Calculating the BPM

    if (beatsPerMinute < 255 && beatsPerMinute > 20)               //To calculate the average we strore some values (4) then do some math to calculate the average
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  else if (irValue < 50000) {      //If no finger is detected it inform the user and put the average BPM to 0 or it will be stored for the next measure
    beatAvg = 0;
    img.loadFont(FONT_SMALL);
    img.setCursor(80, 120);
    img.setTextColor(TFT_CYAN, TFT_BLACK);
    img.fillSprite(TFT_BLACK);
    img.println("Please Place ");
    img.setCursor(80, 140);
    img.println("your finger ");
    img.pushSprite(0, 0);
  }
  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  if (irValue < 50000)
    Serial.print(" No finger?");

  Serial.println();

}
