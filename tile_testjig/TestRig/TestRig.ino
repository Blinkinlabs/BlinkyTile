// Command interface: For remote control of the test rig
#define COMMAND_PROGRAM_ADDRESSES     'p'    // Run the program address routine
#define COMMAND_OUTPUT_COLOR          's'    // Set all outputs to a specific color (0=red, 1=green, 2=blue)
#define COMMAND_OUTPUT_IDENTIFY       'i'    // Identify one output 

#define SWITCH_PIN    9


#define MAX_OUTPUTS  50     // Number of address outputs available
#define OUTPUT_COUNT 16     // Number of address outputs enabled

//// Programmer pins
//const int csPin      = 10;
//const int doutPin    = 11;
//const int dinPin     = 12;
//const int sckPin     = 13;


// Data output pin
const int dataPin    = 0;
const int powerPin   = 2;
const int failLedPin = 22;
const int passLedPin = 23;

static volatile uint8_t *dmxPort;
static uint8_t dmxBit = 0;


#define BYTES_PER_PIXEL    3
uint8_t dataArray[1 + MAX_OUTPUTS*BYTES_PER_PIXEL];    // Storage for DMX output (0 is the start frame)

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
// Note that this table needs to be initialized by a call to 
// makeAddressTable() before using.
#define PROGRAM_ADDRESS_FRAMES 1+3
int addressProgrammingData[MAX_OUTPUTS][PROGRAM_ADDRESS_FRAMES];


void writePixel(int pixel, int r, int g, int b) {
  dataArray[0] = 0;    // Start bit!

  dataArray[(pixel-1)*BYTES_PER_PIXEL + 1] = b;
  dataArray[(pixel-1)*BYTES_PER_PIXEL + 2] = g;
  dataArray[(pixel-1)*BYTES_PER_PIXEL + 3] = r;
}

