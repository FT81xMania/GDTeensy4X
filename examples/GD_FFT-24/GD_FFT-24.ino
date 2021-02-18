// (Heavily) adapted from https://github.com/G6EJD/ESP32-8266-Audio-Spectrum-Display/blob/master/ESP32_Spectrum_Display_02.ino
// Adapted to GDTeensy4X library by TFTLCDCyg

#include <GDTeensy4X.h>
#include <arduinoFFT.h>

#define SAMPLES         1024  // Must be a power of 2
#define SAMPLING_FREQ   40000 // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define AMPLITUDE       250   //1000  Depending on your audio source level, you may need to alter this value. Can be used as a 'sensitivity' control.
#define AUDIO_IN_PIN    16    // Signal in on this pin

#define NUM_BANDS       24    // To change this, you will need to change the bunch of if statements describing the mapping from bins to bands
#define NOISE           500   // Used as a crude noise filter, values below this are ignored
#define TOP             38    //

// Sampling and FFT stuff
unsigned int sampling_period_us;

        byte peak[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};              // The length of these arrays must be >= NUM_BANDS
int oldBarHeights[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
   int bandValues[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  
double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned long newTime;
arduinoFFT FFT = arduinoFFT(vReal, vImag, SAMPLES, SAMPLING_FREQ);

void setup() {
  Serial.begin(115200);
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQ));

  GD.begin();
 
  GD.SaveContext();
  GD.BitmapHandle(15);  
  GD.cmd_loadimage(0, 0);
  GD.load("Fn1.jpg");
  GD.RestoreContext();
}

char TXP[50];
int Datos[24];
long previousMicros0 = 0; 

void loop() {

   GD.Clear();
   GD.get_inputs();
   GD.SaveContext();
    GD.ColorA(100);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(0, 0, 15);
    GD.End();
   GD.RestoreContext();


  for (int i = 0; i<NUM_BANDS; i++){
    bandValues[i] = 0;
  }
  
  // Sample the audio pin
  for (int i = 0; i < SAMPLES; i++) {
    newTime = micros();
    vReal[i] = analogRead(AUDIO_IN_PIN); // A conversion takes about 9.7uS on an ESP32
    vImag[i] = 0;
    while ((micros() - newTime) < sampling_period_us) { /* chill */ }
  }

  // Compute FFT
  FFT.DCRemoval();
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(FFT_FORWARD);
  FFT.ComplexToMagnitude();

  // Analyse FFT results
  for (int i = 2; i < (SAMPLES/2); i++){       // Don't use sample 0 and only first SAMPLES/2 are usable. Each array element represents a frequency bin and its value the amplitude.
    if (vReal[i] > NOISE) {                    // Add a crude noise filter
      
    //24 bands, 14kHz top band
      if (i<=2 )            bandValues[0]  += (int)vReal[i];  // 80
      if (i>=2   && i<=3  ) bandValues[1]  += (int)vReal[i];  // 100
      if (i>=3   && i<=4  ) bandValues[2]  += (int)vReal[i];  // 125
      if (i>=4   && i<=5  ) bandValues[3]  += (int)vReal[i];  // 157
      if (i>=5   && i<=6  ) bandValues[4]  += (int)vReal[i];  // 196
      if (i>=6   && i<=7  ) bandValues[5]  += (int)vReal[i];  // 246
      if (i>=7  &&  i<=9  ) bandValues[6]  += (int)vReal[i];  // 308
      if (i>=8  && i<=11  ) bandValues[7]  += (int)vReal[i];  // 385
      if (i>=11  && i<=14 ) bandValues[8]  += (int)vReal[i];  // 482
      if (i>=14  && i<=17 ) bandValues[9]  += (int)vReal[i];  // 604
      if (i>=17  && i<=22 ) bandValues[10] += (int)vReal[i];  // 756
      if (i>=22  && i<=27 ) bandValues[11] += (int)vReal[i];  // 946
      if (i>=27  && i<=34 ) bandValues[12] += (int)vReal[i];  // 1184
      if (i>=34  && i<=43 ) bandValues[13] += (int)vReal[i];  // 1482
      if (i>=43  && i<=53 ) bandValues[14] += (int)vReal[i];  // 1855
      if (i>=53  && i<=67 ) bandValues[15] += (int)vReal[i];  // 2322
      if (i>=67  && i<=84 ) bandValues[16] += (int)vReal[i];  // 2907
      if (i>=84  && i<=105) bandValues[17] += (int)vReal[i];  // 3639
      if (i>=105 && i<=131) bandValues[18] += (int)vReal[i];  // 4555
      if (i>=131 && i<=164) bandValues[19] += (int)vReal[i];  // 5702
      if (i>=164 && i<=206) bandValues[20] += (int)vReal[i];  // 7138
      if (i>=206 && i<=258) bandValues[21] += (int)vReal[i];  // 8935
      if (i>=258 && i<=322) bandValues[22] += (int)vReal[i];  // 11184
      if (i>=322          ) bandValues[23] += (int)vReal[i];  // 14000      
    }
  }

  // Process the FFT data into bar heights
  for (byte band = 0; band < NUM_BANDS; band++) {
    
    // Scale the bars for the display
    int barHeight = bandValues[band] / AMPLITUDE;
    if (barHeight > TOP) barHeight = TOP;

    // Small amount of averaging between frames
    barHeight = ((oldBarHeights[band] * 1) + barHeight) / 2;

    // Move peak up
    if (barHeight > peak[band]) {
      peak[band] = min(TOP, barHeight);
    }
    
        Serial.print(band);
        Serial.print("-");
        Serial.print(bandValues[band]);
        Serial.print("-");
        Serial.println(barHeight);

        Datos[band] = barHeight;
          
    // Save oldBarHeights for averaging later
    oldBarHeights[band] = barHeight;
  }
  
GD.SaveContext();

  GD.cmd_text(25, 5, 26, 0, "FFT 24 bands");
  sprintf(TXP,"Samples = %d", SAMPLES); GD.ColorRGB(0xFFFF00); GD.cmd_text(25, 25, 26, 0, TXP);
  sprintf(TXP,"Sampling frequency = %d", SAMPLING_FREQ); GD.ColorRGB(0xFFFF00); GD.cmd_text(25, 42, 26, 0, TXP);
  
  sprintf(TXP,"Teensy 4 @ %d MHz", (F_CPU/1000000)); GD.ColorRGB(0xFFFF00); GD.cmd_text(25, GD.h-25, 26, 0, TXP);
  if(SizeFT813==51){GD.cmd_text(GD.w-320, (GD.h)-25, 26, 0, "EVE chip: 5'' FT813@Riverdi RVT50UQFNWC000");}
  GD.ColorRGB(0xFFFFFF);

  GD.cmd_text(63, 390, 20, 0, "80    100   125    157    196    246    308   385    482    604   756    946  1184  1482  1855  2322 2907  3639 4555  5702  7138 8935  11.1k 14.0k  Hz"); //14kHz

 GD.RestoreContext();
  unsigned long currentMicros0 = micros();
  if(currentMicros0 - previousMicros0 > 10000)
  {
    previousMicros0 = currentMicros0;
    BarFFT(55, 380, TOP, 23, 6, 8);
    //CircleFFT();
    //CircleFFTa();
    LineFFT();
  }

    
  Protector01();
  GD.swap();
}

