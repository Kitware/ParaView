"""
API for creating images that correspond to a set of queries.
Used by viewer primarily.
"""
from __future__ import absolute_import
from . import layer_rasters
import abc


class QueryMaker(object):
    __metaclass__ = abc.ABCMeta
    '''
    Translates UI queries into a set of LayerRasters. Abstract class, Derive
    from this class to implement different query specifications. The UI queries
    are expected to fully define the image file paths.
    '''
    def __init__(self):
        self.__store = None

    def __loadLayers(self, layers):
        ''' Loads the required images in the LayerRasters instances.'''
        # send queries to the store to obtain images
        lcnt = 0
        for l in range(0, len(layers)):
            layers[l].loadImages(self.__store)
            lcnt += 1

    @abc.abstractmethod
    def __createBaseLayerFromQuery(self, query):
        '''
        Creates a base LayerRasters containing the values that are shared
        (global) by the set of user-selected layers (e.g. current time,
        camera, etc.). This base LayerRasters will be used as an initial
        template for all of the LayerRasters. Abstract, implement in a derived
        class.
        '''
        return

    @abc.abstractmethod
    def supportsLayering(self):
        '''
        Predicate to query if a translator supports compositing (e.g. Spec-B).
        Abstract, implement in a derived class.
        '''
        return

    @abc.abstractmethod
    def translateQuery(self, query):
        '''
        Receives a set of parameters from the UI. The parameters (or UI
        queries) come in the following form:

        total_query = { "parameter_1" : set([values_of_1]),
                        "parameter_2" : set([values_of_2]), ... }

        Example:

        total_query = { "phi" : set([-180]),
                        "theta" : set([0]),
                        "Slice2" : set([-0.34, -0.1, 0.1]) }

        The queries are passed to the LayerRasters instances which then use
        cinema_store to parse these queries into image file paths. This method
        is expected to return a set of LayerRasters consisting of all the
        layers to render (each LayerRasters instance holds all of its required
        images). Abstract, implement in a derived class.
        '''
        return [layer_rasters.LayerRasters()]

    def setStore(self, store):
        self.__store = store

    def store(self):
        return self.__store


class QueryMaker_SpecA(QueryMaker):
    '''
    This class translates UI queries into a single Spec-A LayerRasters instance
    (single-image).
    '''
    def __init__(self):
        super(QueryMaker_SpecA, self).__init__()

    def _QueryMaker__createBaseLayerFromQuery(self, query):

        baseLayer = layer_rasters.LayerRasters()
        for name in query.keys():
            values = query[name]
            v = list(iter(values))[0]  # no options in query, so only 1 result
            baseLayer.addToBaseQuery({name: v})

        return baseLayer

    def supportsLayering(self):
        return False

    def translateQuery(self, query):
        # create a base layer (which in this case is the only one)
        baseLayer = self._QueryMaker__createBaseLayerFromQuery(query)
        # pass it as a list (for compatibility)
        layerList = [baseLayer]
        self._QueryMaker__loadLayers(layerList)

        return layerList
