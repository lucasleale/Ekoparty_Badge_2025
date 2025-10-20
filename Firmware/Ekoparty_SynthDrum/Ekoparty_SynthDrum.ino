#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include <AMY-Arduino.h>
#include <elapsedMillis.h>
#include <Filters.h>
#include <Filters/Notch.hpp>
#include <AH/Timing/MillisMicrosTimer.hpp>
#include <Wire.h>
#include "TouchSensor.h"

#define PAD_1 A0
#define PAD_2 A1
#define PAD_3 A2
#define PAD_4 A3

#define NUM_LEDS 10
#define LEDS_PIN 25

#define MUTE_PIN 11
#define SENSE_HP 2
#define SDA 8
#define SCL 9
Adafruit_NeoPixel leds(NUM_LEDS, LEDS_PIN, NEO_GRB + NEO_KHZ800);

#define TPA2028_ADDR 0x58
#define TPA2028_REG_GAIN 0x05
#define NUM_NOTES 21
elapsedMillis led1;

elapsedMillis led2;
elapsedMillis led3;
elapsedMillis led4;
TouchSensor sensor1(PAD_1, 1000);  //oreja derecha
TouchSensor sensor2(PAD_2, 1000);  //21
TouchSensor sensor3(PAD_3, 1000);  //oreja izq
TouchSensor sensor4(PAD_4, 1000);  //diamante
const int rightLeds[5] = { 0, 1, 7, 8, 9 };
const int leftLeds[5] = { 2, 3, 4, 5, 6 };


int eMinor[NUM_NOTES] = {
  28, 30, 31, 33, 35, 36, 38,   // E1 a D2
  40, 42, 43, 45, 47, 48, 50,   // E2 a D3
  52, 54, 55, 57, 59, 60, 62,   // E3 a D4
  
};
void setup() {
  // put your setup code here, to run once:
  leds.begin();

  ////////////////////////
  amy_config_t amy_config = amy_default_config();
  amy_config.features.default_synths = 1;
  amy_config.i2s_mclk = 7;
  amy_config.i2s_bclk = 20;
  amy_config.i2s_lrc = 21;
  amy_config.i2s_dout = 22;
  amy_config.i2s_din = 11;
  amy_config.features.default_synths = 1;
  amy_config.features.chorus = 1;
  //amy_config.features.reverb = 1;
  //amy_config.features.echo = 1;
  amy_start(amy_config);
  ////////////////////////
  amy_live_start();
  ////////////////////////
  amy_event global = amy_default_event();
  global.volume = 4;
  global.eq_l = -15;
  global.eq_m = 1;
  global.eq_h = -5;
  amy_add_event(&global);

  ////////////////////////
  Serial.begin(115200);
  Wire.setSDA(SDA);
  Wire.setSCL(SCL);
  Wire.begin();
  delay(500);
  Wire.beginTransmission(0x58);  // dirección del TPA2028
  Wire.write(0x05);              // registro de ganancia
  Wire.write(0x00);
  Wire.endTransmission();
  delay(100);
  Wire.beginTransmission(0x58);
  Wire.write(0x07);  // Registro I
  //Wire.write(0b00001000);  // Compression 1:1 + Output Limiter disabled
  Wire.write(0b00000000);  // Bits 7:4 Max Gain = 0 (no importa), 3:2 = 00, 1:0 Compression = 00 (1:1)
  Wire.endTransmission();
  pinMode(MUTE_PIN, OUTPUT);
  digitalWrite(MUTE_PIN, HIGH);
  pinMode(SENSE_HP, INPUT);
  //config_echo(0.5f, 70.0f, 80.0f, 0.9f, 0.0f); 
  amy_event e = amy_default_event();
  e.reset_osc = RESET_AMY;
  amy_add_event(&e);
  e = amy_default_event();
  e.synth = 2;
  e.patch_number = 4; //23 esta piola tmb, 30 para bass tmb //133, 4
  e.num_voices = 1;  // I get 12 simultaneous juno patch 0 voices on a 250 MHz RP2040.
  amy_add_event(&e);
  e = amy_default_event();
  e.synth = 3;
  e.patch_number = 23; //23 esta piola tmb, 30 para bass tmb //133, 4
  e.num_voices = 1;  // I get 12 simultaneous juno patch 0 voices on a 250 MHz RP2040.
  amy_add_event(&e);
  
  amy_event c = amy_default_event();
  //c.sequence[0] = 0;
  //c.sequence[1] = 48;
  //c.sequence[2] = 1;
  //c.velocity = 1;
  //c.midi_note = 48;
  //c.synth = 2;
  amy_add_event(&c);
  c = amy_default_event();
  //c.sequence[0] = 0;
  //c.sequence[1] = 48;
  //c.sequence[2] = 1;
  //c.velocity = 1;
  //c.midi_note = 48;
  //c.synth = 3;
  amy_add_event(&c);
}

