import h5py
import numpy

class Reader:
    """ A HDF5 CIS Reader. """
    def __init__(self):
        return

    def read(self, cis):
        with h5py.File(cis.fname, "r") as f:
            cis.classname  = f.attrs["class"]
            cis.dims       = numpy.array(list(map(int, f.attrs["dims"].split(","))))
            cis.version    = f.attrs["version"]
            cis.flags      = f.attrs["flags"]

            for i in f["image"]:
                im = cis.add_image(i)
                self.read_image(im, f["image"][i])

    
    def read_image(self, image, data):
        for l in data["layer"]:
            layer = image.add_layer(l)
            # print("  l:{}".format(l))
            self.read_layer(layer, data["layer"][l])
        return

    def read_layer(self, layer, data):
        if "offset" in data.attrs.keys():
            offset = numpy.array(list(map(int, data.attrs["offset"].split(","))))
            layer.set_offset(offset[0], offset[1])
        if "dims" in data.attrs.keys():
            dims = numpy.array(list(map(int, data.attrs["dims"].split(","))))
            layer.set_dims(dims[0], dims[1])

        for c in data["channel"]:
            channel = layer.add_channel(c)
            # print("    c:{}".format(c))
        return
