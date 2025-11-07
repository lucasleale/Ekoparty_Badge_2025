#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "fscale.h"
#include <elapsedMillis.h>
#include <AMY-Arduino.h>
#include <Filters.h>
#include <Arduino.h>
#include <AH/Timing/MillisMicrosTimer.hpp>
#include <Filters/Notch.hpp>
#define MUTE_PIN 11
#define SENSE_HP 2  // 0 speaker, 1 headphones
#define NUM_LEDS 10
#define LENGTH 32
#define MAX_TRACKS 8
elapsedMillis interval;
elapsedMillis bounceAdc3;
elapsedMillis fadeLedKick;
elapsedMillis fadeLedHat;
elapsedMillis fadeLedSn;
elapsedMillis fadeLedVocal;

Adafruit_NeoPixel leds(NUM_LEDS, 25, NEO_GRB + NEO_KHZ800);

#define TPA2028_ADDR 0x58
#define TPA2028_REG_GAIN 0x05
#define PAD_0_PIN A0  //oreja derecha
#define PAD_1_PIN A1  //21
#define PAD_2_PIN A2  //oreja izquierda
#define PAD_3_PIN A3  //diamante
#define THRESH_DIAM 950
#define THRESH_21 900
#define DEBUG //comentar define para sacar debug. Debug imprime valores de las orejas 
const double f_s = 250;            // Frecuencia de muestreo para filtro notch
const double f_c = 60;             // notch 60hz
const double f_n = 2 * f_c / f_s;  //normalizada

Timer<micros> timer = std::round(1e6 / f_s);
auto filter1 = simpleNotchFIR(f_n);      // fundamental
auto filter2 = simpleNotchFIR(2 * f_n);  // segundo armonico
auto filter1b = simpleNotchFIR(f_n);
auto filter2b = simpleNotchFIR(2 * f_n);




volatile bool stepState[MAX_TRACKS][LENGTH] = {
  //Array 2D, primer array voces/paginas, segundo array pasos
  { 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 },
  { 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 },
  { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 },
};

volatile byte currentStep;
volatile byte lastStep;
void playSamples(int track, int preset, float velocity);
void updateSensors();
void setup() {
  // put your setup code here, to run once:
  leds.begin();
  amy_config_t amy_config = amy_default_config();
  amy_config.features.default_synths = 1;
  amy_config.i2s_mclk = 7;
  amy_config.i2s_bclk = 20;
  // On Pi Pico (RP2040, RP2350), i2s_lrc has to be i2s_bclk + 1, otherwise code will stop on an assert.
  amy_config.i2s_lrc = 21;
  amy_config.i2s_dout = 22;
  amy_config.i2s_din = 11;
  amy_config.features.chorus = 0;
  amy_config.features.reverb = 0;
  amy_config.features.echo = 0;
  //amy_config.features.audio_in = 0;
  //amy_config.features.default_synths = 0;
  //amy_config.features.partials = 0;
  //amy_config.features.custom = 0;
  amy_start(amy_config);
  //test_sequencer();
  amy_live_start();
  amy_event global = amy_default_event();

  global.volume = 4;
  global.eq_l = -15;
  global.eq_m = -5;
  global.eq_h = 5;
  amy_add_event(&global);
  //amy_add_message("M1,50,100,0.8,0");
  //config_echo(1.0f, 50.0f, 100.0f, 0.8f, 0.0f);
  //configTpa();
  Wire.setSDA(8);
  Wire.setSCL(9);
  Wire.begin();
  delay(500);
  Wire.beginTransmission(0x58);  // dirección del TPA2028
  Wire.write(0x05);              // registro de ganancia
  Wire.write(0x00);              // +12 dB
  //Wire.write(0x1E);
  Wire.endTransmission();
  delay(100);


  Wire.beginTransmission(0x58);  // dirección 7-bit
  //Wire.write(0x01);              // registro Function Control
  //Wire.write(0x63);              // valor con SWS=1 (shutdown)
  Wire.endTransmission();
  Wire.beginTransmission(0x58);
  Wire.write(0x07);  // Registro I
  //Wire.write(0b00001000);  // Compression 1:1 + Output Limiter disabled
  Wire.write(0b00000000);  // Bits 7:4 Max Gain = 0 (no importa), 3:2 = 00, 1:0 Compression = 00 (1:1)
  Wire.endTransmission();
  pinMode(MUTE_PIN, OUTPUT);
  digitalWrite(MUTE_PIN, HIGH);
  //Serial.begin(115200);
  pinMode(SENSE_HP, INPUT);
}