void loop() {
  // put your main code here, to run repeatedly:
  amy_update();
  checkJack();
  updateSensors();
  //splash(500, 100, 0);
}


void updateSensors() {
  if (sensor1.update()) {
    //Serial.print(sensor1.getFiltered());
    int sensor1Value = sensor1.getFiltered();
    int sensor1Leds = map(sensor1Value, 300, 1021, 255, 0);
    int sensor1Pitch = map(sensor1Value, 300, 1021, NUM_NOTES-1, 0);
    sensor1Pitch = constrain(sensor1Pitch, 0, NUM_NOTES-1);
    sensor1Leds = constrain(sensor1Leds, 0, 255);
    //Serial.println(sensor1Pitch);
    for (int i = 0; i < NUM_LEDS / 2; i++) {
      if(sensor1Leds < 10){
        sensor1Leds = 0;
      }
      leds.setPixelColor(rightLeds[i], leds.Color(0, sensor1Leds, 0));
    }
    leds.show();
    changePitch(3, eMinor[sensor1Pitch]+12);
    if (sensor1.justPressed()) {
      //Serial.println("Sensor1 pressed!");
      sendNote(3, 0.2, eMinor[sensor1Pitch]+12);  //que oscilador, on/off
    }
    if (sensor1.justReleased()) {
      //Serial.println("Sensor1 released!");
      sendNote(3, 0, eMinor[sensor1Pitch]+12);  //que oscilador, on/off

    }
  }
  static bool adc2button;
  if (sensor2.update()) {
    //Serial.print(", ");
    //Serial.print(sensor2.getFiltered());
     if(sensor2.getFiltered() < 900 && adc2button == false){
      adc2button = true;
      playSamples(1, 1);
      led2 = 0;
      
    }
    if(sensor2.getFiltered() > 900 && adc2button == true){
      adc2button = false;
    }
  }
  if (sensor3.update()) {
    //Serial.print(", ");
    //Serial.print(sensor3.getFiltered());
    int sensor3Value = sensor3.getFiltered();
    int sensor3Leds = map(sensor3Value, 300, 1021, 255, 0);
    int sensor3Pitch = map(sensor3Value, 300, 1021, NUM_NOTES-1, 0);
    sensor3Pitch = constrain(sensor3Pitch, 0, NUM_NOTES-1);
    sensor3Leds = constrain(sensor3Leds, 0, 255);
    changePitch(2, eMinor[sensor3Pitch]-12);
    for (int i = 0; i < NUM_LEDS / 2; i++) {
      if(sensor3Leds < 10){
        sensor3Leds = 0;
      }
      leds.setPixelColor(leftLeds[i], leds.Color(0, sensor3Leds, 0));
    }
    leds.show();
    if (sensor3.justPressed()) {
      //Serial.println("Sensor1 pressed!");
      sendNote(2, 1, eMinor[sensor3Pitch]-12);  //que oscilador, on/off
    }
    if (sensor3.justReleased()) {
      //Serial.println("Sensor1 released!");
      sendNote(2, 0, eMinor[sensor3Pitch]-12);  //que oscilador, on/off
    }
  }
  static bool adc4button;
  if (sensor4.update()) {
    //Serial.print(", ");
    //Serial.println(sensor4.getFiltered());
    if(sensor4.getFiltered() < 900 && adc4button == false){
      adc4button = true;
      playSamples(0, 2); // si pongo 4,2 suena mal, anda a saber por que 
      //playSamples(4, 0);
      led4 = 0;
    }
    if(sensor4.getFiltered() > 900 && adc4button == true){
      adc4button = false;
    }
    if (sensor4.justPressed()) {
      //Serial.println("Sensor1 pressed!");
      //sendNote(3, 1);  //que oscilador, on/off
      //playSamples(2, 0);
    }
    if (sensor4.justReleased()) {
      //Serial.println("Sensor1 released!");
      //sendNote(3, 0);  //que oscilador, on/off
      
    }

  }
  if(led4 < 255){
    for (int i = 0; i < NUM_LEDS / 2; i++) {
      leds.setPixelColor(leftLeds[i], leds.Color(0, 255-led4, 255-led4));
      leds.setPixelColor(rightLeds[i], leds.Color(0, 255-led4, 255-led4));
    }
    leds.show();
  }
  if(led2 < 255){
    for (int i = 0; i < NUM_LEDS / 2; i++) {
      leds.setPixelColor(leftLeds[i], leds.Color(255-led2, 0, 0));
      leds.setPixelColor(rightLeds[i], leds.Color(255-led2, 0, 0));
    }
    leds.show();
  }
}
void changePitch(int osc, int pitch){
  
  amy_event note = amy_default_event();
  
  //note.osc = osc;
  note.midi_note = pitch;
  //note.portamento_ms = 100;
  //note.voices[0] = osc;
  //note.wave = 2;
  //note.velocity = velocity;
  //note.midi_note = freq;
  
  note.synth = osc;
  amy_add_event(&note);
}
void sendNote(int osc, float velocity, int pitch) {
  amy_event note = amy_default_event();
  note.midi_note = 36;
  note.velocity = velocity;
  note.portamento_ms = 100;
  
  
  //note.osc = osc;
  
  note.synth = osc;
  amy_add_event(&note);
}
void playSamples(int track, int preset) {

  amy_event sample = amy_default_event();
  sample.osc = track;
  sample.preset = preset;
  //sample.midi_note = ;
  //Serial.println(preset);
  sample.velocity = 1;
  sample.wave = 7;
  amy_add_event(&sample);
}

void checkJack() {
  static bool readSenseHp;
  static bool readSenseHpLast;
  if (readSenseHp != readSenseHpLast) {
    if (readSenseHp == 0) {
      Serial.println("SPEAKER");
      amy_event global = amy_default_event();
      global.volume = 4;
      global.eq_l = -15;
      global.eq_m = 1;
      global.eq_h = -5;
      amy_add_event(&global);
      digitalWrite(MUTE_PIN, LOW);
      Wire.beginTransmission(0x58);
      Wire.write(0x01);  // Function Control register
      Wire.write(0x43);  // 0b01000011 → EN=1
      Wire.endTransmission();
    } else {
      Serial.println("HEADPHONES");
      amy_event global = amy_default_event();
      global.volume = 5;
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
}







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
    leds.setPixelColor(currentLed, leds.Color(0, 255, 255));

    // LED anterior con fade
    int prevLed = currentLed - 1;
    if (prevLed < 0) prevLed = NUM_LEDS - 1;
    int fadeValue = 255 * (1.0 - progress);
    leds.setPixelColor(prevLed, leds.Color(0, fadeValue, fadeValue));

    leds.show();
  }
}
