#include <Adafruit_I2CDevice.h>
#include <Wire.h>

//CPE 301 - Final Project
// Romina Mendez, Alex Robinson, Anmol Sandhu

//LCD
#include <LiquidCrystal.h>
const int RS = 42, EN = 43, D4 = 44, D5 = 45, D6 = 46, D7 = 47;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

//stepper motor
#include <Stepper.h>
Stepper stepper(48, 22, 23, 24, 25); //48 = steps per revolution

//temp and humidity sensor
#include <dht.h>
#define DHTPIN 11
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHT11);
#define temp_threshold 25
#define water_threshold 400 //for water sensor
float temp = 0;
float humidity = 0;
unsigned int water = 1023;

//real time clock
#include <RTClib.h>
RTC_DS1307 RTC;
DateTime currentTime;

//port registers for LEDs
volatile unsigned char* PIN_D  = (unsigned char*) 0x26;
volatile unsigned char* DDR_D  = (unsigned char*) 0x27;
volatile unsigned char* PORT_D = (unsigned char*) 0x28;

//port registers for buttons (interrupts)
volatile unsigned char* PIN_B  = (unsigned char*) 0x23;
volatile unsigned char* DDR_B  = (unsigned char*) 0x24;
volatile unsigned char* PORT_B = (unsigned char*) 0x25;

volatile bool startPressed = false;
volatile bool stopPressed = false;

//analog port registers (for water sensor)
volatile unsigned char* PIN_A  = (unsigned char*) 0x20;
volatile unsigned char* DDR_A  = (unsigned char*) 0x21;
volatile unsigned char* PORT_A = (unsigned char*) 0x22;

//serial monitor register
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
 
//analog to digital conversion
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

volatile uint8_t* EIMSK  = (volatile uint8_t*) 0x3D;
volatile uint8_t* EICRA  = (volatile uint8_t*) 0x69;

#define RDA 0x80
#define TBE 0x20

enum State {
  DISABLED,
  IDLE,
  RUNNING,
  ERROR
};
State state = DISABLED;
State prevstate = DISABLED;

void setup() 
{
  U0init(9600);
  lcd.begin(16, 2);
  adc_init();
  dht.begin();
  RTC.begin();
  stepper.setSpeed(20);

//interrupt buttons
  *EIMSK |= (1 << INT0) | (1 << INT1);
  *EICRA |= (1 << ISC01) | (1 << ISC11);
  sei();

//button
*DDRB &= ~(1 << PB0); //input
*PORTB |= (1 << PB0); //pull up resistor

//leds
*DDRD |= (1 << PD7); //output
*DDRG |= (1 << PG2) | (1 << PG1) | (1 << PG0);
}


ISR(INT0_vect) { //start
    startPressed = true;
}
ISR(INT1_vect) {  //stop
    stopPressed = true;
}

void loop(){
if (state != prevState) {
    LogTransition(prevState, state);  // <-- NEW transition logger
    prevState = state;
  }

  switch (state) {
    case DISABLED:
    DisabledState();
    break;
    case IDLE:
    IdleState();
      break;
    case RUNNING:
    RunningState();
      break;
    case ERROR:
    ErrorState();
      break;
  }
}

void DisabledState(){ //-------------------------------------------------------------
  //serial monitor outputs
  Monitor_Output();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Disabled State");
 
  //yellow LED turns on
  *PORT_D |= (1 << 4); //pin 38 yellow LED
  *PORT_D &= ~(1 << 5);  //39 green led 
  *PORT_D &= ~(1 << 6); //40 red led
  *PORT_D &= ~(1 << 7); //41 blue led

  //fan turns off
  *PORT_B &= ~(1 << 4); //pin 10

  //monitoring button presses
  while (!startPressed) {
  }
  startPressed = false;
  state = IDLE;
}

void IdleState(){ //-----------------------------------------------------------------
   //serial monitor outputs
  Monitor_Output();

  //green LED turns on
  *PORT_D &= ~(1 << 4);
  *PORT_D |= (1 << 5); //39 green led
  *PORT_D &= ~(1 << 6);
  *PORT_D &= ~(1 << 7);

  //fan remains off
  *PORT_B &= ~(1 << 4);

  //idle state functions
  CheckTemp(); //dht11 sensor
  LCDDisplay(); //displays temp and humidity on lcd
  ControlVent(); //controls vent movement 
  WaterLevel(); //checks water sensor

  //if stop button pressed
  if (stopPressed){
    stopPressed = false;
    state = DISABLED;
  }
}

