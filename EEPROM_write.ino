#define D0       2 //D0 of ROM
#define CLK     10 //SRCLK of shift register
#define SER     11 //SER of shift register
#define _CE     12 //_CE of RAM
#define FREEPIN 13 //Otherwise unused pin, can be used for other functions like generating the clock for the CPU
#define CPUCLK  FREEPIN //CLK of breadboard computer
#define CPUCLKSTATE CPUCLK_count & 1


int CPUCLK_count = 0;

void IncCPUCLK(){
  digitalWrite(CPUCLK, CPUCLKSTATE);
  CPUCLK_count++;
  };

void SetAddress(uint16_t Address, bool _OE, bool _WE) {
  digitalWrite(_CE, HIGH);
  digitalWrite(CLK, LOW);
  shiftOut(SER, CLK, MSBFIRST, (Address >> 6) );
  shiftOut(SER, CLK, MSBFIRST, (Address << 2) | (_OE << 1) | _WE );
  digitalWrite(CLK, HIGH);
  delay(4);
  digitalWrite(_CE, LOW);
  delay(2);
  digitalWrite(_CE, HIGH);
}

int ReadByte() {
  int Data = 0;
  for (int i = 0; i < 8; i++) {
    pinMode(D0 + i, INPUT);
    Data |= (digitalRead(D0 + i) << i);
  }
  return Data;
}

void SetByte(int Data) {
  for (int i = 0; i < 8; i++) {
    pinMode(D0 + i, OUTPUT);
    digitalWrite(D0 + i, Data & (1 << i));
  }
}

int ReadAddress(uint16_t Address) {
  SetAddress(Address, LOW, HIGH);
  return ReadByte();
}

void WriteAddress(uint16_t Address, int8_t Data) {
  SetByte(Data);
  SetAddress(Address, HIGH, LOW);
  ReadAddress(Address);//this somehow fixes a bug when writing lots o' data that causes the index to be wrong, idk why
}


//Funtions interfacing with the serial interface

void PrintAddress(unsigned int Address) {
  int b = ReadAddress(Address);
  if (b < 0x10)Serial.print(0);
  Serial.print(b, HEX);
}

void PrintRange(uint16_t StartAddress, uint16_t blocks) {
  uint16_t Address = StartAddress;
  for (uint16_t block = 0; block < blocks; block++) {
    if (Address < 0x1000)Serial.print(0);
    if (Address < 0x100)Serial.print(0);
    if (Address < 0x10)Serial.print(0);
    Serial.print(Address, HEX);
    Serial.write(":");
    for (uint16_t i = 0; i < 64; i++) {
      Serial.write(" ");
      PrintAddress(Address);
      Address++;
    }
    Serial.write("\n");
  }
}

void WriteArray(uint8_t* Data, size_t size) {
  SetByte(Data[0]);
  SetAddress(0, HIGH, LOW);
  for (uint16_t i = 0; i < size; i++) {
    SetByte(Data[i]);
    delay(1);
    SetAddress(i, HIGH, LOW);
    delay(3);
  }
  ReadAddress(size);
}

void help() {
Serial.write("\
Commands:\n\
h:\tPrint this menu\n\
p:\tPrint contents of ROM\n\
w:\tWrites code to ROM\n\
e:\tFills ROM with zeros\n\
c:\tCPU clock go brrr\n\
s:\tSingle-step CPU clock\n\
t:\tTest thing\n\
");
}

uint8_t ASCIItoNibble(uint8_t in) {
  if (in < 0x40)return in - 0x30;
  if (in > 0x60)in -= 0x20;
  return in - 0x31;
}

uint8_t ASCIItoByte(uint16_t in) {
  return (ASCIItoNibble(in >> 8) << 4) + ASCIItoNibble(in);
}

void setup() {
  Serial.begin(230400);// Any higher didn't work for me
  digitalWrite(_CE, HIGH);
  pinMode(_CE, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(SER, OUTPUT);
  digitalWrite(CPUCLK, HIGH);
  pinMode(CPUCLK, OUTPUT);
  delay(200);//had weirdness without this delay
  Serial.write("\n\n/*  NEW SERIAL TRANSMISSION STARTED  */\n\n");
  help();
  Serial.println();
  PrintRange(0x0000, 4);
  Serial.println();
}

void loop() {
  if (Serial.available() > 0) {
    int incomingByte = Serial.read();
    switch (incomingByte) {
      
      default:
        Serial.println(incomingByte, HEX);
        Serial.print("unknown command ");
        for (; (incomingByte != 0x0A);) {
          Serial.write(incomingByte);
          incomingByte = Serial.read();
        }
        Serial.println();
        help();
        break;
        
      case 0x0A://\n
        //Serial.println("End of command");
        break;
        
      case 0x61://a
        SetAddress(0xFFFF, LOW, LOW);
        break;
        
      case 0x63://c
        delay(10);
        for(;Serial.available() != 0;)Serial.println(Serial.read(), HEX);
        for(;Serial.available() == 0;){
          IncCPUCLK();
          Serial.write("Current CPU cycle: ");
          Serial.println(CPUCLK_count, HEX);
          delay(1);
          }
        break;
        
      case 0x65://e
        SetByte(0);
        SetAddress(0, HIGH, LOW);
        for (uint16_t i = 0; i < 0x2000; i++) {
          SetAddress(i, HIGH, LOW);
          delay(3);
          Serial.println(i, HEX);
        }
        ReadAddress(0);
        WriteAddress(0x1FFD, 0xC3);
        PrintRange(0, 128);
        break;
        
      case 0x66://f
        WriteAddress(0xFF, 0);
        break;
        
      case 0x68://h
        help();
        break;
        
      case 0x70://p
        PrintRange(0, 128); //128 is max
        break;
        
      case 0x73://s
        IncCPUCLK();
        Serial.write("Current CPU cycle: ");
        Serial.println(CPUCLK_count, HEX);
        delay(500);
        break;
        
      case 0x74://t
        for(uint16_t i=0; i < 0x2000;){
          Serial.println(i, HEX);
          WriteAddress(i, i>>8);
          i++;
          for(int _=0; _<0xFF; _++){
            WriteAddress(i, i);
            i++;
            }
          }
        PrintRange(0, 0x80);
        break;
        
      case 0x77://w
        uint8_t d[] =/* test */{0x00, 0xC3, 0x00, 0x00, 0xde, 0xad, 0xbe, 0xef};
        size_t size= sizeof(d);
        Serial.println(size, HEX);
        WriteArray(d, size);
        PrintRange(0x0000, 8);
        break;
    }
  }
}
