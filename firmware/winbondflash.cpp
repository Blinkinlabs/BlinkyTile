/*
        Winbond spi flash memory chip operating library for Arduino
        by WarMonkey (luoshumymail@gmail.com)
        for more information, please visit bbs.kechuang.org
        latest version available on http://code.google.com/p/winbondflash
*/

#include "Wprogram.h"
#include <errno.h>
#include "winbondflash.h"

//COMMANDS
#define W_EN    0x06    //write enable
#define W_DE    0x04    //write disable
#define R_SR1   0x05    //read status reg 1
#define R_SR2   0x35    //read status reg 2
#define W_SR    0x01    //write status reg
#define PAGE_PGM        0x02    //page program
#define QPAGE_PGM       0x32    //quad input page program
#define BLK_E_64K       0xD8    //block erase 64KB
#define BLK_E_32K       0x52    //block erase 32KB
#define SECTOR_E        0x20    //sector erase 4KB
#define CHIP_ERASE      0xc7    //chip erase
#define CHIP_ERASE2     0x60    //=CHIP_ERASE
#define E_SUSPEND       0x75    //erase suspend
#define E_RESUME        0x7a    //erase resume
#define PDWN            0xb9    //power down
#define HIGH_PERF_M     0xa3    //high performance mode
#define CONT_R_RST      0xff    //continuous read mode reset
#define RELEASE         0xab    //release power down or HPM/Dev ID (deprecated)
#define R_MANUF_ID      0x90    //read Manufacturer and Dev ID (deprecated)
#define R_UNIQUE_ID     0x4b    //read unique ID (suggested)
#define R_JEDEC_ID      0x9f    //read JEDEC ID = Manuf+ID (suggested)
#define READ            0x03
#define FAST_READ       0x0b

#define SR1_BUSY_MASK   0x01
#define SR1_WEN_MASK    0x02

#define WINBOND_MANUF   0xef

#define DEFAULT_TIMEOUT 200

typedef struct {
    winbondFlashClass::partNumber pn;
    uint16_t id;
    uint32_t bytes;
    uint32_t pages;
    uint16_t sectors;
    uint16_t blocks;
}pnListType;
  
static const pnListType pnList[] = {
    { winbondFlashClass::W25Q80, 0x4014,1048576, 4096, 256, 16  },
    { winbondFlashClass::W25Q16, 0x4015,2097152, 8192, 512, 32  },
    { winbondFlashClass::W25Q32, 0x4016,4194304, 16384,1024,64  },
    { winbondFlashClass::W25Q64, 0x4017,8388608, 32768,2048,128 },
    { winbondFlashClass::W25Q128,0x4018,16777216,65536,4096,256 }
};
  
uint16_t winbondFlashClass::readSR()
{
  uint8_t r1,r2;
  select();
  send(R_SR1);
  r1 = receive(0xff, true);
  deselect();
  deselect();//some delay
  select();
  send(R_SR2);
  r2 = receive(0xff, true);
  deselect();
  return (((uint16_t)r2)<<8)|r1;
}

uint8_t winbondFlashClass::readManufacturer()
{
  uint8_t c;
  select();
  send(R_JEDEC_ID);
  c = receive(0x00);
  receive(0x00);
  receive(0x00, true);
  deselect();
  return c;
}

uint64_t winbondFlashClass::readUniqueID()
{
  uint64_t uid;
  uint8_t *arr;
  arr = (uint8_t*)&uid;
  select();
  send(R_UNIQUE_ID);
  send(0x00);
  send(0x00);
  send(0x00);
  send(0x00);
  //for little endian machine only
  for(int i=7;i>=0;i--)
  {
    arr[i] = receive(0x00, i==0);
  }
  deselect();
  return uid;
}

uint16_t winbondFlashClass::readPartID()
{
  uint8_t a,b;
  select();
  send(R_JEDEC_ID);
  send(0x00);
  a = receive(0x00);
  b = receive(0x00, true);
  deselect();
  return (a<<8)|b;
}

