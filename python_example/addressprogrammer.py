#!/usr/bin/env python3

import blinkytape
import time
import argparse

def flashLed(led, ontime = 1, offtime = .5, ledCount = 163):
    # Flash an LED with R,G,B,W colors
    # ontime and offtime are amounts of time for the lights to be on for each color

    if ledCount < led:
        ledCount = led

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
    nextAddress = 1
    while True:
        print("")
        address = input(f"enter an address to program [{nextAddress}]: ")
        if address:
            address = int(address)
        else:
            address = nextAddress

        startChannel = (address - 1)
 
        print(f"Programming LED to address {address}")
        bt.programAddress(startChannel)
        time.sleep(1)

        flashLed(startChannel,.3,.1)
        nextAddress = address + 1

parser = argparse.ArgumentParser("BlinkyTile Address Programmer")
parser.add_argument('--address', help='address to program the tile', type=int)
args = parser.parse_args()


bt = blinkytape.BlinkyTape()

# Address specified, program it then stop
if args.address!=None:
        print(f"Programming LED to address {args.address}")
        bt.programAddress(args.address-1)
        time.sleep(1)
        flashLed(args.address-1)
    

# No command specified, go into interactive mode
else:
    interactiveMode()
