from . import imageview
from . import cisview
from . import renderer
from . import convert


class cis:
    """Composible Image Set Class
    The data structure to hold properties of a Composible Image Set.
    """

    def __init__(self, filename):
        """ The constructor. """
        self.fname          = filename
        self.classname      = "COMPOSABLE_IMAGE_SET"
        self.dims           = [0,0]
        self.flags          = "CONSTANT_CHANNELS"
        self.version        = "1.0"
        self.parameterlist  = []
        self.parametertable = None
        self.variables      = {}
        self.images         = {}
        self.colormaps      = {}

    def debug_print(self):
        """ Debug print statement for CIS properties. """
        print("printing cis")
        print("  fname:     {}".format(self.fname))
        print("  classname: {}".format(self.classname))
        print("  dims:      {}".format(self.dims))
        print("  flags:     {}".format(self.flags))
        print("  version:   {}".format(self.version))
        print("  colormaps: ")
        for m in self.colormaps:
            print(m)

        for i in self.get_images():
            print("    image: {}".format(self.get_image(i).name))
            for l in self.get_image(i).get_layers():
                print("      layer: {}".format(self.get_image(i).get_layer(l).name))

        print("\n")

    def get_image(self, key):
        """ Returns an image given its key. """
        result = False
        if key in self.images:
            result = self.images[key]
        return result

    def get_images(self):
        """ Returns all images. """
        for i in self.images:
            yield i

    def get_image_names(self):
        """ Returns list of image names. """
        return list(self.images.keys())

    def set_parameter_table(self, table):
        """ Set parameter table using a deep copy. """
        self.parametertable = table.copy(deep=True)

    def add_parameter(self, name, type):
        """ Add a parameter to the list of parameters for the CIS. """
        # check for duplicates
        self.parameterlist.append([name, type])

    def add_variable(self, name, type, min, max):
        """ Add a variable to the set of variables. """
        # check for duplicates
        self.variables[name] = {'type':type, 'min':min, 'max':max}

    def add_image(self, name):
        """ Add an image to the set of images in the CIS. """
        # check for duplicates
        self.images[name] = image.image(name)

        return self.images[name]

    def get_variables(self):
        """ Return all variables. """
        for i in self.variables:
            yield i

    def get_variable(self, name):
        """ Return a variable. """

        variable = None
        if name in self.variables:
            variable = self.variables[name]

        return variable

    def get_image(self,name):
        """ Return an image. """
        image = None

        if name in self.images:
            image = self.images[name]

        return image

    def get_colormap(self,name):
        """ Return a colormap. """
        colormap = None

        if name in self.colormaps:
            colormap = self.colormaps[name]

        return colormap

    def add_colormap(self, name, path):
        """ Add a colormap to the set of colormaps. """
        #if colormap not in dict
        if (name not in self.colormaps):
            self.colormaps[name] = colormap.colormap(path)

    def remove_colormap(self, name):
        """ Remove a colormap from the set of colormaps. """
        self.colormaps.pop(name)

    def get_colormaps(self):
        """ Return all colormaps. """
        for i in self.colormaps:
            yield i

    def set_dims(self, w, h):
        """ Set the dimensions of the CIS given a width and height. """  
        self.dims = [w, h]
