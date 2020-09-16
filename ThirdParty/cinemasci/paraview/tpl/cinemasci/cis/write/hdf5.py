import h5py

class hdf5_writer:
    """ A HDF5 CIS writer. """
    def write(self, cis):
        with h5py.File(cis.fname, "w") as f:
            f.attrs["class"]    = cis.classname
            f.attrs["dims"]     = "{},{}".format(cis.dims[0], cis.dims[1])
            f.attrs["version"]  = cis.version
            f.attrs["flags"]    = cis.flags
            f.attrs["origin"]   = cis.origin

            vlist = f.create_group("variables")
            for v in cis.variables:
                var = vlist.create_group(v)
                values = cis.variables[v]
                var.attrs["type"] = values['type'] 
                var.attrs["min"]  = values['min']
                var.attrs["max"]  = values['max']

            self.write_cis_parameter_table(cis, f)

            imagepath = f.create_group("image")
            self.write_cis_images(cis, imagepath)


    def write_cis_parameter_table(self, cis, h5file):
        if not cis.parametertable is None:
            data = cis.parametertable
            table = h5file.create_group("parametertable")
            table.attrs["columns"] = ','.join(data.columns)
            table.attrs["num_rows"] = data.shape[0]
            table.attrs["num_cols"] = data.shape[1]
            cols = table.create_group("columns")
            for col in data.columns:
                cols.create_dataset( col, data=data[col].values, 
                                     dtype=h5py.string_dtype(encoding='ascii') 
                                   )

    def write_cis_images(self, cis, imagepath):
        for i in cis.images:
            curImage = cis.images[i]
            image = imagepath.create_group(curImage.name)
            layerpath = image.create_group("layer")
            self.write_image_layers(curImage, layerpath)


    def write_image_layers(self, image, layerpath):
        for l in image.layers:
            curLayer = image.layers[l]
            layer = layerpath.create_group(curLayer.name)
            layer.attrs["offset"] = str(curLayer.offset[0]) + "," + str(curLayer.offset[1])
            layer.attrs["dims"]   = str(curLayer.dims[0]) + "," + str(curLayer.dims[1])
            channelpath = layer.create_group("channel")
            self.write_channels(curLayer, channelpath)

    def write_channels(self, layer, channelpath):
        for c in layer.channels:
            curChannel = layer.channels[c]
            channelpath.create_dataset(curChannel.name, shape=curChannel.dims, data=curChannel.data)


