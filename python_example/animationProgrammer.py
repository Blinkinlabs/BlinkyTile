import blinkytape
import time
import random

def loadAnimation(animation):
    ledCount = animation[0]
    frameCount = animation[1]
    speed = animation[2]
    colorData = animation[3]
    animationType = 0 #BGR uncompressed

    animationSize = 16 + 3*ledCount*frameCount
    
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
    data += chr((animationType >> 24) & 0xFF)
    data += chr((animationType >> 16) & 0xFF)
    data += chr((animationType >>  8) & 0xFF)
    data += chr((animationType      ) & 0xFF)
    data += colorData

    if len(data) != animationSize:
        print "color data size incorrect, expected %i, got %i"%(animationSize - 16, len(colorData))
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


# Note! color order is b,g,r!
def makePixel(r,g,b):
    data = ''
    data += chr(b)
    data += chr(g)
    data += chr(r)
    return data


ledCount = 12*3
frameCount = 12
speed = 30
data = ''
for frame in range(0, frameCount):
    for led in range(0, ledCount):
        if (led % frameCount) == frame:
            data += makePixel(0,0,255)
        else:
            data += makePixel(0,0,0)

shadowAnimation = [ledCount, frameCount, speed, data]


ledCount = 12*3
frameCount = 100
speed = 100
data = ''
for frame in range(0, frameCount):
    for led in range(0, ledCount):
        if (frame % 4) == 0:
            data += makePixel(255,0,0)
        if (frame % 4) == 1:
            data += makePixel(0,255,0)
        if (frame % 4) == 2:
            data += makePixel(0,0,255)
        if (frame % 4) == 3:
            data += makePixel(255,255,255)

rgbAnimation = [ledCount, frameCount, speed, data]


while True:
    print "press enter to start..."
    a = raw_input()

    bt = blinkytape.BlinkyTape()

    # first, erase the flash memory
    # bt.flashErase()
    
    for sector in range(0, 50):
        bt.deleteFile(sector)
    
    #print "free space: ", bt.getFreeSpace()
    #print "largest file availabe: ", bt.getLargestFile()
    print "file count: ", bt.getFileCount()
    #print "first free sector: ", bt.getFirstFreeSector()
    
    
    loadAnimation(shadowAnimation)
    loadAnimation(rgbAnimation)

    bt.reloadAnimations()
    bt.close()
