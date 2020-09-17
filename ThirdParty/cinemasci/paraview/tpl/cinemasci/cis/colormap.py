import xml.etree.ElementTree as ET
from urllib.request import urlopen
import os

class colormap:
    """Colormap Class
    A layer is a collection of values that comprise of the colormap. 
    A colormap is usually a list of object, each object in the form (x, r, g, b) where x is a value 
    0 and 1 and r,g,b normalized values between zero and one that correspond to the red, green and blue 
    color channels. 
    Are we considering alpha channels in colormaps?
    What is the dimension of a colormap?
    """

    def __init__(self, pathToFile):
        """Colormap constructor"""
        self.pathToFile = pathToFile
        self.name = os.path.splitext(os.path.basename(pathToFile))[0]
        self.points = []
        self.typeXML = True
        self.edited = False

        #todo: check if file exists
        urlCheck = pathToFile[0:4]
        if (urlCheck == 'http'):
            self.typeXML = False
            pathToFile = urlopen(pathToFile)
        tree = ET.parse(pathToFile)

        self.root = tree.getroot()
        self.__indent(self.root)

#        for cmap in self.root.findall('ColorMap'):
#            self.name = cmap.get('name')

        for point in self.root.iter('Point'):
            value = point.get('x')
            alpha = point.get('o')
            red   = point.get('r')
            green = point.get('g')
            blue  = point.get('b')
            self.points.append((float(value), float(alpha),
                                float(red), float(green), float(blue)))

    def __indent(self, elem, level=0):
        i = "\n" + level*"  "
        if len(elem):
            if not elem.text or not elem.text.strip():
                elem.text = i + "  "
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
            for elem in elem:
                self.__indent(elem, level+1)
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
        else:
            if level and (not elem.tail or not elem.tail.strip()):
                elem.tail = i


    def get_points(self):
        """Return a list of tuples that define the colormap points
        """
        return self.points

    def add_point(self, point):
        """ Add a point, a tuple of (x,o,r,g,b) to points. """
        self.edited = True
        self.points.append(point)
        self.points.sort()
        #to do, if x value of point already exists, replace or return error?
        return

    def remove_point(self, point):
        """ Remove a point, a tuple of (x,o,r,g,b) from points. """
        self.edited = True
        if point in points:
            self.points.remove(point)
        # if point not in points, indicate that with an error?
        return


