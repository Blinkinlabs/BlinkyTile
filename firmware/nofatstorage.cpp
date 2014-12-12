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

#include "nofatstorage.h"
#include "blinkytile.h"

void NoFatStorage::rebuildSectorMap() {
    for(int sector = 0; sector < MAX_SECTORS; sector++)
        sectorMap[sector] = SECTOR_TYPE_FREE;

    for(int sector = 0; sector < sectors(); sector++) {
        // There are three cases here (in the order we should test them)
        // 1. Sector is linked from another sector, and has already been marked used
        if(sectorMap[sector] != 0)
            continue;

        // 2. Sector is a new file, and may link other sectors
        uint8_t buf[MAGIC_NUMBER_SIZE];
        flash->read(sector<<12, buf, MAGIC_NUMBER_SIZE);

        bool isFree =
               (buf[0] != ((SECTOR_MAGIC_NUMBER >> 24) & 0xFF))
            || (buf[1] != ((SECTOR_MAGIC_NUMBER >> 16) & 0xFF))
            || (buf[2] != ((SECTOR_MAGIC_NUMBER >>  8) & 0xFF))
            || (buf[3] != ((SECTOR_MAGIC_NUMBER >>  0) & 0xFF));

        if(!isFree) {
            sectorMap[sector] = SECTOR_TYPE_START;

            // Mark all linked sectors as well
            for(int link = 1; link < sectorsForLength(fileSize(sector)); link++) {
                sectorMap[fileSector(sector, link)] = SECTOR_TYPE_CONTINUATION;
            }
        }

        // 3. Sector is unused
    }
}

int NoFatStorage::sectorsForLength(int length) {
    return (length - (SECTOR_SIZE - FILE_HEADER_SIZE) + (SECTOR_SIZE - 1))/SECTOR_SIZE + 1;
}

void NoFatStorage::begin(FlashClass& _flash) {
    flash = &_flash;

    rebuildSectorMap();
}

int NoFatStorage::sectors() {
    return MAX_SECTORS;

    if(flash->sectors() > MAX_SECTORS)
        return MAX_SECTORS;

    return flash->sectors();
}

bool NoFatStorage::checkSectorFree(int sector) {
    return (sectorMap[sector] == SECTOR_TYPE_FREE);
}

int NoFatStorage::countFreeSectors() {
    int count = 0;

    for(int sector = 0; sector < sectors(); sector++) {
        if(checkSectorFree(sector))
            count++;
    }

    return count;
}

int NoFatStorage::freeSpace() {
    return countFreeSectors()*SECTOR_SIZE;
}

int NoFatStorage::files() {
    int count = 0;

    for(int sector = 0; sector < sectors(); sector++) {
        if(isFile(sector))
            count++;
    }

    return count;
}

int NoFatStorage::largestNewFile() {
    int freeSectors = countFreeSectors();

    // If we have 0 free sectors, we can't store anything.
    if(freeSectors == 0)
        return 0;

    // If we have > MAX_LINKED_SECTORS, we unfortunately can't use them all.
    if(freeSectors > MAX_LINKED_SECTORS + 1)
        freeSectors = MAX_LINKED_SECTORS + 1;

    return freeSectors*SECTOR_SIZE - FILE_HEADER_SIZE;
}

bool NoFatStorage::isFile(int sector) {
    if((sector < 0) || (sector >= sectors()))
        return false;

    return sectorMap[sector] == SECTOR_TYPE_START;
}

int NoFatStorage::fileSize(int sector) {
    if(!isFile(sector))
        return 0;

    uint8_t buff[4];
    flash->read((sector << 12) + 4, buff, 4);

    return (buff[0]  << 24)
         | (buff[1]  << 16)
         | (buff[2]  <<  8)
         | (buff[3]  <<  0);
}

uint8_t NoFatStorage::fileType(int sector) {
    if(!isFile(sector))
        return 0;

    uint8_t buff[1];
    flash->read((sector << 12) + 8, buff, 1);

    return buff[0];
}

int NoFatStorage::fileSectors(int sector) {
    return sectorsForLength(fileSize(sector));
}

int NoFatStorage::fileSector(int sector, int link) {
    if(link>sectorsForLength(fileSize(sector)))
        return -1;

    if(link == 0)
        return sector;

    uint8_t buff[2];
    flash->read((sector << 12) + 8 + link*2, buff, 2);

    return (buff[0]  << 8) | (buff[1]  << 0);
}