bool readSenseHp;
bool readSenseHpLast;


int adc3;
int adc3button;
int adc1;
int adc1button;
bool start;
int count;
int ledHat;
void loop() {
  amy_update();
  readSenseHp = digitalRead(SENSE_HP);

  if (readSenseHp != readSenseHpLast) {
    if (readSenseHp == 0) {
      Serial.println("SPEAKER");
      amy_event global = amy_default_event();
      global.volume = 3;
      global.eq_l = -15;
      global.eq_m = -5;
      global.eq_h = 5;
      amy_add_event(&global);
      digitalWrite(MUTE_PIN, LOW);
      Wire.beginTransmission(0x58);
      Wire.write(0x01);  // Function Control register
      Wire.write(0x43);  // 0b01000011 → EN=1
      Wire.endTransmission();
    } else {
      Serial.println("HEADPHONES");
      amy_event global = amy_default_event();
      global.volume = 1;
      global.eq_l = 0;
      global.eq_m = 0;
      global.eq_h = 0;
      amy_add_event(&global);
      Wire.beginTransmission(0x58);
      Wire.write(0x01);  // Function Control register
      Wire.write(0x03);  // 0b01000011 → EN=1
      Wire.endTransmission();
      digitalWrite(MUTE_PIN, HIGH);
    }
    readSenseHpLast = readSenseHp;
  }
  if (start) {
    //splash(500, 50, 50);
    if (interval > 160) {
      interval = 0;
      currentStep = currentStep % LENGTH;
      if (currentStep != lastStep) {
        //Serial.println(currentStep);

        //ledHat = random(0, MAX_TRACKS);
        //leds.setPixelColor(random(0, MAX_TRACKS), leds.Color(255, 0, 255));
        //leds.show();
        for (int track = 0; track < MAX_TRACKS; track++) {
          if (stepState[track][currentStep] == true) {
            if (track == 0) {  //el kick prende los leds

              fadeLedKick = 0;
            }
            if (track == 3) {
              fadeLedHat = 0;
            }
            if (track == 1) {
              fadeLedSn = 0;
            }
            playSamples(track, track, 1);
          }
        }

        lastStep = currentStep;
      }
      currentStep++;
    }
  } else {
    splash(500, 50, 1000);
  }

  updateSensors();
  if (fadeLedKick < 255) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds.setPixelColor(i, leds.Color(255 - fadeLedKick, 0, 0));
    }
    leds.show();
  }
  if (fadeLedHat < 255) {
    //leds.setPixelColor(ledHat, leds.Color(255 - fadeLedHat, 0, 255 - fadeLedHat));
    leds.setPixelColor(5, leds.Color(255 - fadeLedHat, 255 - fadeLedHat, 255 - fadeLedHat));
    leds.setPixelColor(6, leds.Color(255 - fadeLedHat, 255 - fadeLedHat, 255 - fadeLedHat));
    leds.setPixelColor(7, leds.Color(255 - fadeLedHat, 255 - fadeLedHat, 255 - fadeLedHat));
    leds.setPixelColor(8, leds.Color(255 - fadeLedHat, 255 - fadeLedHat, 255 - fadeLedHat));
    leds.setPixelColor(9, leds.Color(255 - fadeLedHat, 255 - fadeLedHat, 255 - fadeLedHat));
    leds.show();
  }
  if (fadeLedSn < 255) {
    //leds.setPixelColor(ledHat, leds.Color(255-fadeLedHat, 0, 255-fadeLedHat));
    leds.setPixelColor(0, leds.Color(255 - fadeLedSn, 255 - fadeLedSn, 255 - fadeLedSn));
    leds.setPixelColor(1, leds.Color(255 - fadeLedSn, 255 - fadeLedSn, 255 - fadeLedSn));
    leds.setPixelColor(2, leds.Color(255 - fadeLedSn, 255 - fadeLedSn, 255 - fadeLedSn));
    leds.setPixelColor(3, leds.Color(255 - fadeLedSn, 255 - fadeLedSn, 255 - fadeLedSn));
    leds.setPixelColor(4, leds.Color(255 - fadeLedSn, 255 - fadeLedSn, 255 - fadeLedSn));
    leds.show();
  }
  if (fadeLedVocal < 255) {
    leds.setPixelColor(2, leds.Color(0, 255 - fadeLedVocal, 255 - fadeLedVocal));
    leds.setPixelColor(7, leds.Color(0, 255 - fadeLedVocal, 255 - fadeLedVocal));
    leds.show();
  }
}
void updateSensors() {
  if (timer) {
    adc3 = analogRead(PAD_3_PIN);
    adc1 = analogRead(PAD_1_PIN);
    auto raw = analogRead(PAD_0_PIN);
    auto rawb = analogRead(PAD_2_PIN);
    float freq = filter2(filter1(raw)) / 8;
    float freqLed;
    float pitchLed;
    int pitch = filter2b(filter1b(rawb)) / 8;
#ifdef DEBUG
    Serial.print(pitch);
    Serial.print(", ");
    Serial.println(freq);
#endif
    pitchLed = map(pitch, 40, 127, 255, 0);

    pitch = map(pitch, 40, 127, 0, 36);
    freqLed = map(freq, 40, 127, 255, 0);
    freq = map(freq, 40, 127, 15000, 20);
    //Serial.print(freq);
    //Serial.print(", ");
    freq = fscale(freq, 20, 15000, 20, 15000, -5);
    //Serial.println(freq);
    freqLed = constrain(freqLed, 0, 255);
    pitchLed = constrain(pitchLed, 0, 255);
    if (freqLed > 10) {
      leds.setPixelColor(0, leds.Color(0, freqLed, 0));
      leds.setPixelColor(1, leds.Color(0, freqLed, 0));
      leds.setPixelColor(7, leds.Color(0, freqLed, 0));
      leds.setPixelColor(8, leds.Color(0, freqLed, 0));
      leds.setPixelColor(9, leds.Color(0, freqLed, 0));
      leds.show();
    }
    if (pitchLed > 10) {
      leds.setPixelColor(2, leds.Color(pitchLed, pitchLed, 0));
      leds.setPixelColor(3, leds.Color(pitchLed, pitchLed, 0));
      leds.setPixelColor(4, leds.Color(pitchLed, pitchLed, 0));
      leds.setPixelColor(5, leds.Color(pitchLed, pitchLed, 0));
      leds.setPixelColor(6, leds.Color(pitchLed, pitchLed, 0));
      leds.show();
    }
    if (freqLed > 10 || pitchLed > 0) {  // para que no pise splash
      //leds.show();
    }
    for (int track = 0; track < MAX_TRACKS + 3; track++) {
      amy_event sn = amy_default_event();
      sn.osc = track;
      //sn.preset = track;
      sn.wave = 7;
      sn.filter_type = 3;

      sn.filter_freq_coefs[0] = freq;
      amy_add_event(&sn);
    }
    for (int track = 0; track < MAX_TRACKS + 3; track++) {
      amy_event sn = amy_default_event();
      sn.osc = track;
      //sn.preset = track;
      sn.wave = 7;
      //sn.filter_type = 4;
      sn.midi_note = pitch;
      //sn.filter_freq_coefs[0] = freq;
      amy_add_event(&sn);
    }
  }

  //Serial.println(adc3);
  if (adc3 < THRESH_DIAM && adc3button == false) {
    // recién detectado "botón presionado"
    bounceAdc3 = 0;

    adc3button = true;
    if (count % 2 == 0) {
      start = false;

    } else {
      start = true;
      currentStep = 0;
    }
    //Serial.println(start);
    //delay(50);

    count++;
  } else if (adc3 >= THRESH_DIAM && adc3button == true && bounceAdc3 > 25) {
    // se soltó el "botón", reseteo el estado
    //Serial.println("untouch");
    bounceAdc3 = 0;
    adc3button = false;
  }
  if (adc1 < THRESH_21 && adc1button == false) {
    // recién detectado "botón presionado"

    adc1button = true;
    int randomSample = random(8, 12);
    playSamples(9, randomSample, 0.1);  //osc 8 esta raro
    fadeLedVocal = 0;
    //playSamples(9, 8, 1); //osc 8 esta raro
    //Serial.println(start);


  } else if (adc1 >= THRESH_21 && adc1button == true) {
    // se soltó el "botón", reseteo el estado
    //Serial.println("untouch");
    adc1button = false;
  }
}
void playSamples(int track, int preset, float velocity) {

  amy_event sample = amy_default_event();
  sample.osc = track;
  sample.preset = preset;
  //Serial.println(preset);
  sample.velocity = velocity;
  sample.wave = 7;

  amy_add_event(&sample);
}

