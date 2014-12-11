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
print "making new animation: ", bt.createAnimation(256)
