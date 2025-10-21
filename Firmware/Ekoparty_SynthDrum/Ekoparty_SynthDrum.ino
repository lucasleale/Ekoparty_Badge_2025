#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <AMY-Arduino.h>
#include <elapsedMillis.h>
#include <Filters.h>
#include <Filters/Notch.hpp>
#include <AH/Timing/MillisMicrosTimer.hpp>
#include <Wire.h>
#include "TouchSensor.h"
//OVERCLOCKEAR A 250MHz


#define PAD_1 A0
#define PAD_2 A1
#define PAD_3 A2
#define PAD_4 A3
#define NUM_LEDS 10
#define LEDS_PIN 25
#define MUTE_PIN 11  //EN TPA6138
#define SENSE_HP 2   //pin de corte de jack 3.5mm, detecta si se conectó cable
#define SDA 8
#define SCL 9
#define TPA2028_ADDR 0x58
#define NUM_NOTES 21
#define THRESH 1000
#define MIN_SENSORS 300  //varias pruebas y calibraciones dan rango usable de MIN_SENSORS-MAX_SENSORS en ADC 10 bits (0-1023)
#define MAX_SENSORS 1021
#define THRESH_BUTTON 900  //umbral para sensor2 y 4 como botones, responden mejor.

//timers, easy mode. Para fades
elapsedMillis led2;  //pad2
elapsedMillis led4;  //pad4

//En este firmware, sensor2 y sensor4 funcionan como botones para triggear samples. Sensor1 y sensor3 activan nota tenida y modifican pitch.
TouchSensor sensor1(PAD_1, THRESH);  //oreja derecha
TouchSensor sensor2(PAD_2, THRESH);  //21
TouchSensor sensor3(PAD_3, THRESH);  //oreja izq
TouchSensor sensor4(PAD_4, THRESH);  //diamante
Adafruit_NeoPixel leds(NUM_LEDS, LEDS_PIN, NEO_GRB + NEO_KHZ800);
const int rightLeds[5] = { 0, 1, 7, 8, 9 };
const int leftLeds[5] = { 2, 3, 4, 5, 6 };


int eMinor[NUM_NOTES] = {
  //escala mi menor
  28, 30, 31, 33, 35, 36, 38,  // E1 a D2
  40, 42, 43, 45, 47, 48, 50,  // E2 a D3
  52, 54, 55, 57, 59, 60, 62,  // E3 a D4

};
void setup() {
  leds.begin();

  ////////////////////////
  amy_config_t amy_config = amy_default_config();
  amy_config.features.default_synths = 1;
  amy_config.i2s_mclk = 7;  //i2S
  amy_config.i2s_bclk = 20;
  amy_config.i2s_lrc = 21;
  amy_config.i2s_dout = 22;
  amy_config.i2s_din = 11;
  amy_config.features.default_synths = 1;
  amy_config.features.chorus = 1;
  amy_start(amy_config);
  ////////////////////////
  amy_live_start();
  ////////////////////////
  amy_event global = amy_default_event();
  global.volume = 4;  //volumen global y eq
  global.eq_l = -15;
  global.eq_m = 1;
  global.eq_h = -5;
  amy_add_event(&global);

  ////////////////////////
  Serial.begin(115200);

  /////I2C CONFIG TPA2028
  Wire.setSDA(SDA);
  Wire.setSCL(SCL);
  Wire.begin();
  delay(500);
  Wire.beginTransmission(TPA2028_ADDR);  // dirección del TPA2028
  Wire.write(0x05);                      // registro de ganancia
  Wire.write(0x00);
  Wire.endTransmission();
  delay(100);
  Wire.beginTransmission(TPA2028_ADDR);
  Wire.write(0x07);        // Registro I
  Wire.write(0b00000000);  // Bits 7:4 Max Gain = 0 (no importa), 3:2 = 00, 1:0 Compression = 00 (1:1)
  Wire.endTransmission();
  ////////

  pinMode(MUTE_PIN, OUTPUT);
  digitalWrite(MUTE_PIN, HIGH);
  pinMode(SENSE_HP, INPUT);
  config_echo(0.5f, 70.0f, 80.0f, 0.7f, 0.0f);  //DELAY!!!!

  /////CONFIG SYNTHS
  amy_event e = amy_default_event();
  e.reset_osc = RESET_AMY;
  amy_add_event(&e);
  e = amy_default_event();
  e.synth = 2;
  e.patch_number = 4;  //sinte orejas. Cambiar patch_number para otros sonidos
  e.num_voices = 1;
  amy_add_event(&e);
  e = amy_default_event();
  e.synth = 3;
  e.patch_number = 23;
  e.num_voices = 1;
  amy_add_event(&e);
  
}

void loop() {
  amy_update();  //procesamiento de audio
  checkJack();   //detecta estado del jack 3.5mm, activa parlante o auris
  updateSensors();
  fadeLeds();
 
}



void updateSensors() {  //4 sensores de la clase touchSensor (fir filter, debounce, thresholds)
  handleSensor1();
  handleSensor2();
  handleSensor3();
  handleSensor4();
}

