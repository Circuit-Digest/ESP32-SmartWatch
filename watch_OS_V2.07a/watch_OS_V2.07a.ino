#include <SPI.h>
#include <TFT_eSPI.h>  // Hardware-specific library
#include <ESP32Time.h>
#include "driver/gpio.h"
#include "esp_sleep.h"
#include <EEPROM.h>
#include "OneButton.h"
#include <QMC5883L.h>
#include <BH1750.h>  //BH1750 Library
#include "Free_Fonts.h"
#include "MAX30105.h"   // SparkFun librarry for MAX30102 sensor
#include "heartRate.h"  // Heartrate measurement algorithm
#include "dial240.h"    //Image data
#include "fonts.h"
#include "images.h"
#define PIN_INPUT 0
#define EEPROM_SIZE 25
#define FONT_SMALL NotoSansBold15
#define FONT_LARGE NotoSansBold36
#define TFT_GREY 0x5AEB
#define TFT_SKYBLUE 0x067D
#define color1 TFT_WHITE
#define color2 0x8410  //0x8410
#define color3 0x5ACB
#define color4 0x15B3
#define color5 0x00A3
#define colour6 0x0926
#define colour7 TFT_BLACK
#define Light_Green 0x07E8
#define background 0xB635
#define LCD_BACKLIGHT 4
#define TFTW 240          // screen width
#define TFTH 280          // screen height
#define TFTW2 (TFTW / 2)  // half screen width
#define TFTH2 (TFTH / 2)  // half screen height
#define SPEED 1
#define GRAVITY 9.8
#define JUMP_FORCE 2.15
#define SKIP_TICKS 20.0  // 1000 / 50fps
#define MAX_FRAMESKIP 5
#define BIRDW 16      // bird width
#define BIRDH 16      // bird height
#define BIRDW2 8      // half width
#define BIRDH2 8      // half height
#define PIPEW 24      // pipe width
#define GAPHEIGHT 42  // pipe gap height
#define FLOORH 30     // floor height (from bottom of the screen)
#define GRASSH 4      // grass height (inside floor, starts at floor y)
#define COLOR565(r, g, b) ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
#define BCKGRDCOL COLOR565(138, 235, 244)    // background
#define BIRDCOL COLOR565(255, 254, 174)      // bird
#define PIPECOL COLOR565(99, 255, 78)        // pipe
#define PIPEHIGHCOL COLOR565(250, 255, 250)  // pipe highlight
#define PIPESEAMCOL COLOR565(0, 0, 0)        // pipe seam
#define FLOORCOL COLOR565(246, 240, 163)     // floor
#define GRASSCOL COLOR565(141, 225, 87)      // grass (col2 is the stripe color)
#define GRASSCOL2 COLOR565(156, 239, 88)     // grass (col2 is the stripe color)
#define C0 BCKGRDCOL                         // bird sprite,bird sprite colors (Cx name for values to keep the array readable)
#define C1 COLOR565(195, 165, 75)
#define C2 BIRDCOL
#define C3 TFT_WHITE
#define C4 TFT_RED
#define C5 COLOR565(251, 216, 114)

ESP32Time rtc(0);   // RTC instance with offset in seconds
BH1750 lightMeter;  //BH1750 Instance
QMC5883L compass;
MAX30105 particleSensor;  //MAX30102 instance
OneButton button(PIN_INPUT, true);
TFT_eSPI tft = TFT_eSPI();  // Invoke custom library
TFT_eSprite img = TFT_eSprite(&tft);
TFT_eSprite img1 = TFT_eSprite(&tft);
TFT_eSprite img2 = TFT_eSprite(&tft);



static const unsigned int birdcol[] = {
  C0, C0, C1, C1, C1, C1, C1, C0, C0, C0, C1, C1, C1, C1, C1, C0,
  C0, C1, C2, C2, C2, C1, C3, C1, C0, C1, C2, C2, C2, C1, C3, C1,
  C0, C2, C2, C2, C2, C1, C3, C1, C0, C2, C2, C2, C2, C1, C3, C1,
  C1, C1, C1, C2, C2, C3, C1, C1, C1, C1, C1, C2, C2, C3, C1, C1,
  C1, C2, C2, C2, C2, C2, C4, C4, C1, C2, C2, C2, C2, C2, C4, C4,
  C1, C2, C2, C2, C1, C5, C4, C0, C1, C2, C2, C2, C1, C5, C4, C0,
  C0, C1, C2, C1, C5, C5, C5, C0, C0, C1, C2, C1, C5, C5, C5, C0,
  C0, C0, C1, C5, C5, C5, C0, C0, C0, C0, C1, C5, C5, C5, C0, C0
};
// bird structure
static struct BIRD {
  long x, y, old_y;
  long col;
  float vel_y;
} bird;
// pipe structure
static struct PIPES {
  long x, gap_y;
  long col;
} pipes;
// score
int score;
// temporary x and y var
static short tmpx, tmpy;
// ---------------
// draw pixel
// ---------------
// faster drawPixel method by inlining calls and using setAddrWindow and pushColor using macro to force inlining
#define _drawPixel(a, b, c) \
  tft.setAddrWindow(a, b, a, b); \
  tft.pushColor(c)

