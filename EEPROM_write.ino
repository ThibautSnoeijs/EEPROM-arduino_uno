#define D0       2 //D0 of ROM
#define CLK     10 //SRCLK of shift register
#define SER     11 //SER of shift register
#define _CE     13 //_CE of RAM

void SetAdress(int Adress, bool _RE, bool _WR){
  digitalWrite(_CE, HIGH);
  digitalWrite(CLK, LOW);
  shiftOut(SER, CLK, MSBFIRST, (Adress >> 6) );
  shiftOut(SER, CLK, MSBFIRST, (Adress << 2) | (_RE << 1) | _WR );
  digitalWrite(CLK, HIGH);
  delay(4);
  digitalWrite(_CE, LOW);
  delay(2);
  digitalWrite(_CE, HIGH);
  }

int ReadByte(){
  int Data=0;
  for(int i=0; i<8; i++){
    pinMode(D0+i, INPUT);
    Data |= (digitalRead(D0+i)<<i);
    }
  return Data;
  }

void SetByte(int Data){
  for(int i=0; i<8; i++){
    pinMode(D0+i, OUTPUT);
    digitalWrite(D0+i, Data&(1<<i));
    }
  }

int ReadAdress(int Adress){
  SetAdress(Adress, LOW, HIGH);
  return ReadByte();
  }

void WriteAdress(int Adress, int8_t Data){
  SetByte(Data);
  SetAdress(Adress, HIGH, LOW);
  ReadAdress(Adress);//this somehow fixes a bug when writing lots o' data that causes the index to be wrong, idk why
  }


//Funtions interfacing with the serial interface

void PrintAdress(int Adress){
  int b = ReadAdress(Adress);
  if(b<0x10)Serial.print(0);
  Serial.print(b, HEX);
  }

const int blocksize= 64;//The page size, and also the max width that can fit on my screen
void PrintRange(int StartAdress, int blocks){
  int Adress= StartAdress;
  for(int block=0; block<blocks; block++){
    if(Adress<0x1000)Serial.print(0);
    if(Adress<0x100)Serial.print(0);
    if(Adress<0x10)Serial.print(0);
    Serial.print(Adress, HEX);
    Serial.write(":");
    for(int i=0; i<blocksize; i++){
      Serial.write(" ");
      PrintAdress(Adress);
      Adress++;
      }
    Serial.write("\n");
    }
  }

void help(){
  Serial.write("\
Commands:\n\
h:\tPrint this menu\n\
p:\tPrint entire contents of ROM\n\
w:\tin testing, does something but too lazy to document it atm\n\
");
  }

int8_t ASCIItoNibble(int8_t in){
  if(in<0x40)return in-0x30;
  if(in>0x60)in-=0x20;
  return in-0x31;
  }

int8_t ASCIItoByte(int16_t in){
  return (ASCIItoNibble(in>>8)<<4)+ASCIItoNibble(in);
  }

void setup(){
Serial.begin(230400);// Any higher didn't work for me
pinMode(CLK, OUTPUT);
pinMode(SER, OUTPUT);
digitalWrite(_CE, HIGH);
pinMode(_CE, OUTPUT);
delay(200);//had weirdness without this delay
Serial.write("\n\n/*  NEW SERIAL TRANSMISSION STARTED  */\n\n");
help();
Serial.println();
}


void loop(){
  if (Serial.available() > 0) {
    int incomingByte = Serial.read();
    Serial.println(incomingByte, HEX);
    switch(incomingByte){
      case 0x68://h
        help();
        break;
      case 0x70://p
        PrintRange(0, 128);
        break;
      case 0x77://w
        incomingByte = Serial.read();
        for(;(incomingByte<0x30);){//remove beginning whitespace lazily
          incomingByte = Serial.read();
          }
        Serial.println(ASCIItoNibble(incomingByte),HEX);
        break;
      case 0x0A:
        Serial.println("End of command");
        break;
      default:
        Serial.print("unknown command ");
        for(;(incomingByte!=0x0A);){
          Serial.write(incomingByte);
          incomingByte = Serial.read();
          }
        Serial.println();
        help();
    }
    Serial.println();
  }
}
