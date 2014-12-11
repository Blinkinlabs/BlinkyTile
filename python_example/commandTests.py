import blinkytape
import time
import random

bt = blinkytape.BlinkyTape()

# first, erase the flash memory
#bt.eraseFlash()
#time.sleep(10)

print "free space: ", bt.getFreeSpace()
print "largest file availabe: ", bt.getLargestFile()
print "file count: ", bt.getFileCount()
print "first free sector: ", bt.getFirstFreeSector()

# TODO: make sure the flash is cleared first!
pages = 20

print "making new animation: ",
sector = bt.createFile(0xEF, 256 * pages)
print sector 

if sector >= -1:
    data = ""
    for i in range(0,256 * pages):
        data += chr(random.randint(0,255))

    for i in range(0, pages):
        print "writing data to animation: ", bt.writeFileData(sector, i*256, data[i*256:256])

    readData = ""
    print "reading data back: ",
    for i in range(0, pages):
        status, returnData = bt.readFileData(sector, 0, 256)
        readData += returnData
            

    print "got", len(readData)
    for i in range(0, pages*256):
        if data[i] != readData[i]:
            print "got bad data at %i, expected %i, got %i"%(i, ord(data[i]), ord(returnData[i]))