uint maxScore = 0;
float sx = 0, sy = 1, mx = 1, my = 0, hx = -1, hy = 0;  // Saved H, M, S x & y multipliers
float sdeg = 0, mdeg = 0, hdeg = 0;
uint16_t osx = 120, osy = 140, omx = 120, omy = 140, ohx = 120, ohy = 140;  // Saved H, M, S x & y coords
uint16_t x0 = 0, x1 = 0, yy0 = 0, yy1 = 0;
uint32_t targetTime = 0;                       // for next 1 second timeout
static uint8_t conv2d(const char* p);          // Forward declaration needed for IDE 1.6.x
uint8_t hh = 0, t_mm = 0, t_dd = 0, t_mn = 0;  //
uint32_t t_yr = 0;
uint8_t t_hh = 0, mm = 0, ss = 0;
unsigned long lastfacechange = 0;
unsigned long lastwake = 0;
unsigned long lastpressed = 0;
unsigned long lastvaluechange = 0;
bool initial = 1;
volatile int counter = 0;
float VALUE;
float lastValue = 0;
int lastsec = 0;
int pressstate = 0;
unsigned long lastDisplayUpdate = 0;
const byte RATE_SIZE = 4;  //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];     //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0;  //Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;
bool beat = false;
double rad = 0.01745;
float x[360];
float y[360];
bool facechange = false;
bool Screenchange = false;
float px[360];
float py[360];
float lx[360];
float ly[360];
int r = 104;
int ssx = 120;
int ssy = 140;
String cc[12] = { "45", "40", "35", "30", "25", "20", "15", "10", "05", "0", "55", "50" };
String days[] = { "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY" };
String days1[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
int start[12];
int startP[60];
const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;
int angle = 0;
bool onOff = 0;
bool debounce = 0;
int watchface = 0, Screen = 0, SubScreen = 0, Autoscreen, AutoBright, AutoscreenTime, Brigtnesslevel;
String h, m, s, d1, d2, m1, m2;
unsigned long pressStartTime;


// This function is called from the interrupt when the signal on the PIN_INPUT has changed.
// do not use Serial in here.
void IRAM_ATTR checkTicks() {
  // include all buttons here to be checked
  button.tick();  // just call tick() to check the state.
}


// this function will be called for short click.
void ShortClick() {
  Serial.println("singleClick() detected.");
  lastwake = millis();
  if (Screen == 0) {
    SubScreen++;
    if (SubScreen > 2) {
      SubScreen = 0;
      facechange = true;
    }
    if (SubScreen == 1) {
      particleSensor.wakeUp();
    } else {
      particleSensor.shutDown();
    }
  } else if (Screen == 1) {
    SubScreen++;
    if (SubScreen > 4) {
      SubScreen = 0;
    }
    Screenchange = true;
  } else if (Screen == 2) {
    watchface++;
    if (watchface > 5) {
      watchface = 0;
    }

    EEPROM.write(0, watchface);
    EEPROM.commit();
    facechange = true;
    Screenchange = true;
  } else if (Screen == 3) {
    SubScreen++;
    if (SubScreen > 5) {
      SubScreen = 0;
    }
    Screenchange = true;
  } else if (Screen == 4) {
    SubScreen++;
    if (SubScreen > 2) {
      SubScreen = 0;
    }
    Screenchange = true;
  } else if (Screen == 5) {
    Screen = 6;
    game_init();
    game_loop();
  } else if (Screen == 6) {
    Screen = 7;
  } else if (Screen == 7) {
    Screen = 5;
    Screenchange = true;
  }

  Serial.print("Sub ");
  Serial.println(SubScreen);
  tft.fillScreen(colour7);
  pressstate = 1;
  facechange = true;
  lastDisplayUpdate = millis();
  lastpressed = millis();
}  // ShortClick


// long press
void LongPress() {
  Serial.println("pressStart()");
  pressStartTime = millis() - 1000;  // as set in setPressTicks()
  lastwake = millis();
  lastDisplayUpdate = millis();
  particleSensor.shutDown();
  if (Screen == 0) {
    Screen = 1;
    SubScreen = 0;
  } else if (Screen == 1) {
    if (SubScreen == 0) {
      Screen = 2;
      SubScreen = 0;
    } else if (SubScreen == 1) {
      Screen = 3;
      t_hh = rtc.getHour();
      t_mm = rtc.getMinute();
      t_dd = rtc.getDay();
      t_mn = rtc.getMonth();
      t_yr = rtc.getYear();
      Serial.println(rtc.getYear());
      Serial.println(t_yr);
      SubScreen = 0;
    } else if (SubScreen == 2) {
      Screen = 4;
      SubScreen = 0;
      facechange = true;
      Screenchange = true;
    } else if (SubScreen == 3) {
      Screen = 5;
      SubScreen = 0;
    } else if (SubScreen == 4) {
      Screen = 0;
      SubScreen = 0;
    }
  } else if (Screen == 2) {
    Screen = 1;
    SubScreen = 0;
  } else if (Screen == 3) {
    if (SubScreen == 0) {
      t_hh++;
      if (t_hh > 23) {
        t_hh = 0;
      }
    } else if (SubScreen == 1) {
      t_mm++;
      if (t_mm > 59) {
        t_mm = 0;
      }
    } else if (SubScreen == 2) {
      t_dd++;
      if (t_dd > 31) {
        t_dd = 0;
      }
    } else if (SubScreen == 3) {
      t_mn++;
      if (t_mn > 12) {
        t_mn = 0;
      }
    } else if (SubScreen == 4) {
      t_yr++;
      if (t_yr > 2041) {
        t_yr = 0;
      }
    } else {
      rtc.setTime(0, t_mm, t_hh, t_dd, t_mn, t_yr);
      Screen = 1;
      SubScreen = 1;
    }
  } else if (Screen == 4) {
    if (SubScreen == 0) {
      AutoBright++;
      if (AutoBright > 5) {
        AutoBright = 0;
      }
      if (AutoBright > 0) {
        analogWrite(LCD_BACKLIGHT, AutoBright * 50);
      }
      EEPROM.write(2, AutoBright);
      EEPROM.commit();
    } else if (SubScreen == 1) {
      Autoscreen++;
      if (Autoscreen > 5) {
        Autoscreen = 0;
      }
      EEPROM.write(1, Autoscreen);
      EEPROM.commit();
    } else if (SubScreen == 2) {
      Screen = 1;
      SubScreen = 2;
    }
  } else if (Screen == 5) {
    Screen = 1;
    SubScreen = 0;
  } else if (Screen == 7) {
    Screen = 1;
    SubScreen = 0;
  }

  facechange = true;
  Screenchange = true;
  pressstate = 1;
  lastpressed = millis();
}


void setup(void) {
  Serial.begin(115200);
  Serial.println("ESP32 Watch OS.");
  gpio_hold_dis((gpio_num_t)LCD_BACKLIGHT);
  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, LOW);
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.writeInt(10, 0);
  EEPROM.commit();
  if (EEPROM.read(0) > 3) {
    EEPROM.write(0, 4);
    EEPROM.commit();
  }
  watchface = EEPROM.read(0);

  if (EEPROM.read(1) > 5) {
    EEPROM.write(1, 5);
    EEPROM.commit();
  }
  Autoscreen = EEPROM.read(1);

  if (EEPROM.read(2) > 5) {
    EEPROM.write(2, 5);
    EEPROM.commit();
  }
  AutoBright = EEPROM.read(2);
  //rtc.setTime(ss, mm, hh, 0, 0, 0);  // 26th Jjuly 2022 compile date

  particleSensor.begin(Wire, I2C_SPEED_FAST);

  particleSensor.setup();                     //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A);  //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeIR(0xFF);   //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0);   //Turn off Green LED
  particleSensor.shutDown();
  compass.init();
  compass.setSamplingRate(50);

  tft.init();
  tft.setRotation(0);
  //tft.setColorDepth(16);
  tft.setSwapBytes(true);
  tft.fillScreen(colour7);
  int xw = tft.width() / 2;  // xw, yh is middle of screen
  int yh = tft.height() / 2;
  tft.setPivot(xw, yh);  // Set pivot to middle of TFT screen
  img.createSprite(240, 280);
  img.setTextDatum(4);
  img1.createSprite(240, 70);
  img1.setSwapBytes(true);
  img2.createSprite(240, 70);
  img2.setSwapBytes(true);
  targetTime = millis() + 1000;
  facechange = true;
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);  //1 = High, 0 = Low
  // setup interrupt routine
  // when not registering to the interrupt the sketch also works when the tick is called frequently.
  attachInterrupt(digitalPinToInterrupt(PIN_INPUT), checkTicks, CHANGE);

  // link the xxxclick functions to be called on xxxclick event.
  button.attachClick(ShortClick);

  button.setPressTicks(1000);  // that is the time when LongPressStart is called
  button.attachLongPressStart(LongPress);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);  //Init BH1750 library
  if (AutoBright == 0) {
    unsigned int lv = constrain(lightMeter.readLightLevel(), 50, 500);
    analogWrite(LCD_BACKLIGHT, lv / 2);
  } else {
    analogWrite(LCD_BACKLIGHT, AutoBright * 50);
  }

  lastwake = millis();
}
int lastAngle = 0;
float circle = 100;
bool dir = 0;
int rAngle = 359;


