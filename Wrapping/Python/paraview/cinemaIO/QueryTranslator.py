import abc
import cinema_store
from LayerSpec import *


class QueryTranslator(object):
    __metaclass__ = abc.ABCMeta
    ''' Translates ui queries into a set of LayerSpecs. Abstract class, Derive
    from this class to implement different query specifications.'''
    def __init__(self):
        self.__store = None

    def __loadLayers(self, layers):
        ''' Loads the required images in the layers. '''
        #print "->>> Loading layers, received: ", layers
        #send queries to the store to obtain images
        lcnt = 0
        for l in range(0,len(layers)):
            #print "->>> Loading Image:  ",  layers[l].dict, " \n", layers[l]._fields
            layers[l].loadImages(self.__store)
            #print "loaded layer", lcnt,
            #layers[l].printme()
            lcnt +=1

    @abc.abstractmethod
    def __createBaseLayerFromQuery(self, query):
        ''' Create a base LayerSpec on which to 'base' all of the necessary
        layers for rendering. Abstract, implement in a derived class.'''
        return

    @abc.abstractmethod
    def supportsLayering(self):
        ''' Predicate to define if a translator supports layering. This is useful
        only while providing support to Spec-A.'''
        return

    @abc.abstractmethod
    def translateQuery(self, query, colorDefinitions = {}):
        ''' Receives a query and returns a set of LayerSpecs consisting of all
        the layers to render. Abstract, implement in a derived class.'''
        return

    def setStore(self, store):
        self.__store = store

    def store(self):
        return self.__store


class QueryTranslator_SpecA(QueryTranslator):
    ''' This class translates queries coming from a selection in the
    viewer (only supports Viewer 2.0) into a set of Spec-A layers. '''
    def __init__(self):
        super(QueryTranslator_SpecA, self).__init__()

    def _QueryTranslator__createBaseLayerFromQuery(self, query):

        baseLayer = LayerSpec()
        for name in query.keys():
            values = query[name]
            v = list(iter(values))[0] #no options in query, so only 1 result not many
            baseLayer.addToBaseQuery({name:v})

        return baseLayer

    def supportsLayering(self):
        return False

    def translateQuery(self, query, colorDefinitions = {}):
        ''' Receives a query and returns a set of LayerSpecs consisting of all the
        different layers to composite. '''
        # resolve the base layer (time and camera). this also cleans up the query
        baseLayer = self._QueryTranslator__createBaseLayerFromQuery(query)
        # the rest of the framework expects a layer of lists
        layerList = [baseLayer]
        self._QueryTranslator__loadLayers(layerList)

        return layerList