bool winbondFlashClass::checkPartNo(partNumber _partno)
{
  uint8_t manuf;
  uint16_t id;
  
  select();
  send(R_JEDEC_ID);
  manuf = receive(0x00);
  id = receive(0x00) << 8;
  id |= receive(0x00, true);
  deselect();

//  Serial.print("MANUF=0x");
//  Serial.print(manuf,HEX);
//  Serial.print(",ID=0x");
//  Serial.print(id,HEX);
//  Serial.println();
  
  if(manuf != WINBOND_MANUF)
    return false;
//  Serial.println("MANUF OK");

  if(_partno == custom)
    return true;
//  Serial.println("Not a custom chip type");

  if(_partno == autoDetect)
  {
//    Serial.print("Autodetect...");
    for(int i=0;i<sizeof(pnList)/sizeof(pnList[0]);i++)
    {
      if(id == (pnList[i].id))
      {
        _partno = pnList[i].pn;
        //Serial.println("OK");
        return true;
      }
    }
    if(_partno == autoDetect)
    {
//      Serial.println("Failed");
      return false;
    }
  }

  //test chip id and partNo
  for(int i=0;i<sizeof(pnList)/sizeof(pnList[0]);i++)
  {
    if(_partno == pnList[i].pn)
    {
      if(id == pnList[i].id)//id equal
        return true;
      else
        return false;
    }
  }
//  Serial.println("partNumber not found");
  return false;//partNo not found
}

bool winbondFlashClass::busy()
{
  uint8_t r1;
  select();
  send(R_SR1);
  r1 = receive(0xff, true);
  deselect();
  if(r1 & SR1_BUSY_MASK)
    return true;
  return false;
}

void winbondFlashClass::setWriteEnable(bool cmd)
{
  select();
  send( cmd ? W_EN : W_DE, true);
  deselect();
}

long winbondFlashClass::bytes()
{
  for(int i=0;i<sizeof(pnList)/sizeof(pnList[0]);i++)
  {
    if(partno == pnList[i].pn)
    {
      return pnList[i].bytes;
    }
  }
  return 0;
}

uint16_t winbondFlashClass::pages()
{
  for(int i=0;i<sizeof(pnList)/sizeof(pnList[0]);i++)
  {
    if(partno == pnList[i].pn)
    {
      return pnList[i].pages;
    }
  }
  return 0;
}

uint16_t winbondFlashClass::sectors()
{
  for(int i=0;i<sizeof(pnList)/sizeof(pnList[0]);i++)
  {
    if(partno == pnList[i].pn)
    {
      return pnList[i].sectors;
    }
  }
  return 0;
}

uint16_t winbondFlashClass::blocks()
{
  for(int i=0;i<sizeof(pnList)/sizeof(pnList[0]);i++)
  {
    if(partno == pnList[i].pn)
    {
      return pnList[i].blocks;
    }
  }
  return 0;
}

bool winbondFlashClass::begin(partNumber _partno)
{
  select();
  send(RELEASE, true);
  deselect();
  delayMicroseconds(5);//>3us
//  Serial.println("Chip Released");
  
  if(!checkPartNo(_partno))
    return false;

  return true;
}

void winbondFlashClass::end()
{
  select();
  send(PDWN, true);
  deselect();
  delayMicroseconds(5);//>3us
}

uint16_t winbondFlashClass::read (uint32_t addr,uint8_t *buf,uint16_t n)
{
  if(busy())
    return 0;
  
  select();
  send(READ);
  send(addr>>16);
  send(addr>>8);
  send(addr);
  for(uint16_t i=0;i<n;i++)
  {
    buf[i] = receive(0x00, i==n-1);
  }
  deselect();
  
  return n;
}

void winbondFlashClass::writePage(uint32_t addr_start,uint8_t *buf)
{
  select();
  send(PAGE_PGM);
  send(addr_start>>16);
  send(addr_start>>8);
  send(0x00);
  uint8_t i=0;
  do {
    send(buf[i], i==255);
    i++;
  }while(i!=0);
  deselect();
}

void winbondFlashClass::eraseSector(uint32_t addr_start)
{
  select();
  send(SECTOR_E);
  send(addr_start>>16);
  send(addr_start>>8);
  send(addr_start, true);
  deselect();
}

void winbondFlashClass::erase32kBlock(uint32_t addr_start)
{
  select();
  send(BLK_E_32K);
  send(addr_start>>16);
  send(addr_start>>8);
  send(addr_start, true);
  deselect();
}

void winbondFlashClass::erase64kBlock(uint32_t addr_start)
{
  select();
  send(BLK_E_64K);
  send(addr_start>>16);
  send(addr_start>>8);
  send(addr_start, true);
  deselect();
}

void winbondFlashClass::eraseAll()
{
  select();
  send(CHIP_ERASE, true);
  deselect();
}

void winbondFlashClass::eraseSuspend()
{
  select();
  send(E_SUSPEND, true);
  deselect();
}

void winbondFlashClass::eraseResume()
{
  select();
  send(E_RESUME, true);
  deselect();
}

bool winbondFlashSPI::begin(partNumber _partno, uint8_t _nss)
{
  nss = _nss;

  spi4teensy3::init(7);

  pinMode(nss, OUTPUT);
  deselect();

  return winbondFlashClass::begin(_partno);
}

void winbondFlashSPI::end()
{
  winbondFlashClass::end();
}

