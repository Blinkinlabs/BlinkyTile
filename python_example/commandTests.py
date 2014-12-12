import blinkytape
import time
import random

bt = blinkytape.BlinkyTape()

# first, erase the flash memory
#bt.eraseFlash()

print "free space: ", bt.getFreeSpace()
print "largest file availabe: ", bt.getLargestFile()
print "file count: ", bt.getFileCount()
print "first free sector: ", bt.getFirstFreeSector()

pages = 2

print "making new animation: ",
status, sector = bt.createFile(0xEF, 256 * pages)
if not status:
    print "could not create file"
    exit(1)

print "created animation of length %i in sector %i"%(256*pages, sector)

if sector >= -1:
    data = ""
    for i in range(0,256 * pages):
        data += chr(random.randint(0,255))

    for i in range(0, pages):
        status = bt.writeFilePage(sector, i*256, data[i*256:(i+1)*256])
        print "writing page %i to animation: %r"%(i, status)
        if not status:
            exit(1)

    readData = ""
    print "reading data back in pages: "
    for i in range(0, pages):
        status, returnData = bt.readFileData(sector, i*256, 256)
        print "read page %i: status %r got %i"%(i, status, len(returnData))
        if not status:
            exit(1)

        readData += returnData

    print "got", len(readData)
    print "comparing data:"

    for i in range(0, pages*256):
        if data[i] != readData[i]:
            print "got bad data at %i, expected %i, got %i"%(i, ord(data[i]), ord(returnData[i]))
            exit(1)
    print "write successful!"
