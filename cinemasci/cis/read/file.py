import numpy
import os
import json
import glob

class cisfile():
    attrname = "attributes.json"
    dataname = "data.npz"

    def __init__(self, cis):
        self.cis = cis
        return

    def dump(self, dumpfile):
        dumpfile.write("{}/\n".format(self.cis.fname))
        if os.path.isfile(self._get_attribute_file()):
            dumpfile.write("  {}\n".format(cisfile.attrname))
        dumpfile.write("  colormaps/\n")
        for c in self.get_colormaps():
            basename = os.path.basename(c)
            dumpfile.write("    {}\n".format(basename))
        dumpfile.write("  image/\n")
        for i in self.get_images():
            dumpfile.write("    {}/\n".format(i))
            dumpfile.write("      layer/\n")
            for l in self.get_layers(i):
                dumpfile.write("        {}/\n".format(l))
                if os.path.isfile(self._get_layer_attribute_file(i, l)):
                    dumpfile.write("          {}\n".format(cisfile.attrname))
                dumpfile.write("          channel/\n")
                for c in self.get_channels(i, l):
                    dumpfile.write("            {}/\n".format(c))
                    if os.path.isfile(self._get_channel_attribute_file(i, l, c)):
                        dumpfile.write("              {}\n".format(cisfile.attrname))
                    if os.path.isfile(self._get_channel_data_file(i, l, c)):
                        dumpfile.write("              {}\n".format(cisfile.dataname))

    def get_colormaps(self):
        colormaps = glob.glob(os.path.join(self._get_colormap_basedir(), "*"))
        for c in sorted(colormaps):
            yield c

    def get_images(self):
        images = glob.glob(os.path.join(self._get_image_basedir(), "*"))
        for i in sorted(images):
            yield os.path.basename(i)

    def get_layers(self, iname):
        layers = glob.glob(os.path.join(self._get_layer_basedir(iname), "*"))
        for l in sorted(layers):
            yield os.path.basename(l)

    def get_channels(self, iname, lname):
        channels = glob.glob(os.path.join(self._get_channel_basedir(iname, lname), "*"))
        for c in sorted(channels):
            yield os.path.basename(c)

    def verify(self):
        result = True

        if not os.path.isdir( self.cis.fname ) or not os.path.isdir( self._get_image_basedir() ):
            result = False

        return result

    def _get_colormap_basedir(self):
        return os.path.join(self.cis.fname, "colormaps" )

    def _get_image_basedir(self):
        return os.path.join( self.cis.fname, "image" )

    def _get_layer_basedir(self, iname):
        return os.path.join( self.cis.fname, "image", iname, "layer" )

    def _get_channel_basedir(self, iname, lname):
        return os.path.join( self.cis.fname, "image", iname, "layer", lname, "channel" )

    def _get_layer_dir(self):
        return os.path.join( self.cis.fname, "image", iname, "layer" )

    def _get_attribute_file(self):
        return os.path.join( self.cis.fname, cisfile.attrname ) 

    def _get_layer_attribute_file(self, iname, lname):
        return os.path.join( self.cis.fname, "image", iname, "layer", lname, cisfile.attrname ) 

    def _get_channel_attribute_file(self, iname, lname, cname):
        return os.path.join( self.cis.fname, "image", iname, "layer", lname, "channel", cname, cisfile.attrname ) 

    def _get_channel_data_file(self, iname, lname, cname):
        return os.path.join( self.cis.fname, "image", iname, "layer", lname, "channel", cname, cisfile.dataname ) 

    def _read_attributes(self, attrfile):
        attributes = {}
        if os.path.isfile(attrfile):
            with open(attrfile) as afile:
                attributes = json.load(afile)
        else:
            print("ERROR loading attributes file: {}".format( attrfile ))
            exit(1)

        return attributes



class reader(cisfile):
    """ A file-based CIS Reader. """
    def __init__(self, cis):
        self.cis = cis
        return

    def read(self):
        # read attributes
        attributes = self._read_attributes(self._get_attribute_file()) 

        # required attributes
        self.cis.classname = attributes["classname"]
        self.cis.dims      = attributes["dims"]
        self.cis.version   = attributes["version"]
        # optional attributes
        if "flags" in attributes:
            self.cis.flags     = attributes["flags"]
        if "origin" in attributes:
            self.cis.origin    = attributes["origin"]

        for colormapPath in self.get_colormaps():
            self.__read_colormap(colormapPath)

        for image in self.get_images():
            self.__read_image(image)

        return
    
    def __read_colormap(self, cPath):
        file_extension = os.path.splitext(cPath)[1]
        name = os.path.splitext(os.path.basename(cPath))[0]
        if (file_extension == ".xml"):
            newcolormap = self.cis.add_colormap(name, cPath)
        if (file_extension == ".json"):
            if os.path.isfile(cPath):
                with open(cPath) as jFile:
                    data = json.load(jFile)
            newcolormap = self.cis.add_colormap(name, data['url'])


    def __read_image(self, iname):
        newimage = self.cis.add_image(iname)

        for layer in self.get_layers( iname ):
            self.__read_layer(newimage, layer)

    def __read_layer(self, image, lname):
        newlayer = image.add_layer(lname)

        # read attributes
        attributes = self._read_attributes(self._get_layer_attribute_file( image.name, lname ))
        # set attributes
        if "offset" in attributes:
            newlayer.offset = attributes["offset"]
        if "dims" in attributes:
            newlayer.dims   = attributes["dims"]

        for channel in self.get_channels( image.name, lname ):
            self.__read_channel(image.name, newlayer, channel)

    def __read_channel(self, iname, layer, cname):
        newchannel = layer.add_channel(cname)

        # read attributes
        attributes = self._read_attributes(self._get_channel_attribute_file( iname, layer.name, cname ))
        # set attributes
        newchannel.type = attributes["type"]
        # read data
        datafile = self._get_channel_data_file(iname, layer.name, cname)
        if os.path.isfile(datafile):
            zdata = numpy.load(datafile)
            newchannel.data = zdata['data']

