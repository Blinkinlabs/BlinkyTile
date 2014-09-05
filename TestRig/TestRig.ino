
#define COMMAND_SEND_DATA          's'    // Send the address data
#define COMMAND_PROGRAM_ADDRESSES  'p'    // Send the address data

#define MAX_OUTPUTS 16
#define OUTPUT_COUNT 14

//// Programmer pins
//const int csPin      = 10;
//const int doutPin    = 11;
//const int dinPin     = 12;
//const int sckPin     = 13;


// Data output pin
const int dataPin    = 0;

#define BYTES_PER_FRAME    3
int dataArray[OUTPUT_COUNT*BYTES_PER_FRAME];

// Address pin connections
const int addressPins[MAX_OUTPUTS] = {
  21,      // 1
  20,      // 2
  19,      // 3
  18,      // 4
  17,      // 5
  16,      // 6
  15,      // 7
  14,      // 8
  8,       // 9
  7,       // 10
  6,       // 11
  5,       // 12
  4,       // 13
  3,       // 14
  2,       // 15
  1,       // 16
};

// Fudge factor for microseconds timing
const int MICROSECONDS_FUDGE = 0;

// Output addresses to program
const int addresses[MAX_OUTPUTS] = {
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10,
  11,
  12,
  13,
  14,
  15,
  16
};

// Data table of bytes we need to send to program the addresses
#define ADDRESS_PROGRAM_FRAMES 3
int addressProgrammingData[MAX_OUTPUTS][ADDRESS_PROGRAM_FRAMES];


void writePixel(int pixel, int r, int g, int b) {
  dataArray[(pixel-1)*BYTES_PER_FRAME + 0] = b;
  dataArray[(pixel-1)*BYTES_PER_FRAME + 1] = g;
  dataArray[(pixel-1)*BYTES_PER_FRAME + 2] = r;
}


void setAddresses(int level) {
  for(int address = 0; address < OUTPUT_COUNT; address++) {
    digitalWriteFast(addressPins[address], level);
  }
}

uint8_t flipEndianness(uint8_t input) {
  uint8_t output = 0;
  for(uint8_t bit = 0; bit < 8; bit++) {
    if(input & (1 << bit)) {
      output |= (1 << (7 - bit));
    }
  }
  
  return output;
}


// Make a 2d table of the values we have to send to program the addressess
void makeAddressTable() {
  for(int address = 0; address < MAX_OUTPUTS; address++) {
    // WS2822S (from datasheet)
    int channel = (addresses[address]-1)*3+1;
    addressProgrammingData[address][0] = flipEndianness(channel%256);
    addressProgrammingData[address][1] = flipEndianness(240 - (channel/256)*15);
    addressProgrammingData[address][2] = flipEndianness(0xD2);
  }
}


// Send programming data
// TODO: Make this interrupt driven?
void programAddresses() {
  
  makeAddressTable();
  
  // Pull data pin low
  digitalWriteFast(dataPin, LOW);
  
  // Set address pins to outputs
  for(int i = 0; i < MAX_OUTPUTS; i++) {
    pinMode(addressPins[i], OUTPUT);
  }
  
  setAddresses(LOW);    // Programming Break
  delay(1000);
//  delayMicroseconds(88 - MICROSECONDS_FUDGE);

  elapsedMicros startTime;
  
  setAddresses(HIGH);   // MAB
  while(startTime < 8) {};
//  delayMicroseconds(8 - MICROSECONDS_FUDGE);
  
  setAddresses(LOW);    // Start bit
  while(startTime < (8+4)) {};
//  delayMicroseconds(4 - MICROSECONDS_FUDGE);
  
  setAddresses(LOW);    // Start bit
  while(startTime < (8+4+4)) {};
//  delayMicroseconds(4 - MICROSECONDS_FUDGE);

  for(int bit = 0; bit < 8; bit++) {  // SC (channel 0)
    setAddresses(LOW);
    while(startTime < (8+4+4 + bit*4)) {};
//    delayMicroseconds(4 - MICROSECONDS_FUDGE);
  }
  
  setAddresses(HIGH);    // Stop bit
  while(startTime < (8+4+4+32+8)) {};
//  delayMicroseconds(8 - MICROSECONDS_FUDGE);
  
  // For each address
  for(int frame = 0; frame < ADDRESS_PROGRAM_FRAMES; frame++) {
    setAddresses(LOW);    // Start bit
    while(startTime < (8+4+4+32+8+frame*(4+8*4+8)+4)) {};
//    delayMicroseconds(4 - MICROSECONDS_FUDGE);
  
    for(int bit = 0; bit < 8; bit++) {  // data bits
      for(int address = 0; address < OUTPUT_COUNT; address++) {
        digitalWriteFast(addressPins[address],
                    (addressProgrammingData[address][frame] >> bit) & 0x01);
      }
      while(startTime < (8+4+4+32+8+frame*(4+8*4+8)+4+bit*4+4)) {};
//      delayMicroseconds(4 - MICROSECONDS_FUDGE);
    }
    
    setAddresses(HIGH);    // Stop bit
      while(startTime < (8+4+4+32+8+frame*(4+8*4+8)+4+8*4+8)) {};
//    delayMicroseconds(8 - MICROSECONDS_FUDGE);
  }
  
  // TODO: Something here?
  
  setAddresses(HIGH);    // MTBP
  delayMicroseconds(88 - MICROSECONDS_FUDGE);
  
  
  // Set the address pins to inputs again
  for(int i = 0; i < OUTPUT_COUNT; i++) {
    pinMode(addressPins[i], INPUT);
  }
  
  // And set the data pin high again
  digitalWriteFast(dataPin, LOW);
}



