import blinkytape
import time
import random

def createAnimation(ledCount, frameCount, speed, colorData):
    animationSize = 12 + 3*ledCount*frameCount
    
    data = ""
    data += chr((ledCount >> 24) & 0xFF)
    data += chr((ledCount >> 16) & 0xFF)
    data += chr((ledCount >>  8) & 0xFF)
    data += chr((ledCount      ) & 0xFF)
    data += chr((frameCount >> 24) & 0xFF)
    data += chr((frameCount >> 16) & 0xFF)
    data += chr((frameCount >>  8) & 0xFF)
    data += chr((frameCount      ) & 0xFF)
    data += chr((speed >> 24) & 0xFF)
    data += chr((speed >> 16) & 0xFF)
    data += chr((speed >>  8) & 0xFF)
    data += chr((speed      ) & 0xFF)
    data += colorData

    if len(data) != animationSize:
        print "color data size incorrect, expected %i, got %i"%(animationSize - 12, len(colorData))
        return

    # Pad the file size to a page size
    fileSize = ((animationSize + 255) / 256) * 256

    while(len(data) < fileSize):
        data+= ' '

    status, sector = bt.createFile(0x12, fileSize)
    if (not status) or (sector < 0):
        print "could not create file"
        exit(1)
    print "created animation of length %i in sector %i"%(fileSize, sector)

    for page in range(0, (fileSize >> 8)):
        status = bt.writeFilePage(sector, page*256, data[page*256:(page+1)*256])
        if not status:
            print "Error writing page %i to animation: %r"%(page, status)
            exit(1)


bt = blinkytape.BlinkyTape()

# first, erase the flash memory
# bt.flashErase()

print "free space: ", bt.getFreeSpace()
print "largest file availabe: ", bt.getLargestFile()
print "file count: ", bt.getFileCount()
print "first free sector: ", bt.getFirstFreeSector()

for sector in range(0, 50):
    bt.deleteFile(sector)


ledCount = 12
frameCount = 12
speed = 30
data = ''
for frame in range(0, frameCount):
    for led in range(0, ledCount):
        if led == frame:
            data+= chr(255)
            data+= chr(0)
            data+= chr(0)
        else:
            data+= chr(0)
            data+= chr(0)
            data+= chr(0)

createAnimation(ledCount, frameCount, speed, data)

ledCount = 12
frameCount = 400
speed = 100
data = ''
for frame in range(0, frameCount):
    for led in range(0, ledCount):
        if (frame % 4) == 0:
            data+= chr(255);
            data+= chr(0);
            data+= chr(0);
        if (frame % 4) == 1:
            data+= chr(0);
            data+= chr(255);
            data+= chr(0);
        if (frame % 4) == 2:
            data+= chr(0);
            data+= chr(0);
            data+= chr(255);
        if (frame % 4) == 3:
            data+= chr(255);
            data+= chr(255);
            data+= chr(255);

createAnimation(ledCount, frameCount, speed, data)

for sector in range(0, 50):
    status, fileType = bt.getIsFile(sector)
    print "%3i: %r(%i)"%(sector, status, fileType)

bt.reloadAnimations()
