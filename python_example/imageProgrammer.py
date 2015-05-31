# Save an animation to the Lightbuddy (Blinkytile Controller)
# Example usage:
# python imageProgrammer.py blink.png
#
# Options:
# -s 30  Change the playback speed (frames per second, higher is faster)
#
#
# Note: you'll need to install Python and PIL (the Python Imaging Library) to use this.


import blinkytape
import time
import Image

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



def makeAnimationFromFile(filename, speed):

    image = Image.open(filename)

    ledCount = image.size[1]
    frameCount = image.size[0]
    speed = 1000/int(speed)

    pixels = image.load()

    data = ''
    for frame in range(0, frameCount):
        for led in range(0, ledCount):
            pixel = pixels[frame, led]
            data += makePixel(pixel[0], pixel[1], pixel[2])

    return [ledCount, frameCount, speed, data]


while True:
    import optparse

    parser = optparse.OptionParser()
    parser.add_option("-e", "--erase", action="store_true", dest="erase",
                      help="Erase existing animations before loading new ones", default=False)
    parser.add_option("-s", "--speed", dest="speed",
                      help="Animation speed, in frames per second", type=int, default=30)
    parser.add_option("-p", "--port", dest="portname",
                      help="serial port (ex: /dev/ttyUSB0)", default=None)
    (options, args) = parser.parse_args()

    animations = list()
    for arg in args:
        animations.append(makeAnimationFromFile(arg, options.speed))

    port = options.portname

    print "press enter to start..."
    a = raw_input()

    bt = blinkytape.BlinkyTape()

    # first, erase the flash memory
    if(options.erase == True):
        print "erasing the flash... (this will take a while)"
        bt.flashErase()
    
    print "original animation count: ", bt.getFileCount()
   
    for animation in animations: 
        loadAnimation(animation)

    bt.reloadAnimations()

    print "new animation count: ", bt.getFileCount()

    bt.close()
