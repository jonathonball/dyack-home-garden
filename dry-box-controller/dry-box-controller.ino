#include <DS3231.h>
#include <Wire.h>
#include <dht.h>
#include <LiquidCrystal.h>
#include <string.h>
#include <IRremote.hpp>

/* Application behavior */
const int PAD_LEFT                = 0;
const int PAD_RIGHT               = 1;
const unsigned long STARTUP_DELAY = 3000ul;
const unsigned long BAUD_RATE     = 9600ul;

/* LCD1602 Module */
const int LCD_WIDTH  = 16;
const int LCD_HEIGHT = 2;
// the magic numbers on the following line are pin assignments for an Arduino Uno R3
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

/* DHT11 Temp/Humidity Module */
dht DHT;
const unsigned long ENVIRONMENT_READ_INTERVAL = 3000ul;
const int DHT11_PIN                           = 2;
const int TEMPERATURE_COL                     = 12;
const int TEMPERATURE_ROW                     = 0;
const int HUMIDITY_COL                        = 12;
const int HUMIDITY_ROW                        = 1;
const int TEMPERATURE_SCREEN_WIDTH            = 4;
const int HUMIDITY_SCREEN_WIDTH               = 4;
const int RESULT_INDEX_TEMPERATURE            = 0;
const int RESULT_INDEX_HUMIDITY               = 1;
const int ENVIRONMENT_PRECISION               = 0; // how many decimal places to show

/* DS3231 RTC Module */
RTClib myRTC;
const unsigned long RTC_READ_INTERVAL = 10000ul;
const int DATE_COL                    = 0;
const int DATE_ROW                    = 0;
const int TIME_COL                    = 0;
const int TIME_ROW                    = 1;
const int DATE_TIME_WIDTH             = 2;

/* HK-M121 IR Receiver Module */
const int IR_PIN          = 4;
const int IR_LED_FEEDBACK = 1;

struct IR_COMMAND {
  int command;
  String button;
};
const int IR_COMMAND_COUNT = 12;
const IR_COMMAND IR_COMMANDS[IR_COMMAND_COUNT] = {
  { 0xC,  "1" },
  { 0x18, "2" },
  { 0x5E, "3" },
  { 0x8,  "4" },
  { 0x1C, "5" },
  { 0x5A, "6" },
  { 0x42, "7" },
  { 0x52, "8" },
  { 0x4A, "9" },
  { 0x40, "play/pause" },
  { 0x9,  "up" },
  { 0x7,  "down" }
};

/* ScreenBuffer Object stores screen data so that we can switch between screen. */
class ScreenBuffer
{
private:
  String rows[ LCD_HEIGHT ]; // contains rows of characters
  int cursorX;               // current x position of the cursor on the screen
  int cursorY;               // current y position of the cursor on the screen
  // top left is 0, 0

  /**
   * Makes a new blank row
   */
  String createRow()
  {
    String output = "";
    for ( int i = 0; i < LCD_WIDTH; i++ )
    {
      output += " ";
    }
    return output;
  }

public:
  /**
   * Initializer, ran when object is created
   */
  ScreenBuffer()
  {
    cursorX = 0;
    cursorY = 0;
    for ( int i = 0; i < LCD_HEIGHT; i++ )
    {
      rows[ i ] = createRow();
    }
  }

  /**
   * Adjust the postion of the cursor
   */
  void setCursorX( int x ) { cursorX = x; }
  void setCursorY( int y ) { cursorY = y; }
  void setCursorPosition( int x, int y )
  {
    cursorX = x;
    cursorY = y;
  }

  String getRow( int row ) { return rows[ row ]; }

  void write( String str )
  {
    int strIndex = 0;
    int terminator = cursorX + str.length();
    for ( int x = cursorX; x < terminator; x++ )
    {
      rows[ cursorY ][ x ] = str[ strIndex ];
      strIndex++;
    }
  }

  void write( int x, int y, String str )
  {
    setCursorPosition( x, y );
    write( str );
  }
};

// Create a screen buffer object for each screen
ScreenBuffer screens[] = {
  ScreenBuffer(), ScreenBuffer()
};
// These definitions exist so we can call the screens by name instead of number
const int SCREEN_MAIN = 0;
const int SCREEN_SCHEDULE = 1;
// Set the start-up screen
int active_screen = SCREEN_MAIN;

/**
 * Send a reference to a ScreenBuffer object to the LCD screen
 */
void writeScreenToLCD( )
{
  for ( int i = 0; i < LCD_HEIGHT; i++ )
  {
    lcd.setCursor( 0, i );
    lcd.write( screens[ active_screen ].getRow( i ).c_str() );
  }
}

/**
 * Converts celsius (C) to fehrenheit (F)
 * @param celsius - The temperature measured celcius
 */
float celsius2fahrenheit( float celsius )
{
  return ( celsius * 1.8 ) + 32.0;
}

/**
 * Reads from a DHT11 sensor returns array containing two strings
 * containing temperature and humidity
 */
