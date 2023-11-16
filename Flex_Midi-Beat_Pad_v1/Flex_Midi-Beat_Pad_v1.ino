#include <ResponsiveAnalogRead.h>
#include <MIDIUSB_Defs.h>
#include <MIDIUSB.h>

#define MIDI_START 0xFA
#define MIDI_CONTINUE 0xFB
#define MIDI_STOP  0xFC

const int Pad_pin[8] = {A10, A0, A1, A2, A3, A9, A8, A7};  //Define os pinos de entrada de cada Pad
const int Button_pin[6] = {16, 14, 15, 2, 3, 5};           //Define os pinos de entrada de cada botão
const int Pot_pin = A6; 
int Pad_Zero[8];                                           //Lista que guarda qual o valor de repouso do pad
int Pad_PVal[8];                                           //Lista que guarda o valor bruto ANTERIOR do pad
int Pad_CVal[8];                                           //Lista que guarda o valor bruto ATUAL do pad
int Pad_PVelocity[8];                                      //Lista que guarda os valores tratados ANTERIORES do pad
int Pad_CVelocity[8];                                      //Lista que guarda os valores tratados ANTUAIS do pad
const int Pad_Note[8] = {36, 37, 38, 39, 41, 42, 43};      //Define o valor de cada nota de cada Pad     
int buttonPState[6] = {1};                                 //Lista que guarda o estado ANTERIOR de cada botão
int buttonCState[6] = {1};                                 //Lista que guarda o estado ATUAL de cada botão
int Pot_PVal = 0;
int Pot_CVal = 0;
bool FullMode = 0;
float error;

int lastDebounceTime[6] = {0};                             //Relogio verificador de acomodação de botão (debounce)
const int debounceDelay = 3;                               //Define o tempo de debounce
const float TrashHold = 0.5;                               //Define a variação de ruido ignorada

byte midiCh = 0;                                           //Define Canal Midi Usado
byte cc = 1;

ResponsiveAnalogRead Pad1(Pad_pin[0], true, 0.15);          //Inicializa os objetos de cada Pad para biblioteca Responsive analog read
ResponsiveAnalogRead Pad2(Pad_pin[1], true, 0.15);
ResponsiveAnalogRead Pad3(Pad_pin[2], true, 0.15);
ResponsiveAnalogRead Pad4(Pad_pin[3], true, 0.15);
ResponsiveAnalogRead Pad5(Pad_pin[4], true, 0.15);
ResponsiveAnalogRead Pad6(Pad_pin[5], true, 0.15);
ResponsiveAnalogRead Pad7(Pad_pin[6], true, 0.15);
ResponsiveAnalogRead Pad8(Pad_pin[7], true, 0.15);

ResponsiveAnalogRead Pot(Pot_pin, true, 0.01);