int NoFatStorage::createNewFile(uint8_t type, int length) {
    // Length must be page aligned.
    if((length & 0xFF) != 0)
        return -1;

    // If the size is too large to fit the flash, bail.
    if(length > largestNewFile())
        return -1;

    // Fill in the header data for this file
    uint8_t headerData[PAGE_SIZE];
    for(int i = 0; i < PAGE_SIZE; i++)
        headerData[i] = 0xFF;

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
    int linkedSectorNumber = startingSectorNumber;

    for(int linkedSector = 1; linkedSector < sectorsForLength(length); linkedSector++) {
        // Claim the next free sector
        linkedSectorNumber = findFreeSector(linkedSectorNumber+1);
        headerData[8 + linkedSector*2    ] = (linkedSectorNumber >> 8) & 0xFF;
        headerData[8 + linkedSector*2 + 1] = (linkedSectorNumber     ) & 0xFF;
        
        // And erase it
        flash->setWriteEnable(true);
        flash->eraseSector(linkedSectorNumber << 12);
        while(flash->busy()) {
            watchdog_refresh();
            delay(FLASH_WAIT_DELAY);
        }
        flash->setWriteEnable(false);
    }

    // Erase the starting sector
    flash->setWriteEnable(true);
    flash->eraseSector(startingSectorNumber << 12);
    while(flash->busy()) {
        watchdog_refresh();
        delay(FLASH_WAIT_DELAY);
    }
    flash->setWriteEnable(false);

    // Write the header data to the sector
    flash->setWriteEnable(true);
    flash->writePage(startingSectorNumber << 12, headerData);
    while(flash->busy()) {
        watchdog_refresh();
        delay(FLASH_WAIT_DELAY);
    }
    flash->setWriteEnable(false);

    rebuildSectorMap();

    return startingSectorNumber;
}


void NoFatStorage::deleteFile(int sector) {
    // First, erase all of the linked sectors (by chance they might have data that causes them to be mistaken for a 
    // starting sector)
    for(int linkedSector = 1; linkedSector < sectorsForLength(fileSize(sector)); linkedSector++) {
        flash->setWriteEnable(true);
        flash->eraseSector(fileSector(sector, linkedSector) << 12);
        while(flash->busy()) {
            watchdog_refresh();
            delay(FLASH_WAIT_DELAY);
        }
        flash->setWriteEnable(false);
    }

    // Then erase the starting sector
    flash->setWriteEnable(true);
    flash->eraseSector(sector << 12);
    while(flash->busy()) {
        watchdog_refresh();
        delay(FLASH_WAIT_DELAY);
    }
    flash->setWriteEnable(false);

    rebuildSectorMap();
}

int NoFatStorage::writePageToFile(int sector, int offset, uint8_t* data) {
    // Check that the page contains the start of a file
    if(!isFile(sector))
        return 0;

    // Check that the offset is page-aligned
    if((offset & 0xFF) != 00)
        return 0;

    // Check that the page fits into the file
    // TODO: Support files with non-page aligned lengths...
    if(offset + PAGE_SIZE > fileSize(sector))
        return 0;

    // Determine which sector, and what offset, this page should be written to.
    // First, the sector is determined by calling linkedSectorsPerLength on the offset.
    int outputSector = sectorsForLength(offset) - 1;

    // Now, subtract the size of the output sectors from the file offset to find the sector
    // offset
    int outputOffset = offset + FILE_HEADER_SIZE - outputSector*SECTOR_SIZE;


    flash->setWriteEnable(true);
    flash->writePage((fileSector(sector, outputSector) << 12) + outputOffset, data);
    while(flash->busy()) {
        watchdog_refresh();
        delay(FLASH_WAIT_DELAY);
    }
    flash->setWriteEnable(false);

    return PAGE_SIZE;
}

int NoFatStorage::readFromFile(int sector, int offset, uint8_t* data, int length) {
    // Check that the page contains the start of a file
    if(!isFile(sector))
        return 0;

    // Check that the data is in range
    if(offset+length > fileSize(sector))
        return 0;

    // So we don't span more than two partial sectors
    if(length > SECTOR_SIZE)
        return 0;

    // If all the data fits into the first sector, then we only need to make one read
    if(sectorsForLength(offset) == sectorsForLength(offset+length)) {
        int outputSector = sectorsForLength(offset) - 1;
        int outputOffset = offset + FILE_HEADER_SIZE - outputSector*SECTOR_SIZE;

        return flash->read((fileSector(sector, outputSector) << 12) + outputOffset, data, length);
    }
    // Otherwise, make two reads
    else {
        int readCount = 0;

        int outputSectorA = sectorsForLength(offset) - 1;
        int outputOffsetA = offset + FILE_HEADER_SIZE - outputSectorA*SECTOR_SIZE;
        int lengthA = SECTOR_SIZE - outputOffsetA;

        int outputSectorB = outputSectorA + 1;
        int outputOffsetB = 0;
        int lengthB = length - lengthA;

        readCount += flash->read((fileSector(sector, outputSectorA) << 12) + outputOffsetA, data, lengthA);
        readCount += flash->read((fileSector(sector, outputSectorB) << 12) + outputOffsetB, data + lengthA, lengthB);
        return readCount;
    }
}

int NoFatStorage::findFreeSector(int start) {
    for(int i = start; i < sectors(); i++) {
        if(checkSectorFree(i)) {
            return i;
        }
    }

    return -1;
}
