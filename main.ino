int WATERPUMP = 13; //motor pump connected to pin 13
int sensor = 0; //sensor digital pin connected to pin 0
const int AirValue = 462;   //you need to replace this value with Value_1
const int WaterValue = 218;  //you need to replace this value with Value_2
int intervals = 81.3;  //(AirValue - WaterValue)/3
int soilMoistureValue = 0;

void setup() {
  Serial.begin(9600); // open serial port, set the baud rate to 9600 bps
  pinMode(13,OUTPUT); //Set pin 13 as OUTPUT pin
}

void loop()
{
  soilMoistureValue = analogRead(sensor);  //put Sensor insert into soil
  if(soilMoistureValue < AirValue && soilMoistureValue > (AirValue - intervals))
  {
    digitalWrite(13,HIGH);
  } else {
    digitalWrite(13,LOW)
  }
  delay(7,200,000); //Wait for 2 hours and then continue the loop.
}

