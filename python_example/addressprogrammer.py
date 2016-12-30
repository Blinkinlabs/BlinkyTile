#!/usr/bin/python

import blinkytape
import time
import argparse

def flashLed(led, ontime = 1, offtime = .5, ledCount = 10):
    # Flash an LED with R,G,B,W colors
    # ontime and offtime are amounts of time for the lights to be on for each color

    print("Flashing pixel Red")
    for pos in range(0, ledCount):
        if pos == led:
            bt.sendPixel(100,0,0)
        else:
            bt.sendPixel(0,0,0)
    bt.show()
    time.sleep(ontime)
    bt.show()
    time.sleep(offtime)

    print("Flashing pixel Green")
    for pos in range(0, ledCount):
        if pos == led:
            bt.sendPixel(0,100,0)
        else:
            bt.sendPixel(0,0,0)
    bt.show()
    time.sleep(ontime)
    bt.show()
    time.sleep(offtime)

    print("Flashing pixel Blue")
    for pos in range(0, ledCount):
        if pos == led:
            bt.sendPixel(0,0,100)
        else:
            bt.sendPixel(0,0,0)
    bt.show()
    time.sleep(ontime)
    bt.show()
    time.sleep(offtime)

    print("Flashing pixel White")
    for pos in range(0, ledCount):
        if pos == led:
            bt.sendPixel(100,100,100)
        else:
            bt.sendPixel(0,0,0)
    bt.show()
    time.sleep(ontime)
    bt.show()
    time.sleep(offtime)

def interactiveMode():
    while True:
        print("")
        address = input("enter an address to program:")
        address = int(address) - 1
   
        print("Programming LED to address %i"%(address)) 
        bt.programAddress(address)
        time.sleep(1)

        flashLed(address)

parser = argparse.ArgumentParser("BlinkyTile Address Programmer")
parser.add_argument('--address', help='address to program the tile', type=int)
args = parser.parse_args()


bt = blinkytape.BlinkyTape()

# Address specified, program it then stop
if args.address!=None:
        print("Programming LED to address %i"%(args.address))
        bt.programAddress(args.address-1)
        time.sleep(1)
        flashLed(args.address-1)
    

# No command specified, go into interactive mode
else:
    interactiveMode()
