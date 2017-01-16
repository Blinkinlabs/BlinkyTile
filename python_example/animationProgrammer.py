import blinkytape
import time
import random
import shimmer
import colorloop
import pornjshimmer

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


ledCount = 160

frameCount = 12
speed = 30
data = ''
for frame in range(0, frameCount):
    for led in range(0, ledCount):
        if (led % frameCount) == frame:
            data += makePixel(100,0,255)
        elif ((led + frameCount/2) % frameCount) == frame:
            data += makePixel(255,0,100)
        else:
            data += makePixel(0,0,0)

shadowAnimation = [ledCount, frameCount, speed, data]


frameCount = 750
speed = 50
data = ''

shim = shimmer.shimmer(ledCount)
for frame in range(0, frameCount):
    data += shim.getData()

shimmerAnimation = [ledCount, frameCount, speed, data]



frameCount = 2250
speed = 20
data = ''

pornj = pornjshimmer.pornjshimmer(ledCount)
for frame in range(0, frameCount):
    data += pornj.getData()

pornjshimmerAnimation = [ledCount, frameCount, speed, data]


frameCount = 985
speed = 105
data = ''

colors = colorloop.colorloop(ledCount,1,1,1)
for frame in range(0, frameCount):
    data += colors.getData()

colorAnimation = [ledCount, frameCount, speed, data]


frameCount = 1
speed = 100
data = ''
for frame in range(0, frameCount):
    for led in range(0, ledCount):
        data += makePixel(255,255,255)

flashlightAnimation = [ledCount, frameCount, speed, data]


while True:
    print "press enter to start..."
    a = raw_input()

    import subprocess

    #time.sleep(1)
    #subprocess.call(["dfu-util", "-d", "1d50", "-D", "../bin/lightbuddy-firmware-v100.dfu"])
    #time.sleep(1)


    bt = blinkytape.BlinkyTape()

    # first, erase the flash memory
    print "erasing the flash... (this will take a while)"
    bt.flashErase()
    
    #for sector in range(0, 100):
    #    bt.deleteFile(sector)
    
    #print "free space: ", bt.getFreeSpace()
    #print "largest file availabe: ", bt.getLargestFile()
    print "file count: ", bt.getFileCount()
    #print "first free sector: ", bt.getFirstFreeSector()
    
    
    loadAnimation(shadowAnimation)
    loadAnimation(shimmerAnimation)
    loadAnimation(colorAnimation)
    loadAnimation(flashlightAnimation)
    #loadAnimation(pornjshimmerAnimation)

    bt.reloadAnimations()
    bt.close()
