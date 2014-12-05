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


void FlashStorage::begin(FlashClass& _flash) {
    flash = &_flash;
}

int FlashStorage::sectors() {
	return flash->sectors();
}

int FlashStorage::sectorSize() {
	return flash->bytes() / flash->sectors();
}

bool FlashStorage::freeSector(int sector) {
	uint8_t buf[MAGIC_NUMBER_SIZE];
	// TODO: This assumes 4096 byte sector size!
	flash->read(sector<<12, buf, MAGIC_NUMBER_SIZE);

	bool isFree =
		(buf[0] != (SECTOR_MAGIC_NUMBER >> 16)%0xFF)
		|| (buf[1] != (SECTOR_MAGIC_NUMBER >>  8)%0xFF)
		|| (buf[2] != (SECTOR_MAGIC_NUMBER >>  0)%0xFF);

	return isFree;
}

int FlashStorage::freeSectors() {
	int count = 0;

	for(int sector = 0; sector < sectors(); sector++) {
		if(freeSector(sector)) {
			count++;
		}
	}

	return count;
}

int FlashStorage::freeSpace() {
	return freeSectors()*(sectorSize() - SECTOR_HEADER_SIZE);
}

int FlashStorage::findFreeSector(int start) {
	for(int i = start; i < sectors(); i++) {
		if(freeSector(i)) {
			return i;
		}
	}

	return -1;
}

//int FlashStorage::writeSector(int sector, int length, uint8_t* data) {
//	if(length != sectorSize()) {
//		return 0;
//	}
//
//	for(int page = 0; page < flash->pages()/flash->sectors(); page++) {
//		int offset = page*256;
//
//		flash->setWriteEnable(true);
//	    	flash->writePage(
//  			sector*sectorSize() + offset,
//    			data + offset
//    		);
//	    	while(flash->busy()) {
//    			delay(10);
//			// TODO: refresh watchdog
//    		}
//    		flash->setWriteEnable(false); 		
//	}
//
//	return length;
//}
