import os
import numpy
import xml.etree.ElementTree as ET
import json

# TODO: parametertable

class file_writer:
    """ A disk file CIS writer. """
    Attribute_file = "attributes.json"

    def write(self, cis):
        self.__create_toplevel_dir(cis)
        self.__write_class_metadata(cis)

        path = self.__create_toplevel_image_dir(cis)
        for i in cis.get_images():
            self.__write_image(path, cis.get_image(i))

        path = self.__create_toplevel_colormap_dir(cis)
        for i in cis.get_colormaps():
            self.__write_colormap(path, cis.get_colormap(i))

        path = self.__create_toplevel_variable_dir(cis)
        for i in cis.get_variables():
            vdata = cis.get_variable(i)
            self.__write_variable(path, i, vdata)

    def __create_toplevel_channel_dir(self, path):
        path = os.path.join( path, "channel" )
        try:
            os.mkdir(path)
        except OSError:
            print("Creation of {} failed".format(path))

        return path 

    def __create_channel_dir(self, path, channel):
        path = os.path.join( path, channel.name )
        try:
            os.mkdir(path)
        except OSError:
            print("Creation of {} failed".format(path))

        return path 

    def __create_toplevel_layer_dir(self, path, image):
        path = os.path.join( path, image.name, "layer" )
        try:
            os.mkdir(path)
        except OSError:
            print("Creation of {} failed".format(path))

        return path 

    def __create_layer_dir(self, path, layer):
        path = os.path.join( path, layer.name )
        try:
            os.mkdir(path)
        except OSError:
            print("Creation of {} failed".format(path))

        return path 


    def __create_image_dir(self, path, image):
        path = os.path.join( path, image.name )
        try:
            os.mkdir(path)
        except OSError:
            print("Creation of {} failed".format(path))

        return path

    def __create_toplevel_image_dir(self, cis):
        path = os.path.join( cis.fname, "image" )
        try:
            os.mkdir(path)
        except OSError:
            print("Creation of {} failed".format(path))

        return path

    def __create_toplevel_colormap_dir(self, cis):
        path = os.path.join( cis.fname, "colormaps" )
        try:
            os.mkdir(path)
        except OSError:
            print("Creation of {} failed".format(path))

        return path

    def __create_toplevel_variable_dir(self, cis):
        path = os.path.join( cis.fname, "variables" )
        try:
            os.mkdir(path)
        except OSError:
            print("Creation of {} failed".format(path))

        return path

    def __create_toplevel_dir(self, cis):
        path = cis.fname
        try:
            os.makedirs(cis.fname)
        except OSError:
            print("Creation of {} failed".format(cis.fname))

        return path

    def __write_class_metadata(self, cis):
        with open(os.path.join( cis.fname, self.Attribute_file), "w") as f:
            f.write("{\n")
            f.write("  \"classname\" : \"{}\",\n".format(cis.classname))
            f.write("  \"dims\"      : [{}, {}],\n".format(cis.dims[0], cis.dims[1]))
            f.write("  \"version\"   : \"{}\",\n".format(cis.version))
            f.write("  \"flags\"     : \"{}\",\n".format(cis.flags))
            f.write("  \"origin\"    : \"{}\"\n".format(cis.origin))
            f.write("}\n")

    def __write_layer_metadata(self, path, layer):
        with open(os.path.join( path, self.Attribute_file), "w") as f:
            f.write("{\n")
            f.write("  \"offset\" : [{}, {}],\n".format(layer.offset[0], layer.offset[1]))
            f.write("  \"dims\"   : [{}, {}]\n".format(layer.dims[0], layer.dims[1]))
            f.write("}\n")

    def __write_channel_metadata(self, path, channel):
        with open(os.path.join( path, self.Attribute_file), "w") as f:
            f.write("{\n")
            f.write("  \"type\" : \"{}\"\n".format(channel.type))
            f.write("}\n")

    def __write_channel_data(self, path, channel):
        path = os.path.join(path, "data")
        numpy.savez_compressed(path, data=channel.data) 

    def __write_image(self, path, image):
        self.__create_image_dir(path, image)
        path = self.__create_toplevel_layer_dir(path, image)

        for l in image.get_layers():
            self.__write_layer(path, image.get_layer(l))

    def __write_layer(self, path, layer):
        path = self.__create_layer_dir(path, layer)
        self.__write_layer_metadata(path, layer) 
        path = self.__create_toplevel_channel_dir(path)

        for c in layer.get_channels():
            self.__write_channel(path, layer.get_channel(c))

    def __write_channel(self, path, channel):
        path = self.__create_channel_dir(path, channel)
        self.__write_channel_metadata(path, channel)
        self.__write_channel_data(path, channel)

    def __write_variable(self, path, variable, vdata):
        vpath = os.path.join(path, variable + ".json") 
        with open(vpath, 'w') as f:
            f.write("{\n")
            f.write("    \"type\" : \"{}\",\n".format(vdata['type']))
            f.write("    \"min\"  : \"{}\",\n".format(vdata['min']))
            f.write("    \"max\"  : \"{}\",\n".format(vdata['max']))
            f.write("}\n")

    def __write_colormap(self, path, colormap):
        if (colormap.typeXML):
            if (colormap.edited):
                data = ET.Element('ColorMaps')
                items = ET.SubElement(data, 'Colormap')
                items.set('name', colormap.name)
                for p in colormap.points:
                    item = ET.SubElement(items, 'Point')
                    #data will be saved alphabetically, not in this order
                    item.set('x', str(p[0]))
                    item.set('o', str(p[1]))
                    item.set('r', str(p[2]))
                    item.set('g', str(p[3]))
                    item.set('b', str(p[4]))

                xmlData = ET.tostring(data, encoding="unicode")
            else: #colormap is not edited
                xmlData = ET.tostring(colormap.root, encoding="unicode")

            path = os.path.join(path, colormap.name)
            cmpFile = open(path+'.xml', "w")
            cmpFile.write(xmlData)
            cmpFile.close()

        else: #file type is json
            if (colormap.edited):
                data = ET.Element('ColorMaps')
                items = ET.SubElement(data, 'Colormap')
                items.set('name', colormap.name)
                for p in colormap.points:
                    item = ET.SubElement(items, 'Point')
                    #data will be saved alphabetically, not in this order
                    item.set('x', str(p[0]))
                    item.set('o', str(p[1]))
                    item.set('r', str(p[2]))
                    item.set('g', str(p[3]))
                    item.set('b', str(p[4]))

                xmlData = ET.tostring(data, encoding="unicode")
            else: #colormap is not edited
                path = os.path.join(path, colormap.name)
                json_dict = {"url" : colormap.pathToFile}
                with open(path+'.json', 'w') as json_file:
                    json.dump(json_dict, json_file)


        #xmlData = ET.tostring(data)
        #cmpFile = open(path+'.xml', "wb")
        #cmpFile.write(xmlData)
        #cmpFile.close()