void loop() {
  button.tick();
  if (Screen < 5) {
    watchtask();
  } else {
    game();
  }
}



void watchtask() {

  if (pressstate == 1 && digitalRead(0) == 1) {
    pressstate = 0;
  }
  if (AutoBright == 0) {
    unsigned int lv = constrain(lightMeter.readLightLevel(), 50, 500);
    analogWrite(LCD_BACKLIGHT, lv / 2);
    Serial.print("Light");
    Serial.println(lv / 2);
  }
  if (Autoscreen != 0 && millis() - lastwake > Autoscreen * 60000) {
    analogWrite(LCD_BACKLIGHT, 0);
    delay(1000);

    tft.fillScreen(colour7);
    gpio_deep_sleep_hold_en();
    gpio_hold_en((gpio_num_t)LCD_BACKLIGHT);
    esp_deep_sleep_start();
  }
  if (Screen == 0) {
    if (SubScreen == 0) {
      watchfacedsp();
    } else if (SubScreen == 1) {
      HRApp();
    } else {
      CompassApp();
    }
  } else if (Screen == 1 && Screenchange == true) {
    if (SubScreen == 0) {
      tft.pushImage(0, 0, 240, 280, facechangeicon);
    } else if (SubScreen == 1) {
      tft.pushImage(0, 0, 240, 280, timeseticon);
    } else if (SubScreen == 2) {
      tft.pushImage(0, 0, 240, 280, settingsicon);
    } else if (SubScreen == 3) {
      tft.pushImage(0, 0, 240, 280, gamesicon);
    } else if (SubScreen == 4) {
      tft.pushImage(0, 0, 240, 280, exiticon);
    }
  } else if (Screen == 3 && Screenchange == true) {

    timesetiings();
    Screenchange = false;
  } else if (Screen == 2 && Screenchange == true) {
    watchfacedsp();
    Screenchange = false;
  } else if (Screen == 4 && Screenchange == true) {
    settings();
    Screenchange = false;
  } else if (Screen == 3 && millis() - lastpressed > 2000 && millis() - lastvaluechange > 500 && pressstate == 1) {
    if (SubScreen == 0) {
      t_hh++;
      if (t_hh > 23) {
        t_hh = 0;
      }
      facechange = true;
      Screenchange = true;
    } else if (SubScreen == 1) {
      t_mm++;
      if (t_mm > 59) {
        t_mm = 0;
      }
      facechange = true;
      Screenchange = true;
    } else if (SubScreen == 2) {
      facechange = true;
      Screenchange = true;
      t_dd++;
      if (t_dd > 31) {
        t_dd = 0;
      }
    } else if (SubScreen == 3) {
      t_mn++;
      if (t_mn > 12) {
        t_mn = 0;
      }
      facechange = true;
      Screenchange = true;
    } else if (SubScreen == 4) {
      t_yr++;
      if (t_yr > 2041) {
        t_yr = 0;
      }
      facechange = true;
      Screenchange = true;
    }
    lastvaluechange = millis();
  }
}



