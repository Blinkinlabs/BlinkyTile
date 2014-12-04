// Command interface: For remote control of the test rig
#define COMMAND_PROGRAM_ADDRESSES     'p'    // Run the program address routine
#define COMMAND_OUTPUT_COLOR          's'    // Set all outputs to a specific color (0=red, 1=green, 2=blue)
#define COMMAND_OUTPUT_IDENTIFY       'i'    // Identify one output 
#define COMMAND_ERASE_ADDRESSES       'e'    // Erase address routine
#define COMMAND_TEST_COLOR            't'    // Test color routine 

#define SWITCH_PIN    9


#define MAX_OUTPUTS  14     // Number of address outputs available
#define OUTPUT_COUNT 14     // Number of address outputs enabled


#define BIT_LENGTH                 (4)
#define BREAK_LENGTH              (95)    //(88)
#define MAB_LENGTH                (12)    //(8)

#define PREFIX_BREAK_TIME        (BREAK_LENGTH)
#define PREFIX_MAB_TIME          (PREFIX_BREAK_TIME + MAB_LENGTH)

#define FRAME_START_BIT_TIME     (BIT_LENGTH)
#define FRAME_STOP_BITS_TIME     (FRAME_START_BIT_TIME + 8*BIT_LENGTH + 2*BIT_LENGTH)


#define GROUPS        2    // These should equal the MAX_OUTPUTS
#define GROUP_SIZE    7

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

#define COMMAND_LENGTH  3
char inputBuffer[COMMAND_LENGTH];
int bufferPosition = 0;

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
  3       // 14
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
  14
};

// Data table of bytes we need to send to program the addresses
// Note that this table needs to be initialized by a call to 
// makeAddressTable() before using.
#define PROGRAM_ADDRESS_FRAMES 4
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

//=============================================================================================================
//    make Address Table
//=============================================================================================================

// Make a table of the values we have to send to program the addressess
void makeAddressTable() {
  for(int address = 0; address < MAX_OUTPUTS; address++) {
    // WS2822S (from datasheet)
    int channel = (addresses[address]-1)*3+1;
    addressProgrammingData[address][0] = 0;
    addressProgrammingData[address][1] = flipEndianness(channel%256);
    addressProgrammingData[address][2] = flipEndianness(240 - (channel/256)*15);
    addressProgrammingData[address][3] = flipEndianness(0xD2);
  }
}

//=============================================================================================================
//    reset Power
//=============================================================================================================

void resetPower(void)
{
    digitalWrite(powerPin, HIGH);
    delay(250);
    digitalWrite(powerPin, LOW);
    delay(100);
}

//=============================================================================================================
//    dmxSendByte
//=============================================================================================================

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


//=============================================================================================================
//    erase Addresses
//=============================================================================================================
void eraseAddresses(){
  Serial.println("Start Program addresses");
  resetPower();
  
  // Pull data pin low
  digitalWriteFast(dataPin, LOW);
  
  
  // Set address pins to outputs
  for(int i = 0; i < MAX_OUTPUTS; i++) {
    pinMode(addressPins[i], OUTPUT);
    digitalWrite(addressPins[i], HIGH);
  }
    
  for(int group = 0; group < GROUPS; group++) {
    
    delay(50);
    Serial.print("Group = ");
    Serial.println(group, DEC);
    setAddresses(LOW, group*GROUP_SIZE, GROUP_SIZE);                 // BREAK
    delay(500);
  
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
                      (addressProgrammingData[0][frame] >> bit) & 0x01);
        }
        while(addressFrameStartTime < (FRAME_START_BIT_TIME+(bit+1)*BIT_LENGTH)) {};
      }
      
      setAddresses(HIGH, group*GROUP_SIZE, GROUP_SIZE);    // Stop bit
      while(addressFrameStartTime < (FRAME_STOP_BITS_TIME)) {};
    }
    
    setAddresses(LOW, group*GROUP_SIZE, GROUP_SIZE);  // BREAK
    delay(50);
    
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
  
  resetPower();
  Serial.println("Programming END");
}

//=============================================================================================================
//    program Addresses
//=============================================================================================================

