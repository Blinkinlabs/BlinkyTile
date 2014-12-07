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

#include "sectorDescriptor.h"
#include "blinkytile.h"

void FlashStorage::rebuildSectorMap() {
    // TODO: Warn if sectors() > MAX_SECTORS

    for(int sector = 0; sector < MAX_SECTORS; sector++)
	sectorMap[sector] = SECTOR_TYPE_FREE;

    for(int sector = 0; sector < sectors(); sector++) {
	// There are three cases here (in the order we should test them)
	// 1. Sector is linked from another sector, and has already been marked used
	if(sectorMap[sector] != 0)
		continue;

	// 2. Sector is a new file, and may link other sectors
	uint8_t buf[SECTOR_HEADER_SIZE];
	flash->read(sector<<12, buf, SECTOR_HEADER_SIZE);

	bool isFree =
		   (buf[0] != ((SECTOR_MAGIC_NUMBER >> 24) & 0xFF))
		|| (buf[1] != ((SECTOR_MAGIC_NUMBER >> 16) & 0xFF))
		|| (buf[2] != ((SECTOR_MAGIC_NUMBER >>  8) & 0xFF))
		|| (buf[3] != ((SECTOR_MAGIC_NUMBER >>  0) & 0xFF));

	if(!isFree) {
		sectorMap[sector] = SECTOR_TYPE_START;

		// Mark all linked sectors as well
		for(int link = 0; link < linkedSectorsForLength(fileSize(sector)); link++) {
			sectorMap[linkedSector(sector, link)] = SECTOR_TYPE_LINKED;
		}
	}

	// 3. Sector is unused
    }
}

int FlashStorage::linkedSectorsForLength(int length) {
    return (length - (SECTOR_SIZE - FILE_HEADER_SIZE) + (SECTOR_SIZE - 1))/SECTOR_SIZE;
}

void FlashStorage::begin(FlashClass& _flash) {
    flash = &_flash;

    rebuildSectorMap();
}

int FlashStorage::sectors() {
	return flash->sectors();
}

bool FlashStorage::checkSectorFree(int sector) {
	return (sectorMap[sector] == SECTOR_TYPE_FREE);
}

int FlashStorage::countFreeSectors() {
	int count = 0;

	for(int sector = 0; sector < sectors(); sector++) {
		if(checkSectorFree(sector))
			count++;
	}

	return count;
}

int FlashStorage::freeSpace() {
	return countFreeSectors()*SECTOR_SIZE;
}

int FlashStorage::files() {
	int count = 0;

	for(int sector = 0; sector < sectors(); sector++) {
		if(sectorMap[sector] == SECTOR_TYPE_START)
			count++;
	}

	return count;
}

int FlashStorage::largestNewFile() {
        int freeSectors = countFreeSectors();

	// If we have 0 free sectors, we can't store anything.
	if(freeSectors == 0)
		return 0;

	// If we have > MAX_LINKED_SECTORS, we unfortunately can't use them all.
	if(freeSectors > MAX_LINKED_SECTORS + 1)
		freeSectors = MAX_LINKED_SECTORS + 1;

	return freeSectors*SECTOR_SIZE - FILE_HEADER_SIZE;
}

bool FlashStorage::isFile(int sector) {
	return sectorMap[sector] == SECTOR_TYPE_START;
}

int FlashStorage::fileSize(int sector) {
	if(sectorMap[sector] != SECTOR_TYPE_START)
		return 0;

        uint8_t buff[4];
        flash->read((sector << 12) + 4, buff, 4);

	return    (buff[0]  << 24)
		| (buff[1]  << 16)
		| (buff[2]  <<  8)
		| (buff[3]  <<  0);
}

uint8_t FlashStorage::fileType(int sector) {
	if(sectorMap[sector] != SECTOR_TYPE_START)
		return 0;

        uint8_t buff[1];
        flash->read((sector << 12) + 8, buff, 1);

	return buff[0];
}

int FlashStorage::fileSectors(int sector) {
	return linkedSectorsForLength(fileSize(sector)) + 1;
}

int FlashStorage::linkedSector(int sector, int link) {
	if(link>linkedSectorsForLength(fileSize(sector)))
		return -1;

        uint8_t buff[2];
        flash->read((sector << 12) + 10 + link*2, buff, 2);

	return    (buff[0]  << 8)
		| (buff[1]  << 0);
}