//Função de ativação de nota por MIDI =================================================
void noteOn(byte channel, byte pitch, byte velocity) {   
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

//Função de desativação de nota por MIDI ==============================================
void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

//Função utilizada no filtro exponencial ==============================================
float snapCurve(float x){
  float y = 1.0 / (x + 1.0);
  y = (1.0 - y) * 2.0;
  if(y > 1.0) {
    return 1.0;
  }
  return y;
}

//Função que adequa a variação logaritimica do pad, para linear =======================
int expFilter(int newValue, int lastValue, int resolution, float snapMult){
  unsigned int diff = abs(newValue - lastValue);
  error += ((newValue - lastValue) - error) * 0.4;
  float snap = snapCurve(diff * snapMult);
  float outputValue = lastValue;
  outputValue  += (newValue - lastValue) * snap;
  if(outputValue < 0.0){
    outputValue = 0.0;
  }
  else if(outputValue > resolution - 1){
    outputValue = resolution - 1;
  }
  return (int)outputValue;
}

//Função que identifica qual valor base com os pads soltos ============================
void setZero(void){
  int accumulator = 0;
  int sleep = 37;
  for (int i = 0; i < 10; i++){
    Pad1.update();
    accumulator += Pad1.getValue();
    delay(sleep);
  }
  Pad_Zero[0] = accumulator/10;
  accumulator = 0;

  for (int i = 0; i < 10; i++){
    Pad2.update();
    accumulator += Pad2.getValue();
    delay(sleep);
  }
  Pad_Zero[1] = accumulator/10;
  accumulator = 0;

  for (int i = 0; i < 10; i++){
    Pad3.update();
    accumulator += Pad3.getValue();
    delay(sleep);
  }
  Pad_Zero[2] = accumulator/10;
  accumulator = 0;

  for (int i = 0; i < 10; i++){
    Pad4.update();
    accumulator += Pad4.getValue();
    delay(sleep);
  }
  Pad_Zero[3] = accumulator/10;
  accumulator = 0;

  for (int i = 0; i < 10; i++){
    Pad5.update();
    accumulator += Pad5.getValue();
    delay(sleep);
  }
  Pad_Zero[4] = accumulator/10;
  accumulator = 0;

  for (int i = 0; i < 10; i++){
    Pad6.update();
    accumulator += Pad6.getValue();
    delay(sleep);
  }
  Pad_Zero[5] = accumulator/10;
  accumulator = 0;

  for (int i = 0; i < 10; i++){
    Pad7.update();
    accumulator += Pad7.getValue();
    delay(sleep);
  }
  Pad_Zero[6] = accumulator/10;
  accumulator = 0;

  for (int i = 0; i < 10; i++){
    Pad8.update();
    accumulator += Pad8.getValue();
    delay(37);
  }
  Pad_Zero[7] = accumulator/10;
  accumulator = 0;
}

//Função que le o valor de cada objeto com a biblioteca Res.An.Read ===================
int padRead(int index){
  int RVal = 0;

  if (index == 0){
    Pad1.update();
    RVal = Pad1.getValue();
    Pad_PVal[index] = RVal;
    if ((Pad_Zero[index] - RVal) < int(TrashHold * Pad_Zero[index])) RVal = Pad_Zero[index];
  }

  if (index == 1){
    Pad2.update();
    RVal = Pad2.getValue();
    Pad_PVal[index] = RVal;
    if ((Pad_Zero[index] - RVal) < int(TrashHold * Pad_Zero[index])) RVal = Pad_Zero[index];
  }
  if (index == 2){
    Pad3.update();
    RVal = Pad3.getValue();
    Pad_PVal[index] = RVal;
    if ((Pad_Zero[index] - RVal) < int(TrashHold * Pad_Zero[index])) RVal = Pad_Zero[index];
  }
  if (index == 3){
    Pad4.update();
    RVal = Pad4.getValue();
    Pad_PVal[index] = RVal;
    if ((Pad_Zero[index] - RVal) < int(TrashHold * Pad_Zero[index])) RVal = Pad_Zero[index];
  }
  if (index == 4){
    Pad5.update();
    RVal = Pad5.getValue();
    Pad_PVal[index] = RVal;
    if ((Pad_Zero[index] - RVal) < int(TrashHold * Pad_Zero[index])) RVal = Pad_Zero[index];
  }
  if (index == 5){
    Pad6.update();
    RVal = Pad6.getValue();
    Pad_PVal[index] = RVal;
    if ((Pad_Zero[index] - RVal) < int(TrashHold * Pad_Zero[index])) RVal = Pad_Zero[index];
  }
  if (index == 6){
    Pad7.update();
    RVal = Pad7.getValue();
    Pad_PVal[index] = RVal;
    if ((Pad_Zero[index] - RVal) < int(TrashHold * Pad_Zero[index])) RVal = Pad_Zero[index];
  }
  if (index == 7){
    Pad8.update();
    RVal = Pad8.getValue();
    Pad_PVal[index] = RVal;
    if ((Pad_Zero[index] - RVal) < int(TrashHold * Pad_Zero[index])) RVal = Pad_Zero[index];
  }
  return RVal;
}

//Inicialização... ====================================================================
void setup() {
  Serial.begin(9600);
  pinMode(A6, INPUT);
  
  for (int i = 0; i <6; i++){
    pinMode(Button_pin[i], INPUT_PULLUP);
  }

  Pad1.setActivityThreshold(30.0);
  Pad2.setActivityThreshold(30.0);
  Pad3.setActivityThreshold(30.0);
  Pad4.setActivityThreshold(30.0);
  Pad5.setActivityThreshold(30.0);
  Pad6.setActivityThreshold(30.0);
  Pad7.setActivityThreshold(30.0);
  Pad8.setActivityThreshold(30.0);

  Pot.setActivityThreshold(1.0);

  setZero();
}

//====================================================================================
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}
//=====================================================================================

void loop() {

//Leitura e ação Botoes======================================================
  for (int i = 0; i < 5; i++){

    if((millis() - lastDebounceTime[i]) > debounceDelay){

      buttonCState[i] = digitalRead(Button_pin[i]);
      
      if(buttonPState[i] != buttonCState[i]){
        lastDebounceTime[i] = millis();
        
        if((buttonPState[i] != LOW) && (i == 0)){ 
          setZero();
        }
        if((buttonPState[i] != LOW) && (i == 1)){ 
          FullMode = !FullMode;
        }
        if((buttonPState[i] != LOW) && (i == 2)){ 
          
        }
        if((buttonPState[i] != LOW) && (i == 3)){ 


        }
        if((buttonPState[i] != LOW) && (i == 4)){ 


        }
        if((buttonPState[i] != LOW) && (i == 5)){ 


        }
        buttonPState[i] = buttonCState[i];
      }
    }
    
  }
//=============================================================================

  for (int i = 0; i < 8; i++){
    Pad_CVal[i] = padRead(i);
    Pad_CVelocity[i] = map(Pad_CVal[i], 0, (Pad_Zero[i] - (Pad_Zero[i]*TrashHold)), 127, 0);
    if (Pad_CVelocity[i] <= 5) Pad_CVelocity[i] = 0;
  }

  Pot.update();
  Pot_CVal = map(Pot.getValue(), 0, 1023, 0, 127);
  if (Pot_CVal != Pot_PVal){
    controlChange(midiCh, 0, Pot_CVal);
    Pot_PVal = Pot_CVal;
  }

  for (int i = 0; i <8; i++){

    if ((Pad_CVelocity[i] != Pad_PVelocity[i]) && (Pad_PVelocity[i] <= 5) && (Pad_CVelocity[i] > 5)){
      if (FullMode) noteOn(midiCh, Pad_Note[i], 127);
      else noteOn(midiCh, Pad_Note[i], Pad_CVelocity[i]);
      MidiUSB.flush();
    }

    if ((Pad_CVelocity[i] != Pad_PVelocity[i]) && ((Pad_CVelocity[i] <= 5) || (Pad_CVelocity[i] == 0))){
      noteOn(midiCh, Pad_Note[i], 0);
      MidiUSB.flush();
    }

    Pad_PVelocity[i] = Pad_CVelocity[i];

  }

  for (int i = 0; i <8; i++){
    if (i == 7){
      Serial.println(String(Pad_CVelocity[i])+",");
    }else{
      Serial.print(String(Pad_CVelocity[i])+",");
    }
  }

}