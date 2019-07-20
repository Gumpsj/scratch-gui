
#ifndef led_animations_H
#define led_animations_H

// Index number of which pattern is current.
uint8_t gCurrentPatternNumber = 0;      
// Currently 'delay' value. No, I don't use delays, I use EVERY_N_MILLIS_I instead.
uint8_t timeval = 20;    

CRGBPalette16 currentPalette(PartyColors_p);
TBlendType    currentBlending = LINEARBLEND;                                    // NOBLEND or LINEARBLEND 

#define qsubd(x, b)  ((x>b)?b:0)                              // Digital unsigned subtraction macro. if result <0, then => 0. Otherwise, take on fixed value.
#define qsuba(x, b)  ((x>b)?x-b:0)                            // Analog Unsigned subtraction macro. if result <0, then => 0

// Supporting visual functions ----------------------------------------------------------------------------------------------
// Send the pixels one or the other direction down the line.
bool thisdir = 0;                                          
void lineit() {                                                                 
  if (thisdir == 0) 
    for (int i = NUM_LEDS-1; i >0 ; i-- ) leds[i] = leds[i-1];
  else 
    for (int i = 0; i < NUM_LEDS-1 ; i++ ) leds[i] = leds[i+1];  
} // waveit()


 // Shifting pixels from the center to the left and right.
void waveit() {
  for (int i = NUM_LEDS - 1; i > NUM_LEDS/2; i--)                              // Move to the right.
    leds[i] = leds[i - 1];
  for (int i = 0; i < NUM_LEDS/2; i++)                                         // Move to the left.
    leds[i] = leds[i + 1];    
} // waveit()


// Let's add some glitter, thanks to Mark
void addGlitter( fract8 chanceOfGlitter) {                                        
  if( random8() < chanceOfGlitter) 
    leds[random16(NUM_LEDS)] += CRGB::White;
} // addGlitter()



// LED Animations

void confetti(){
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}


