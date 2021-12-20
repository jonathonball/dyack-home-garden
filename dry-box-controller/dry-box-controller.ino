#include <Wire.h>          // built-in
#include <string.h>        // built-in
#include <LiquidCrystal.h> // built-in
#include <dht.h>
#include <DS3231.h>
#include <IRremote.hpp>
#include <LinkedList.h>

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
const unsigned long ENVIRONMENT_READ_INTERVAL = 15000ul; // 15 seconds
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
const unsigned long RTC_READ_INTERVAL = 10000ul; // 10 seconds
const int DATE_COL                    = 0;
const int DATE_ROW                    = 0;
const int TIME_COL                    = 0;
const int TIME_ROW                    = 1;
const int DATE_TIME_WIDTH             = 2;

/* HK-M121 IR Receiver Module */
const int IR_PIN                           = 4;
const int IR_LED_FEEDBACK                  = 1;
const unsigned long IR_PROCESSING_INTERVAL = 250ul; // 0.25 seconds
const String IR_SERIAL_NAME                = "[HX-M121]: ";

struct IrCommand {
  int command;
  String button;
};
const int IR_COMMAND_COUNT = 21;
// These remote codes were read from an Elegoo remote
const int IR_BUTTON_ZERO  = 0x16;
const int IR_BUTTON_ONE   = 0xC;
const int IR_BUTTON_TWO   = 0x18;
const int IR_BUTTON_THREE = 0x5E;
const int IR_BUTTON_FOUR  = 0x8;
const int IR_BUTTON_FIVE  = 0x1C;
const int IR_BUTTON_SIX   = 0x5A;
const int IR_BUTTON_SEVEN = 0x42;
const int IR_BUTTON_EIGHT = 0x52;
const int IR_BUTTON_NINE  = 0x4A;
const int IR_BUTTON_PLAY  = 0x40;
const int IR_BUTTON_UP    = 0x9;
const int IR_BUTTON_DOWN  = 0x7;
const int IR_BUTTON_POWER = 0x45;
const int IR_BUTTON_VUP   = 0x46;
const int IR_BUTTON_VDOWN = 0x15;
const int IR_BUTTON_PREV  = 0x44;
const int IR_BUTTON_NEXT  = 0x43;
const int IR_BUTTON_FUNC  = 0x47;
const int IR_BUTTON_EQ    = 0x19;
const int IR_BUTTON_ST    = 0xD;
const IrCommand IR_COMMANDS[ IR_COMMAND_COUNT ] = {
  { IR_BUTTON_ZERO,  "0" },
  { IR_BUTTON_ONE,   "1" },
  { IR_BUTTON_TWO,   "2" },
  { IR_BUTTON_THREE, "3" },
  { IR_BUTTON_FOUR,  "4" },
  { IR_BUTTON_FIVE,  "5" },
  { IR_BUTTON_SIX,   "6" },
  { IR_BUTTON_SEVEN, "7" },
  { IR_BUTTON_EIGHT, "8" },
  { IR_BUTTON_NINE,  "9" },
  { IR_BUTTON_PLAY,  "playPause" },
  { IR_BUTTON_UP,    "up" },
  { IR_BUTTON_DOWN,  "down" },
  { IR_BUTTON_POWER, "power" },
  { IR_BUTTON_VUP,   "volUp" },
  { IR_BUTTON_VDOWN, "volDown" },
  { IR_BUTTON_PREV,  "prev" },
  { IR_BUTTON_NEXT,  "next" },
  { IR_BUTTON_FUNC,  "funcStop" },
  { IR_BUTTON_EQ,    "eq" },
  { IR_BUTTON_ST,    "stRept" }
};
LinkedList<int> irCommandQueue = LinkedList<int>();
const int MAX_IR_COMMAND_QUEUE_SIZE = 5;

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
   * Initializer, ran when ScreenBuffer object is created
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

  /**
   * Gets a row by int index
   */
  String getRow( int row ) { return rows[ row ]; }

  /**
   * Copies a string into the buffer starting at internal 
   */
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

  /**
   * Set internal cursor position and copy string into the buffer
   */
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
const int SCREEN_SCHEDULE_SET = 1;

int activeScreen = SCREEN_MAIN; // The currently selected (visbile) screen

/**
 * Send a reference to a ScreenBuffer object to the LCD screen
 */
void writeScreenToLCD( )
{
  for ( int i = 0; i < LCD_HEIGHT; i++ )
  {
    lcd.setCursor( 0, i );
    lcd.write( screens[ activeScreen ].getRow( i ).c_str() );
  }
}

/**
 * Converts celsius (C) to fehrenheit (F)
 * @param celsius - The temperature measured celcius
 */
