import blinkytape
import time

bt = blinkytape.BlinkyTape()

maxAddress = 126

while True:
    address = input("enter an address to program:")
    address = int(address) - 1
    
    bt.programAddress(address)
    time.sleep(1)

    for pos in range(0, maxAddress):
        if pos == address:
            bt.sendPixel(100,0,0)
        else:
            bt.sendPixel(0,0,0)
    bt.show()
    time.sleep(.1)
    bt.show()
    time.sleep(.5)

    for pos in range(0, maxAddress):
        if pos == address:
            bt.sendPixel(0,100,0)
        else:
            bt.sendPixel(0,0,0)
    bt.show()
    time.sleep(.1)
    bt.show()
    time.sleep(.5)

    for pos in range(0, maxAddress):
        if pos == address:
            bt.sendPixel(0,0,100)
        else:
            bt.sendPixel(0,0,0)
    bt.show()
    time.sleep(.1)
    bt.show()
    time.sleep(.5)

    for pos in range(0, maxAddress):
        if pos == address:
            bt.sendPixel(100,100,100)
        else:
            bt.sendPixel(0,0,0)
    bt.show()
    time.sleep(.1)
    bt.show()