void rainbow() {  
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() {  
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
  addGlitter(120);
}


void bpm(){
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;  
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(currentPalette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void sinelon(){
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 100);
  int pos = beatsin16( 8, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}


void dot_beat() {
  uint8_t   count =   0;                                        // Count up to 255 and then reverts to 0
  uint8_t fadeval = 224;                                        // Trail behind the LED's. Lower => faster fade.  
  uint8_t bpm = 30;
  uint8_t inner = beatsin8(bpm, NUM_LEDS/4, NUM_LEDS/4*3);    // Move 1/4 to 3/4
  uint8_t outer = beatsin8(bpm, 0, NUM_LEDS-1);               // Move entire length
  uint8_t middle = beatsin8(bpm, NUM_LEDS/3, NUM_LEDS/3*2);   // Move 1/3 to 2/3

  leds[middle] = CRGB::Purple;
  leds[inner] = CRGB::Blue;
  leds[outer] = CRGB::Aqua;
  nscale8(leds,NUM_LEDS,fadeval);                             // Fade the entire array. Or for just a few LED's, use  nscale8(&leds[2], 5, fadeval);
} // dot_beat()


void fill_grad() {  
  uint8_t starthue = beatsin8(5, 0, 255);
  uint8_t endhue = beatsin8(7, 0, 255);  
  if (starthue < endhue) 
    fill_gradient(leds, NUM_LEDS, CHSV(starthue,255,255), CHSV(endhue,255,255), FORWARD_HUES); 
   else 
    fill_gradient(leds, NUM_LEDS, CHSV(starthue,255,255), CHSV(endhue,255,255), BACKWARD_HUES);   
} // fill_grad()


void inoise8_fire() {  
  uint32_t xscale = 20;                                          // How far apart they are
  uint32_t yscale = 3;                                           // How fast they move
  uint8_t index = 0;
  for(int i = 0; i < NUM_LEDS; i++) {
    index = inoise8(i*xscale,millis()*yscale*NUM_LEDS/255);                                           // X location is constant, but we move along the Y at the rate of millis()
    leds[i] = ColorFromPalette(currentPalette, min(i*(index)>>6, 255), i*255/NUM_LEDS, LINEARBLEND);  // With that value, look up the 8 bit colour palette value and assign it to the current LED.
  }                                                                                                   // The higher the value of i => the higher up the palette index (see palette definition).
                                                                                                      // Also, the higher the value of i => the brighter the LED.
} // inoise8_fire()


void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}


void onesine() {
// Persistent local variable.
  static int thisphase = 0;                                                     // Phase change value gets calculated.
// Local variables. Play around with these.
  uint8_t allfreq = 32;                                                         // You can change the frequency, thus distance between bars. Wouldn't recommend changing on the fly.
  uint8_t thiscutoff = 192;                                                     // You can change the cutoff value to display this wave. Lower value = longer wave.
  uint8_t bgbright = 10;                                                        // Brightness of background colour.
  uint8_t colorIndex;
  timeval = 30;                                                                 // Our EVERY_N_MILLIS_I timer value.

  thiscutoff = beatsin8(12,64, 224);
  thisphase = beatsin16(20,-600, 600);
  colorIndex = millis() >> 4;                                                   // millis() can be used for so many things.  
  for (int k=0; k<NUM_LEDS; k++) {                                            // For each of the LED's in the strand, set a brightness based on a wave as follows:
    int thisbright = qsuba(cubicwave8((k*allfreq)+thisphase), thiscutoff);      // qsub sets a minimum value called thiscutoff. If < thiscutoff, then bright = 0. Otherwise, bright = 128 (as defined in qsub)..
    leds[k] = ColorFromPalette( currentPalette, colorIndex, thisbright, currentBlending);    // Let's now add the foreground colour. By Andrew Tuline.
    colorIndex +=3;
  }
  
} // onesine()


void plasma() {
// Persistent local variables
  static int16_t thisphase = 0;                                                 // Phase of a cubicwave8.
  static int16_t thatphase = 0;                                                 // Phase of the cos8.

// Temporary local variables
  uint16_t thisbright;
  uint16_t colorIndex;
  timeval = 20;                                                                 // Our EVERY_N_MILLIS_I timer value.  
  thisphase += beatsin8(6,-4,4);                                                // You can change direction and speed individually.
  thatphase += beatsin8(7,-4,4);                                                // Two phase values to make a complex pattern. By Andrew Tuline.

  for (int k=0; k<NUM_LEDS; k++) {                                              // For each of the LED's in the strand, set a brightness based on a wave as follows.
    thisbright = cubicwave8((k*8)+thisphase)/2;    
    thisbright += cos8((k*10)+thatphase)/2;                                     // Let's munge the brightness a bit and animate it all with the phases.
    colorIndex=thisbright;
    thisbright = qsuba(thisbright, random(255));                              // qsuba chops off values below a threshold defined by sampleavg. Gives a cool effect.    
    leds[k] = ColorFromPalette( currentPalette, colorIndex, thisbright, currentBlending);   // Let's now add the foreground colour.
  }
  addGlitter(random(50));                                                      // Add glitter based on sampleavg.
} // plasma()


void besin() {                                                             // Add a Perlin noise soundbar. This looks really cool.
  timeval = 30;                                                                 // Our EVERY_N_MILLIS_I timer value.
// This works.
  leds[NUM_LEDS/2] = ColorFromPalette(currentPalette, millis(), random(128), NOBLEND);
  leds[NUM_LEDS/2-1] = ColorFromPalette(currentPalette, millis(), random(128), NOBLEND);
  waveit();                                                                     // Move the pixels to the left/right, but not too fast.
  fadeToBlackBy(leds+NUM_LEDS/2-1, 2, 128);                                   // Fade the center, while waveit moves everything out to the edges.
  //fadeToBlackBy(leds, NUM_LEDS, 2);                                                                                 
} // besin()


// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();                                          
SimplePatternList gPatterns = { inoise8_fire, besin, dot_beat, fill_grad, sinelon, bpm, confetti, rainbowWithGlitter, juggle, onesine, plasma };
                                //matrix, onesine,  noisefire, rainbowbit, noisefiretest, 
                                //rainbowg, noisewide, plasma, besin, noisepal};  
                                
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))                             
void nextPattern() {                                                            // Add one to the current pattern number, and wrap around at the end.
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);  // Another Kriegsman piece of magic.
} 


#endif
