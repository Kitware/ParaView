"""
Module for handling color lookup tables.
"""

import json
import numpy as np
import math


class LookupTable:
    '''
    Color table container.
    - self.lut          : Actual color LUT.
    - self.x            : Vector containing the value bins.
    - self.adjustedBins : Vector containing the value bins
    Where adjustedBins is adjusted to hold a value > 1.0 in the end.
    '''
    def __init__(self):
        self.name = 'None'
        self.colorSpace = 'None'
        self.lut = None
        self.x = None
        self.adjustedBins = None

    def ingest(self, rgbPoints):
        xs = []
        tlut = []
        minx = float("inf")
        maxx = float("-inf")
        for i in xrange(0, len(rgbPoints), 4):
            x = rgbPoints[i]
            maxx = x if x > maxx else maxx
            minx = x if x < minx else minx
            xs.append(rgbPoints[i])
            rgb = (rgbPoints[i + 1],
                   rgbPoints[i + 2],
                   rgbPoints[i + 3])
            tlut.append(rgb)

        # Remap xs from [minx, max] -> [0, 1]
        scale = maxx - minx
        xs = [(xn - minx) / scale for xn in xs]

        for i in range(0, len(tlut)):
            tlut[i] = (tlut[i][0]*255, tlut[i][1]*255, tlut[i][2]*255)
        lut = np.array(tlut, dtype=np.uint8)
        self.lut = lut
        self.x = xs

        # use a histogram to support non-uniform colormaps (fetch bin indices)
        colorLutXValue = self.x
        bins = list(colorLutXValue)
        if colorLutXValue[-1] < 1.0:
            bins.append(1.0)
        else:
            bins.append(1.01)  # 1.0 gets its own color so need an extra bin
        self.adjustedBins = bins


class LookupTableManager:
    '''
    Loads a set of color lookup tables from a .json file.
    '''
    def __init__(self):
        self.luts = []
        self.lut = None
        self.x = None

        # Add a default color map ('None')
        self.luts.append(LookupTable())

    def read(self, file_path):
        with open(file_path) as json_file:
            data = json.load(json_file)
            for i in range(0, len(data)):
                lutentry = LookupTable()
                lutentry.name = data[i]['Name']
                lutentry.colorSpace = data[i]['ColorSpace']
                lutentry.ingest(data[i]['RGBPoints'])
                self.luts.append(lutentry)

    def getColorLutStructByName(self, name):
        for lut in self.luts:
            if lut.name == name:
                return lut
        return None

    def names(self):
        """
        Return an array of the names of all available lookup tables.
        """
        names = []
        for lut in self.luts:
            names.append(lut.name)
        return names


def add_spectral(luts):
    lutentry = {}
    lutentry['ColorSpace'] = 'RGB'
    lutentry['Name'] = 'Spectral'
    lutentry['NanColor'] = [
        0.6196078431372549, 0.00392156862745098, 0.2588235294117647]
    tlut = [
        0.0, 0.6196078431372549, 0.00392156862745098, 0.2588235294117647,
        0.1, 0.8352941176470589, 0.2431372549019608, 0.3098039215686275,
        0.2, 0.9568627450980393, 0.4274509803921568, 0.2627450980392157,
        0.3, 0.9921568627450981, 0.6823529411764706, 0.3803921568627451,
        0.4, 0.996078431372549, 0.8784313725490196, 0.5450980392156862,
        0.5, 1, 1, 0.7490196078431373,
        0.6, 0.9019607843137255, 0.9607843137254902, 0.596078431372549,
        0.7, 0.6705882352941176, 0.8666666666666667, 0.6431372549019608,
        0.8, 0.4, 0.7607843137254902, 0.6470588235294118,
        0.9, 0.196078431372549, 0.5333333333333333, 0.7411764705882353,
        1.0, 0.3686274509803922, 0.3098039215686275, 0.635294117647058
        ]
    lutentry['RGBPoints'] = tlut
    luts.append(lutentry)


def add_grayscale(luts):
    lutentry = {}
    lutentry['ColorSpace'] = 'RGB'
    lutentry['Name'] = "Grayscale"
    lutentry['NanColor'] = [0, 0, 0]
    s = 32.0
    tlut = [x / s for x in range(0, int(s)+1) for i in range(0, 4)]
    lutentry['RGBPoints'] = tlut
    luts.append(lutentry)


def add_rainbow(luts):
    lutentry = {}
    lutentry['ColorSpace'] = 'RGB'
    lutentry['Name'] = "Rainbow"
    lutentry['NanColor'] = [0, 0, 0]
    tlut = []
    for x in range(0, 64):
        r = 0
        g = 0
        b = 0
        a = (1.0-x/63.0)/0.25
        X = math.floor(a)
        Y = a-X
        if X == 0:
            r = 1.0
            g = Y
            b = 0
        if X == 1:
            r = 1.0-Y
            g = 1.0
            b = 0
        if X == 2:
            r = 0
            g = 1.0
            b = Y
        if X == 3:
            r = 0
            g = 1.0-Y
            b = 1.0
        if X == 4:
            r = 0
            g = 0
            b = 1.0
        tlut.append(x / 63.0)
        tlut.append(r)
        tlut.append(g)
        tlut.append(b)
    lutentry['RGBPoints'] = tlut
    luts.append(lutentry)


def add_ocean(luts):
    lutentry = {}
    lutentry['ColorSpace'] = 'RGB'
    lutentry['Name'] = 'Ocean'
    lutentry['NanColor'] = [0, 0, 0]
    tlut = [
        0.0, 0.039215, 0.090195, 0.25098,
        0.125, 0.133333, 0.364706, 0.521569,
        0.25, 0.321569, 0.760784, 0.8,
        0.375, 0.690196, 0.960784, 0.894118,
        0.5, 0.552941, 0.921569, 0.552941,
        0.625, 0.329412, 0.6, 0.239216,
        0.75, 0.211765, 0.349020, 0.078435,
        0.875, 0.011765, 0.207843, 0.023525,
        1.0, 0.286275, 0.294118, 0.30196
        ]
    lutentry['RGBPoints'] = tlut
    luts.append(lutentry)

if __name__ == "__main__":
    luts = []
    add_spectral(luts)
    add_grayscale(luts)
    add_rainbow(luts)
    add_ocean(luts)

    formatted = json.dumps(
        luts, sort_keys=True, indent=2,
        separators=(',', ': '))
    with open('builtin_tables.json', 'w') as output:
        output.write(formatted)
