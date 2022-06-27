from enum import Enum

class CISPARAMS(Enum):
        image           = "CISImage"
        version         = "CISVersion" 
        imageFlags      = "CISImageFlags"
        imageWidth      = "CISImageWidth"
        imageHeight     = "CISImageHeight"
        layer           = "CISLayer"
        layerOffsetX    = "CISLayerOffsetX"
        layerOffsetY    = "CISLayerOffsetY"
        layerWidth      = "CISLayerWidth"
        layerHeight     = "CISLayerHeight"
        channel         = "CISChannel"
        channelVar      = "CISChannelVar"
        channelVarType  = "CISChannelVarType"
        channelVarMin   = "CISChannelVarMin"
        channelVarMax   = "CISChannelVarMax"
        channelColormap = "CISChannelColormap"
        shadowChannel   = "CISShadow"
        depthChannel    = "CISDepth"

        @staticmethod
        def contains(value):
            if value in [e.value for e in CISPARAMS]: 
                return True
            else:
                return False

class cisview:
    """Composible Image Set View Class
       
       Given a cinema database, this class provides access to 
       CIS data within the database.

       We note that CIS entries are expected to use Cinema's FILE column
       to record any files needed.
    """

    @property
    def depth(self):
        return self._depth

    @depth.setter
    def cis(self, value):
        self._depth = value        

    @property
    def shadow(self):
        return self._shadow

    @shadow.setter
    def cis(self, value):
        self._shadow = value        


    def __init__(self, cdb):
        self.cdb = cdb
        self.parameters = []
        self.CISParams = []

        # state
        self.images = []
        self.layers = [] 
        self.channels = {} 

        # find the CIS parameters that are in the database 
        params = cdb.get_parameter_names()
        for p in params:
            if CISPARAMS.contains(p):
                self.CISParams.append(p)
            else:
                self.parameters.append(p)

        # get layer and channel information
        self.images     = self.__get_image_names()
        self.layers     = self.__get_layer_names()
        self.channels   = self.__get_channel_names()
        self._depth     = self.__query_depth()
        self._shadow    = self.__query_shadow()

    def get_cdb_parameters(self):
        return self.parameters

    def get_cis_parameters(self):
        return self.CISParams

    def __get_image_names(self):
        query = "SELECT DISTINCT CISImage from {}".format(self.cdb.tablename)
        result = self.cdb.execute(query)

        names = []
        for row in result: 
            names.append(str(row[0]))

        return names

    def __get_layer_names(self):
        query = "SELECT DISTINCT CISLayer from {}".format(self.cdb.tablename)
        result = self.cdb.execute(query)

        names = []
        for row in result: 
            names.append(str(row[0]))

        return names

    def __get_channel_names(self):
        results = {} 

        for l in self.layers:
            results[l] = []
            query = "SELECT DISTINCT CISChannel from {} WHERE CISLayer = \'{}\'".format(self.cdb.tablename, l)
            result = self.cdb.execute(query)

            for row in result: 
                results[l].append(str(row[0]))

        return results

    def __query_depth(self):
        query = "SELECT DISTINCT CISChannel from {}".format(self.cdb.tablename)
        result = self.cdb.execute(query)

        names = []
        for row in result: 
            names.append(str(row[0]))

        return "CISDepth" in names 

    def __query_shadow(self):
        query = "SELECT DISTINCT CISChannel from {}".format(self.cdb.tablename)
        result = self.cdb.execute(query)

        names = []
        for row in result: 
            names.append(str(row[0]))

        return "CISShadow" in names 

    def get_image_layers(self, image):
        query = "SELECT DISTINCT CISLayer from {} WHERE CISImage = \'{}\'".format(self.cdb.tablename, image)
        result = self.cdb.execute(query) 

        names = []
        for row in result: 
            names.append(str(row[0]))

        return names

    def get_layer_channels(self, image, layer):
        query = "SELECT DISTINCT CISChannel from {} WHERE CISImage = \'{}\' AND CISLayer = \'{}\'".format(self.cdb.tablename, image, layer)
        result = self.cdb.execute(query) 

        names = []
        for row in result: 
            names.append(str(row[0]))

        return names


    def get_channel_extracts(self, image, layer):
        extracts = []
        for channel in self.channels[layer]:
            ext = self.get_channel_extract(image, layer, channel)
            for e in ext:
                extracts.append(e)

        return extracts

    def get_channel_extract(self, image, layer, channel):
        extracts = []
        params = {
                    "CISImage": image,
                    "CISLayer": layer,
                    "CISChannel": channel 
                 }
        extract = self.cdb.get_extracts(params)

        return extract

    #
    #
    #
    def get_image_parameters(self):
        query = "SELECT CISImageWidth, CISImageHeight from {} LIMIT 1".format(self.cdb.tablename)
        results = self.cdb.execute(query)
        data = {
                    "dims": [results[0][0], results[0][1]]
                }

        return data

    #
    #
    #
    def get_layer_parameters(self, image, layer):
        data = {
                    "dims"  : [0.0, 0.0],
                    "offset": [0.0, 0.0] 
               }

        # both must be present, or they are ignored
        if ("CISLayerWidth" in self.CISParams) and ("CISLayerHeight" in self.CISParams):
            query = "SELECT CISLayerWidth, CISLayerHeight from {} WHERE CISImage = \'{}\' and CISLayer = \'{}\' LIMIT 1".format(self.cdb.tablename, image, layer)
            results = self.cdb.execute(query)
            data["dims"] = [results[0][0], results[0][1]] 

        else:
            params = self.get_image_parameters()
            data["dims"] = params["dims"] 

        # both must be present, or they are ignored
        if ("CISLayerOffsetX" in self.CISParams) and ("CISLayerOffsetY" in self.CISParams):
            query = "SELECT CISLayerOffsetX, CISLayerOffsetY from {} WHERE CISImage = \'{}\' and CISLayer = \'{}\' LIMIT 1".format(self.cdb.tablename, image, layer)
            results = self.cdb.execute(query)
            data["offset"] = [results[0][0], results[0][1]] 

        return data

    #
    # get variable, variable min/max and colormap from a channel if the data exists
    # 
    # Variable: required
    # Min, Max are set to default, if not present (optional)
    # Colormap are set to default, if not present (optional)
    #
    def get_channel_parameters(self, image, layer, channel):
        # set the default colormap value, in preparation for the next logic about the colormap
        data = { 
                    "variable": {
                        "name"  : None, 
                        "type"  : "float",
                        "min"   : -100000.0,
                        "max"   :  100000.0
                    },
                    "colormap": {
                        "type" : "default"
                    }
               }

        if ("CISChannelVar" in self.CISParams):
            query = "SELECT CISChannelVar from {} WHERE CISImage = \'{}\' and CISLayer = \'{}\' and CISChannel = \'{}\'".format(
                        self.cdb.tablename, image, layer, channel)
            results = self.cdb.execute(query)
            data["variable"]["name"] = results[0][0]

        if ("CISChannelVarType" in self.CISParams):
            query = "SELECT CISChannelVarType FROM {} WHERE CISImage = \'{}\' and CISLayer = \'{}\' and CISChannel = \'{}\'".format(
                        self.cdb.tablename, image, layer, channel)
            results = self.cdb.execute(query)

            data["variable"]["type"] = results[0][0]

        if ("CISChannelVarMin" in self.CISParams) and ("CISChannelVarMax" in self.CISParams):
            query = "SELECT CISChannelVarMin, CISChannelVarMax FROM {} WHERE CISImage = \'{}\' and CISLayer = \'{}\' and CISChannel = \'{}\'".format(
                        self.cdb.tablename, image, layer, channel)
            results = self.cdb.execute(query)

            data["variable"]["min"] = results[0][0]
            data["variable"]["max"] = results[0][1] 

        if "CISChannelColormap" in self.CISParams:
            query = "SELECT CISChannelColormap FROM {} WHERE CISImage = \'{}\' and CISLayer = \'{}\' and CISChannel = \'{}\'".format(
                        self.cdb.tablename, image, layer, channel)
            results = self.cdb.execute(query)

            if not results[0][0] is "":
                data["colormap"]["type"] = "url"
                data["colormap"]["url"] = results[0][0]
                    
        return data

    #
    # get the image for a parameter set
    #
    def get_image(self, params):
        query = "SELECT DISTINCT CISImage from {} WHERE ".format(self.cdb.tablename)
        
        first = True
        for key in params: 
            if not first:
                query = query + " AND "
            else:
                first = False
            value = params[key]
            query = query + "{} = \'{}\' ".format(key, value)
        results = self.cdb.execute(query)

        retval = results[0][0]
        if len(results) != 1:
            print("ERROR: incorrect return from image query")

        return retval 

