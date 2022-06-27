import os
import numpy

#
# channel class
#
class channel:
    """Composible Image Set channel class

    A channel is:
    - a name
    - a variable name
    - a variable min
    - a variable max
    - a variable type
    - a two dimensional array of values. 
    """
    def __init__(self):
        self.data = None

    @property
    def name(self):
        return self._name

    @name.setter
    def name(self, value):
        self._name = value

    @property
    def colormap(self):
        """ A colormap is a dictionary with at least one entry, 
            'points', which is a list of dicts each of which is expected 
            to have five elements:

            'r': r float value
            'g': g float value
            'b': b float value
            'x': x position float value
            'a': alpha float value

            If there is no entry "colorspace", then "rgb" is assumed.
        """
        return self._colormap

    @colormap.setter
    def colormap(self, value):
        self._colormap = value

    @property
    def url(self):
        return self._url

    @url.setter
    def url(self, value):
        self._url = value

    @property
    def active(self):
        return self._active

    @active.setter
    def active(self, value):
        self._active = value

    @property
    def data(self):
        return self._data

    @data.setter
    def data(self, value):
        self._data = value

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, value):
        self._type = value

    @property
    def var(self):
        return self._var

    @var.setter
    def var(self, value):
        self._var = value

    @property
    def varmin(self):
        return self._varmin

    @varmin.setter
    def varmin(self, value):
        self._varmin = value

    @property
    def varmax(self):
        return self._varmax

    @varmax.setter
    def varmax(self, value):
        self._varmax = value

    @property
    def vartype(self):
        return self._vartype

    @var.setter
    def vartype(self, value):
        self._vartype = value

    def load(self, url):
        self.url = url
        if os.path.isfile(url):
            zdata = numpy.load(url)
            self.data = zdata['data']
