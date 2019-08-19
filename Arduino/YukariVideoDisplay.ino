/***************************************************

 Yukari
 Arduino Nano
 Raw bitmap file transfer from SD card to the ST7735 IPS TFT LCD display
 Used at Uniqlo shop window display

 ****************************************************/

//#define DEBUG

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <SD.h>

extern char *__brkval;

int freeMemory() {
  char top;
#if defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

// TFT display and SD card will share the hardware SPI interface.
#define TFT_CS  10  // Chip select line for TFT display
#define TFT_RST  9  // Reset line for TFT (or see below...)
#define TFT_DC   8  // Data/command line for TFT
#define TFT_BK   3  // Backlight control
#define SD_CS    6  // Chip select line for SD card

// Initialize the ST7735 library in hardware SPI more
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void setup(void) {
  #ifdef DEBUG
  Serial.begin(115200);
  #endif             

  pinMode(TFT_BK, OUTPUT);
  analogWrite(TFT_BK,0);
  
  tft.initR(INITR_MINI160x80);    // initialize the ST7735S chip
  tft.setRotation(2);             // orientation bottom up
  tft.invertDisplay(1);           // required by some versions of display, reason unknown
  tft.fillScreen(ST7735_WHITE);   // clear screen
  analogWrite(TFT_BK,100);

  if (!SD.begin(SD_CS)) {
    tft.fillRect( 0,  0,20,20,0xF800);  // error message: bottom left rectangle
    tft.fillRect( 0,140,20,20,0xF800);  // error message: top left rectangle
    #ifdef DEBUG
    Serial.println("SD card initialization failed!");
    #endif             
    return;
  }
}

void loop() {
  File bmpFile;
  
  // Open requested file on SD card
  if ((bmpFile = SD.open("uniqlo.raw")) == NULL) {
    tft.fillRect(30,  0,20,20,0xF800);  // error message: bottom centre rectangle
    tft.fillRect(30,140,20,20,0xF800);  // error message: top centre rectangle
    #ifdef DEBUG
    Serial.print("uniqlo.raw | File not found");
    #endif             
    return;
  }

  while(1)  {
    bmpDraw(bmpFile);
    bmpFile.seek(0);
  }
}

// This function reads a raw bitmap file from bmpFile and displays it at the given x,y coordinates. It's sped up
// by reading many pixels worth of data at a time (rather than pixel by pixel). Increasing the buffer
// size takes more of the Arduino's precious RAM but makes loading a little faster.
// The image size and format must match the display window size and format.

// 12bpp version
// One line = bmpWidth pixels = bmpWidth * 1.5 bytes
// One buffer = BUFFSIZE bytes = BUFFSIZE / 1.5 pixels
// One image = bmpSize pixels = bmpSize * 1.5 bytes

void bmpDraw(File bmpFile) {

  const uint16_t bufferSizeBytes = 720;   // Seems to be the biggest buffer possible based on the amount of RAM available
  uint8_t colorbuffer[bufferSizeBytes];   // Pixel color buffer
  uint16_t pos;
  uint32_t startTime, stopTime;
  uint16_t image;
  const uint16_t numImages = 339;
  const uint16_t bmpWidth = 80;
  const uint16_t bmpHeight = 120;
  const uint16_t bmpSize = bmpWidth * bmpHeight;
  const uint16_t bmpSizeBytes = bmpSize + bmpSize/2;
  const uint8_t  winX = 0;
  const uint8_t  winY = 20;
  
  for(image = 0;image < numImages;image++)  {
    #ifdef DEBUG
    Serial.println(); Serial.print("RAM:"); Serial.print(freeMemory()); 
    startTime = millis();
    #endif             
    
    // Set TFT address window to image bounds
    tft.setAddrWindow(winX, winY, winX+bmpWidth-1, winY+bmpHeight-1);
      
    for (pos=0; pos<bmpSizeBytes; pos+=sizeof(colorbuffer)) {
      // Fill colorbuffer from SD card
      bmpFile.read(colorbuffer, sizeof(colorbuffer));
  
      // Push it to the display          
      tft.pushColorBuf(colorbuffer, sizeof(colorbuffer));
    }
    
    #ifdef DEBUG
    stopTime = millis();
    Serial.print(" | Loaded in ");   Serial.print(stopTime - startTime);   Serial.print(" ms");
    #endif
  }
}