void setAddresses(int level, int offset, int count) {
  for(int i = 0; i < count; i++) {
    int address = i + offset;
    digitalWriteFast(addressPins[address], level);
  }
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


// Make a table of the values we have to send to program the addressess
void makeAddressTable() {
  for(int address = 0; address < MAX_OUTPUTS; address++) {
    // WS2822S (from datasheet)
    int channel = (addresses[address]-1)*3+1;
    addressProgrammingData[address][1] = 0;
    addressProgrammingData[address][1] = flipEndianness(channel%256);
    addressProgrammingData[address][2] = flipEndianness(240 - (channel/256)*15);
    addressProgrammingData[address][3] = flipEndianness(0xD2);
  }
}


#define BIT_LENGTH                 (4)
#define BREAK_LENGTH              (95)    //(88)
#define MAB_LENGTH                (12)    //(8)

#define PREFIX_BREAK_TIME        (BREAK_LENGTH)
#define PREFIX_MAB_TIME          (PREFIX_BREAK_TIME + MAB_LENGTH)

#define FRAME_START_BIT_TIME     (BIT_LENGTH)
#define FRAME_STOP_BITS_TIME     (FRAME_START_BIT_TIME + 8*BIT_LENGTH + 2*BIT_LENGTH)


#define GROUPS        4    // These should equal the MAX_OUTPUTS
#define GROUP_SIZE    4


void dmxSendByte(uint8_t value)
{
	uint32_t begin, target;
	uint8_t mask;

	noInterrupts();
	ARM_DEMCR |= ARM_DEMCR_TRCENA;
	ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
	begin = ARM_DWT_CYCCNT;
	*dmxPort = 0;
	target = F_CPU / 250000;
	while (ARM_DWT_CYCCNT - begin < target) ; // wait, start bit
	for (mask=1; mask; mask <<= 1) {
		*dmxPort = (value & mask) ? 1 : 0;
		target += (F_CPU / 250000);
		while (ARM_DWT_CYCCNT - begin < target) ; // wait, data bits
	}
	*dmxPort = 1;
	target += (F_CPU / 125000);
	while (ARM_DWT_CYCCNT - begin < target) ; // wait, 2 stops bits
	interrupts();
}



// Send programming data
void programAddresses() {
  
  // Pull data pin low
  digitalWriteFast(dataPin, LOW);
  
  // Set address pins to outputs
  for(int i = 0; i < MAX_OUTPUTS; i++) {
    pinMode(addressPins[i], OUTPUT);
    digitalWriteFast(addressPins[i], LOW);
  }
  
  for(int group = 0; group < GROUPS; group++) {
  
    delay(500);
    //delayMicroseconds(BREAK_LENGTH);
  
    noInterrupts();
    elapsedMicros startTime;            // Microsecond timing starts here
    
    setAddresses(HIGH, group*GROUP_SIZE, GROUP_SIZE);                 // MAB
    while(startTime < MAB_LENGTH) {};
    
    // Program the address data in parallel
    for(int frame = 0; frame < PROGRAM_ADDRESS_FRAMES; frame++) {
      elapsedMicros addressFrameStartTime;
      
      setAddresses(LOW, group*GROUP_SIZE, GROUP_SIZE);    // Start bit
      while(addressFrameStartTime < FRAME_START_BIT_TIME) {};
    
      for(int bit = 0; bit < 8; bit++) {  // data bits
        for(int address = 0; address < GROUP_SIZE; address++) {
          int groupAddress = address + group*GROUP_SIZE;
          
          digitalWriteFast(addressPins[groupAddress],
                      (addressProgrammingData[groupAddress][frame] >> bit) & 0x01);
        }
        while(addressFrameStartTime < (FRAME_START_BIT_TIME+(bit+1)*BIT_LENGTH)) {};
      }
      
      setAddresses(HIGH, group*GROUP_SIZE, GROUP_SIZE);    // Stop bit
      while(addressFrameStartTime < (FRAME_STOP_BITS_TIME)) {};
    }
    
    
    setAddresses(HIGH, group*GROUP_SIZE, GROUP_SIZE);    // MTBP
    delayMicroseconds(88);
    
    interrupts();
  }
  
  // Set the address pins to inputs again
  for(int i = 0; i < OUTPUT_COUNT; i++) {
    pinMode(addressPins[i], INPUT);
  }
  
  // And set the data pin low
  digitalWriteFast(dataPin, HIGH);
}




// Send a DMX frame with new data
void draw() {
  noInterrupts();
  
  elapsedMicros prefixStartTime;
  
  digitalWriteFast(dataPin, LOW);    // Break - 88us
  while(prefixStartTime < PREFIX_BREAK_TIME) {};

  digitalWriteFast(dataPin, HIGH);   // MAB - 8 us
  while(prefixStartTime < PREFIX_MAB_TIME) {};
  
  // For each address
  for(int frame = 0; frame < 1 + OUTPUT_COUNT*BYTES_PER_PIXEL; frame++) {    
//    noInterrupts();
//    
//    elapsedMicros frameStartTime;
//    
//    digitalWriteFast(dataPin, LOW);    // Start bit
//    while(frameStartTime < FRAME_START_BIT_TIME) {};
//  
//    for(int bit = 0; bit < 8; bit++) {  // data bits
//      digitalWriteFast(dataPin, (dataArray[frame] >> bit) & 0x01);
//      while(frameStartTime < (FRAME_START_BIT_TIME+(bit+1)*BIT_LENGTH)) {};
//    }
//    
//    digitalWriteFast(dataPin, HIGH);    // Stop bit
//    while(frameStartTime < (FRAME_STOP_BITS_TIME)) {};
//    
//    interrupts();

    dmxSendByte(dataArray[frame]);
  }

//  digitalWriteFast(dataPin, LOW);    // Stop bit
//  delayMicroseconds(20);
//  digitalWriteFast(dataPin, HIGH);    // Stop bit  
  // We're done - MTBP is high, same as the stop bit.
  
  interrupts();
}


void setup() {
  // Set the data pin high
  pinMode(dataPin, OUTPUT);
  digitalWriteFast(dataPin, HIGH);
  
  pinMode(powerPin, OUTPUT);
  digitalWrite(powerPin, LOW);
  
  pinMode(failLedPin, OUTPUT);
  digitalWrite(failLedPin, LOW);
  
  pinMode(passLedPin, OUTPUT);
  digitalWrite(passLedPin, LOW);
  
  // Set up port pointers for interrupt routine
  dmxPort = portOutputRegister(digitalPinToPort(dataPin));
  dmxBit = digitalPinToBitMask(dataPin);
  
  // And make the address pins inputs for the moment
  for(int i = 0; i < OUTPUT_COUNT; i++) {
    pinMode(addressPins[i], INPUT);
  }
  
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  
  // Fill the address table with corret data
  makeAddressTable();
  
  // Finally, start listening on the serial port
  Serial.begin(115200);
  
  digitalWrite(failLedPin, HIGH);
  digitalWrite(passLedPin, HIGH);
  delay(1000);
  digitalWrite(failLedPin, LOW);
}


void handleCommand(char* commandBuffer) {
  char command = commandBuffer[0];
  char p = commandBuffer[1];

  if(command == COMMAND_OUTPUT_IDENTIFY) {
    int parameter = p - '0';
    
    Serial.print("Identify pixel: ");
    Serial.println(parameter);
     
    for(int output = 0; output < OUTPUT_COUNT; output++) {
      if(output + 1 == parameter) {
        writePixel(output + 1, 255, 255, 255);
      }
      else {
        writePixel(output + 1, 1, 1, 1);
      }
    }
    
    draw();
  }
  
  if(command == COMMAND_OUTPUT_COLOR) {
    int parameter = p - '0';
    
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
      else {
        writePixel(output + 1, 255, 255, 255);
      }
    }
    
    draw();
    delay(10);
    draw();  // TODO: Fix this bug :-P
  }
  if(command == COMMAND_PROGRAM_ADDRESSES) {
    digitalWrite(failLedPin, HIGH);
    digitalWrite(passLedPin, LOW);
    Serial.print("Programming addresses");
    Serial.println("");
    digitalWrite(powerPin, HIGH);
    delay(100);
    digitalWrite(powerPin, LOW);
    
    programAddresses();
    programAddresses(); // TODO: Fix this bug :-P
    
    //delay(5000);

    digitalWrite(powerPin, HIGH);
    delay(500);
    digitalWrite(powerPin, LOW);
    Serial.print("Programming END");
    Serial.println("");
    digitalWrite(failLedPin, LOW);
    digitalWrite(passLedPin, HIGH);
  }
}


