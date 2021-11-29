#include <DS3231.h>
#include <Wire.h>
#include <dht.h>
#include <LiquidCrystal.h>
#include <string.h>

/* Application behavior */
const int PAD_LEFT                = 0;
const int PAD_RIGHT               = 1;
const int buttonPin               = 1;
const int BUTTON_INDICATOR_COL    = 10;
const int BUTTON_INDICATOR_ROW    = 1;
const unsigned long STARTUP_DELAY = 3000ul;
const unsigned long BAUD_RATE     = 9600ul;
bool buttonIsPushed               = false;

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
  }
  return current_time;
}

/**
 * This function is called once on microcontroller startup
 */
void setup()
{
  // Initialized serial connection
  Serial.begin( BAUD_RATE );

  // Screen
  lcd.begin( LCD_WIDTH, LCD_HEIGHT );
  lcd.print( "Initializing..." );

  // DS3231 requires we start the wire library
  Wire.begin();

  // Pull up the pin for our button
  pinMode( buttonPin, INPUT_PULLUP );

  // Wait a moment before starting
  delay( STARTUP_DELAY );
  lcd.clear();
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
  lcd.setCursor( TEMPERATURE_COL, TEMPERATURE_ROW );
  lcd.write( formattedTemperature.c_str() );
  lcd.setCursor( HUMIDITY_COL, HUMIDITY_ROW );
  lcd.write( formattedHumidity.c_str() );
}

/**
 * Gathers information from the RTC, formats it, and sends it to the screen
 */
void datetime_tasks()
{
  // Get current/date time and then write them to the LCD
  String formattedDate = getDateFromRTC();
  String formattedTime = getTimeFromRTC();
  lcd.setCursor( DATE_COL, DATE_ROW );
  lcd.write( formattedDate.c_str() );
  lcd.setCursor( TIME_COL, TIME_ROW );
  lcd.write( formattedTime.c_str() ); 
}

/**
 * Handles button presses
 */
void button_tasks()
{
  if ( digitalRead( buttonPin ) == LOW && buttonIsPushed == false ) {
    buttonIsPushed = true;
    Serial.println( "button press" );
    lcd.setCursor( BUTTON_INDICATOR_COL, BUTTON_INDICATOR_ROW );
    lcd.write( "B" );
  }
  if ( digitalRead( buttonPin ) == HIGH && buttonIsPushed == true ) {
    buttonIsPushed = false;
    Serial.println( "button release" );
    lcd.setCursor( BUTTON_INDICATOR_COL, BUTTON_INDICATOR_ROW );
    lcd.write( " " );
    delay( 100 );
  }
}

/**
 * This function is ran in an endless loop while the microcontroller is running
 */
void loop()
{
  environment_tasks();
  datetime_tasks();
  button_tasks();
}
