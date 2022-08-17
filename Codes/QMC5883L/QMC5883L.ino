#include <SPI.h>
#include <Wire.h>
#include <TFT_eSPI.h> // Modified library for ST7789 Display
#include <DFRobot_QMC5883.h>
#include "dial240.h" //Image data
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
DFRobot_QMC5883 compass;
int angle = 0;
bool cw = true;
void setup(void) {
  Serial.begin(115200);
  compass.begin();
    if(compass.isHMC()){
        Serial.println("Initialize HMC5883");
        compass.setRange(HMC5883L_RANGE_1_3GA);
        compass.setMeasurementMode(HMC5883L_CONTINOUS);
        compass.setDataRate(HMC5883L_DATARATE_15HZ);
        compass.setSamples(HMC5883L_SAMPLES_8);
    }
   else if(compass.isQMC()){
        Serial.println("Initialize QMC5883");
        compass.setRange(QMC5883_RANGE_2GA);
        compass.setMeasurementMode(QMC5883_CONTINOUS); 
        compass.setDataRate(QMC5883_DATARATE_50HZ);
        compass.setSamples(QMC5883_SAMPLES_8);
   }
  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  int xw = tft.width()/2;   // xw, yh is middle of screen
  int yh = tft.height()/2;
  tft.setPivot(xw, yh);     // Set pivot to middle of TFT screen
  //img.setColorDepth(8);
  img.createSprite(240, 240);
  img.setTextDatum(4);
}

void loop() {
  Vector norm = compass.readNormalize();

  // Calculate heading
  float heading = atan2(norm.YAxis, norm.XAxis);

  // Set declination angle on your location and fix heading
  // You can find your declination on: http://magnetic-declination.com/
  // (+) Positive or (-) for negative
  // For my location declination angle is -1'22W (negative)
  // Formula: (deg + (min / 60.0)) / (180 / M_PI);
  float declinationAngle = (-1.0 + (-22.0 / 60.0)) / (180 / PI);
  heading += declinationAngle;

  // Correct for heading < 0deg and heading > 360deg
  if (heading < 0){
    heading += 2 * PI;
  }

  if (heading > 2 * PI){
    heading -= 2 * PI;
  }

  // Convert to degrees
  float headingDegrees = heading * 180/M_PI; 
  img.fillSprite(TFT_BLACK);
  img.pushImage(0, 0, 240, 240, dial240);
  img.pushRotated(angle,TFT_PINK);// create rotated image as per the angle from the compass sensor
  delay(14);
}