void LineFFT()
{
  GD.SaveContext(); 
   GD.ColorRGB(0xFF0000);
   GD.Begin(LINE_STRIP);
   GD.LineWidth(1.2 * 16);

    for (int j = 0; j < NUM_BANDS; j++) 
     {    
        GD.Vertex2f((55+(23/2)+(((23)*j)+6*j)) * 16, ((380) - (Datos[j]*8))* 16);
     }
   
  GD.RestoreContext();
}

void CircleFFT()
{
  GD.SaveContext(); 
   GD.ColorRGB(0xFF0000);
   GD.Begin(POINTS);
   GD.PointSize(5*16);
    for (int j = 0; j < NUM_BANDS; j++) 
      { 
        GD.Vertex2f((55+(23/2)+(((23)*j)+6*j)) * 16, ((380-8) - (Datos[j]*9))* 16); 
      }
  GD.RestoreContext(); 
}


void CircleFFTa()
{
  GD.SaveContext(); 
   GD.ColorRGB(0xFF0000);
   GD.Begin(POINTS);
   GD.PointSize(4*16);
    for (int j = 0; j < NUM_BANDS; j++) 
      { 
        GD.Vertex2f((55+(23/2)+(((23)*j)+6*j)) * 16, ((380) - (Datos[j]*8))* 16); 
      }
  GD.RestoreContext(); 
}

void BarFFT(int xi, int yi, int segmentos, int largoX, int sepX, int sepY)
{
  GD.SaveContext();  
  GD.ColorA(255);
  GD.Begin(LINES);
  GD.LineWidth(35);
  GD.ColorRGB(0xffffff);   //color de los segmentos

    for (int j = 0; j < NUM_BANDS; j++) 
      { 
      int dato = map(Datos[j], 0, TOP, 0, segmentos);
      for (int i = 0; i < dato; i++) 
        { 
          GD.Vertex2f((xi+(j*largoX+j*sepX))*16, (yi - (i*sepY))*16); // inicio
          GD.Vertex2f((xi+((j+1)*largoX+j*sepX))*16, (yi - (i*sepY))*16); //  fin   
        }
      }
  GD.RestoreContext();
}
