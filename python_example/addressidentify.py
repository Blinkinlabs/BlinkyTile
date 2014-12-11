import blinkytape
import time

bt = blinkytape.BlinkyTape()

while True:
    address = input("enter an address to identify:")
    address = int(address) - 1
    
    for pos in range(0, 14):
        if pos == address:
            bt.sendPixel(100,100,100)
        else:
            bt.sendPixel(0,0,0)
    bt.show()
    time.sleep(.1)
    bt.show()
