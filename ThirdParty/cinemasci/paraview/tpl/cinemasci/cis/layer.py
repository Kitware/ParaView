from . import channel

class layer:
    """Layer Class
    A layer is a collection of values that comprise an element of an image. 
    A layer is an integer size (wxh) that is equal to or lesser than the size of the image. 
    Therefore the offset from the image origin must be given in integer form to describe its placement.
    A layer contains one or more channels. 
    """

    def __init__(self, name):
        self.name     = name
        self.offset   = [0,0]
        self.dims     = [0,0]
        self.channels = {}

    def set_offset(self, x, y):
        self.offset = [x, y]

    def set_dims(self, w, h):
        self.dims = [w, h]

    def add_channel(self, name):
        self.channels[name] = channel.channel(name)
        self.channels[name].set_dims(self.dims[0], self.dims[1])

        return self.channels[name]

    def get_channels(self):
        for c in self.channels:
            yield c

    def get_channel(self, name):
        result = False
        if name in self.channels:
            result = self.channels[name]
        return result
