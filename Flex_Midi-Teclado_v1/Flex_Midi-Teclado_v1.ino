#include <MIDIUSB.h>
#include <MIDIUSB_Defs.h>
#include <light_CD74HC4067.h>

const int signal_pin = A8; // Pin Connected to Sig pin of CD74HC4067
const int en_pin1 = 2;
const int en_pin2 = 3;

const int pot_pin[4] = {A3, A2, A1, A0};

const int button_pin[5] = {9, 15, 16, 10, 14};

byte midiCh = 0;
byte note = 48;
byte cc = 1;

unsigned long lastDebounceTimeNote[25] = {0};
unsigned long lastDebounceTimeButton[5] = {0};
unsigned long debounceDelay = 5;

CD74HC4067 mux(4, 5, 6, 7);  // create a new CD74HC4067 object with its four select lines

int noteChannel[25] = {14, 15, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 0, 1, 2, 3};
int noteCState[25] = {1};
int notePState[25] = {1};
int buttonCState[5] = {1};
int buttonPState[5] = {1}; 
int PotCVal[0];
int PotPVal[0];

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void setup(){
  
  pinMode(en_pin1, OUTPUT);
  pinMode(en_pin2, OUTPUT);
  
  pinMode(signal_pin, INPUT);

  for (int i = 0; i < 4; i++){
    pinMode(pot_pin[i], INPUT);
  }

  for (int i = 0; i < 5; i++){
    pinMode(button_pin[i], INPUT_PULLUP);
  }
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void loop(){
  
//Leitura =================================================================================//
  for (int i = 0; i < 25; i++){
    if (i < 12){
      digitalWrite(en_pin1, 1);
      digitalWrite(en_pin2, 0);
    }
    if (i >= 12){
      digitalWrite(en_pin1, 0);
      digitalWrite(en_pin2, 1);
    }
    mux.channel(noteChannel[i]);
    noteCState[i] = digitalRead(signal_pin);
  }

  for (int i = 0; i < 4; i++){
    if (i == 0){
      PotCVal[i] = map(analogRead(pot_pin[i]), 0, 1023, 16383, 0);
    }else{
      PotCVal[i] = map(analogRead(pot_pin[i]), 0, 1023, 127, 0);
    }
  }

  for (int i = 0; i < 5; i++){
    buttonCState[i] = digitalRead(button_pin[i]);
  }
  
  //Gravação =================================================================================//

  for (int i = 0; i < 25; i++){

    if((millis() - lastDebounceTimeNote[i]) > debounceDelay){
      
      if(notePState[i] != noteCState[i]){
        lastDebounceTimeNote[i] = millis();
        
        if(notePState[i] != LOW){ 
          noteOn(midiCh, note + i, 127);
          MidiUSB.flush();
        }else{
          noteOn(midiCh, note + i, 0);
          MidiUSB.flush();
        }
        notePState[i] = noteCState[i];
      }
    }
    
  }
  
  for (int i = 0; i < 4; i++){

    if (PotCVal[i] != PotPVal[i]){

      if (i == 0){
        byte lowValue = PotCVal[i] & 0x7F;
        byte highValue = PotCVal[i] >> 7;
        midiEventPacket_t event = {0x0E, 0xE0 | midiCh, lowValue, highValue};
        MidiUSB.sendMIDI(event);
      }else{
        controlChange(midiCh, i, PotCVal[i]);
      }
      PotPVal[i] = PotCVal[i];
    }
  }

  for (int i = 0; i < 5; i++){

    if((millis() - lastDebounceTimeButton[i]) > debounceDelay){
      
      if(buttonPState[i] != buttonCState[i]){
        lastDebounceTimeNote[i] = millis();
        
        if((buttonPState[i] != LOW) && (i == 0) && (note <= 84)){ 
          note = note + 12;
        }
        if((buttonPState[i] != LOW) && (i == 1) && (note >= 12)){ 
          note = note - 12;
        }
        
        buttonPState[i] = buttonCState[i];
      }
    }
  }
}