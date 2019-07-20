#include <SPI.h>
#include <SoftwareSerial.h>
#include <Servo.h>
#include "DFRobotDFPlayerMini.h"
#include <FastLED.h>

#define LED_TYPE    APA102
#define COLOR_ORDER BGR
#define BRIGHTNESS  50
#define NUM_LEDS  92
#define NUM_RGB_LEDS  14
CRGB leds[NUM_LEDS];
CRGB leds_RGB1[NUM_RGB_LEDS];
CRGB leds_RGB2[NUM_RGB_LEDS];
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
bool animation = false;

enum commands {
  START_SYS = 0xF0,  
  SET_OUTPUT = 0xF1,  
  SET_SERVO = 0xF2,  
  SET_PWM = 0xF3,  
  SET_ANIM = 0xF4,  
  SET_RGB = 0xF5,  
  DFP_MSG = 0xF6,  
  END_SYS = 0xF7
};

enum DSP_cmd {
  PLAY = 0x0D, 
  PAUSE = 0x0E, 
  NEXT = 0x01,
  PREV = 0x02, 
  SONG_N = 0x03,
  VOLUME = 0x06
};

enum Animation_cmd{
  ANIM_NUM = 0xE0,
  LEDS_ON = 0xE1,
  LEDS_MOVE = 0xE2
};
  
#define PotPin A0
#define LightPin A1 
uint32_t PotValMedio;
uint32_t LightValMedio;

// txFrame => START + digIn, digOut, AnPot, AnLdr, Servo1, Servo2, RED1, GREEN1, BLUE1, RED2, GREEN2, BLUE2 + END + CRC16: Tot 16 byte.
uint8_t txFrame[16];
uint8_t rxFrame[10]; // START + CMD + ELEM + PAR_1 + PAR_2 + PAR3 + STOP + CRC16
volatile uint8_t rxCount = 0;
bool alive = false;
uint8_t cmd, elem, par1, par2, par3, par4;

#define SERVO1  6
#define SERVO2  7
uint8_t Servo1Pos = 90, Servo2Pos = 90;
uint32_t StopServo1, StopServo2;
uint32_t StartServo1, StartServo2;
Servo  Servo1, Servo2; 

SoftwareSerial DFPlayer(4, 5); 
DFRobotDFPlayerMini myDFPlayer;

#define TRIGGER 2
#define ECHO    3
uint32_t startPulse, pulseDuration;
uint8_t Distance = 0;

#define INPUTS 4
const uint8_t inPins[INPUTS] = { A2, A3, A4, A5};
uint32_t updateTime = millis(), testTime, rxTime;

