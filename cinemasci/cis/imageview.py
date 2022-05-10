from . import layer
from . import channel
import json
import os

#
# imageview class
#
class imageview:
    """ImageView Class

    A collection of settings that define a specific way of compositing the
    elements of an image. Once it is set up, the imageview's layers can
    be iterated over to return an order-dependent set of data plus colormaps
    that can be composited.
    """

    @property
    def background(self):
        return self._background

    @background.setter
    def background(self, color):
        if (len(color) == 3) and (all(isinstance(value, (float, int)) for value in color)):
            self._background = color
        else:
            print("ERROR: color arg must be three values, each either an int or float") 
            self._background = [0.0, 0.0, 0.0]

    @property
    def use_depth(self):
        return self._use_depth

    @use_depth.setter
    def use_depth(self, value):
        self._use_depth = value

    @property
    def use_shadow(self):
        return self._use_shadow

    @use_shadow.setter
    def use_shadow(self, value):
        self._use_shadow = value

    @property
    def depth(self):
        return self._depth

    @depth.setter
    def depth(self, value):
        self._depth = value

    @property
    def shadow(self):
        return self._shadow

    @shadow.setter
    def shadow(self, value):
        self._shadow = value

    @property
    def image(self):
        return self._image

    @image.setter
    def image(self, value):
        self._image = value

    @property
    def dims(self):
        return self._dims

    @dims.setter
    def dims(self, value):
        self._dims = value

    def __init__(self, cview):
        self.active_layers = []
        self.active_channels = {} 
            # a CIS view of a cinema database
        self.cisview = cview
        self.data = {}
        self.use_depth = False
        self.use_shadow = False
        self.background = [0.0, 0.0, 0.0]

    def get_active_layers(self):
        return self.active_layers

    def activate_layer(self, layer):
        if not layer in self.active_layers:
            self.active_layers.append(layer)

    def deactivate_layer(self, layer):
        if layer in self.active_layers:
            self.active_layers.remove(layer)

    def is_active_layer(self, layer):
        return layer in self.active_layers

    #
    # will activate a channel in an inactive layer
    #
    def activate_channel(self, layer, channel):
        self.active_channels[layer] = channel

    def get_active_channel_data(self, layer):
        channel = self.get_active_channel(layer) 
        data = self.cisview.get_image(self.image).get_layer(layer).get_channel(channel).data
        return data 

    def get_layer(self, layer):
        # TODO: error check
        return self.data[self.image]

    def get_channel(self, layername):
        # TODO: error check
        return self.data[layername].channel

    def get_layer_dims(self, layer):
        return self.cisview.get_image(self.image).get_layer(layer).dims 

    def get_layer_offset(self, layer):
        return self.cisview.get_image(self.image).get_layer(layer).offset 

    def get_active_channel(self, layer):
        results = None

        if layer in self.active_channels:
            results = self.active_channels[layer] 

        return results    

    def get_variable_range(self, layer):
        var = self.cisview.get_variable(self.get_active_channel(layer))

        data = [var["min"], var["max"]] 
        if var["type"] == "float":
            data = [float(var["min"]), float(var["max"])]
        elif var["type"] == "float":
            data = [int(var["min"]), int(var["max"])]

        return data 

    def update(self):
        imdata = self.cisview.get_image_parameters()
        self.dims   = imdata["dims"]

        # TODO: error if image not set
        for l in self.active_layers:
            ldata = self.cisview.get_layer_parameters(self.image, l)
            newlayer = layer.layer(l)
            newlayer.name = l
            newlayer.dims = ldata["dims"]
            newlayer.offset = ldata["offset"]
            self.data[l] = newlayer

            extract = self.cisview.get_channel_extract(self.image, l, self.active_channels[l])
            cdata = self.cisview.get_channel_parameters(self.image, l, self.active_channels[l])
            newchannel = channel.channel()
            newchannel.name = self.active_channels[l]
            newchannel.load(extract[0])
            newchannel.colormap = self.load_colormap(cdata["colormap"])
            newchannel.var      = cdata["variable"]["name"]
            newchannel.vartype  = cdata["variable"]["type"]
            newchannel.varmin   = cdata["variable"]["min"]
            newchannel.varmax   = cdata["variable"]["max"]
            newlayer.channel = newchannel

            # load the depth map
            if self.use_depth:
                extract = self.cisview.get_channel_extract(self.image, l, "CISDepth") 
                newchannel = channel.channel()
                newchannel.name = "CISDepth" 
                newchannel.load(extract[0])
                newlayer.depth = newchannel

            # load the shadow map
            if self.use_shadow:
                extract = self.cisview.get_channel_extract(self.image, l, "CISShadow") 
                # did the data load?
                # this is equivalent to asking if the channel is there
                # TODO: find a better way to express requesting a load, but getting
                #       no data, i.e. the channel is not there
                if extract:
                    newchannel = channel.channel()
                    newchannel.name = "CISShadow"
                    newchannel.load(extract[0])
                    newlayer.shadow = newchannel

    def get_layer_data(self):
        return self.data

    def load_colormap(self, params):
        # return a default gray colormap if nothing else
        colormap = {
                    "colorspace" : "rgb",
                    "name" : "default",
                    "points" : [{'x': 0.0, 'r': 0.0, 'g': 0.0, 'b': 0.0, 'a': 1.0},
                                {'x': 1.0, 'r': 1.0, 'g': 1.0, 'b': 1.0, 'a': 1.0},
                               ]
                   }

        if "type" in params:
            # for now, parse as a local file
            # TODO: add logic to detect and load remote URLs
            if params["type"] is "url":
                # this path is currently required to be a ParaView json colormap
                # local to the cinema database 
                fullpath = os.path.join(self.cisview.cdb.path, params["url"])

                colormap["colorspace"] = "rgb"
                colormap["points"] = []
                with open(fullpath) as cmfile:
                    data = json.load(cmfile)
                    numpoints = int(len(data[0]["RGBPoints"])/4)
                    for i in range(numpoints):
                        colormap["points"].append(
                                                   {'x': data[0]["RGBPoints"][i*4],
                                                    'r': data[0]["RGBPoints"][i*4+1],
                                                    'g': data[0]["RGBPoints"][i*4+2],
                                                    'b': data[0]["RGBPoints"][i*4+3],
                                                    'a': 1.0 
                                                   }
                                                 )

        return colormap


