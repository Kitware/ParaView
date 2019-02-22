"""
module for generating cinema databases from arbitrary codes.
Essentially this provides a way to runs over all possible combinations of
values in database and call out to arbitrary codes so that they have a chance
to make corresponding changes before inserting the result into the database.
"""
from __future__ import absolute_import

from ..database import store


class Explorer(object):
    """
    Middleman that connects an arbitrary producing codes to the CinemaStore.
    The purpose of this class is to run through the parameter sets, and tell a
    set of tracks (in order) to do something with the parameter values
    it cares about.
    """

    def __init__(
        self,
        store,
        parameters,  # the things that this explorer is responsible for
        tracks  # the things we pass off values to in order to do the work
    ):
        self.__store = store
        self.parameters = parameters
        self.tracks = tracks

    @property
    def store(self):
        return self.__store

    def list_parameters(self):
        """
        parameters is an ordered list of parameters that the Explorer varies
        over
        """
        return self.parameters

    def prepare(self):
        """ Give tracks a chance to get ready for a run """
        if self.tracks:
            for e in self.tracks:
                e.prepare(self)

    def execute(self, desc):
        # Create the document/data product for this sample.
        doc = store.Document(desc)
        for e in self.tracks:
            # print ("EXECUTING track ", e, doc.descriptor)
            e.execute(doc)
        self.insert(doc)

    def explore(self, fixedargs=None, progressObject=None):
        """
        Explore the problem space to populate the store being careful not to
        hit combinations where dependencies are not satisfied.
        Fixed arguments are the parameters that we want to hold constant in
        the exploration.
        """
        self.prepare()

        for descriptor in self.store.iterate(
                self.list_parameters(), fixedargs, progressObject):
            self.execute(descriptor)

        self.finish()

    def finish(self):
        """ Give tracks a chance to clean up after a run """
        if self.tracks:
            for e in self.tracks:
                e.finish()

    def insert(self, doc):
        self.store.insert(doc)


class Track(object):
    """
    abstract interface for things that can produce data

    to use this:
    caller should set up some visualization
    then tie a particular set of parameters to an action with a track
    """

    def __init__(self):
        pass

    def prepare(self, explorer):
        """ subclasses get ready to run here """
        pass

    def finish(self):
        """ subclasses cleanup after running here """
        pass

    def execute(self, document):
        """ subclasses operate on parameters here"""
        pass


class LayerControl(object):
    """
    Prototype for something that Layer track can control
    """
    def __init__(self, name, showFunc, hideFunc):
        self.name = name
        self.callShow = showFunc
        self.callHide = hideFunc


class Layer(Track):
    """
    A track that connects a layer to the set of objects in the scene that it
    controls.
    """
    def __init__(self, layer, objectlist):
        super(Layer, self).__init__()
        self.parameter = layer
        # objlist is an array of class instances, they must have a name and
        # show and hide method, use LayerControl to make them.
        self.objectlist = objectlist

    def execute(self, doc):
        o = None
        if self.parameter in doc.descriptor:
            o = doc.descriptor[self.parameter]
        for obj in self.objectlist:
            if obj.name == o:
                try:
                    obj.callShow()  # method
                except TypeError:
                    obj.callShow(obj)  # function
            else:
                try:
                    obj.callHide()
                except TypeError:
                    obj.callHide(obj)