void timesetiings() {
  tft.pushImage(0, 0, 240, 280, TimeSettings);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setFreeFont(FF18);
  tft.setTextSize(0);
  int tt_hh = t_hh;
  if (tt_hh > 12) {
    tt_hh = tt_hh - 12;
  }
  if (tt_hh < 10) {
    tft.drawString("0" + String(tt_hh), 25, 96);
  } else {
    tft.drawString(String(tt_hh), 25, 96);
  }

  if (t_mm < 10) {
    tft.drawString("0" + String(t_mm), 93, 96);
  } else {
    tft.drawString(String(t_mm), 93, 96);
  }
  if (t_hh < 13) {
    tft.drawString("AM", 164, 96);
  } else {
    tft.drawString("PM", 164, 96);
  }
  if (t_dd < 10) {
    tft.drawString("0" + String(t_dd), 25, 183);
  } else {
    tft.drawString(String(t_dd), 25, 183);
  }
  if (t_mn < 10) {
    tft.drawString("0" + String(t_mn), 93, 183);
  } else {
    tft.drawString(String(t_mn), 93, 183);
  }
  if (t_yr < 2022) {
    t_yr = 2022;
  }
  tft.drawString(String(t_yr), 161, 183);
  if (SubScreen == 0) {
    tft.drawRoundRect(9, 81, 58, 48, 6, Light_Green);
  } else if (SubScreen == 1) {
    tft.drawRoundRect(77, 81, 58, 48, 6, Light_Green);
  } else if (SubScreen == 2) {
    tft.drawRoundRect(9, 168, 58, 48, 6, Light_Green);
  } else if (SubScreen == 3) {
    tft.drawRoundRect(77, 168, 58, 48, 6, Light_Green);
  } else if (SubScreen == 4) {
    tft.drawRoundRect(144, 168, 88, 48, 6, Light_Green);
  } else {
    tft.drawRoundRect(79, 226, 88, 48, 6, Light_Green);
  }
}



void settings() {
  tft.pushImage(0, 0, 240, 280, Settingspage);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setFreeFont(FF18);
  tft.setTextSize(0);
  switch (AutoBright) {
    case 0: tft.drawString("  Auto  ", 80, 100); break;
    case 1: tft.drawString("  20%   ", 85, 100); break;
    case 2: tft.drawString("  40%   ", 85, 100); break;
    case 3: tft.drawString("  60%   ", 85, 100); break;
    case 4: tft.drawString("  80%   ", 85, 100); break;
    case 5: tft.drawString(" 100%   ", 80, 100); break;
  }
  switch (Autoscreen) {
    case 0: tft.drawString("Always On", 65, 185); break;
    case 1: tft.drawString("1 Minute ", 75, 185); break;
    case 2: tft.drawString("2 Minute ", 75, 185); break;
    case 3: tft.drawString("3 Minute ", 75, 185); break;
    case 4: tft.drawString("4 Minute ", 75, 185); break;
    case 5: tft.drawString("5 Minute ", 75, 185); break;
  }

  if (SubScreen == 0) {
    tft.drawRoundRect(16, 85, 208, 48, 6, Light_Green);
  } else if (SubScreen == 1) {
    tft.drawRoundRect(16, 169, 208, 48, 6, Light_Green);
  } else {
    tft.drawRoundRect(66, 225, 108, 48, 6, Light_Green);
  }
}


