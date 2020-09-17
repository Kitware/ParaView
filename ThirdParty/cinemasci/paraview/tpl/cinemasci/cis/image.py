from . import layer

class image:
    """Image Class
    A logical collection of data, as a MxN array of values intended to be
    rasterized into a color image. 
    An image has a known origin, given by UL (upper left), UR (upper right),
    LL (lower left) and LR (lower right).
    An image has dimensions defined by 2 integers (MxN).
    An image contains one or more layers.  
    """

    def __init__(self, name):
        self.name   = name
        self.layers = {} 

    def add_layer(self, name):
        self.layers[name] = layer.layer(name)

        return self.layers[name]

    def get_layers(self):
        for l in self.layers:
            yield l

    def get_layer(self, name):
        result = False
        if name in self.layers:
            result = self.layers[name]

        return result
