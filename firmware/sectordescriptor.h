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

#ifndef SECTOR_DESCRIPTOR_H
#define SECTOR_DESCRIPTOR_H

#include <inttypes.h>
#include "jedecflash.h"

#define SECTOR_HEADER_SIZE		8 // Size of a sector header (magic number)
#define MAGIC_NUMBER_SIZE		3 // Size of the magic number

#define SECTOR_MAGIC_NUMBER   	0x123456
#define ANIMATION_START      	(SECTOR_MAGIC_NUMBER | 78)
#define ANIMATION_CONTINUATION	(SECTOR_MAGIC_NUMBER | 79)

typedef struct {
	uint32_t sectorType;		// Sector type, must be ANIMATION_START
	uint32_t nextSector;		// If animation size >1 sector, sector number for next sector, otherwise 0xFFFFFFFF
	uint32_t ledCount;			// Number of LEDS/frame in the animation
	uint32_t frameCount;		// Number of frames in the animation
	uint32_t playbackSpeed;		// Playback speed for frames, in ms per frame
	uint32_t format;			// Animation format, 0xFFFFFFFF=Uncompressed RGB24
} AnimationStartDescriptor;

typedef struct {
	uint32_t sectorType;		// Sector type, must be ANIMATION_CONTINUATION
	uint32_t nextSector;		// If animation size >1 sector, sector number for next sector, otherwise 0xFFFFFFFF
} AnimationContinuationDescriptor;


class FlashStorage {
  private:
  	FlashClass* flash;

  public:
    void begin(FlashClass& _flash);

    // Total number of sectors in the flash memory.
    // return Total number of sectors
    int sectors();

    // Get the number of bytes in a sector.
    // Note: This is expected to always be 4096
    // return Number of bytes in a sector
    int sectorSize();

    // Test if a sector is free.
    // return True if the sector is free, false otherwise.
    bool freeSector(int sector);

    // Count the number of free sectors
    // return 
    int freeSectors();

    // Count the amount of free storage space, in bytes. Note that there is an
    // 8-byte overhead in each sector for the sector header that is not
    // included.
  	// return Free space
    int freeSpace();

    // Find a free sector, starting at the specified sector location
    // param start Sector number to start looking at.
    // return First free sector after the start sector, or -1 if no free
    // sector found.
    int findFreeSector(int start);

    // Write data to a sector. Data in this sector will be erased before the
    // write operation commences.
    // param sector Sector to write to
    // param length Length of data to write (must be a multiple of page size)
    // param data Data to write
    // return Length of data written
    int writeSector(int sector, int length, uint8_t* data);

    // Erase a sector chain.
    // param sector Beginning sector in the chain to erase
    void eraseSectorChain(int sector);
};


#endif
