#
# layer class
#
class layer:
    """Composible Image Set layer class

    A layer contains:
    - a set of channels.
        - one channel may be depth
        - one channel may be shadow
    - dimensions: integer width and height.
    - offset: integer x, y dimension from the image origin.
    """

    def __init__(self, name):
        self.depth = None
        self.shadow = None

    @property
    def name(self):
        return self._name

    @name.setter
    def name(self, value):
        self._name = value

    @property
    def channel(self):
        return self._channel

    @channel.setter
    def channel(self, value):
        self._channel = value

    @property
    def dims(self):
        return self._dims

    @dims.setter
    def dims(self, value):
        self._dims = value

    @property
    def offset(self):
        return self._offset

    @offset.setter
    def offset(self, value):
        self._offset = value

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
