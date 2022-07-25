

#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <ESP32Time.h>
#include "fonts.h"
#include "Casio.h"
//ESP32Time rtc;
ESP32Time rtc(3600);  // offset in seconds GMT+1

#define TFT_GREY 0x5AEB
#define TFT_SKYBLUE 0x067D
#define color1 TFT_WHITE
#define color2  0x8410//0x8410
#define color3 0x5ACB
#define color4 0x15B3
#define color5 0x00A3
#define background 0xB635
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
TFT_eSprite img = TFT_eSprite(&tft);
float sx = 0, sy = 1, mx = 1, my = 0, hx = -1, hy = 0;    // Saved H, M, S x & y multipliers
float sdeg = 0, mdeg = 0, hdeg = 0;
uint16_t osx = 120, osy = 140, omx = 120, omy = 140, ohx = 120, ohy = 140; // Saved H, M, S x & y coords
uint16_t x0 = 0, x1 = 0, yy0 = 0, yy1 = 0;
uint32_t targetTime = 0;                    // for next 1 second timeout
static uint8_t conv2d(const char* p); // Forward declaration needed for IDE 1.6.x
uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3), ss = conv2d(__TIME__ + 6); // Get H, M, S from compile time
unsigned long lastfacechange = 0;
bool initial = 1;
volatile int counter = 0;
float VALUE;
float lastValue = 0;
int lastsec = 0;
double rad = 0.01745;
float x[360];
float y[360];
int watchface = 0;
bool facechange = false;
float px[360];
float py[360];

float lx[360];
float ly[360];


int r = 104;
int ssx = 120;
int ssy = 140;