int FlashStorage::createNewFile(uint8_t type, int length) {
	// Length must be page aligned.
	if((length & 0xFF) != 0)
		return -1;

	// If the size is too large to fit the flash, bail.
	if(length > largestNewFile())
		return -1;

	// Fill in the header data for this file
	uint8_t headerData[PAGE_SIZE];
	headerData[0] = (SECTOR_MAGIC_NUMBER >> 24) & 0xFF;
	headerData[1] = (SECTOR_MAGIC_NUMBER >> 16) & 0xFF;
	headerData[2] = (SECTOR_MAGIC_NUMBER >>  8) & 0xFF;
	headerData[3] = (SECTOR_MAGIC_NUMBER >>  0) & 0xFF;
	headerData[4] = (length >> 24) & 0xFF;
	headerData[5] = (length >> 16) & 0xFF;
	headerData[6] = (length >>  8) & 0xFF;
	headerData[7] = (length >>  0) & 0xFF;
	headerData[8] = type;
	headerData[9] = 0xFF;

	// Locate a sector to store the file header
	const int startingSectorNumber = findFreeSector(0);

	// Build a table of linked sectors 
	int linkedSectorsRequired = linkedSectorsForLength(length);

	int linkedSectorNumber = startingSectorNumber;
	for(int linkedSector = 0; linkedSector < linkedSectorsRequired; linkedSector++) {
		linkedSectorNumber = findFreeSector(linkedSectorNumber+1);
		headerData[10 + linkedSector*2    ] = (linkedSectorNumber >> 8) & 0xFF;
		headerData[10 + linkedSector*2 + 1] = (linkedSectorNumber     ) & 0xFF;
		
		flash->eraseSector(linkedSectorNumber);
		watchdog_refresh();
	}

	// Erase the starting sector
	flash->eraseSector(startingSectorNumber);

	// Write the header data to the sector
	flash->setWriteEnable(true);
	flash->writePage(startingSectorNumber << 12, headerData);
	while(flash->busy()) {
    		watchdog_refresh();
    		delay(100);
	}
	flash->setWriteEnable(false);

	rebuildSectorMap();

	return startingSectorNumber;
}


void FlashStorage::deleteFile(int sector) {
	// TODO: Erase the linked sectors first

	flash->setWriteEnable(true);
	flash->eraseSector(sector << 12);
	while(flash->busy()) {
    		watchdog_refresh();
    		delay(100);
	}
	flash->setWriteEnable(false);

	rebuildSectorMap();
}

int FlashStorage::writePageToFile(int sector, int offset, uint8_t* data) {
	// Check that the page contains the start of a file
	if(sectorMap[sector] != SECTOR_TYPE_START)
		return 0;

	// Check that the offset is page-aligned
	if((offset & 0xFF) != 00)
		return 0;

	// Check that the page fits into the file
	// TODO: Support files with non-page aligned lengths...
	if(offset + PAGE_SIZE > fileSize(sector))
		return 0;

	int outputSector = 0;
	int outputOffset = 0;

	// If we are writing to data in the first sector, then 
	if (offset < SECTOR_SIZE - FILE_HEADER_SIZE) {
		outputSector = sector;
		outputOffset = offset + FILE_HEADER_SIZE;
	}
	else {
		// TODO: look into the header to find the linked sector to store the data to
		return 0;
	}

	flash->writePage((outputSector << 12) + outputOffset, data);
	return PAGE_SIZE;
}

int FlashStorage::readFromFile(int sector, int offset, uint8_t* data, int length) {
	// Check that the page contains the start of a file
	if(sectorMap[sector] != SECTOR_TYPE_START)
		return 0;

	// Check that the data is in range
	if(offset+length > fileSize(sector))
		return 0;

	// So we don't span more than two partial sectors
	if(length > SECTOR_SIZE)
		return 0;

	// Get the sector that the data starts in
	// TODO

	return 0;
}

int FlashStorage::findFreeSector(int start) {
	for(int i = start; i < sectors(); i++) {
		if(checkSectorFree(i)) {
			return i;
		}
	}

	return -1;
}
