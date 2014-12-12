import blinkytape
import time
import random


def dumpSectorHeader(sector):
    address = sector*256*16
    status, data = bt.flashRead(address, 16)
    if (not status) or (len(data) != 16):
        print "error reading data from address %x, got %i bytes"%(address, len(data))
        exit(1)

    print "%08X:"%(sector*256*16),
    for byte in range(0, 16):
        print "%02X"%(ord(data[byte])),

    address = sector*256*16 + 256
    status, data = bt.flashRead(address, 16)
    if (not status) or (len(data) != 16):
        print "error reading data from address %x, got %i bytes"%(address, len(data))
        exit(1)

    print "%08X:"%(address),
    for byte in range(0, 16):
        print "%02X"%(ord(data[byte])),
    print ""

def dumpSector(sector):
    startingAddress = sector*256*16

    for page in range(0, 16):
        address = startingAddress + page*256

        status, data = bt.flashRead(address, 256)
        if (not status) or (len(data) != 256):
            print "error reading data from address %x, got %i bytes"%(address, len(data))
        for byte in range(0, 256):
            if (byte % 16) == 0:
                print "%08X:"%(address+byte),

            print "%02X"%(ord(data[byte])),

            if (byte % 16) == 15:
                print ""



bt = blinkytape.BlinkyTape()

# first, erase the flash memory
#bt.flashErase()

print "free space: ", bt.getFreeSpace()
print "largest file availabe: ", bt.getLargestFile()
print "file count: ", bt.getFileCount()
print "first free sector: ", bt.getFirstFreeSector()

for sector in range(0,32):
    dumpSectorHeader(sector)

dumpSector(0)

exit(1)

pages = 1983
length = 256*pages

print "making new animation: ",
status, sector = bt.createFile(0xEF, length)
if not status:
    print "could not create file"
    exit(1)

print "created animation of length %i in sector %i"%(length, sector)

if sector >= -1:
    data = ""
    for i in range(0,length):
        data += chr(random.randint(0,255))

    for page in range(0, pages):
        status = bt.writeFilePage(sector, page*256, data[page*256:(page+1)*256])
        if not status:
            print "Error writing page %i to animation: %r"%(page, status)
            exit(1)

#    dumpSector(sector)
#    dumpSector(sector+1)
#    dumpSector(sector+2)


#    readData = ""
#    print "reading data back in pages: "
#    for page in range(0, pages):
#        status, returnData = bt.readFileData(sector, page*256, 256)
#        if (not status) or (len(returnData) != 256):
#            print "Error reading page %i: status %r got %i"%(page, status, len(returnData))
#            exit(1)
#
#        readData += returnData
#
#    for position in range(0, length):
#        if data[position] != readData[position]:
#            print "got bad data at %i, expected %X, got %X"%(position, ord(data[position]), ord(returnData[position]))
#            exit(1)


    readData = ""
    print "reading data back in random-sized chunks: "
    position = 0
    while position < length:
        readLength = random.randint(1,256)
        if (readLength + position) > length:
            readLength = length - position

        status, returnData = bt.readFileData(sector, position, readLength)
        if (not status) or (len(returnData) != readLength):
            print "Error reading %i bytes of data from %08X: status %r got %i"%(readLength, position, status, len(returnData))
            exit(1)

        readData += returnData
        position += readLength

    print "verifying data"
    for position in range(0, length):
        if data[position] != readData[position]:
            print "got bad data at offset %08X, expected %X, got %X"%(position, ord(data[position]), ord(readData[position]))
#            exit(1)

    print "test successful!"
