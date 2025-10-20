#include <Wire.h>
#include <limits.h>

#include <MD_MIDIFile.h>
#include <LittleFS.h>
#include <MicroDexedArduino.h>

#include "xmasLights_fastLED.h"
#include "midiJukebox.h"

// Note: Not sure why it doesn't work on Arduino Pico 5.3.0 or below! (Asserts on DMA setting at I2S)

#define LED_PIN 25
#define MAX98357_SD 16

constexpr bool DEBUG_MODE = true;

const int LED_TOGGLE_TIME_SETUP_DONE = 1000;
const int LED_TOGGLE_TIME_SETUP_ERROR = 250;
int LedToggleTime = 0;
bool LedState = false;
static uint32_t previousLedToggleTime = 0;

MicroDexedPicoI2sAudio* pMdI2s = nullptr;
Dexed* dexed = nullptr;
MD_MIDIFile SMF;
Dir rootDir;

uint32_t fill_audio_buffer = 0;
uint32_t t0, t1 = 0; // Time taken for MicroDexed engine to generate samples!
const uint16_t audio_block_time_us = 1000000 / (SAMPLE_RATE / AUDIO_BLOCK_SAMPLES);

const uint32_t MAX_STUTTERS_PER_SECOND = 128;
uint32_t numberOfStuttersPerSecond = 0;
const bool STUTTER_REBOOT = true;

static enum { S_IDLE,
              S_PLAYING,
              S_NEXT,
              S_RESTART } state = S_IDLE;
static enum { SETUP_PROGRESS,
              SETUP_DONE_WITH_ERROR,
              SETUP_DONE } setupState = SETUP_PROGRESS;
static enum { DEXED_EPIANO = 11,
              DEXED_GUITAR = 12,
              DEXED_GLOCKENSPIEL_2 = 30,
              DEXED_HANDBELL_1 = 31 } dexedInstruments = DEXED_EPIANO;

uint8_t pickDexedVoice = DEXED_EPIANO;

void midiSilence(void)
// Turn everything off on every channel.
// Some midi files are badly behaved and leave notes hanging, so between songs turn
// off all the notes and sound
{
  midi_event ev;

  // All sound off
  // When All Sound Off is received all oscillators will turn off, and their volume
  // envelopes are set to zero as soon as possible.
  ev.size = 0;
  ev.data[ev.size++] = 0xb0;
  ev.data[ev.size++] = 120;
  ev.data[ev.size++] = 0;

  for (ev.channel = 0; ev.channel < 16; ev.channel++)
    midiCallback(&ev);
}

void midiCallback(midi_event* pev)
// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
{
  // Define constants for MIDI channel voice message IDs
  const uint8_t NOTE_OFF = 0x80;  // note on
  const uint8_t NOTE_ON = 0x90;   // note off. NOTE_ON with velocity 0 is same as NOTE_OFF

  switch (pev->data[0]) {
    case NOTE_OFF:  // [1]=note no, [2]=velocity
      dexed->keyup(pev->data[1]);
      Serial.printf("NOTE_OFF ch: %d\n", pev->data[1]);
      break;

    case NOTE_ON:  // [1]=note_no, [2]=velocity
      // Note ON with velocity 0 is the same as off
      // Velocity is not included for now!
      dexed->keydown(pev->data[1], 127);
      Serial.printf("NOTE_ON trk: %d, ch: %d, no: %d\n", pev->track, pev->channel, pev->data[1]);
      break;

    default:
      break;
  }
}

void dexedPlaySamples() {
  // Main sound calculation
  fill_audio_buffer = micros();
  if ((i2s.availableForWrite() >= AUDIO_BLOCK_SAMPLES) && (fill_audio_buffer > (audio_block_time_us - 10))) {
    fill_audio_buffer = 0;
    t0 = micros();
    dexed->getSamples(AUDIO_BLOCK_SAMPLES, pMdI2s->audio_buffer);
    t1 = micros();
    if((t1 - t0) > audio_block_time_us)
    {
      // Serial.printf("Exceeded calculation time! %d microsecs.\n", (t1 - t0));
      numberOfStuttersPerSecond++;
    }
    pMdI2s->playSamples(AUDIO_BLOCK_SAMPLES, pMdI2s->audio_buffer);
  }
}

// Get random number from the rp2040/rp2350's hardware pseudo-random generator:
uint32_t getRandomNumber(uint32_t min, uint32_t max) {
  uint32_t randomNumber = rp2040.hwrand32();
  return (uint32_t)((randomNumber % (max - min + 1)) + min);
}

void nextSong(String midiFileName) {

  // Shut off notes at dexed during change
  // to prevent notes from going out of sync at next song!!
  dexed->notesOff();
  dexed->doRefreshVoice();

  int err = SMF.load(midiFileName.c_str());
  if (midiFileName == "NoMoreMidiFileInDir") {
    Serial.println("No more midi files in directory!");
    return;
  }

  Serial.printf("Filename: %s\n", SMF.getFilename());
  Serial.printf("Format: %d\n", SMF.getFormat());

  if (err != MD_MIDIFile::E_OK)
    Serial.printf("SMF load error during playing: %d\n", err);
  else
    SMF.restart();
}

