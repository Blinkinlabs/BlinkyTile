import random

# Note! color order is b,g,r!
def makePixel(r,g,b):
    data = ''
    data += chr(int(b))
    data += chr(int(g))
    data += chr(int(r))
    return data

class shimmer(object):
    stepSize = 5.0
    ledMax = 255.0
    
    def __init__(self, ledCount):
        self.ledCount = ledCount

        self.values = list(0 for i in range(0,self.ledCount))
        self.maxValues = list(0 for i in range(0, self.ledCount))
        self.deaths = list(0 for i in range(0, self.ledCount))
        self.directions = list(0 for i in range(0, self.ledCount))

        for i in range(0, self.ledCount):
            self.values[i] = 0
            self.maxValues[i] = random.randrange(0,self.ledMax)
            self.deaths[i] = self.maxValues[i] + self.stepSize / 2 + (.5 * random.randrange(0,self.ledMax))
            self.directions[i] = 1

    def getData(self):
        accelerated_step = 0
        unit = 0

        data = ''
        for i in range(0, self.ledCount):
            unit = self.maxValues[i] / self.ledMax
            accelerated_step = self.stepSize + (self.ledMax * (0.015 * self.stepSize * unit * unit * unit * unit))

            if self.directions[i] == 1:
                if self.values[i] < self.maxValues[i]:
                    self.values[i] += accelerated_step

                    if self.values[i] > self.ledMax:
                        self.values[i] = self.ledMax
                else:
                    self.directions[i] = 0
            else:
                if self.values[i] > 0:
                    self.deaths[i] = self.deaths[i] - self.stepSize
                    self.values[i] = self.values[i] - self.stepSize

                    if self.values[i] < 0:
                        self.values[i] = 0

                else:
                    self.deaths[i] = self.deaths[i] - self.stepSize
                    if(self.deaths[i] < 0):
                        self.directions[i] = 1
                        self.maxValues[i] = random.randint(0,self.ledMax)
                        self.deaths[i] = self.maxValues[i] + self.stepSize / 2 + (.5*random.randint(0,self.ledMax))

            data += makePixel(self.values[i], self.values[i], self.values[i])

        return data

if __name__ == "__main__":

    import blinkytape
    import time

    animation = shimmer(14*3)
    bt = blinkytape.BlinkyTape()
    while True:
        data = animation.getData()

        for pixel in range(0, 14*3):
            bt.sendPixel(ord(data[pixel*3+0]), ord(data[pixel*3+1]), ord(data[pixel*3+2]))
        bt.show()

        time.sleep(.03)
