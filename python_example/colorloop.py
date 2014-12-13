import math

# Note! color order is b,g,r!
def makePixel(r,g,b):
    data = ''
    data += chr(int(b))
    data += chr(int(g))
    data += chr(int(r))
    return data

class colorloop(object):
    stepSize = 5.0
    ledMax = 255.0
    
    def __init__(self, ledCount, rBalance, gBalance, bBalance):
        self.ledCount = ledCount

        self.rBalance = rBalance
        self.gBalance = gBalance
        self.bBalance = bBalance

        self.j = 0
        self.f = 0
        self.k = 0

    def getData(self):
        data = ''
        for i in range(0, self.ledCount):
            r = 128*(1+math.sin(i/10.0 + self.j/4.0  )) * self.rBalance
            g = 128*(1+math.sin(i/7.0 + self.f/9.0 + 2.1)) * self.gBalance
            b = 128*(1+math.sin(i/12.0 + self.k/14.0 + 4.2)) * self.bBalance


            data += makePixel(r,g,b)

        self.j += .3
        self.f += .3
        self.k += .5

        return data

if __name__ == "__main__":

    import blinkytape
    import time

    animation = colorloop(14*3,1,1,1)
    bt = blinkytape.BlinkyTape()

    i = 0
    while True:
        data = animation.getData()

        for pixel in range(0, 14*3):
            bt.sendPixel(ord(data[pixel*3+0]), ord(data[pixel*3+1]), ord(data[pixel*3+2]))
        bt.show()


        i += 1
        if i > 985:
            animation = colorloop(14*3,1,1,1)
            i = 0

        if i == 100:
            i = 900
            print "ok"
            a = raw_input()

        time.sleep(.03)
