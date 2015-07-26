#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include <EEPROM.h>

#ifndef USE_ADAFRUIT_SHIELD_PINOUT 
 #error "This sketch is intended for use with the TFT LCD Shield. Make sure that USE_ADAFRUIT_SHIELD_PINOUT is #defined in the Adafruit_TFTLCD.h library file."
#endif

// These are the pins for the shield!
#define YP A1  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 7   // can be a digital pin
#define XP 6   // can be a digital pin

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 320);

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0

// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x002F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define MINPRESSURE 10
#define MAXPRESSURE 1000

#define DRAW_LOOP_INTERVAL 50

#define HS_ADDRESS 0

Adafruit_TFTLCD tft;

int wing;
int fx, fy, fallRate;
int pillarPos, gapPos;
int score;
int highScore = 0;
bool running = false;
bool crashed = false;
bool scrPress = false;
long nextDrawLoopRunTime;

void setup(void) {
  tft.reset();
  tft.begin(0x7575);
  tft.setRotation(1);
  
  // Hold touchscreen at top right at startup to clear highscores
  digitalWrite(13, HIGH);
  Point p = ts.getPoint();
  digitalWrite(13, LOW);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  if (p.x < 200 && p.y < 200)
    EEPROM.write(HS_ADDRESS, 0);
  
  startGame();
}

void startGame() {
  fx=50;
  fy=125;
  fallRate=1;
  pillarPos = 320;
  gapPos = 60;
  crashed = false;
  score = 0;
  highScore = EEPROM.read(HS_ADDRESS);
  
  tft.fillScreen(BLUE);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(5, 5);
  tft.println("Flappy-duino: tap to begin");
  
  tft.setTextColor(GREEN);
  tft.setCursor(60, 60);
  tft.print("High Score : ");
  tft.print(highScore);

  // Draw Ground  
  int ty = 230;
  for (int tx = 0; tx <= 300; tx +=20) {
    tft.fillTriangle(tx,ty, tx+10,ty, tx,ty+10, GREEN);
    tft.fillTriangle(tx+10,ty+10, tx+10,ty, tx,ty+10, YELLOW);
    tft.fillTriangle(tx+10,ty, tx+20,ty, tx+10,ty+10, YELLOW);
    tft.fillTriangle(tx+20,ty+10, tx+20,ty, tx+10,ty+10, GREEN);
  }

  nextDrawLoopRunTime = millis() + DRAW_LOOP_INTERVAL;
}

void loop(){
  if (millis() > nextDrawLoopRunTime && !crashed) {
      drawLoop();
        checkCollision();
      nextDrawLoopRunTime += DRAW_LOOP_INTERVAL;
  }
  
  // Read touchscreen
  digitalWrite(13, HIGH);
  Point p = ts.getPoint();
  digitalWrite(13, LOW);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  // Process "user input"
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE && !scrPress) {
    if (crashed) {
      // restart game
      startGame();
    }
    else if (!running) {
      // clear text & start scrolling
      tft.fillRect(0,0,320,80,BLUE);
      running = true;
    }
    else
    {
      // fly up
      fallRate = -8;
      scrPress = true;
    }
  }
  else if (p.z == 0 && scrPress) {
    // Attempt to throttle presses
    scrPress = false;
  }
}

void drawLoop() {
  // clear moving items
  clearPillar(pillarPos, gapPos);
  clearFlappy(fx, fy);

  // move items
  if (running) {
    fy += fallRate;
    fallRate++;
  
    pillarPos -=5;
    if (pillarPos == 0) {
      score++;
    }
    else if (pillarPos < -50) {
      pillarPos = 320;
      gapPos = random(20, 120);
    }
  }

  // draw moving items & animate
  drawPillar(pillarPos, gapPos);
  drawFlappy(fx, fy);
  switch (wing) {
      case 0: case 1: drawWing1(fx, fy); break;
      case 2: case 3: drawWing2(fx, fy); break;
      case 4: case 5: drawWing3(fx, fy); break;
  }
  wing++;
  if (wing == 6  ) wing = 0;
}

void checkCollision() {
  // Collision with ground
  if (fy > 206) crashed = true;
  
  // Collision with pillar
  if (fx + 34 > pillarPos && fx < pillarPos + 50)
    if (fy < gapPos || fy + 24 > gapPos + 90)
      crashed = true;
  
  if (crashed) {
    tft.setTextColor(RED);
    tft.setTextSize(3);
    tft.setCursor(75, 75);
    tft.print("Game Over!");
    tft.setCursor(75, 125);
    tft.print("Score:");
    tft.setCursor(220, 125);
    tft.print(score);
    
    if (score > highScore) {
      highScore = score;
      EEPROM.write(HS_ADDRESS, highScore);
      tft.setCursor(75, 175);
      tft.print("NEW HIGH!");
    }

    // stop animation
    running = false;
    
    // delay to stop any last minute clicks from restarting immediately
    delay(1000);
  }
}

void drawPillar(int x, int gap) {
  tft.fillRect(x+2, 2, 46, gap-4, GREEN);
  tft.fillRect(x+2, gap+92, 46, 136-gap, GREEN);
  
  tft.drawRect(x,0,50,gap,BLACK);
  tft.drawRect(x+1,1,48,gap-2,BLACK);
  tft.drawRect(x, gap+90, 50, 140-gap, BLACK);
  tft.drawRect(x+1,gap+91 ,48, 138-gap, BLACK);
}

