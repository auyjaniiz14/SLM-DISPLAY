/*
 * Solar Move SOC(%) Li-Battery Calculation and display
 * Create by Rangson Pluemkamon
 * Version 1.00
 */
#include <SD.h>
#include <SPI.h>

#include <EEPROM.h> // Import EEprom for store Battry data
int eeaddress = 1;
float eedivider = 2.55;

#include <SPFD5408_Adafruit_GFX.h>    // Core graphics library
#include <SPFD5408_Adafruit_TFTLCD.h> // Hardware-specific library

#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RS A4
#define SD_CS 10

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GREY    0xD6BA

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RS);

double sizetext = 2.0;

// Variables for recieves Serial data
String ADC_STRING_IN, VBATT_ADC_STRING, IBATT_ADC_STRING, VSOLAR_ADC_STRING ;
int RawVBATT_ADC, RawIBATT_ADC, RawVSOLAR_ADC;
int VBATT_ADC, IBATT_ADC, VSOLAR_ADC; // for input serial data
double istVBATT_ADC, istIBATT_ADC;
bool ChargingState, DischargingState;
int SOCt, SOCt0, eedata, Qin, Qout;
double Qintotal, Qouttotal;
double Qmax = 252000; // Q in battery unit Coulomb(C)

double VBATT_GAIN   = 0.1543 ; // 0.1543 from calibrate , 0.1566 from circuit calculation
double VBATT_TRUE, IBATT_TRUE;
double IBATT_GAIN    = 194.00 ;
int SOLAR_POWER_TRUE ;
int LOAD_POWER_TRUE ;
int BATT_PERCENT ;
double IBATT_OUT ;
double LOAD_CURRENT ;
double istVoltage, RawVoltage;

void setup()
{
    Serial.begin(9600);
    //tft.begin(tft.readID());
    tft.begin(0x9341);
    tft.setRotation(1);
    bool good = SD.begin(SD_CS);
    if(good){
      bmpDraw("display.bmp", 0, 0);
    }
    if (!good) {
        tft.fillScreen(BLACK);
    }
    ChargingState = false;
    DischargingState = false;
}

void loop()
{
  SOLARMOVE_LABEL();
  BATTERY_LABEL();
  tft.drawRect(0,0,318,238,WHITE);
  if(Serial.available() > 0){
    GET_VAL();
    LCD_OUT();
  }
}

#define BUFFPIXEL 20

void bmpDraw(char *filename, int x, int y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel in buffer (R+G+B per pixel)
  uint16_t lcdbuffer[BUFFPIXEL];  // pixel out buffer (16-bit per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();
  uint8_t  lcdidx = 0;
  boolean  first = true;

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  progmemPrint(PSTR("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');
  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    progmemPrintln(PSTR("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    progmemPrint(PSTR("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    progmemPrint(PSTR("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    progmemPrint(PSTR("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      progmemPrint(PSTR("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        progmemPrint(PSTR("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...
          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each column...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              // Push LCD buffer to the display first
              if(lcdidx > 0) {
                tft.pushColors(lcdbuffer, lcdidx, first);
                lcdidx = 0;
                first  = false;
              }
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            lcdbuffer[lcdidx++] = tft.color565(r,g,b);
          } // end pixel
        } // end scanline
        // Write any remaining data to LCD
        if(lcdidx > 0) {
          tft.pushColors(lcdbuffer, lcdidx, first);
        } 
        progmemPrint(PSTR("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) progmemPrintln(PSTR("BMP format not recognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

// Copy string from flash to serial port
// Source string MUST be inside a PSTR() declaration!
void progmemPrint(const char *str) {
  char c;
  while(c = pgm_read_byte(str++)) Serial.print(c);
}

// Same as above, with trailing newline
void progmemPrintln(const char *str) {
  progmemPrint(str);
  Serial.println();
}