void handleSensor1() {
  if (sensor1.update()) {
    int sensor1Value = sensor1.getFiltered();                                          //filtro FIR
    int sensor1Leds = map(sensor1Value, MIN_SENSORS, MAX_SENSORS, 255, 0);             //map ADC a brillo de leds
    int sensor1Pitch = map(sensor1Value, MIN_SENSORS, MAX_SENSORS, NUM_NOTES - 1, 0);  //map ADC a array mi menor
    sensor1Pitch = constrain(sensor1Pitch, 0, NUM_NOTES - 1);                          //limit
    sensor1Leds = constrain(sensor1Leds, 0, 255);                                      //limit

    for (int i = 0; i < NUM_LEDS / 2; i++) {
      if (sensor1Leds < 10) {
        sensor1Leds = 0;  //a veces por un poco de ruido quedan los leds prendidos, asi forzamos
      }
      leds.setPixelColor(rightLeds[i], leds.Color(0, sensor1Leds, 0));
    }
    leds.show();

    changePitch(3, eMinor[sensor1Pitch] + 12);  //cambia pitch de oscilador prendido
    if (sensor1.justPressed()) {                //con debounce
      sendNote(3, 0.2);                         //que oscilador, on
    }
    if (sensor1.justReleased()) {
      sendNote(3, 0);  //que oscilador, off
    }
  }
}

void handleSensor2() {
  static bool adc2button;
  if (sensor2.update()) {  //probando respuesta de sensores... es mas responsivo sin debounce?
    if (sensor2.getFiltered() < THRESH_BUTTON && adc2button == false) {
      adc2button = true;
      playSamples(1, 1);  //dispara sample voice, preset
      led2 = 0;           //start fade
    }
    if (sensor2.getFiltered() > THRESH_BUTTON && adc2button == true) {
      adc2button = false;
    }
  }
}

void handleSensor3() {
  if (sensor3.update()) {
    int sensor3Value = sensor3.getFiltered();
    int sensor3Leds = map(sensor3Value, MIN_SENSORS, MAX_SENSORS, 255, 0);
    int sensor3Pitch = map(sensor3Value, MIN_SENSORS, MAX_SENSORS, NUM_NOTES - 1, 0);
    sensor3Pitch = constrain(sensor3Pitch, 0, NUM_NOTES - 1);
    sensor3Leds = constrain(sensor3Leds, 0, 255);
    changePitch(2, eMinor[sensor3Pitch] - 12);  //cambia pitch de oscilador prendido
    for (int i = 0; i < NUM_LEDS / 2; i++) {
      if (sensor3Leds < 10) {
        sensor3Leds = 0;
      }
      leds.setPixelColor(leftLeds[i], leds.Color(0, sensor3Leds, 0));
    }
    leds.show();
    if (sensor3.justPressed()) {
      sendNote(2, 1);  //que oscilador, on/off
    }
    if (sensor3.justReleased()) {
      sendNote(2, 0);  //que oscilador, on/off
    }
  }
}

void handleSensor4() {
  static bool adc4button;
  if (sensor4.update()) {
    if (sensor4.getFiltered() < THRESH_BUTTON && adc4button == false) {
      adc4button = true;
      playSamples(0, 2);  //dispara sample voice, preset
      led4 = 0;           //start fade
    }
    if (sensor4.getFiltered() > THRESH_BUTTON && adc4button == true) {
      adc4button = false;
    }
  }
}

void sendNote(int osc, float velocity) {  //velocity > 0 = note on, == 0 note off.
  amy_event note = amy_default_event();
  note.midi_note = 36;  //siempre prende y apaga la nota en la misma altura
  note.velocity = velocity;
  note.portamento_ms = 100;
  note.synth = osc;
  amy_add_event(&note);
}

void changePitch(int osc, int pitch) {  //osc vendria a ser voz activa, pitch cambia la nota de la voz correspondiente

  amy_event note = amy_default_event();
  note.midi_note = pitch;  // ahora si, modifica la nota segun pitch de los sensores
  note.synth = osc;
  amy_add_event(&note);
}

void playSamples(int track, int preset) {

  amy_event sample = amy_default_event();
  sample.osc = track;      //voz activa
  sample.preset = preset;  //distintos sonidos del banco PCM
  sample.velocity = 1;
  sample.wave = 7;  //wave 7 es banco PCM de samples
  amy_add_event(&sample);
}

void fadeLeds() {
  if (led4 < 255) {  //dirty...
    for (int i = 0; i < NUM_LEDS / 2; i++) {
      leds.setPixelColor(leftLeds[i], leds.Color(0, 255 - led4, 255 - led4));
      leds.setPixelColor(rightLeds[i], leds.Color(0, 255 - led4, 255 - led4));
    }
    leds.show();
  }
  if (led2 < 255) {
    for (int i = 0; i < NUM_LEDS / 2; i++) {
      leds.setPixelColor(leftLeds[i], leds.Color(255 - led2, 0, 0));
      leds.setPixelColor(rightLeds[i], leds.Color(255 - led2, 0, 0));
    }
    leds.show();
  }
}

void checkJack() {
  static bool readSenseHp;
  static bool readSenseHpLast;
  readSenseHp = digitalRead(SENSE_HP);
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
      global.volume = 1;
      global.eq_l = 0;
      global.eq_m = 0;
      global.eq_h = 0;
      amy_add_event(&global);
      Wire.beginTransmission(0x58);
      Wire.write(0x01);  // Function Control register
      Wire.write(0x03);  // 0b01000011 → EN=0
      Wire.endTransmission();
      digitalWrite(MUTE_PIN, HIGH);
    }
    readSenseHpLast = readSenseHp;
  }
}
