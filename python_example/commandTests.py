import blinkytape
import time

bt = blinkytape.BlinkyTape()

# first, erase the flash memory
#bt.eraseFlash()
#time.sleep(10)

print "free space: ", bt.getFreeSpace()
print "largest file availabe: ", bt.getLargestFile()
print "file count: ", bt.getFileCount()
print "first free sector: ", bt.getFirstFreeSector()

# TODO: make sure the flash is cleared first!
print "making new animation: ",
sector = bt.createFile(0xEF, 256)
print sector 

if sector >= -1:
    data = ""
    for i in range(0,256):
        data += chr(i)
    print "writing data to animation: ", bt.writeFileData(sector, 0, data)

    print "reading data back: ",
    status, returnData = bt.readFileData(sector, 0, 256)

    print "got", len(returnData)
    for i in range(0, 256):
        if data[i] != returnData[i]:
            print "got bad data at %i, expected %i, got %i"%(i, ord(data[i]), ord(returnData[i]))
