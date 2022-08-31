#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define TFT_GREY 0x5AEB

float ltx = 0;    // Saved x coord of bottom of needle
uint16_t osx = 120, osy = 120; // Saved x & y coords

int old_analog =  -999; // Value last displayed

#include "DHT.h"

#define DHTPIN 5
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

#define TEMP_PERIOD 2000
uint32_t updateTime = 0;       // time for next update
float temp;
uint8_t temp_thresh = 60;

#define TMP_ADJ_DN 32
#define TMP_ADJ_UP 33

#define IR_SEND_PIN 4
#include <IRremote.hpp>

#define DECODE_DISTANCE

uint32_t tRawData[]={0xED12FF01}; 
bool acIsRunning = true;   // assume unit is off before starting program

int lastDnBtnState = LOW;
int lastUpBtnState = LOW;

int dnBtnState;
int upBtnState;

unsigned long lastDnBtnDBTime = 0;
unsigned long lastUpBtnDBTime = 0;

unsigned long debounceDelay = 50;

#define SCRLED 27

void sendOnOff(void) {
   IrSender.sendPulseDistance(9000, 4350, 650, 1550, 650, 450, &tRawData[0], 32, false, 1000, 0);
}

void setup(void) {
  pinMode(SCRLED, OUTPUT);
  digitalWrite(SCRLED, HIGH);

  dht.begin();

  pinMode(TMP_ADJ_DN, INPUT_PULLUP);
  pinMode(TMP_ADJ_UP, INPUT_PULLUP);

  IrSender.begin();
  
  tft.init();
  tft.setRotation(0);
  Serial.begin(9600);  
  tft.fillScreen(TFT_BLACK);

  analogMeter(); // Draw analogue meter

  updateTime = millis(); // Next update time

  // Read temperature as Fahrenheit (isFahrenheit = true)
  temp = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(temp)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // assume AC is on if temp is <= 60 at start
  if (temp <= 60 && acIsRunning == true) {
    acIsRunning = false;
  } 
  
  tft.setTextSize(7);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextDatum(MC_DATUM);
  
  tft.drawNumber(temp_thresh, 120, 250);
}


void loop() {
  if (updateTime <= millis()) {
    updateTime = millis() + TEMP_PERIOD;

    // Read temperature as Fahrenheit (isFahrenheit = true)
    temp = dht.readTemperature(true);
  
    // Check if any reads failed and exit early (to try again).
    if (isnan(temp)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    if (temp <= temp_thresh && acIsRunning == false) {
      sendOnOff();
      acIsRunning = true;
    } 
    
    if (temp > (temp_thresh + 10) && acIsRunning == true) {
      sendOnOff();
      acIsRunning = false;
    }
   
    Serial.print(temp);
    Serial.print(F("°F "));
    Serial.print(F(" acIsRunning "));
    Serial.println(acIsRunning);

    tft.setTextSize(1);
    plotNeedle(temp, 0);
  } else {   
    tft.setTextSize(7);
    tft.setTextColor(TFT_YELLOW);

    // reset the debouncing timers
    if(digitalRead(TMP_ADJ_DN) != lastDnBtnState) {
      lastDnBtnDBTime = millis();
    }

    if(digitalRead(TMP_ADJ_UP) != lastUpBtnState) {
      lastUpBtnDBTime = millis();
    }

    // debounce and react to state change of buttons
    if ((millis() - lastDnBtnDBTime) > debounceDelay) {
      if (digitalRead(TMP_ADJ_DN) != dnBtnState) {
        dnBtnState = digitalRead(TMP_ADJ_DN);

        if(!digitalRead(TMP_ADJ_DN) && temp_thresh > 59) {
          temp_thresh -= 1;
          tft.fillRect(0, 180, 239, 200, TFT_BLACK);  
          tft.drawNumber(temp_thresh, 120, 250);
        }
      }
    }

    if ((millis() - lastUpBtnDBTime) > debounceDelay) {
      if (digitalRead(TMP_ADJ_UP) != upBtnState) {
        upBtnState = digitalRead(TMP_ADJ_UP);

        if(!digitalRead(TMP_ADJ_UP) && temp_thresh < 81) {
          temp_thresh += 1;         
          tft.fillRect(0, 180, 239, 200, TFT_BLACK);    
          tft.drawNumber(temp_thresh, 120, 250);
        }
      }
    }

    lastDnBtnState = digitalRead(TMP_ADJ_DN);
    lastUpBtnState = digitalRead(TMP_ADJ_UP);

    tft.setTextSize(1);
  }
}


// #########################################################################
//  Draw the analogue meter on the screen
// #########################################################################
void analogMeter()
{
  // Meter outline
  tft.fillRect(0, 0, 239, 126, TFT_GREY);
  tft.fillRect(5, 3, 230, 119, TFT_WHITE);

  tft.setTextColor(TFT_BLACK);  // Text colour

  // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
  for (int i = -50; i < 51; i += 5) {
    // Long scale tick length
    int tl = 15;

    // Coodinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (100 + tl) + 120;
    uint16_t y0 = sy * (100 + tl) + 140;
    uint16_t x1 = sx * 100 + 120;
    uint16_t y1 = sy * 100 + 140;

    // Coordinates of next tick for zone fill
    float sx2 = cos((i + 5 - 90) * 0.0174532925);
    float sy2 = sin((i + 5 - 90) * 0.0174532925);
    int x2 = sx2 * (100 + tl) + 120;
    int y2 = sy2 * (100 + tl) + 140;
    int x3 = sx2 * 100 + 120;
    int y3 = sy2 * 100 + 140;


    // Blue zone limits
    if (i >= 0 && i < 25) {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_BLUE);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_BLUE);
    }

    // Red zone limits
    if (i >= 25 && i < 50) {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_RED);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_RED);
    }

    // Short scale tick length
    if (i % 25 != 0) tl = 8;

    // Recalculate coords incase tick lenght changed
    x0 = sx * (100 + tl) + 120;
    y0 = sy * (100 + tl) + 140;
    x1 = sx * 100 + 120;
    y1 = sy * 100 + 140;

    // Draw tick
    tft.drawLine(x0, y0, x1, y1, TFT_BLACK);

    // Check if labels should be drawn, with position tweaks
    if (i % 25 == 0) {
      // Calculate label positions
      x0 = sx * (100 + tl + 10) + 120;
      y0 = sy * (100 + tl + 10) + 140;
      switch (i / 25) {
        case -2: tft.drawCentreString("0", x0, y0 - 12, 2); break;
        case -1: tft.drawCentreString("25", x0, y0 - 9, 2); break;
        case 0: tft.drawCentreString("50", x0, y0 - 6, 2); break;
        case 1: tft.drawCentreString("75", x0, y0 - 9, 2); break;
        case 2: tft.drawCentreString("100", x0, y0 - 12, 2); break;
      }
    }

    // Now draw the arc of the scale
    sx = cos((i + 5 - 90) * 0.0174532925);
    sy = sin((i + 5 - 90) * 0.0174532925);
    x0 = sx * 100 + 120;
    y0 = sy * 100 + 140;
    // Draw scale arc, don't draw the last part
    if (i < 50) tft.drawLine(x0, y0, x1, y1, TFT_BLACK);
  }

  tft.drawString("`F", 5 + 230 - 40, 119 - 20, 2); // Units at bottom right
  tft.drawCentreString("`F", 120, 70, 4); // Comment out to avoid font 4
  tft.drawRect(5, 3, 230, 119, TFT_BLACK); // Draw bezel line

  plotNeedle(0, 0); // Put meter needle at 0
}