String * checkEnvironment()
{
  static String results[2];
  static String temperature                  = "?F";
  static String humidity                     = "?%";
  static unsigned long measurement_timestamp = millis();

  if ( millis() - measurement_timestamp > ENVIRONMENT_READ_INTERVAL ) {
    measurement_timestamp = millis();
    Serial.print( "[DHT11]: " );
    int check = DHT.read11( DHT11_PIN );
    if ( check == DHTLIB_OK ) {
      Serial.println( "SENSOR READ OK" );
      temperature =  String( celsius2fahrenheit( DHT.temperature ), ENVIRONMENT_PRECISION );
      temperature += String( "F" );
      humidity    =  String( DHT.humidity, ENVIRONMENT_PRECISION );
      humidity    += String( "%" );
    } else {
      temperature = "ERR";
      humidity    = "ERR";
      switch( check ) {
        case DHTLIB_ERROR_CHECKSUM: 
          Serial.println( "Checksum error" );
          break;
        case DHTLIB_ERROR_TIMEOUT: 
          Serial.println( "Time out error" ); 
          break;
        case DHTLIB_ERROR_CONNECT:
          Serial.println( "Connect error" );
          break;
        case DHTLIB_ERROR_ACK_L:
          Serial.println( "Ack Low error" );
          break;
        case DHTLIB_ERROR_ACK_H:
          Serial.println( "Ack High error" );
          break;
        default: 
          Serial.println( "Unknown error" );
          break;
      }
    }
  }
  results[RESULT_INDEX_TEMPERATURE] = temperature;
  results[RESULT_INDEX_HUMIDITY]    = humidity;
  return results;
}

/**
 * Ensures that strings are a minimum width by padding them with a character
 * @param side  - Which side of the number to pad
 * @param width - Minimum size of the string
 * @param padding - Character to use as padding
 * @param subject - The string that will be padded
 */
String padString( int side, int width, String padding, String subject )
{
  while ( subject.length() < width ) {
    if ( side == PAD_LEFT ) {
      subject = padding + subject;
    } else {
      subject = subject + padding;
    }
  }
  return subject;
}

/**
 * Reads date data from a DS3231
 */
String getDateFromRTC()
{
  static unsigned long measurement_timestamp = RTC_READ_INTERVAL;
  static String current_date                 = "????-??-??";

  if ( millis() - measurement_timestamp > RTC_READ_INTERVAL ) {
    measurement_timestamp = millis();
    DateTime now          = myRTC.now();
    String year           = padString( PAD_LEFT, DATE_TIME_WIDTH, "0", String( now.year() ) );
    String month          = padString( PAD_LEFT, DATE_TIME_WIDTH, "0", String( now.month() ) );
    String day            = padString( PAD_LEFT, DATE_TIME_WIDTH, "0", String( now.day() ) );
    current_date          = year + "/" + month + "/" + day;
    Serial.println( "[DS3231]: " + current_date );
  }
  return current_date;
}

/**
 * Reads date data from a DS3241
 */
String getTimeFromRTC()
{
  static unsigned long measurement_timestamp = RTC_READ_INTERVAL;
  static String current_time                 = "??:??";

  if ( millis() - measurement_timestamp > RTC_READ_INTERVAL ) {
    measurement_timestamp = millis();
    DateTime now          = myRTC.now();
    String hour           = padString( PAD_LEFT, DATE_TIME_WIDTH, "0", String( now.hour() ) );
    String minute         = padString( PAD_LEFT, DATE_TIME_WIDTH, "0", String( now.minute() ) );
    current_time          = hour + ":" + minute;
    Serial.println( "[DS3231]: " + current_time );
  }
  return current_time;
}

/**
 * Gathers information from environmental sensors, formats it, and sends it to the screen
 */
void environment_tasks()
{
  // Get temperature and humidity then write them to the LCD
  String* environment         = checkEnvironment();
  String formattedTemperature = padString( PAD_LEFT, TEMPERATURE_SCREEN_WIDTH, " ", environment[RESULT_INDEX_TEMPERATURE] );
  String formattedHumidity    = padString( PAD_LEFT, HUMIDITY_SCREEN_WIDTH,    " ", environment[RESULT_INDEX_HUMIDITY]    );
  screens[ SCREEN_MAIN ].write( TEMPERATURE_COL, TEMPERATURE_ROW, formattedTemperature.c_str() );
  screens[ SCREEN_MAIN ].write( HUMIDITY_COL, HUMIDITY_ROW, formattedHumidity.c_str() );
}

/**
 * Gathers information from the RTC, formats it, and sends it to the screen
 */
void datetime_tasks()
{
  // Get current/date time and then write them to the LCD
  String formattedDate = getDateFromRTC();
  String formattedTime = getTimeFromRTC();
  screens[ SCREEN_MAIN ].write( DATE_COL, DATE_ROW, formattedDate.c_str() );
  screens[ SCREEN_MAIN ].write( TIME_COL, TIME_ROW, formattedTime.c_str() );
}

/**
 * Checks to see if the IR receiver has received a code
 */
void ir_tasks()
{
  if ( IrReceiver.decode() ) {
    int command = IrReceiver.decodedIRData.command;
    // Check if the command is a repeat
    if ( !( IrReceiver.decodedIRData.flags & (IRDATA_FLAGS_IS_AUTO_REPEAT | IRDATA_FLAGS_IS_REPEAT)) ) {
      IrReceiver.printIRResultShort( &Serial );
      // Check to see if the IR command is one we recognized
      for ( int index = 0; index < IR_COMMAND_COUNT; index++ ) {
        if ( IR_COMMANDS[index].command == command ) {
          Serial.println( "[HX-M121]: Received button " + IR_COMMANDS[index].button );
        }
      }
    }
    IrReceiver.resume();
  }
}

/**
 * This function is called once on microcontroller startup
 */
void setup()
{
  // Initialized serial connection
  Serial.begin( BAUD_RATE );
  Serial.println( "Initializing..." );

  // Screen
  lcd.begin( LCD_WIDTH, LCD_HEIGHT );
  lcd.print( "Initializing..." );

  // DS3231 requires we start the wire library
  Wire.begin();

  // Start the infrared receiver
  IrReceiver.begin(IR_PIN, IR_LED_FEEDBACK);

  // Wait a moment before starting
  delay( STARTUP_DELAY );
  lcd.clear();
}

/**
 * This function is ran in an endless loop while the microcontroller is running
 */
void loop()
{
  environment_tasks();
  datetime_tasks();
  ir_tasks();
  writeScreenToLCD( );
}