float celsiusToFahrenheit( float celsius )
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
  static String temperature                  = "LOAD";
  static String humidity                     = "LOAD";
  static unsigned long measurementTimestamp  = millis();

  if ( millis() - measurementTimestamp > ENVIRONMENT_READ_INTERVAL ) {
    measurementTimestamp = millis();
    Serial.print( "[DHT11]: " );
    int check = DHT.read11( DHT11_PIN );
    if ( check == DHTLIB_OK ) {
      Serial.println( "SENSOR READ OK" );
      temperature =  String( celsiusToFahrenheit( DHT.temperature ), ENVIRONMENT_PRECISION );
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
  static unsigned long measurementTimestamp = RTC_READ_INTERVAL;
  static String currentDate                 = "????-??-??";

  if ( millis() - measurementTimestamp > RTC_READ_INTERVAL ) {
    measurementTimestamp = millis();
    DateTime now          = myRTC.now();
    String year           = padString( PAD_LEFT, DATE_TIME_WIDTH, "0", String( now.year() ) );
    String month          = padString( PAD_LEFT, DATE_TIME_WIDTH, "0", String( now.month() ) );
    String day            = padString( PAD_LEFT, DATE_TIME_WIDTH, "0", String( now.day() ) );
    currentDate           = year + "/" + month + "/" + day;
    Serial.println( "[DS3231]: " + currentDate );
  }
  return currentDate;
}

/**
 * Reads date data from a DS3241
 */
String getTimeFromRTC()
{
  static unsigned long measurementTimestamp = RTC_READ_INTERVAL;
  static String currentTime                 = "??:??";

  if ( millis() - measurementTimestamp > RTC_READ_INTERVAL ) {
    measurementTimestamp = millis();
    DateTime now          = myRTC.now();
    String hour           = padString( PAD_LEFT, DATE_TIME_WIDTH, "0", String( now.hour() ) );
    String minute         = padString( PAD_LEFT, DATE_TIME_WIDTH, "0", String( now.minute() ) );
    currentTime           = hour + ":" + minute;
    Serial.println( "[DS3231]: " + currentTime );
  }
  return currentTime;
}

/**
 * Gathers information from environmental sensors, formats it, and sends it to the screen
 */
void environmentTasks()
{
  // Get temperature and humidity then write them to the LCD
  String* environment         = checkEnvironment();
  String formattedTemperature = padString( PAD_LEFT, TEMPERATURE_SCREEN_WIDTH, " ", environment[ RESULT_INDEX_TEMPERATURE ] );
  String formattedHumidity    = padString( PAD_LEFT, HUMIDITY_SCREEN_WIDTH,    " ", environment [RESULT_INDEX_HUMIDITY ]    );
  // writes directly to the main screen even if its not visible
  screens[ SCREEN_MAIN ].write( TEMPERATURE_COL, TEMPERATURE_ROW, formattedTemperature.c_str() );
  screens[ SCREEN_MAIN ].write( HUMIDITY_COL, HUMIDITY_ROW, formattedHumidity.c_str() );
}

/**
 * Gathers information from the RTC, formats it, and sends it to the screen
 */
void datetimeTasks()
{
  // Get current/date time and then write them to the LCD
  String formattedDate = getDateFromRTC();
  String formattedTime = getTimeFromRTC();
  // writes directly to the main screen even if its not visible
  screens[ SCREEN_MAIN ].write( DATE_COL, DATE_ROW, formattedDate.c_str() );
  screens[ SCREEN_MAIN ].write( TIME_COL, TIME_ROW, formattedTime.c_str() );
}

/**
 * Checks to see if the IR receiver has received a code
 */
void checkIrDataReceived()
{
  if ( IrReceiver.decode() ) {
    // Check if the command is a repeat
    if ( !( IrReceiver.decodedIRData.flags & (IRDATA_FLAGS_IS_AUTO_REPEAT | IRDATA_FLAGS_IS_REPEAT)) ) {
      IrReceiver.printIRResultShort( &Serial );
      findKnownIrCommand(); 
    }
    IrReceiver.resume();
  }
}

/**
 * Checks to see if the last command received is one we are aware of
 */
void findKnownIrCommand()
{
  int command = IrReceiver.decodedIRData.command;
  // Check to see if the IR command is one we recognized
  for ( int index = 0; index < IR_COMMAND_COUNT; index++ ) {
    if ( IR_COMMANDS[ index ].command == command ) {
      Serial.println( IR_SERIAL_NAME + "received button " + IR_COMMANDS[ index ].button );
      addToIrCommandQueue( index );
    }
  }
}

/**
 * Use an index to add an infrared command to the queue
 */
void addToIrCommandQueue( int index )
{
  if ( irCommandQueue.size() < MAX_IR_COMMAND_QUEUE_SIZE ) {
    IrCommand irCommand = IR_COMMANDS[ index ];
    Serial.println( IR_SERIAL_NAME + "Adding " + irCommand.button + " to queue." );
    irCommandQueue.add( index );
    Serial.println( IR_SERIAL_NAME + "Items in queue " + irCommandQueue.size() );
  }
}

/**
 * Will process commands out of the ir queue until its exhausted
 */
void processIrCommandQueue()
{
  static unsigned long measurementTimestamp = 0ul;

  if ( millis() - measurementTimestamp > IR_PROCESSING_INTERVAL ) {
    if ( irCommandQueue.size() > 0 ) {
      int index = irCommandQueue.shift();
      Serial.println( IR_SERIAL_NAME + "Processing " + IR_COMMANDS[ index ].button );
      switch ( activeScreen ) {
        case SCREEN_MAIN:
          handleMainScreenIRCommand( index );
          break;
        case SCREEN_SCHEDULE_SET:
          handleScheduleScreenIRCommand( index );
          break;
      }
    }
  }
}

/**
 * Handle IR input when main screen is active
 */
void handleMainScreenIRCommand( int index )
{
  IrCommand irCommand = IR_COMMANDS[ index ];
  switch ( irCommand.command ) {
    case IR_BUTTON_FUNC:
      Serial.println( IR_SERIAL_NAME + "Switching to schedule screen" );
      activeScreen = SCREEN_SCHEDULE_SET;
      break;
  }
}

/**
 * Handle IR input when schedule screen is active
 */
void handleScheduleScreenIRCommand( int index )
{
  IrCommand irCommand = IR_COMMANDS[ index ];
  switch ( irCommand.command ) {
    case IR_BUTTON_FUNC:
      Serial.println( IR_SERIAL_NAME + "Switching to main screen" );
      activeScreen = SCREEN_MAIN;
      break;
  }
}

/**
 * Run infrared related tasks
 */
void irTasks()
{
  checkIrDataReceived();
  processIrCommandQueue();
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
  environmentTasks();
  datetimeTasks();
  irTasks();
  writeScreenToLCD();
}