#define COMMAND_LENGTH  2
char inputBuffer[COMMAND_LENGTH];
int bufferPosition = 0;

void loop() {
  if(Serial.available() > 0) {
      char c = Serial.read();
      
      if(c == '\n') {
        if(bufferPosition == COMMAND_LENGTH) {
           handleCommand(inputBuffer);
        }
        else {
          Serial.print("Malformed command, length:");
          Serial.println(bufferPosition);
        }
 
        bufferPosition = 0;       
      }
      else {
        if(bufferPosition < COMMAND_LENGTH) {
          inputBuffer[bufferPosition] = c;
          bufferPosition++;
        }
        else {
          Serial.println("Too many characters!");
        }
      }
  }
  
  if(digitalRead(SWITCH_PIN) == LOW) {
    delay(50);
    handleCommand("p");
    while(digitalRead(SWITCH_PIN) == LOW) {}
  }

    static int counts = 0;
    static int countsPerChange = 300000;
    static int color = 0;
    
    counts++;
    if(counts > countsPerChange) {
      counts = 0;
      
      for(int output = 0; output < OUTPUT_COUNT; output++) {
        if(color == 0) {
          writePixel(output + 1, 255, 0, 0);
        }
        else if(color == 1) {
          writePixel(output + 1, 0, 255, 0);
        }
        else if(color == 2) {
          writePixel(output + 1, 0, 0, 255);
        }
        else if(color == 3) {
          writePixel(output + 1, 255, 255, 255);
        }
      }
      
      draw();
      color = (color + 1) % 4;
    }
}

