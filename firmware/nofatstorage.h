/*
 * Simple sector table format for storing animations in flash memory
 * 
 * Copyright (c) 2014 Matt Mets
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

/*
 * NoFAT file system: Store files without a partition map!
 *
 * This is most certainly Yet Another Filesystem, so let's keep it simple.
 *
 * The goal is to store animation data files onto small external flash memories.
 * Using FAT was considered first but seemed like more work because of the large
 * sector size and need to write a shim to connect to the flash interface. Don't
 * bother to use this for a large memory or SD card...
 *
 * Designed for use with flash memories with these things in mind:
 * -Small memory (<16MiB), but large sector size (4KiB)
 * -Fast enough to just scan the first bytes of each sector, so no file allocation
 *  table needed
 * -Files are write-once, and the only chance to remove them is to wipe the whole
 *  file
 * -Files are described by a master sector, which contains the length of the file 
 *  as well as sector address(es) of any continuing data
 *
 * There are three kinds of sector:
 * -Free Sector: Sector that is not used
 * -Starting Sector: This sector starts a file. The format of this sector is
 *  described below. There is one starting sector per file
 * -Continuation sector: Sector that contains extended data from a file. This sector
 *  only has data in it. There are 0-n continuation sectors per file.
 *
 *
 * Starting sector layout:
 *
 * 0x0000: Magic number: [0x12]
 * 0x0001: Magic number: [0x34]
 * 0x0002: Magic number: [0x56]
 * 0x0003: Magic number: [0x78]
 * 0x0004: File size in bytes >> 24
 * 0x0005: File size in bytes >> 16
 * 0x0006: File size in bytes >>  8
 * 0x0007: File size in bytes >>  0
 * 0x0008: File type (implementation dependent)
 * 0x0009: Reserved
 * 0x000A: Continuation sector number 1 >> 8, if file size > 3840
 * 0x000B: Continuation sector number 1 >> 0, if file size > 3840
 * 0x000C: Continuation sector number 2 >> 8, if file size > 7936
 * 0x000D: Continuation sector number 2 >> 0, if file size > 7936
 * ... (as required)
 * 0x00FE: Continuation sector number 123 >> 0, if file size > 503552
 * 0x00FF: Continuation sector number 123 >> 0, if file size > 503552
 *
 * Note: With the restriction that the file table reside in the first page,
 *  there is only enough storage space to list 123 continuation sectors, so
 *  the maximum file size is 507648 bytes.
 */


#ifndef NOFAT_STORAGE_H
#define NOFAT_STORAGE_H 

#include <inttypes.h>
#include "jedecflash.h"

#define MAX_SECTORS             512 // Maximum number of sectors we can manage

#define SECTOR_SIZE             4096    // Number of bytes in a sector. This is the smallest unit we can erase
#define PAGE_SIZE               256     // Number of bytes in a page. This is the smallest unit we can write

#define FILE_HEADER_SIZE        PAGE_SIZE       // Size of the file header. There is one header per file
#define MAX_LINKED_SECTORS      ((FILE_HEADER_SIZE-10)/2)       // Maximum number of linked sectors we can store

#define MAGIC_NUMBER_SIZE       4       // Size of the magic number
#define SECTOR_MAGIC_NUMBER     0x12345679      // Magic number, at the beginning of every starting sector

#define SECTOR_TYPE_FREE        0x00    // For the sector map: This sector is free
#define SECTOR_TYPE_START       0x01    // For the sector map: This sector contains a file start
#define SECTOR_TYPE_CONTINUATION 0x02   // For the sector map: This sector contains continued data from a file

#define FLASH_WAIT_DELAY        2       // Number of ms to wait between flash status polls


class NoFatStorage {
  private:
    FlashClass* flash;
    // TODO: Make this a bitfield to save 448b ram
    uint8_t sectorMap[MAX_SECTORS];     // Map of all sectors and whether they are availabe

    // Rebuild the map of free/used sectors
    void rebuildSectorMap();

    // Compute the number of sectors that a file of the given length requires
    int sectorsForLength(int length);

  public:
    void begin(FlashClass& _flash);

    // Count the amount of free storage space, in bytes. Note that this is just
    // the unused sector count- to get the largest size file that can be allocated,
    // use largestNewFile()
    // @return Free space
    int freeSpace();

    // Count the number of files stored on the flash
    // @return number of files found
    int files();

    // Get the size of the largest file that can be allocated, in bytes. Note that
    // this will be smaller than freeSpace()
    int largestNewFile();

    // True if the given sector contains a file start
    bool isFile(int sector);

    // Get the size of a file, in bytes
    // @param sector Beginning sector of the file
    // @return size of the file, in bytes
    int fileSize(int sector);

    // Get the type of a file, in bytes
    // @param sector Beginning sector of the file
    // @return File type
    uint8_t fileType(int sector);

    // Get the number of sectors that the given file spans across
    // @param sector Beginning sector of the file
    // @return Number of sectors used to store this file
    int fileSectors(int sector);

    // Get the sector number for the nth linked sector in the file
    // @param sector Beginning sector of file
    // @param link Linked sector to examine (0=first sector, 1=first linked sector)
    // @return Sector number for this link
    int fileSector(int sector, int link);

    // Create a new file.
    // New file will be in 'write' mode until 
    // @param type file type (see above list)
    // @param size amount of data to store in the file, in bytes
    // @return -1 if file could not be created, starting sector # if successful
    int createNewFile(uint8_t type, int length);

    // Delete a file
    // @param sector Beginning sector of the file
    void deleteFile(int sector);

    // Write a page of data to the file
    // Note that the data length must equal to PAGE_SIZE!
    // @param sector Beginning sector of the file
    // @param offset offset into the file, in bytes. Must be page-aligned
    // @param data data of length PAGE_SIZE to write to the flash
    // @return length of data written, in bytes
    int writePageToFile(int sector, int offset, uint8_t* data);

    // Read data from a file
    // @param sector Beginning sector of the file
    // @param offset offset into the file, in bytes
    // @param data buffer to write file data to
    // @param length of data to read, in bytes
    // @return length of data read, in bytes
    int readFromFile(int sector, int offset, uint8_t* data, int length);

    // Erase a sector chain.
    // @param sector Beginning sector in the chain to erase
    void eraseSectorChain(int sector);

    // Total number of sectors in the flash memory.
    // @return Total number of sectors
    int sectors();

    // Get the number of bytes in a sector.
    // Note: This is expected to always be 4096
    // @return Number of bytes in a sector
    int sectorSize();

    // Check if a sector is free.
    // @return True if the sector is free, false otherwise.
    bool checkSectorFree(int sector);

    // Count the number of free sectors
    // @return Number of unused sectors
    int countFreeSectors();

    // Find a free sector, starting at the specified sector location
    // @param start Sector number to start looking at.
    // @return First free sector after the start sector, or -1 if no free
    // sector found.
    int findFreeSector(int start);

};


#endif