// Send programming data
void programAddresses() { 
  Serial.println("Start Program addresses");
  resetPower();
  
  // Pull data pin low
  digitalWriteFast(dataPin, LOW);
  
  
  // Set address pins to outputs
  for(int i = 0; i < MAX_OUTPUTS; i++) {
    pinMode(addressPins[i], OUTPUT);
    digitalWrite(addressPins[i], HIGH);
  }
    
  for(int group = 0; group < GROUPS; group++) {
    
    delay(50);
    Serial.print("Group = ");
    Serial.println(group, DEC);
    setAddresses(LOW, group*GROUP_SIZE, GROUP_SIZE);                 // BREAK
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
    
    setAddresses(LOW, group*GROUP_SIZE, GROUP_SIZE);  // BREAK
    delay(50);
    
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
  
  resetPower();
  Serial.println("Programming END");
  
}


//=============================================================================================================
//    Draw
//=============================================================================================================

// Send a DMX frame with new data
void draw() {
  resetPower();
  noInterrupts();
  
  elapsedMicros prefixStartTime;
  
  digitalWriteFast(dataPin, LOW);    // Break - 88us
  while(prefixStartTime < PREFIX_BREAK_TIME) {};

  digitalWriteFast(dataPin, HIGH);   // MAB - 8 us
  while(prefixStartTime < PREFIX_MAB_TIME) {};
  
  // For each address
  for(int frame = 0; frame < 1 + OUTPUT_COUNT*BYTES_PER_PIXEL; frame++) {    
    dmxSendByte(dataArray[frame]);
  }
  
  digitalWriteFast(dataPin, LOW);    // Break - 88us
  delayMicroseconds(88);

  digitalWriteFast(dataPin, HIGH);   // MAB - 8 us
  interrupts();
}

//=============================================================================================================
//    Test color
//=============================================================================================================
void test_color(void){
    resetPower();
    for(int color = 0; color<4; color++){ 
        Serial.print("Color =  ");
        Serial.println(color, DEC);
        for(int output = 0; output < OUTPUT_COUNT; output++) {
          if(color == 0) {
            writePixel(output + 1, 50, 0, 0);
          }
          else if(color == 1) {
            writePixel(output + 1, 0, 50, 0);
          }
          else if(color == 2) {
            writePixel(output + 1, 0, 0, 50);
          }
          else if(color == 3) {
            writePixel(output + 1, 50, 50, 50);
          }
        }
        draw();
        delay(3000);
    }
}  

//=============================================================================================================
//    Setup
//=============================================================================================================

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

//=============================================================================================================
//    handle Command
//=============================================================================================================

void handleCommand(char* commandBuffer) {
  char command = commandBuffer[0];
  char p = commandBuffer[1];
  char q = commandBuffer[2];
  int parameter = 0;

  if(command == COMMAND_OUTPUT_IDENTIFY) {
      if(q >= '0' && q<= '9')
      {
        parameter = q - '0';
        if(p >= '0' && p<= '9')
           parameter += 10*(p - '0');
      }
      else if(p >= '0' && p<= '9')
      {
           parameter = p - '0';
      }
      else
        Serial.println("Unknow value");
   
    Serial.print("Identify pixel: ");
    Serial.println(parameter, DEC);
     
    for(int output = 0; output < OUTPUT_COUNT; output++) {
      if(output + 1 == parameter) {
        writePixel(output + 1, 255, 0, 0);
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
        writePixel(output + 1, 50, 0, 0);
      }
      else if(parameter == 1) {
        writePixel(output + 1, 0, 50, 0);
      }
      else if(parameter == 2) {
        writePixel(output + 1, 0, 0, 50);
      }
      else {
        writePixel(output + 1, 50, 50, 50);
      }
    }  
    draw();

  }
  if(command == COMMAND_PROGRAM_ADDRESSES) {
    programAddresses();
  }
  if(command == COMMAND_ERASE_ADDRESSES) {
    eraseAddresses();
  }
  if(command == COMMAND_TEST_COLOR){
    test_color();
  }
}

//=============================================================================================================
//    Main loop
//=============================================================================================================

void loop() {
  if(Serial.available() > 0) {
      char c = Serial.read();
      
      if(c == 0x0A) {
        if(bufferPosition <= COMMAND_LENGTH) {
           digitalWrite(failLedPin, HIGH);
           digitalWrite(passLedPin, LOW);
           handleCommand(inputBuffer);
           digitalWrite(failLedPin, LOW);
           digitalWrite(passLedPin, HIGH);
           inputBuffer[0] = 0;
           inputBuffer[1] = 0;
           inputBuffer[2] = 0;
        }
        else {
          Serial.print("Malformed command, length:");
          Serial.println(bufferPosition);
        }
        bufferPosition = 0;       
      }
      else {
        if(bufferPosition < COMMAND_LENGTH && c != 0x0D) {
          inputBuffer[bufferPosition] = c;
          bufferPosition++;
        }
        else {
          Serial.println("Too many characters!");
        }
      }
  }
  
  if(digitalRead(SWITCH_PIN) == LOW) {
  digitalWrite(failLedPin, HIGH);
  digitalWrite(passLedPin, LOW);
  
    delay(50);
    handleCommand("p");
    delay(50);
    inputBuffer[0] = 'i';
    for(int tile=1; tile<=14; tile++){
      if(tile <= 9)
        inputBuffer[1] = tile + '0';
      else{
        inputBuffer[1] = '1';
        inputBuffer[2] = '0'+ tile%10;
      }
      handleCommand(inputBuffer);
      delay(250);
    }
    inputBuffer[0] = 0;
    inputBuffer[1] = 0;
    inputBuffer[2] = 0;
    bufferPosition = 0;
    delay(50);
    handleCommand("t");
    while(digitalRead(SWITCH_PIN) == LOW) {}
    digitalWrite(failLedPin, LOW);
    digitalWrite(passLedPin, HIGH);
  }
  
  //===================================================================
  // Continue test Panel color
  //===================================================================
    static int counts = 0;
    static int countsPerChange = 300000;
    static int color = 0;
    
    counts++;
    if(counts > countsPerChange) {
      counts = 0;
      
      for(int output = 0; output < OUTPUT_COUNT; output++) {
        if(color == 0) {
          writePixel(output + 1, 50, 0, 0);
        }
        else if(color == 1) {
          writePixel(output + 1, 0, 50, 0);
        }
        else if(color == 2) {
          writePixel(output + 1, 0, 0, 50);
        }
        else if(color == 3) {
          writePixel(output + 1, 50, 50, 50);
        }
      }
      
      draw();
      color = (color + 1) % 4;
    }

}