void ErrorState() { //-----------------------------------------------------------------------------------
   //serial monitor outputs
  Monitor_Output();

  //red led turns on 
  *PORT_D &= ~(1 << 4);
  *PORT_D &= ~(1 << 5);      
  *PORT_D |= (1 << 6); //40 red led   
  *PORT_D &= ~(1 << 7);     

  //fan off
  *PORT_B &= ~(1 << 4);

//error state functions
  LCDDisplay();

//error >> idle if start button pressed
  if (startPressed) {
    startPressed = false;
    state = IDLE;
  }

//error >> disabled if stop button pressed
  if (stopPressed) {
    stopPressed = false;
    state = DISABLED;
  }
}

void RunningState(){ //------------------------------------------------------------------------
 //serial monitor outputs
  Monitor_Output();

//blue led turns on 
  *PORT_D &= ~(1 << 4);
  *PORT_D &= ~(1 << 5);
  *PORT_D &= ~(1 << 6);
  *PORT_D |=  (1 << 7); //41 blue led

//fan turns on
  *PORT_B |= (1 << 4);

//running state functions
  CheckTemp();
  LCDDisplay();
  ControlVent();
  WaterLevel();

//stop button = disabled state
  if (stopPressed) {
    stopPressed = false;
    state = DISABLED;
  }

//start button = idle state
  if (startPressed){
    startPressed = false;
    state = IDLE;
  }
}

//functions ---------------------------------------------------------------------
void CheckTemp(){
  temp = DHT.readTemperature();
  humidity = DHT.readHumidity();
  delay(60000); //1 minute delay
  
  if (temp < temp_threshold){
    state = IDLE;
    return;
  }
}

void LCDDisplay(){
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(humidity);
  lcd.print("%");
  delay(500); 

  if (water < water_threshold){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error!");
    lcd.setCursor(0, 1);
    lcd.print("Water level too low!");
  }
}

void WaterLevel(){
  water = adc_read(0); //pin A0
  if (water < water_threshold) {
    state = ERROR;
  }
}

bool direction = true;
void ControlVent(){
  if (*PIN_B & (1 << 0)) { //pin 8 
    if (direction) {
      stepper.step(48);
      direction = false;
      Monitor_Output(1);
    } else {
      stepper.step(-48);
      direction = true;
      Monitor_Output(1);
    }
  }
}


void LogTransition(State from, State to) {
  Monitor_Output();  
  const char* names[] = {"DISABLED", "IDLE", "RUNNING", "ERROR"};
  for (int i = 0; i < 3; i++) U0putchar(' ');
  const char* toName = names[to];
  while (*fromName) U0putchar(*fromName++);
  U0putchar(' ');
  U0putchar('-');
  U0putchar('>');
  U0putchar(' ');
  while (*toName) U0putchar(*toName++);
  U0putchar('\n');
}

void Monitor_Output(int value){
  DateTime now = RTC.now();
  int hours = now.hour();
  int minutes = now.minute();
  int seconds = now.second();
  U0putchar(hours / 10 + '0');
  U0putchar(hours % 10 + '0');
  U0putchar(':');
  U0putchar(minutes / 10 + '0');
  U0putchar(minutes % 10 + '0');
  U0putchar(':');
  U0putchar(seconds / 10 + '0');
  U0putchar(seconds % 10 + '0');
  U0putchar(' ');
}

void adc_init(){
  *my_ADCSRA |= 0b10000000;
  *my_ADCSRA &= 0b11011111;
  *my_ADCSRA &= 0b11011111;
  *my_ADCSRA &= 0b11011111;
  *my_ADCSRB &= 0b11110111;
  *my_ADCSRB &= 0b11111000;
  *my_ADMUX  &= 0b01111111;
  *my_ADMUX  |= 0b01000000;
  *my_ADMUX  &= 0b11011111;
  *my_ADMUX  &= 0b11011111;
  *my_ADMUX  &= 0b11100000;
}

unsigned int adc_read(unsigned char adc_channel_num){
  *my_ADMUX  &= 0b11100000;
  *my_ADCSRB &= 0b11110111;
  if (adc_channel_num > 7){
    adc_channel_num -= 8;
    *my_ADCSRB |= 0b00001000;
  }
  *my_ADMUX  += adc_channel_num;
  *my_ADCSRA |= 0x40;
  while ((*my_ADCSRA & 0x40) != 0);
  return *my_ADC_DATA;
}

unsigned char U0kbhit(){
  return *myUCSR0A & RDA;
}

unsigned char U0getchar(){
  return *myUDR0;
}

void U0putchar(unsigned char U0pdata){
  while ((*myUCSR0A & TBE) == 0);
  *myUDR0 = U0pdata;
}

void U0init(int U0baud){
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}

