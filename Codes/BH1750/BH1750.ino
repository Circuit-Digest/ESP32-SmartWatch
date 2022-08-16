

#include <SPI.h>
#include <Wire.h>
#include <TFT_eSPI.h> // Modified library for ST7789 Display
#include <BH1750.h> //BH1750 Library
#include "Luxmetervector.h" //Image data
#include "fonts.h" // Font file
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
BH1750 lightMeter;    //BH1750 Instance
int angle = 0;


void setup(void) {
  Wire.begin();//Init I2C
  lightMeter.begin();//Init BH1750 library
  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  tft.pushImage(0, 0, 240, 280, Lux_meter_vector);
  img.setSwapBytes(true);
  img.createSprite(163, 57);
  img.setTextDatum(4);
  img.setTextColor(0x0081, background);
  img.setFreeFont(&DSEG7_Classic_Bold_30);
}

void loop() {
  int lux = lightMeter.readLightLevel();
  img.fillSprite(background);
  img.drawString(String(lux), 30, 30);
  img.pushSprite(40, 40);
  delay(500);
}