void stutterCheck()
{
  static uint32_t previousTime;
  if(millis() - previousTime >= 1000)
  {
    // Serial.printf("Stutters per second: %d\n", numberOfStuttersPerSecond);
    if(numberOfStuttersPerSecond > MAX_STUTTERS_PER_SECOND)
    {
      Serial.printf("Too much stuttering, exceeding max. number of stutter! %d\n", numberOfStuttersPerSecond);

      if(STUTTER_REBOOT)
        rp2040.reboot();
      else
      {
        while(1)
        {
           digitalWrite(LED_PIN, HIGH);
           delay(100);
           digitalWrite(LED_PIN, LOW);
           delay(100);
        }
      }
    }
    numberOfStuttersPerSecond = 0;
    previousTime = millis();
  }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);

  uint32_t randomNumber = getRandomNumber(0, 3);

  delay(1000);

  Serial.println("arduino-xmas-883-2025");
  Serial.printf("Random number on startup: %d\n", randomNumber);

  pinMode(LED_PIN, OUTPUT);
  pinMode(WS2812_PIN, OUTPUT);

  Serial.println("Starting FastLED!");
  setupXmasLights();

  // Switch off the MAX98357 module first before init MicroDexed and I2S interface!
  pinMode(MAX98357_SD, OUTPUT);
  digitalWrite(MAX98357_SD, LOW);

  Serial.println(F("MicroDexed based on https://github.com/asb2m10/dexed"));
  Serial.println(F("(c)2018,2019-2025 H. Wirtz <wirtz@parasitstudio.de>"));
  Serial.println(F("https://github.com/dcoredump/MicroDexed"));
  Serial.printf("Max number of notes: %d\n", MAX_NOTES);
  Serial.println(F("<setup start>"));

  switch (randomNumber) {
    case 0:
      pickDexedVoice = DEXED_EPIANO;
      break;
    case 1:
      pickDexedVoice = DEXED_GUITAR;
      break;
    case 2:
      pickDexedVoice = DEXED_GLOCKENSPIEL_2;
      break;
    case 3:
      pickDexedVoice = DEXED_HANDBELL_1;
      break;
    default:
      break;
  }

  pMdI2s = new MicroDexedPicoI2sAudio(pickDexedVoice);
  dexed = new Dexed(SAMPLE_RATE);
  dexed->loadSysexVoice(pMdI2s->voice_data);

  Serial.println(F("<setup end>"));

  delay(250);

  digitalWrite(MAX98357_SD, HIGH);

  // Initialize LittleFS:
  if (!LittleFS.begin()) {
    Serial.println("LittleFS init fail!");
    state = S_IDLE;
    digitalWrite(MAX98357_SD, LOW);
    rp2040.fifo.push(SETUP_DONE_WITH_ERROR);
    return;
  }

  // Initialize MIDIFile:
  SMF.begin(&LittleFS);
  SMF.setMidiHandler(midiCallback);
  Serial.println("Start playing song now!");

  rootDir = LittleFS.openDir("/");

  // Load a first MIDI file from there:
  // Jukebox continuously plays next MIDI file if there is
  // one working MIDI file loaded for the first time.
  String midiFileName = getNextMidiFileNameFromDir(rootDir);
  int err = SMF.load(midiFileName.c_str());

  if (err != MD_MIDIFile::E_OK) {
    Serial.printf("SMF load error: %d\n", err);
    state = S_IDLE;
    digitalWrite(MAX98357_SD, LOW);
    rp2040.fifo.push(SETUP_DONE_WITH_ERROR);
    return;
  }

  Serial.printf("Filename: %s\n", SMF.getFilename());
  Serial.printf("Format: %d\n", SMF.getFormat());

  state = S_PLAYING;

  rp2040.fifo.push(SETUP_DONE);
}

void loop() {
  xmasLightsLoop();

  if (millis() - previousLedToggleTime >= LedToggleTime) {
    LedState = LedState ^ 1;
    digitalWrite(LED_PIN, LedState);
    previousLedToggleTime = millis();
  }
}

void setup1() {

  // put your setup code here, to run once:
  while (!rp2040.fifo.available())
    ;

  if (rp2040.fifo.pop() == SETUP_DONE) {
    Serial.println("Setup done! :D");
    LedToggleTime = LED_TOGGLE_TIME_SETUP_DONE;
  } else {
    Serial.println("Setup error! Please check setup!");
    LedToggleTime = LED_TOGGLE_TIME_SETUP_ERROR;
  }
}

void loop1() {
  // put your main code here, to run repeatedly:
  if (DEBUG_MODE) {
    if (Serial.available() > 0) {
      char incomingChar = Serial.read();
      if (incomingChar == 's')
        state = S_NEXT;
      else if (incomingChar == 'r')
        state = S_RESTART;
    }
  }
  switch (state) {
    case S_IDLE:
      delay(500);
      break;
    case S_PLAYING:
      if (!SMF.isEOF()) {
        dexedPlaySamples();
        SMF.getNextEvent();
        stutterCheck();
      } else {
        // Reached end of file.
        // Play next file, and if no next file, go idle mode!
        SMF.close();
        SMF.pause(true);
        midiSilence();

        String midiFileName = getNextMidiFileNameFromDir(rootDir);
        if (midiFileName == "NoMoreMidiFileInDir") {
          Serial.println("Playing done! Idle mode now...");
          // Switch off the MAX98357 module at idle!
          digitalWrite(MAX98357_SD, LOW);
          state = S_IDLE;
        } else {
          Serial.println("Play song done! Next song...");
          delay(1000);
          nextSong(midiFileName);
          state = S_PLAYING;
          SMF.pause(false);
        }
      }
      break;
    case S_NEXT:
      {
        Serial.println("Skip to next song...");

        SMF.close();
        SMF.pause(true);
        midiSilence();

        delay(100);

        String midiFileName = getNextMidiFileNameFromDir(rootDir);
        nextSong(midiFileName);
        state = S_PLAYING;
        SMF.pause(false);
      }
      break;
    case S_RESTART:
      Serial.println("Restarting song...");
      midiSilence();
      dexed->notesOff();
      dexed->doRefreshVoice();
      dexed->panic();
      SMF.restart();
      delay(100);
      state = S_PLAYING;
      break;
    default:
      break;
  }
}