String cc[12] = {"45", "40", "35", "30", "25", "20", "15", "10", "05", "0", "55", "50"};
String days[] = {"SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"};
String days1[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
int start[12];
int startP[60];

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;


int angle = 0;
bool onOff = 0;
bool debounce = 0;

String h, m, s, d1, d2, m1, m2;
void setup(void) {
  rtc.setTime(ss, mm, hh, 26, 7, 2022);  // 26th Jjuly 2022 compile date
  tft.init();
  tft.setRotation(0);

  //tft.fillScreen(TFT_BLACK);
  //tft.fillScreen(TFT_RED);
  //tft.fillScreen(TFT_GREEN);
  //tft.fillScreen(TFT_BLUE);
  //tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true); //*/
  tft.fillScreen(TFT_BLACK);
  img.setColorDepth(8);
  img.setSwapBytes(true);
  img.createSprite(240, 280);
  img.setTextDatum(4);
  targetTime = millis() + 1000;
  facechange = true;
}
int lastAngle = 0;
float circle = 100;
bool dir = 0;
int rAngle = 359;

void loop() {
  if (targetTime < millis()) {
    targetTime += 1000;
    ss++;              // Advance second
    if (ss == 60) {
      ss = 0;
      mm++;            // Advance minute
      if (mm > 59) {
        mm = 0;
        hh++;          // Advance hour
        if (hh > 23) {
          hh = 0;
        }
      }
    }
  }
  if(millis() - lastfacechange > 10000)
  {
    watchface++;
    if(watchface > 3)
    {
      watchface = 0;
    }
    facechange = true;
  }
  if (facechange)
  {
    tft.fillScreen(TFT_BLACK);
    if (watchface == 1)
    {
      tft.pushImage(0, 0, 240, 280, Casio2);
      tft.setTextColor(0x0081, background);
      tft.fillRoundRect(48, 127, 128, 48, 5, background);
    }
    else if (watchface == 2)
    {
      tft.pushImage(0, 0, 240, 280, Casio1);
      tft.setTextColor(0x0081, background);
      tft.fillRoundRect(48, 127, 128, 48, 5, background);
    }
    facechange = false;
    lastfacechange = millis();
  }
  if (watchface == 0)
  {
        int b = 0;
    int b2 = 0;
    for (int i = 0; i < 360; i++)
    {
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


    if (rtc.getDay() > 10)
    {
    d1 = rtc.getDay() / 10;
    d2 = rtc.getDay() % 10;
    }
    else
    {
    d1 = "0";
    d2 = String(rtc.getDay());
    }

    if (rtc.getMonth() > 10)
    {
    m1 = rtc.getMonth() / 10;
    m2 = rtc.getMonth() % 10;
    }
    else
    {
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



    if (angle > -1)
    {
    lastAngle = angle;

    VALUE = ((angle - 270) / 3.60) * -1;
    if (VALUE < 0)
      VALUE = VALUE + 100;



    img.fillSprite(TFT_BLACK);
    img.fillCircle(ssx, ssy, 124, color5);

    img.setTextColor(TFT_WHITE, color5);

    img.drawString(days[rtc.getDayofWeek()], circle, 140, 2);


    for (int i = 0; i < 12; i++)
      if (start[i] + angle < 360) {
        img.drawString(cc[i], x[start[i] + angle], y[start[i] + angle], 2);
        img.drawLine(px[start[i] + angle], py[start[i] + angle], lx[start[i] + angle], ly[start[i] + angle], color1);
      }
      else
      {
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

    img.setTextColor(0x35D7, TFT_BLACK);
    img.drawString("MONTH", 84, 78);
    img.drawString("DAY", 162, 78);
    img.setTextColor(TFT_SKYBLUE, TFT_BLACK);
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
  }
  else if (rtc.getSecond() != lastsec)
  {
    if (watchface == 1 ||watchface == 2)
    {
      String med;
      if (rtc.getSecond() % 2) {
        med = ":";
      }
      else
      {
        med = " " ;
      }
      tft.setFreeFont(&DSEG7_Classic_Bold_30);
      if (rtc.getHour() > 10 && rtc.getMinute() > 10)
      {
        tft.drawString(String(rtc.getHour()) + ":" + String(rtc.getMinute()), 46, 136);
      }
      else if (rtc.getHour() < 10 && rtc.getMinute() > 10)
      {

        tft.drawString("0" + String(rtc.getHour()) + ":" + String(rtc.getMinute()), 46, 136);
      }
      else if (rtc.getHour() > 10 && rtc.getMinute() < 10)
      {

        tft.drawString(String(rtc.getHour()) + ":0" + String(rtc.getMinute()), 46, 136);
      }
      else
      {

        tft.drawString("0" + String(rtc.getHour()) + ":0" + String(rtc.getMinute()), 46, 146);
      }

      tft.setFreeFont(&DSEG7_Classic_Bold_20);
      if (rtc.getSecond() < 10)
      {
        tft.drawString("0" + String(rtc.getSecond()), 154, 146);
      }
      else
      {
        tft.drawString(String(rtc.getSecond()), 154, 146);
      }


      tft.setFreeFont(&DSEG14_Classic_Bold_18);
      tft.drawString(days1[rtc.getDayofWeek()], 94, 106);
      tft.drawString(String(rtc.getDay()), 156, 106);
    }
    else if (watchface == 3)
    {
      img.setTextColor(TFT_WHITE, TFT_BLACK);  // Adding a background colour erases previous text automatically

      // Draw clock face
      img.fillCircle(120, 140, 118, TFT_GREEN);
      img.fillCircle(120, 140, 110, TFT_BLACK);

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
      sdeg = ss * 6;                // 0-59 -> 0-354
      mdeg = mm * 6 + sdeg * 0.01666667; // 0-59 -> 0-360 - includes seconds
      hdeg = hh * 30 + mdeg * 0.0833333; // 0-11 -> 0-360 - includes minutes and seconds
      hx = cos((hdeg - 90) * 0.0174532925);
      hy = sin((hdeg - 90) * 0.0174532925);
      mx = cos((mdeg - 90) * 0.0174532925);
      my = sin((mdeg - 90) * 0.0174532925);
      sx = cos((sdeg - 90) * 0.0174532925);
      sy = sin((sdeg - 90) * 0.0174532925);

      if (ss == 0 || initial) {
        initial = 0;
        // Erase hour and minute hand positions every minute
        img.drawLine(ohx, ohy, 120, 141, TFT_BLACK);
        ohx = hx * 62 + 121;
        ohy = hy * 62 + 141;
        img.drawLine(omx, omy, 120, 141, TFT_BLACK);
        omx = mx * 84 + 120;
        omy = my * 84 + 141;
      }

      // Redraw new hand positions, hour and minute hands not erased here to avoid flicker
      img.drawLine(osx, osy, 120, 141, TFT_BLACK);
      osx = sx * 90 + 121;
      osy = sy * 90 + 141;
      img.drawLine(osx, osy, 120, 141, TFT_RED);
      img.drawLine(ohx, ohy, 120, 141, TFT_WHITE);
      img.drawLine(omx, omy, 120, 141, TFT_WHITE);
      img.drawLine(osx, osy, 120, 141, TFT_RED);

      img.fillCircle(120, 141, 3, TFT_RED);
      img.pushSprite(0, 0);
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
