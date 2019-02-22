"""
Manages the set of one or more fields that go into a layer.
"""

import copy


class LayerRasters(object):
    def __init__(self):
        self.depth = None
        self.luminance = None
        self.colors = []
        self.values = []
        self.dict = {}
        self._fields = {}
        self.__customizationName = ""
        self.__valueRange = None

    def addToBaseQuery(self, query):
        """ add queries that together define the layer """
        self.dict.update(query)

    def addQuery(self, img_type, fieldname, fieldchoice):
        """ add a query for a particular field of the layer """
        self._fields[img_type] = {fieldname: fieldchoice}

    def loadImages(self, store):
        """
        Take the queries we've been given and get images for them.
        Later call get* to get the images out.
        """
        nfields = len(self._fields)
        if nfields == 0:
            img = store.get(self.dict).data
            self._addColor(img)
        else:
            for f in self._fields.keys():
                query = copy.deepcopy(self.dict)
                query.update(self._fields[f])

                foundPaths = store.get(query)
                if foundPaths is None:
                    return

                img = foundPaths.data

                if f == 'RGB':
                    self._addColor(img)
                elif f == 'Z':
                    self._setDepth(img)
                elif f == 'VALUE' or f == 'MAGNITUDE':
                    self._addValues(img)
                elif f == 'LUMINANCE':
                    self._setLuminance(img)

    def _setDepth(self, image):
        self.depth = image

    def getDepth(self):
        return self.depth

    def _addColor(self, image):
        self.colors.append(image)

    def _addValues(self, image):
        self.values.append(image)

    def hasColorArray(self):
        return (len(self.colors) > 0)

    def hasValueArray(self):
        return (len(self.values) > 0)

    # TODO, do we still need indices for the rgb arrays? or would it always
    # be one?
    def getColorArray(self, index=0):
        c = None
        if index < len(self.colors):
            c = self.colors[index]

        return c

    def getValueArray(self, index=0):
        v = None
        if index < len(self.values):
            # print "->>> getValueArray: available ", len(self.values)
            v = self.values[index]

        return v

    def _setLuminance(self, image):
        self.luminance = image

    def getLuminance(self):
        return self.luminance

    def setCustomizationName(self, name):
        self.__customizationName = name

    @property
    def customizationName(self):
        return self.__customizationName

    def setValueRange(self, vRange):
        self.__valueRange = vRange

    @property
    def valueRange(self):
        return self.__valueRange
