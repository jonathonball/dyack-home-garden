const int WATER_PUMP_PIN_ID   = 13;
const int WATER_SENSOR_PIN_ID = 0;
const int AIRVALUE            = 462;
const int WATERVALUE          = 218;
const int SERIAL_BAUD_RATE    = 9600;
const int TIME_DELAY          = 60*60*2*1000;
int intervals                 = (AIRVALUE - WATERVALUE) / 3;
int soilMoistureValue         = 0;
int airValuePerInterval       = AIRVALUE - intervals;

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  pinMode(WATER_PUMP_PIN_ID, OUTPUT);
}

void loop()
{
  soilMoistureValue = analogRead(WATER_SENSOR_PIN_ID);
 
  if(soilMoistureValue < AIRVALUE && soilMoistureValue > airValuePerInterval) {
    digitalWrite(WATER_PUMP_PIN_ID, HIGH);
  } else {
    digitalWrite(WATER_PUMP_PIN_ID, LOW);
  }
  delay(TIME_DELAY);
}
