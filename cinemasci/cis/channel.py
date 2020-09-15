import numpy

class channel:
    """Channel Class

    A channel is a set of values, the size of the layer that contains it (wxh) 
    and it relative to the *layer* it is a part of. 
    It can be of any type, the default if of type float. 
    A channel can contain **depth** or **lighting** information.
    A channel may reference a variable or colormap to use for rasterization.  
    """

    def __init__(self, name):
        self.name = name
        self.type = "float" 
        self.data = None
        self.dims = [0,0]

    def set_type(self, type):
        self.type = type

    def set_dims(self, w, h):
        self.dims = [w, h]

    def create_test_data(self):
        self.data = numpy.random.random_sample(self.dims)