void setup() {    
  Serial.begin(115200);
  for(int i=0; i<5; i++){
    Serial.println("ArduinoScratch");
    Serial.flush();
    delay(200);  
  }

  pinMode(TRIGGER, OUTPUT);   
  pinMode(ECHO, INPUT_PULLUP);   
  for(uint8_t i=0; i<INPUTS; i++){
    pinMode(inPins[i], INPUT_PULLUP);      
  }
  
  setupTimer2();
  
  // Set Volume and stop DFPlayer  
  DFPlayer.begin(9600);  
  myDFPlayer.begin(DFPlayer);
  myDFPlayer.setTimeOut(500);
  myDFPlayer.volume(20);
  //myDFPlayer.reset(); 
  
  FastLED.addLeds<LED_TYPE, 12,10, COLOR_ORDER>(leds_RGB1, NUM_RGB_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, 9,8 , COLOR_ORDER>(leds_RGB2, NUM_RGB_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
}

#include "led_animations.h"

void loop() {   
  if(animation){
    EVERY_N_MILLISECONDS( 50 ) { gHue++; }          // slowly cycle the "base color" through the rainbow      
    EVERY_N_MILLIS_I(thisTimer,timeval) {
      thisTimer.setPeriod(timeval);                 // We can change the timeval on the fly with this line, which is pretty sweet.
      gPatterns[gCurrentPatternNumber]();           // Call the current pattern function.
    }    
    EVERY_N_SECONDS(5) {                            // Change the target palette to a random one every 5 seconds.
      static uint8_t baseC = random8();             // You can use this as a baseline colour if you want similar hues in the next line.
      currentPalette = CRGBPalette16(CHSV(random8(), 255, random8(128,255)), CHSV(random8(), 255, random8(128,255)), CHSV(random8(), 192, random8(128,255)), CHSV(random8(), 255, random8(128,255)));
    }
  }
  FastLED.show();

  Distance = pulseDuration/58;  
  PotValMedio = map(analogRead(PotPin), 0, 1023, 0, 100) ;    
  LightValMedio = map(analogRead(LightPin), 0, 1023, 0, 100) ; 
  
  runServo();

  EVERY_N_MILLISECONDS(200){
    // Ultrasonic pulse
    attachInterrupt(digitalPinToInterrupt(ECHO), echoPulse, CHANGE);
    digitalWrite( TRIGGER, HIGH );  
    delayMicroseconds(10);
    digitalWrite( TRIGGER, LOW );  
  } 
  
  // Evaluate commands
  if(cmd != 0x00){     
    switch(cmd){
      case SET_OUTPUT:
        digitalWrite(elem, par1);
        break;
      case SET_SERVO:
        setServo(elem, par1);
        break;
      case SET_PWM:
        analogWrite(elem, par1);
        break;
      case SET_ANIM:  
        setAnimation(elem, par1, par2);
        break;
      case SET_RGB:
        setRGB(elem, par1, par2, par3);
        break;
      case DFP_MSG:
        DFP_msg(elem, par1);
        break;
    }       
    cmd = 0x00;
    txSerialData();
  }
}

void setAnimation(uint8_t cmd, uint8_t val, uint8_t dir){
  switch(cmd){
    case ANIM_NUM:
      gCurrentPatternNumber = val -1;
      if(gCurrentPatternNumber >= 0)
        animation = true;
      else{
        animation = false;
        for(int i = 0; i<NUM_LEDS; i++)   
          leds[i] = CRGB(0,0,0);   
        FastLED.show();    
      }
      break;
    case LEDS_ON:    
      for(int i = 0; i<val; i++){        
        leds[i] = ColorFromPalette(currentPalette, gHue+(i*2), gHue+(i*5));
        gHue++;
      }
      break;
    case LEDS_MOVE:      
      if(dir != 0)
        for(int i = 0; i<NUM_LEDS; i++) {                 
            leds[i] = leds[i+val];          
            leds[i+val] = CRGB(0,0,0);  
        }
      else
        for(int i = NUM_LEDS; i>=0; i--){
            leds[i+val] = leds[i];          
            leds[i] = CRGB(0,0,0);             
        }
        
      break;
  }
  
}

void setRGB(uint8_t n, uint8_t r, uint8_t g, uint8_t b){  
  if(n == 1){    
    for(int i = 0; i<NUM_RGB_LEDS; i++)   
      leds_RGB1[i] = CRGB(r,g,b);      
  }
  else{
    for(int i = 0; i<NUM_RGB_LEDS; i++)   
      leds_RGB2[i] = CRGB(r,g,b);  
  }     
}


void setServo(uint8_t num, uint8_t val){
  if(num == 1){
    //if(OldServo1Pos != val)
    Servo1Pos = val;
    StopServo1 = 4* abs(Servo1Pos - val) + 500; 
    StartServo1 = millis();
  }
  else{
    //if(OldServo2Pos != val)
    Servo2Pos = val; 
    StopServo2 = 4* abs(Servo2Pos - val) + 500; 
    StartServo2 = millis();
  }
}

void runServo(){    
  if(millis() - StartServo1 > StopServo1){
    Servo1.detach();
  } 
  else {
    if(!Servo1.attached())
      Servo1.attach(SERVO1);
    Servo1.write(Servo1Pos);
  }    

 if(millis() - StartServo2 > StopServo2){
    Servo2.detach();
  } 
  else {
    if(!Servo2.attached())
      Servo2.attach(SERVO2);
    Servo2.write(Servo2Pos);
  }
}

void DFP_msg(uint8_t cmd, uint8_t arg){  
  switch(cmd){
    case PLAY:
      myDFPlayer.start(); 
      break;
    case PAUSE:
      myDFPlayer.pause(); 
      break;
    case NEXT:
      myDFPlayer.next();
      break;
    case SONG_N:
      myDFPlayer.play(arg);  
      break;
    case VOLUME:
      static uint16_t oldVolume;      
      if(arg != oldVolume){
        arg = map(arg, 0, 100, 0, 32);      
        myDFPlayer.volume(arg);
        oldVolume = arg;
      }    
      break;
  }
}

//                     [ 0xF0,   0xF3, 0x03,  0x5E,   0x00,   0x00,   0x00,   0xF7 ];  
// uint8_t rxFrame[6] // START + CMD + ELEM + PAR_1 + PAR_2 + PAR_3 + PAR_4 + STOP + CRCH + CRCL
void serialEvent() {  
  bool crcComplete = false;
  uint16_t crc, crcRx;  
  if(Serial.available()){
    delay(1);  
    while (Serial.available()) {    
      char inChar = (char)Serial.read();      
      //Serial.write(inChar);  
      rxFrame[rxCount] = inChar;
      rxCount++;    
      if(rxCount == 8)    
        crc = calcrc(rxFrame, 8);          
      if(rxCount >= 10){      
        rxCount = 0;    
        crcRx = (rxFrame[8] << 8) | rxFrame[9];
        //Serial.print("\nRx CRC16: ");  Serial.println(crcRx, HEX);      
        //Serial.print("Calc CRC16: "); Serial.println(crc, HEX);
        if(crcRx == crc){
          cmd = rxFrame[1];
          elem = rxFrame[2];
          par1 = rxFrame[3];
          par2 = rxFrame[4];
          par3 = rxFrame[5];
          par4 = rxFrame[6];         
          memset(rxFrame, 0, sizeof(rxFrame));
        }
        
        // Clear incoming serial buffer
        while(Serial.available())
          Serial.read();
      }               
    }
  }
}

//txFrame => START + digIn, digOut, AnPot, AnLdr, Servo1, Servo2, RED1, GREEN1, BLUE1, RED2, GREEN2, BLUE2 + STOP + CRC16: Tot 16 byte.
void txSerialData(){
  uint8_t inByte = 0x00, outByte = 0x00;
  for(uint8_t i=0; i<INPUTS; i++){
    inByte = ( digitalRead(inPins[i]) << i) | inByte;   
  }  
  txFrame[0] = START_SYS;
  txFrame[1] = inByte;
  txFrame[2] = (uint8_t) Distance;
  txFrame[3] = PotValMedio;
  txFrame[4] = LightValMedio;
  txFrame[5] = Servo1Pos;
  txFrame[6] = Servo2Pos;
  txFrame[7] = leds_RGB1[0].r;
  txFrame[8] = leds_RGB1[0].g;
  txFrame[9] = leds_RGB1[0].b;
  txFrame[10] = leds_RGB2[0].r;
  txFrame[11] = leds_RGB2[0].g;
  txFrame[12] = leds_RGB2[0].b;
  txFrame[13] = END_SYS;
  txFrame[14] = 0x09 ;  //(TAB)
  txFrame[15] = 0x0A ;  // NL
  for(uint8_t i=0; i< 16; i++)
    Serial.write(txFrame[i]);
  
}

uint16_t get_checksum (uint16_t *thebuf){
  uint16_t sum = 0;
  for (byte i = 1; i < 7; i++) 
      sum += thebuf[i];
  return -sum;
}

uint16_t calcrc(char *ptr, int count){
  uint16_t  crc;
  uint8_t i;
  crc = 0;
  while (--count >= 0)  {
    crc = crc ^ (int) *ptr++ << 8;
    i = 8;
    do {
      if (crc & 0x8000)
        crc = crc << 1 ^ 0x1021;
      else
        crc = crc << 1;
    } while(--i);
  }
  return (crc);
}

void setupTimer2() {
  noInterrupts();  
  TCCR2A = 0;  TCCR2B = 0;  TCNT2 = 0;                // Clear registers
  OCR2A = 255;                                        // 61.03515625 Hz (16000000/((255+1)*1024))  
  TCCR2A |= (1 << WGM21);                             // CTC  
  TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);  // Prescaler 1024
  TIMSK2 |= (1 << OCIE2A);                            // Output Compare Match A Interrupt Enable
  interrupts();
}

ISR(TIMER2_COMPA_vect) {
  volatile static bool half;
  half = !half;
  if(half)
    txSerialData();     
}


void echoPulse(){
  if(PIND & (1 << ECHO )) 
    startPulse = micros();  
  else {
    pulseDuration = micros() - startPulse; 
    detachInterrupt(digitalPinToInterrupt(ECHO));    
  }
}