void HRApp() {
  long irValue = particleSensor.getIR();  //Reading the IR value it will permit us to know if there's a finger on the sensor or not
  //Also detecting a heartbeat
  if (checkForBeat(irValue) == true)  //If a heart beat is detected
  {

    long delta = millis() - lastBeat;  //Measure duration between two beats
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);  //Calculating the BPM

    if (beatsPerMinute < 255 && beatsPerMinute > 20)  //To calculate the average we strore some values (4) then do some math to calculate the average
    {
      rates[rateSpot++] = (byte)beatsPerMinute;  //Store this reading in the array
      rateSpot %= RATE_SIZE;                     //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }
  if (millis() - lastDisplayUpdate > 500) {
    if (irValue < 60000) {  //If no finger is detected it inform the user and put the average BPM to 0 or it will be stored for the next measure
      beatAvg = 0;
      img.loadFont(FONT_SMALL);
      img.setCursor(80, 120);
      img.setTextColor(TFT_CYAN, colour7);
      img.fillSprite(colour7);
      img.println("Please Place ");
      img.setCursor(80, 140);
      img.println("your finger ");
      img.pushSprite(0, 0);
    } else {
      img.fillSprite(colour7);  //Clear the display
      if (beat == true) {
        img.pushImage(0, 0, 240, 280, hr1);
      } else {
        img.pushImage(0, 0, 240, 280, hr2);
      }
      beat = !beat;

      img.setTextColor(TFT_CYAN, colour7);
      img.loadFont(FONT_LARGE);
      img.setCursor(100, 130);
      img.print(beatAvg);
      img.setCursor(100, 175);
      img.loadFont(FONT_SMALL);
      img.print("BPM ");
      img.pushRotated(0);
    }
    lastDisplayUpdate = millis();
  }
  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  if (irValue < 60000)
    Serial.print(" No finger?");
  if (millis() - lastBeat > 5000) {
    beatsPerMinute = 0;
    beatAvg = 0;
  }
  Serial.println();
}


void CompassApp() {
  for (int i = 0; i < 10; i++) {
    angle = angle + compass.readHeading();
  }
  angle = random(355, 360);
  angle = angle / 10;
  img.fillSprite(colour7);
  img.pushImage(0, 20, 240, 240, dial240);
  img.pushRotated(angle);  // create rotated image as per the angle from the compass sensor
  angle = 0;
}
void watchfacedsp() {
  if (facechange) {
    tft.fillScreen(colour7);
    if (watchface == 1) {
      tft.setTextSize(0);
      tft.pushImage(0, 0, 240, 280, Casio2);
      tft.setTextColor(0x0081, background);
      tft.fillRoundRect(48, 127, 128, 48, 5, background);
    } else if (watchface == 2) {
      tft.setTextSize(0);
      tft.pushImage(0, 0, 240, 280, Casio1);
      tft.setTextColor(0x0081, background);
      tft.fillRoundRect(48, 127, 128, 48, 5, background);
    } else if (watchface == 4) {
      tft.pushImage(0, 0, 240, 280, cdface1);
      img2.pushImage(0, 0, 240, 100, cdface11);
      tft.setTextColor(TFT_WHITE);
      tft.setFreeFont(FF18);
      tft.setTextSize(2);
    } else if (watchface == 5) {
      tft.pushImage(0, 0, 240, 280, cdface2);
      img2.pushImage(0, 0, 240, 100, cdface12);
      tft.setTextColor(TFT_WHITE);
      tft.setFreeFont(FF18);
      tft.setTextSize(2);
    }
    facechange = false;
    lastfacechange = millis();
  }
  if (watchface == 0) {
    int b = 0;
    int b2 = 0;
    for (int i = 0; i < 360; i++) {
      x[i] = (r * cos(rad * i)) + ssx;
      y[i] = (r * sin(rad * i)) + ssy;
      px[i] = ((r - 16) * cos(rad * i)) + ssx;
      py[i] = ((r - 16) * sin(rad * i)) + ssy;

      lx[i] = ((r - 26) * cos(rad * i)) + ssx;
      ly[i] = ((r - 26) * sin(rad * i)) + ssy;

      if (i % 30 == 0) {
        start[b] = i;
        b++;
      }

      if (i % 6 == 0) {
        startP[b2] = i;
        b2++;
      }
    }

    rAngle = rAngle - 2;

    angle = rtc.getSecond() * 6;

    s = String(rtc.getSecond());
    m = String(rtc.getMinute());
    h = String(rtc.getHour());

    if (m.toInt() < 10)
      m = "0" + m;

    if (h.toInt() < 10)
      h = "0" + h;

    if (s.toInt() < 10)
      s = "0" + s;


    if (rtc.getDay() > 10) {
      d1 = rtc.getDay() / 10;
      d2 = rtc.getDay() % 10;
    } else {
      d1 = "0";
      d2 = String(rtc.getDay());
    }

    if (rtc.getMonth() > 10) {
      m1 = rtc.getMonth() / 10;
      m2 = rtc.getMonth() % 10;
    } else {
      m1 = "0";
      m2 = String(rtc.getMonth());
    }


    if (angle >= 360)
      angle = 0;

    if (rAngle <= 0)
      rAngle = 359;



    if (dir == 0)
      circle = circle + 0.5;
    else
      circle = circle - 0.5;

    if (circle > 140)
      dir = !dir;

    if (circle < 100)
      dir = !dir;



    if (angle > -1) {
      lastAngle = angle;

      VALUE = ((angle - 270) / 3.60) * -1;
      if (VALUE < 0)
        VALUE = VALUE + 100;



      img.fillSprite(colour7);
      img.fillCircle(ssx, ssy, 124, colour7);

      img.setTextColor(TFT_WHITE, colour7);

      img.drawString(days[rtc.getDayofWeek()], circle, 140, 2);


      for (int i = 0; i < 12; i++)
        if (start[i] + angle < 360) {
          img.drawString(cc[i], x[start[i] + angle], y[start[i] + angle], 2);
          img.drawLine(px[start[i] + angle], py[start[i] + angle], lx[start[i] + angle], ly[start[i] + angle], color1);
        } else {
          img.drawString(cc[i], x[(start[i] + angle) - 360], y[(start[i] + angle) - 360], 2);
          img.drawLine(px[(start[i] + angle) - 360], py[(start[i] + angle) - 360], lx[(start[i] + angle) - 360], ly[(start[i] + angle) - 360], color1);
        }
      img.setFreeFont(&DSEG7_Modern_Bold_20);
      img.drawString(s, ssx, ssy - 36);
      img.setFreeFont(&DSEG7_Classic_Regular_28);
      img.drawString(h + ":" + m, ssx, ssy + 28);
      img.setTextFont(0);

      img.fillRect(70, 86, 12, 20, color3);
      img.fillRect(84, 86, 12, 20, color3);
      img.fillRect(150, 86, 12, 20, color3);
      img.fillRect(164, 86, 12, 20, color3);

      img.setTextColor(0x35D7, colour7);
      img.drawString("MONTH", 84, 78);
      img.drawString("DAY", 162, 78);
      img.setTextColor(TFT_SKYBLUE, colour7);
      img.drawString("Circuit Digest", 120, 194);
      img.drawString("***", 120, 124);
      img.setTextColor(TFT_WHITE, color3);
      img.drawString(m1, 77, 96, 2);
      img.drawString(m2, 91, 96, 2);
      img.drawString(d1, 157, 96, 2);
      img.drawString(d2, 171, 96, 2);
      for (int i = 0; i < 60; i++)
        if (startP[i] + angle < 360)
          img.fillCircle(px[startP[i] + angle], py[startP[i] + angle], 1, color1);
        else
          img.fillCircle(px[(startP[i] + angle) - 360], py[(startP[i] + angle) - 360], 1, color1);
      img.fillTriangle(ssx - 1, ssy - 70, ssx - 5, ssy - 56, ssx + 4, ssy - 56, TFT_ORANGE);
      img.fillCircle(px[rAngle], py[rAngle], 6, TFT_RED);
      img.pushSprite(0, 0);
    }
  } else if (rtc.getSecond() != lastsec || Screen == 3) {
    if (watchface == 1 || watchface == 2) {
      /*
      String med;
      if (rtc.getSecond() % 2) {
        med = ":";
      } else {
        med = " ";
      }
      */
      tft.setFreeFont(&DSEG7_Classic_Bold_30);
      if (rtc.getHour() > 9 && rtc.getMinute() > 9) {
        tft.drawString(String(rtc.getHour()) + ":" + String(rtc.getMinute()), 46, 135);
      } else if (rtc.getHour() < 10 && rtc.getMinute() > 9) {

        tft.drawString("0" + String(rtc.getHour()) + ":" + String(rtc.getMinute()), 46, 135);
      } else if (rtc.getHour() > 9 && rtc.getMinute() < 10) {

        tft.drawString(String(rtc.getHour()) + ":0" + String(rtc.getMinute()), 46, 135);
      } else {

        tft.drawString("0" + String(rtc.getHour()) + ":0" + String(rtc.getMinute()), 46, 135);
      }
      tft.setFreeFont(&DSEG7_Classic_Bold_20);
      if (rtc.getSecond() < 10) {
        tft.drawString("0" + String(rtc.getSecond()), 154, 145);
      } else {
        tft.drawString(String(rtc.getSecond()), 154, 145);
      }
      tft.setFreeFont(&DSEG14_Classic_Bold_18);
      tft.drawString(days1[rtc.getDayofWeek()], 94, 106);
      tft.drawString(String(rtc.getDay()), 156, 106);
    } else if (watchface == 3) {
      img.setTextColor(TFT_WHITE, colour7);  // Adding a background colour erases previous text automatically
      // Draw clock face
      img.fillCircle(120, 140, 118, TFT_GREEN);
      img.fillCircle(120, 140, 110, colour7);
      // Draw 12 lines
      for (int i = 0; i < 360; i += 30) {
        sx = cos((i - 90) * 0.0174532925);
        sy = sin((i - 90) * 0.0174532925);
        x0 = sx * 114 + 120;
        yy0 = sy * 114 + 140;
        x1 = sx * 100 + 120;
        yy1 = sy * 100 + 140;
        img.drawLine(x0, yy0, x1, yy1, TFT_GREEN);
      }
      // Draw 60 dots
      for (int i = 0; i < 360; i += 6) {
        sx = cos((i - 90) * 0.0174532925);
        sy = sin((i - 90) * 0.0174532925);
        x0 = sx * 102 + 120;
        yy0 = sy * 102 + 140;
        // Draw minute markers
        img.drawPixel(x0, yy0, TFT_WHITE);
        // Draw main quadrant dots
        if (i == 0 || i == 180) img.fillCircle(x0, yy0, 2, TFT_WHITE);
        if (i == 90 || i == 270) img.fillCircle(x0, yy0, 2, TFT_WHITE);
      }
      img.fillCircle(120, 141, 3, TFT_WHITE);
      // Pre-compute hand degrees, x & y coords for a fast screen update
      sdeg = rtc.getSecond() * 6;                      // 0-59 -> 0-354
      mdeg = rtc.getMinute() * 6 + sdeg * 0.01666667;  // 0-59 -> 0-360 - includes seconds
      hdeg = rtc.getHour() * 30 + mdeg * 0.0833333;    // 0-11 -> 0-360 - includes minutes and seconds
      hx = cos((hdeg - 90) * 0.0174532925);
      hy = sin((hdeg - 90) * 0.0174532925);
      mx = cos((mdeg - 90) * 0.0174532925);
      my = sin((mdeg - 90) * 0.0174532925);
      sx = cos((sdeg - 90) * 0.0174532925);
      sy = sin((sdeg - 90) * 0.0174532925);
      if (rtc.getSecond() == 0 || initial) {
        initial = 0;
        // Erase hour and minute hand positions every minute
        img.drawLine(ohx, ohy, 120, 141, colour7);
        ohx = hx * 62 + 121;
        ohy = hy * 62 + 141;
        img.drawLine(omx, omy, 120, 141, colour7);
        omx = mx * 84 + 120;
        omy = my * 84 + 141;
      }
      // Redraw new hand positions, hour and minute hands not erased here to avoid flicker
      img.drawLine(osx, osy, 120, 141, colour7);
      osx = sx * 90 + 121;
      osy = sy * 90 + 141;
      img.drawLine(osx, osy, 120, 141, TFT_RED);
      img.drawLine(ohx, ohy, 120, 141, TFT_WHITE);
      img.drawLine(omx, omy, 120, 141, TFT_WHITE);
      img.drawLine(osx, osy, 120, 141, TFT_RED);
      img.fillCircle(120, 141, 3, TFT_RED);
      img.pushSprite(0, 0);
    } else if (watchface == 4 || watchface == 5) {
      img1.setTextColor(TFT_WHITE, TFT_BLACK);
      img1.setFreeFont(FF24);
      img1.fillSprite(TFT_BLACK);
      //img1.setTextSize(2);

      if (rtc.getHour() > 9 && rtc.getMinute() > 9) {
        img1.drawString(String(rtc.getHour()) + ":" + String(rtc.getMinute()), 66, 30);
      } else if (rtc.getHour() < 10 && rtc.getMinute() > 9) {

        img1.drawString("0" + String(rtc.getHour()) + ":" + String(rtc.getMinute()), 66, 30);
      } else if (rtc.getHour() > 9 && rtc.getMinute() < 10) {

        img1.drawString(String(rtc.getHour()) + ":0" + String(rtc.getMinute()), 66, 30);
      } else {

        img1.drawString("0" + String(rtc.getHour()) + +":0" + String(rtc.getMinute()), 66, 30);
      }
      img1.setFreeFont(FF22);
      if (rtc.getSecond() < 10) {
        img1.drawString("0" + String(rtc.getSecond()), 190, 40);
      } else {
        img1.drawString(String(rtc.getSecond()), 190, 40);
      }
      img1.drawString(days1[rtc.getDayofWeek()] + " " + String(rtc.getDay()) + " " + String(rtc.getYear()), 54, 0);
      //img1.drawString(String(rtc.getDay()), 156, 0);
      img2.pushSprite(0, 180);
      img1.pushSprite(0, 180, TFT_BLACK);
    }
    lastsec = rtc.getSecond();
  }
}

static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}

