#include <stdlib.h>
#include <stdio.h>
#include "WProgram.h"
#include "pins_arduino.h"
#include "addressprogrammer.h"
#include "blinkytile.h"
#include "nofatstorage.h"
#include "jedecflash.h"
#include "usb_serial.h"
#include "defaultanimation.h"
#include "animation.h"

extern FlashSPI flash;
extern NoFatStorage flashStorage;
extern Animations animations;

void singleCharacterHack(char in) {
    char buffer[100];
    int bufferSize;

    switch(in) {
        case 'p': // program an address
            programAddress(1);
            break;
        case 'i': // Get info about the flash contents
            bufferSize = sprintf(buffer, "Sector Count: %i\r\n", flashStorage.sectors());
            usb_serial_write(buffer, bufferSize);
            bufferSize = sprintf(buffer, "Free sectors: %i\r\n", flashStorage.countFreeSectors());
            usb_serial_write(buffer, bufferSize);
            bufferSize = sprintf(buffer, "Free space: %i\r\n", flashStorage.freeSpace());
            usb_serial_write(buffer, bufferSize);
            bufferSize = sprintf(buffer, "First free sector: %i\r\n", flashStorage.findFreeSector(0));
            usb_serial_write(buffer, bufferSize);
            bufferSize = sprintf(buffer, "File count: %i\r\n", flashStorage.files());
            usb_serial_write(buffer, bufferSize);
            bufferSize = sprintf(buffer, "Largest new file: %i\r\n", flashStorage.largestNewFile());
            usb_serial_write(buffer, bufferSize);
            break;
        case 'w': // Create a file of some length
            for(int i = 0; i < 3; i++) {
                // Create a small empty animation of size 
                bufferSize = sprintf(buffer, "Creating animation...");
                usb_serial_write(buffer, bufferSize);

                int animationSize = 256*16*i;

                int sector = flashStorage.createNewFile(0xEE, animationSize);

                if(sector < 0) {
                    bufferSize = sprintf(buffer, "Error creating file!");
                    usb_serial_write(buffer, bufferSize);
                }
                else {
                    for(int offset = 0; offset < animationSize; offset+=PAGE_SIZE) {
                        uint8_t data[PAGE_SIZE];
                        for(int i = 0; i < PAGE_SIZE; i++)
                            data[i] = (offset >> 8);

                        int written = flashStorage.writePageToFile(sector, offset, data);
                        if(written!=PAGE_SIZE) {
                            bufferSize = sprintf(buffer, "Error writing to offset %i, wrote %i bytes\r\n", offset, written);
                            usb_serial_write(buffer, bufferSize);
                        }
                        else {
                            bufferSize = sprintf(buffer, "Successfully wrote to offset %i, wrote %i bytes\r\n", offset, written);
                            usb_serial_write(buffer, bufferSize);
                        }
                    }

                    bufferSize = sprintf(buffer, "done, wrote to %i\r\n", sector);
                    usb_serial_write(buffer, bufferSize);
                }

            }
            break;
        case 'd': // delete the first file
            {
                int sector = 0;
                while(flashStorage.checkSectorFree(sector)) {
                    sector++;
                }
                if(sector < flashStorage.sectors()) {
                    flashStorage.deleteFile(sector);
                    bufferSize = sprintf(buffer, "deleted file in sector %i\r\n", sector);
                    usb_serial_write(buffer, bufferSize);
                }
                else {
                    bufferSize = sprintf(buffer, "no files found!\r\n");
                    usb_serial_write(buffer, bufferSize);
                }
            }
            break;
        case 'e': // erase the whole flash
            bufferSize = sprintf(buffer, "erasing flash...");
            usb_serial_write(buffer, bufferSize);
    
            flash.setWriteEnable(true);
            flash.eraseAll();
            while(flash.busy()) {
                watchdog_refresh();
                delay(100);
            }
            flash.setWriteEnable(false);

            flashStorage.begin(flash);
            
            bufferSize = sprintf(buffer, "done\r\n");
            usb_serial_write(buffer, bufferSize);
            break;
        case 'l': // dump some data about each sector
            {
                for(int sector = 0; sector < flashStorage.sectors(); sector++) {
                    bufferSize = sprintf(buffer, "sector %3i ", sector);
                    usb_serial_write(buffer, bufferSize);
                    if(flashStorage.isFile(sector)) {
                        int fileType = flashStorage.fileType(sector);
                        int fileSize = flashStorage.fileSize(sector);
                        int fileSectors = flashStorage.fileSectors(sector);
                        bufferSize = sprintf(buffer, "contains a file. Type=%x, Size=%i, Sectors=%i",
                                                     fileType, fileSize, fileSectors);
                        usb_serial_write(buffer, bufferSize);
                        for(int link = 1; link < fileSectors; link++) {
                            bufferSize = sprintf(buffer, " %i:%i", link, flashStorage.fileSector(sector, link));
                            usb_serial_write(buffer, bufferSize);
                        }
                        bufferSize = sprintf(buffer, "\r\n");
                        usb_serial_write(buffer, bufferSize);
                    }
                    else if(flashStorage.checkSectorFree(sector)) {
                        bufferSize = sprintf(buffer, "is free\r\n");
                        usb_serial_write(buffer, bufferSize);
                    }
                    else {
                        bufferSize = sprintf(buffer, "contains a linked portion of a file\r\n");
                        usb_serial_write(buffer, bufferSize);
                    }

                }
            }
            break;
        case 'r': // read the first file out
            {
                int sector = 0;
                while(!flashStorage.isFile(sector)) {
                    sector++;
                }
                if(sector < flashStorage.sectors()) {
                    int fileType = flashStorage.fileType(sector);
                    int fileSize = flashStorage.fileSize(sector);
                    bufferSize = sprintf(buffer, "Reading file in sector: %i. Type=%i, Size=%i\r\n", sector, fileType, fileSize);
                    usb_serial_write(buffer, bufferSize);

                    #define READ_SIZE_A 29
                    for(int offset = 0; offset < fileSize; offset += READ_SIZE_A) {
                        uint8_t buff[READ_SIZE_A];
                        for(int i = 0; i < READ_SIZE_A; i++)
                            buff[i] = 0xFF;

                        int read = 0;
                        read = flashStorage.readFromFile(sector, offset, buff, READ_SIZE_A);

                        if(read == READ_SIZE_A) {

                            bufferSize = sprintf(buffer, "%08X: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x, %02x, %02x\r\n",
                                offset,
                                buff[0], buff[1], buff[2], buff[3], buff[4], buff[5], buff[6], buff[7],
                                buff[8], buff[9], buff[10], buff[11], buff[12], buff[13], buff[15], buff[15]);
                        }
                        else {
                            bufferSize = sprintf(buffer, "%08X: Error reading data; expected %i bytes, read %i",
                                offset, READ_SIZE_A, read);
                        }
                        usb_serial_write(buffer, bufferSize);
                    }

                }
                else {
                    bufferSize = sprintf(buffer, "no files found!\r\n");
                    usb_serial_write(buffer, bufferSize);
                }
            }
            break;
        case 'm': // dump some data from address 0
            {
                #define READ_SIZE 16
                for(int offset = 0; offset < SECTOR_SIZE*3; offset += READ_SIZE) {
                     uint8_t buff[READ_SIZE];
                     flash.read(0 + offset, buff, READ_SIZE);

                     bufferSize = sprintf(buffer, "%08X: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
                         offset,
                         buff[0], buff[1], buff[2], buff[3], buff[4], buff[5], buff[6], buff[7],
                         buff[8], buff[9], buff[10], buff[11], buff[12], buff[13], buff[14], buff[15]);
                     usb_serial_write(buffer, bufferSize);
                }
            }
            break;
        case 'a': // add a default animation
            makeDefaultAnimation(flashStorage);
            animations.begin(flashStorage);
            break;
        default:
            bufferSize = sprintf(buffer, "?\r\n");
            usb_serial_write(buffer, bufferSize);
    }
}
