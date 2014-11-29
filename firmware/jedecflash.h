/*
 * Generic JEDEC-compatible SPI flash library
 *
 * Copyright (c) 2014 Matt Mets
 *
 * based on Winbond spi flash memory chip operating library for Arduino
 * by WarMonkey (luoshumymail@gmail.com)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _JEDECFLASH_H__
#define _JEDECFLASH_H__

#include <inttypes.h>
#include "spi4teensy3.h"

//W25Q64 = 256_bytes_per_page * 16_pages_per_sector * 16_sectors_per_block * 128_blocks_per_chip
//= 256b*16*16*128 = 8Mbyte = 64MBits

class FlashClass {
public:  
    enum manufacturerId {
        Spansion = 0x01,
        Winbond = 0xEF, // todo: testme
    };

    enum partNumber {
        autoDetect,
        W25Q80,
        W25Q16,
        W25Q32,
        W25Q64,
        W25Q128,
        S25FL208K,
        S25FL216K,
    };

    bool begin(partNumber _partno = autoDetect);
    void end();

    long bytes();
    uint16_t pages();
    uint16_t sectors();
    uint16_t blocks();

    uint16_t read(uint32_t addr,uint8_t *buf,uint16_t n=256);

    void setWriteEnable(bool cmd = true);
    
    //WE() every time before write or erase
    inline void WE(bool cmd = true) {setWriteEnable(cmd);}
    
    //write a page, sizeof(buf) is 256 bytes  
    void writePage(uint32_t addr_start,uint8_t *buf);//addr is 8bit-aligned, 0x00ffff00

    //erase a sector ( 4096bytes ), return false if error
    //addr is 12bit-aligned, 0x00fff000
    void eraseSector(uint32_t addr);

    //erase a 64k block ( 65536b )
    //addr is 16bit-aligned, 0x00ff0000
    void erase64kBlock(uint32_t addr);

    //chip erase, return true if successfully started, busy()==false -> erase complete
    void eraseAll();


    // void eraseSuspend();
    // void eraseResume();

    bool busy();
    
    uint8_t  readManufacturer();
    uint16_t readPartID();
    // uint64_t readUniqueID();
    uint16_t readSR();

private:
    partNumber partno;
    bool checkPartNo(partNumber _partno);

protected:
    virtual void select() = 0;
    virtual void send(uint8_t x, bool deselect = false) = 0;
    virtual uint8_t receive(uint8_t x, bool deselect = false) = 0;
    virtual void deselect() = 0;
    
};

class FlashSPI: public FlashClass {
private:
//  uint8_t nss;
    inline void select() {
//    digitalWrite(nss,LOW);
    }

    inline void send(uint8_t x, bool deselect = false) {
        spi4teensy3::send(x, deselect);
    }

    inline uint8_t receive(uint8_t x, bool deselect = false) {
        return spi4teensy3::receive(deselect);
    }

    inline void deselect() {
//    digitalWrite(nss,HIGH);
    }

public:
//bool begin(partNumber _partno = autoDetect,uint8_t _nss = SS);
    bool begin(partNumber _partno = autoDetect);
    void end();
};

#endif