byte ledTable[10] = { 9, 0, 8, 1, 7, 2, 6, 3, 5, 4 };



int currentLed = 0;
int direction = 1;  // 1 = adelante, -1 = atrás

unsigned long lastUpdate = 0;
unsigned long fadeStart = 0;
bool stopped = false;
unsigned long pauseStart = 0;
unsigned long pauseEnd = 1000;
unsigned long stepInterval = 100;  // tiempo entre LEDs (ms)
unsigned long fadeTime = 50;       // tiempo de fade entre LEDs (ms)
bool fading = false;

void splash(unsigned long tiempoTotal, unsigned long fadeDuracion, unsigned long pausaDuracion) {
  stepInterval = tiempoTotal / NUM_LEDS;
  fadeTime = fadeDuracion;
  pauseEnd = pausaDuracion;

  unsigned long now = millis();

  if (stopped) {
    if (now - pauseStart >= pauseEnd) {
      // fin de la pausa, arrancar de nuevo
      stopped = false;
      currentLed = 0;
      lastUpdate = now;
      fadeStart = now;
      fading = true;
    } else {
      // durante la pausa apagar todo
      leds.clear();
      leds.show();
      return;
    }
  }

  if (!fading && now - lastUpdate >= stepInterval) {
    // avanzar al siguiente LED
    currentLed++;
    if (currentLed >= NUM_LEDS) {
      // terminó la vuelta, activar pausa
      stopped = true;
      pauseStart = now;
      leds.clear();
      leds.show();
      return;
    }
    lastUpdate = now;
    fadeStart = now;
    fading = true;
  }

  if (fading) {
    float progress = float(now - fadeStart) / fadeTime;
    if (progress >= 1.0) {
      progress = 1.0;
      fading = false;
    }

    leds.clear();

    // LED principal al máximo
    leds.setPixelColor(currentLed, leds.Color(255, 255, 255));

    // LED anterior con fade
    int prevLed = currentLed - 1;
    if (prevLed < 0) prevLed = NUM_LEDS - 1;
    int fadeValue = 255 * (1.0 - progress);
    leds.setPixelColor(prevLed, leds.Color(fadeValue, 0, 0));

    leds.show();
  }
}