// Send a DMX frame with new data
// TODO: Make this interrupt driven?

#define PREFIX_BREAK_TIME        (88)
#define PREFIX_MAB_TIME          (PREFIX_BREAK_TIME + 8)
#define PREFIX_SC_START_BIT_TIME (PREFIX_MAB_TIME + 4)
#define PREFIX_SC_STOP_BIT_TIME  (PREFIX_SC_START_BIT_TIME + 8*4 + 8)

#define FRAME_START_BIT_TIME  (4)
#define FRAME_STOP_BIT_TIME   (FRAME_START_BIT_TIME + 8*4 + 8)

void draw() {
  elapsedMicros prefixStartTime;
  
  digitalWriteFast(dataPin, LOW);    // Break - 88us
  while(prefixStartTime < PREFIX_BREAK_TIME) {};

  digitalWriteFast(dataPin, HIGH);   // MAB - 8 us
  while(prefixStartTime < PREFIX_MAB_TIME) {};
  
  digitalWriteFast(dataPin, LOW);    // Start bit
  while(prefixStartTime < PREFIX_SC_START_BIT_TIME) {};

  for(int bit = 0; bit < 8; bit++) {  // SC (channel 0)
    digitalWriteFast(dataPin, LOW);
    while(prefixStartTime < (PREFIX_SC_START_BIT_TIME+(bit+1)*4)) {};
  }
  
  digitalWriteFast(dataPin, HIGH);    // Stop bit
  while(prefixStartTime < (PREFIX_SC_STOP_BIT_TIME)) {};
  
  // For each address
  for(int frame = 0; frame <OUTPUT_COUNT*3; frame++) {
    elapsedMicros frameStartTime;
    
    digitalWriteFast(dataPin, LOW);    // Start bit
    while(frameStartTime < FRAME_START_BIT_TIME) {};
  
    for(int bit = 0; bit < 8; bit++) {  // data bits
      digitalWriteFast(dataPin, (dataArray[frame] >> bit) & 0x01);
      while(frameStartTime < (FRAME_START_BIT_TIME+(bit+1)*4)) {};
    }
    
    digitalWriteFast(dataPin, HIGH);    // Stop bit
    while(frameStartTime < (FRAME_STOP_BIT_TIME)) {};
  }
  
  // TODO: Another low drop here?
}


void setup() {
  // Set the data pin high
  pinMode(dataPin, OUTPUT);
  digitalWriteFast(dataPin, HIGH);
  
  // And make the address pins inputs for the moment
  for(int i = 0; i < OUTPUT_COUNT; i++) {
    pinMode(addressPins[i], INPUT);
  }
  
  Serial.begin(115200);
}



void loop() {
  if(Serial.available() > 0) {
    char command = Serial.read();
    int parameter = Serial.parseInt();
    
    if(command == COMMAND_SEND_DATA) {
      Serial.print("Sending DMX frame, color: ");
      Serial.println(parameter);
      
      for(int output = 0; output < OUTPUT_COUNT; output++) {
        if(parameter == 0) {
          writePixel(output + 1, 255, 0, 0);
        }
        else if(parameter == 1) {
          writePixel(output + 1, 0, 255, 0);
        }
        else if(parameter == 2) {
          writePixel(output + 1, 0, 0, 255);
        }
      }
      
      draw();
    }
    if(command == COMMAND_PROGRAM_ADDRESSES) {
      Serial.print("Programming addresses");
      Serial.println("");
      
      programAddresses();
    }
  }
}