void game() {
  if (Screen == 5 && Screenchange == 1) {
    Screenchange = 0;
    tft.fillScreen(TFT_BLACK);
    tft.fillRect(10, TFTH2 - 20, TFTW - 20, 1, TFT_WHITE);
    tft.fillRect(10, TFTH2 + 32, TFTW - 20, 1, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(0);
    tft.setTextSize(2);
    // half width - num char * char width in pixels
    tft.setCursor(TFTW2 - (6 * 9), TFTH2 - 16);
    tft.println("FLAPPY");
    tft.setTextSize(2);
    tft.setCursor(TFTW2 - (6 * 9), TFTH2 + 8);
    tft.println("-BIRD-");
  } else if (Screen == 7 && Screenchange == 1) {
    Screenchange = 0;
    tft.fillScreen(TFT_BLACK);
    maxScore = EEPROM.readInt(10);

    if (score > maxScore) {
      EEPROM.writeInt(10, score);
      EEPROM.commit();
      maxScore = score;
      tft.setTextColor(TFT_RED);
      tft.setTextSize(2);
      tft.setCursor(TFTW2 - (13 * 6), TFTH2 - 26);
      tft.println("NEW HIGHSCORE");
    }
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    // half width - num char * char width in pixels
    tft.setCursor(TFTW2 - (9 * 9), TFTH2 - 6);
    tft.println("GAME OVER");
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.print("score: ");
    tft.print(score);
    tft.setCursor(TFTW2 - (12 * 6), TFTH2 + 18);
    tft.println("press button");
    tft.setCursor(10, 28);
    tft.print("Max Score:");
    tft.print(maxScore);
    tft.setTextSize(0);
    facechange = 1;
  }
}

void game_init() {
  // clear screen
  tft.fillScreen(BCKGRDCOL);
  // reset score
  score = 0;
  // init bird
  bird.x = 144;
  bird.y = bird.old_y = TFTH2 - BIRDH;
  bird.vel_y = -JUMP_FORCE;
  tmpx = tmpy = 0;
  // generate new random seed for the pipe gape
  randomSeed(analogRead(12));
  // init pipe
  pipes.x = 0;
  pipes.gap_y = random(20, TFTH - 60);
}

void game_loop() {

  // ===============
  // prepare game variables
  // draw floor
  // ===============
  // instead of calculating the distance of the floor from the screen height each time store it in a variable
  const unsigned char GAMEH = TFTH - FLOORH;
  // draw the floor once, we will not overwrite on this area in-game
  // black line
  tft.drawFastHLine(0, GAMEH, TFTW, TFT_BLACK);
  // grass and stripe
  tft.fillRect(0, GAMEH + 1, TFTW2, GRASSH, GRASSCOL);
  tft.fillRect(TFTW2, GAMEH + 1, TFTW2, GRASSH, GRASSCOL2);
  // black line
  tft.drawFastHLine(0, GAMEH + GRASSH, TFTW, TFT_BLACK);
  // mud
  tft.fillRect(0, GAMEH + GRASSH + 1, TFTW, FLOORH - GRASSH, FLOORCOL);
  // grass x position (for stripe animation)
  long grassx = TFTW;
  // game loop time variables
  double delta, old_time, next_game_tick, current_time;
  next_game_tick = current_time = millis();
  // passed pipe flag to count score
  bool passed_pipe = false;
  // temp var for setAddrWindow
  unsigned char px;
  while (true) {
    yield();

    int loops = 0;
    while (millis() > next_game_tick && loops < MAX_FRAMESKIP) {
      // ===============
      // input
      // ===============
      if (digitalRead(0) == LOW) {
        // if the bird is not too close to the top of the screen apply jump force
        if (bird.y > BIRDH2 * 0.5)
          bird.vel_y = -JUMP_FORCE;
        // else zero velocity
        else
          bird.vel_y = 0;
      }
      // ===============
      // update
      // ===============
      // calculate delta time
      // ---------------
      old_time = current_time;
      current_time = millis();
      delta = (current_time - old_time) / 1000;
      // bird
      // ---------------
      bird.vel_y += GRAVITY * delta;
      bird.y += bird.vel_y;
      // pipe
      // ---------------
      pipes.x -= SPEED;
      // if pipe reached edge of the screen reset its position and gap
      if (pipes.x < -PIPEW) {
        pipes.x = TFTW;
        pipes.gap_y = random(10, GAMEH - (10 + GAPHEIGHT));
      }
      // ---------------
      next_game_tick += SKIP_TICKS;
      loops++;
    }

    // ===============
    // draw
    // ===============
    // pipe
    // ---------------
    // we save cycles if we avoid drawing the pipe when outside the screen
    if (pipes.x >= 0 && pipes.x < TFTW) {
      // pipe color
      tft.drawFastVLine(pipes.x + 3, 0, pipes.gap_y, PIPECOL);
      tft.drawFastVLine(pipes.x + 3, pipes.gap_y + GAPHEIGHT + 1, GAMEH - (pipes.gap_y + GAPHEIGHT + 1), PIPECOL);
      // highlight
      tft.drawFastVLine(pipes.x, 0, pipes.gap_y, PIPEHIGHCOL);
      tft.drawFastVLine(pipes.x, pipes.gap_y + GAPHEIGHT + 1, GAMEH - (pipes.gap_y + GAPHEIGHT + 1), PIPEHIGHCOL);
      // bottom and top border of pipe
      _drawPixel(pipes.x, pipes.gap_y, PIPESEAMCOL);
      _drawPixel(pipes.x, pipes.gap_y + GAPHEIGHT, PIPESEAMCOL);
      // pipe seam
      _drawPixel(pipes.x, pipes.gap_y - 6, PIPESEAMCOL);
      _drawPixel(pipes.x, pipes.gap_y + GAPHEIGHT + 6, PIPESEAMCOL);
      _drawPixel(pipes.x + 3, pipes.gap_y - 6, PIPESEAMCOL);
      _drawPixel(pipes.x + 3, pipes.gap_y + GAPHEIGHT + 6, PIPESEAMCOL);
    }
    // erase behind pipe
    if (pipes.x <= TFTW)
      tft.drawFastVLine(pipes.x + PIPEW, 0, GAMEH, BCKGRDCOL);
    // bird
    // ---------------
    tmpx = BIRDW - 1;
    do {
      px = bird.x + tmpx + BIRDW;
      // clear bird at previous position stored in old_y
      // we can't just erase the pixels before and after current position
      // because of the non-linear bird movement (it would leave 'dirty' pixels)
      tmpy = BIRDH - 1;
      do {
        _drawPixel(px, bird.old_y + tmpy, BCKGRDCOL);
      } while (tmpy--);
      // draw bird sprite at new position
      tmpy = BIRDH - 1;
      do {
        _drawPixel(px, bird.y + tmpy, birdcol[tmpx + (tmpy * BIRDW)]);
      } while (tmpy--);
    } while (tmpx--);
    // save position to erase bird on next draw
    bird.old_y = bird.y;
    // grass stripes
    // ---------------
    grassx -= SPEED;
    if (grassx < 0)
      grassx = TFTW;

    tft.drawFastVLine(grassx % TFTW, GAMEH + 1, GRASSH - 1, GRASSCOL);
    tft.drawFastVLine((grassx + 64) % TFTW, GAMEH + 1, GRASSH - 1, GRASSCOL2);
    // ===============
    // collision
    // ===============
    // if the bird hit the ground game over
    if (bird.y > GAMEH - BIRDH)
      break;
    // checking for bird collision with pipe
    if (bird.x + BIRDW >= pipes.x - BIRDW2 && bird.x <= pipes.x + PIPEW - BIRDW) {
      // bird entered a pipe, check for collision
      if (bird.y < pipes.gap_y || bird.y + BIRDH > pipes.gap_y + GAPHEIGHT)
        break;
      else
        passed_pipe = true;
    }
    // if bird has passed the pipe increase score
    else if (bird.x > pipes.x + PIPEW - BIRDW && passed_pipe) {
      passed_pipe = false;
      // erase score with background color
      tft.setTextColor(BCKGRDCOL);
      tft.setCursor(TFTW2, 4);
      tft.print(score);
      // set text color back to white for new score
      tft.setTextColor(TFT_WHITE);
      // increase score since we successfully passed a pipe
      score++;
    }
    // update score
    // ---------------
    tft.setCursor(TFTW2, 4);
    tft.print(score);
  }
  // add a small delay to show how the player lost
  Screen = 7;
  Screenchange = 1;
  delay(1200);
}