void clearPillar(int x, int gap) {
  // "cheat" slightly and just clear the right hand pixels
  // to help minimise flicker, the rest will be overdrawn
  tft.fillRect(x+45, 0, 5, gap, BLUE);
  tft.fillRect(x+45, gap+90, 5, 140-gap, BLUE);
}

void clearFlappy(int x, int y) {
 tft.fillRect(x, y, 34, 24, BLUE); 
}

void drawFlappy(int x, int y) {
  // Upper & lower body
  tft.fillRect(x+2, y+8, 2, 10, BLACK);
  tft.fillRect(x+4, y+6, 2, 2, BLACK);
  tft.fillRect(x+6, y+4, 2, 2, BLACK);
  tft.fillRect(x+8, y+2, 4, 2, BLACK);
  tft.fillRect(x+12, y, 12, 2, BLACK);
  tft.fillRect(x+24, y+2, 2, 2, BLACK);
  tft.fillRect(x+26, y+4, 2, 2, BLACK);
  tft.fillRect(x+28, y+6, 2, 6, BLACK);
  tft.fillRect(x+10, y+22, 10, 2, BLACK);
  tft.fillRect(x+4, y+18, 2, 2, BLACK);
  tft.fillRect(x+6, y+20, 4, 2, BLACK);
  
  // Body fill
  tft.fillRect(x+12, y+2, 6, 2, YELLOW);
  tft.fillRect(x+8, y+4, 8, 2, YELLOW);
  tft.fillRect(x+6, y+6, 10, 2, YELLOW);
  tft.fillRect(x+4, y+8, 12, 2, YELLOW);
  tft.fillRect(x+4, y+10, 14, 2, YELLOW);
  tft.fillRect(x+4, y+12, 16, 2, YELLOW);
  tft.fillRect(x+4, y+14, 14, 2, YELLOW);
  tft.fillRect(x+4, y+16, 12, 2, YELLOW);
  tft.fillRect(x+6, y+18, 12, 2, YELLOW);
  tft.fillRect(x+10, y+20, 10, 2, YELLOW);
  
  // Eye
  tft.fillRect(x+18, y+2, 2, 2, BLACK);
  tft.fillRect(x+16, y+4, 2, 6, BLACK);
  tft.fillRect(x+18, y+10, 2, 2, BLACK);
  tft.fillRect(x+18, y+4, 2, 6, WHITE);
  tft.fillRect(x+20, y+2, 4, 10, WHITE);
  tft.fillRect(x+24, y+4, 2, 8, WHITE);
  tft.fillRect(x+26, y+6, 2, 6, WHITE);
  tft.fillRect(x+24, y+6, 2, 4, BLACK);
  
  // Beak
  tft.fillRect(x+20, y+12, 12, 2, BLACK);
  tft.fillRect(x+18, y+14, 2, 2, BLACK);
  tft.fillRect(x+20, y+14, 12, 2, RED);
  tft.fillRect(x+32, y+14, 2, 2, BLACK);
  tft.fillRect(x+16, y+16, 2, 2, BLACK);
  tft.fillRect(x+18, y+16, 2, 2, RED);
  tft.fillRect(x+20, y+16, 12, 2, BLACK);
  tft.fillRect(x+18, y+18, 2, 2, BLACK);
  tft.fillRect(x+20, y+18, 10, 2, RED);
  tft.fillRect(x+30, y+18, 2, 2, BLACK);
  tft.fillRect(x+20, y+20, 10, 2, BLACK);
}

// Wing down
void drawWing1(int x, int y) {
  tft.fillRect(x, y+14, 2, 6, BLACK);
  tft.fillRect(x+2, y+20, 8, 2, BLACK);
  tft.fillRect(x+2, y+12, 10, 2, BLACK);
  tft.fillRect(x+12, y+14, 2, 2, BLACK);
  tft.fillRect(x+10, y+16, 2, 2, BLACK);
  tft.fillRect(x+2, y+14, 8, 6, WHITE);
  tft.fillRect(x+8, y+18, 2, 2, BLACK);
  tft.fillRect(x+10, y+14, 2, 2, WHITE);
}

// Wing middle
void drawWing2(int x, int y) {
  tft.fillRect(x+2, y+10, 10, 2, BLACK);
  tft.fillRect(x+2, y+16, 10, 2, BLACK);
  tft.fillRect(x, y+12, 2, 4, BLACK);
  tft.fillRect(x+12, y+12, 2, 4, BLACK);
  tft.fillRect(x+2, y+12, 10, 4, WHITE);
}

// Wing up
void drawWing3(int x, int y) {
  tft.fillRect(x+2, y+6, 8, 2, BLACK);
  tft.fillRect(x, y+8, 2, 6, BLACK);
  tft.fillRect(x+10, y+8, 2, 2, BLACK);
  tft.fillRect(x+12, y+10, 2, 4, BLACK);
  tft.fillRect(x+10, y+14, 2, 2, BLACK);
  tft.fillRect(x+2, y+14, 2, 2, BLACK);
  tft.fillRect(x+4, y+16, 6, 2, BLACK);
  tft.fillRect(x+2, y+8, 8, 6, WHITE);
  tft.fillRect(x+4, y+14, 6, 2, WHITE);
  tft.fillRect(x+10, y+10, 2, 4, WHITE);
}