// #########################################################################
// Update needle position
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################
void plotNeedle(float value, byte ms_delay)
{
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  char buf[8]; 
  dtostrf(value, 4, 1, buf);
  tft.drawRightString(buf, 40, 119 - 20, 2);

  if (value < -10) value = -10; // Limit value to emulate needle end stops
  if (value > 110) value = 110;

  // Move the needle util new value reached
  while (!((int)value == old_analog)) {
    if (old_analog < value) old_analog++;
    else old_analog--;

    if (ms_delay == 0) old_analog = (int)value; // Update immediately id delay is 0

    float sdeg = map(old_analog, -10, 110, -150, -30); // Map value to angle
    // Calcualte tip of needle coords
    float sx = cos(sdeg * 0.0174532925);
    float sy = sin(sdeg * 0.0174532925);

    // Calculate x delta of needle start (does not start at pivot point)
    float tx = tan((sdeg + 90) * 0.0174532925);

    // Erase old needle image
    tft.drawLine(120 + 20 * ltx - 1, 140 - 20, osx - 1, osy, TFT_WHITE);
    tft.drawLine(120 + 20 * ltx, 140 - 20, osx, osy, TFT_WHITE);
    tft.drawLine(120 + 20 * ltx + 1, 140 - 20, osx + 1, osy, TFT_WHITE);

    // Re-plot text under needle
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString("`F", 120, 70, 4);  // Comment out to avoid font 4

    // Store new needle end coords for next erase
    ltx = tx;
    osx = sx * 98 + 120;
    osy = sy * 98 + 140;

    // Draw the needle in the new postion, magenta makes needle a bit bolder
    // draws 3 lines to thicken needle
    tft.drawLine(120 + 20 * ltx - 1, 140 - 20, osx - 1, osy, TFT_RED);
    tft.drawLine(120 + 20 * ltx, 140 - 20, osx, osy, TFT_MAGENTA);
    tft.drawLine(120 + 20 * ltx + 1, 140 - 20, osx + 1, osy, TFT_RED);

    // Slow needle down slightly as it approaches new postion
    if (abs(old_analog - (int)value) < 10) ms_delay += ms_delay / 5;

    // Wait before next update
    delay(ms_delay);
  }
}
