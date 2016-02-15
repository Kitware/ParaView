from QueryTranslator import QueryTranslator
from QueryTranslator_SpecB import QueryTranslator_SpecB
import cinema_store
from LayerSpec import *


class QueryTranslator_SpecC(QueryTranslator_SpecB):
    ''' This class translates queries coming from a selection in the
    viewer (only supports Viewer 2.0) into a set of layers to be composed. '''
    def __init__(self):
        super(QueryTranslator_SpecC, self).__init__()

    def _QueryTranslator__createBaseLayerFromQuery(self, query):
        ''' Creates a base query (a base LayerSpec used to create all the Layers).
        This base layer holds the static parameters (e.g. current time and camera).'''
        baseLayer = LayerSpec()
        otherLayers = []
        for name in query.keys():

            if (name in self.store().parameter_list and
                not self.store().isdepender(name)):

                values = query[name]
                v = list(iter(values))[0] #no options in query, so only 1 result not many
                baseLayer.addToBaseQuery({name:v})

            else:
                otherLayers.append(name) #not static, thus maybe a top level

        return baseLayer, otherLayers

    def _QueryTranslator_SpecB__generateQueriedLayers(self, query, baseLayer, otherQueriedNames, colorDefinitions):
        ''' Generates a set of LayerSpecs depending on the queried parameters. It currently
        supports two different cases:
        1. When the queried parameter name is a layer with values (e.g. Clip1, Slice1, etc...;
        layers with numerical values). In this case it finds the corresponding field and creates
        LayersSpecs for it.

        2. When the queried parameter name is a field itself with 'value type' values (e.g. colorCalculator1;
        fields with value arrays). In this case the field is known, so  it follows the association
        chain recursively to add these as base queries.
        '''
        createdLayers = []
        for argName in otherQueriedNames:
            parameter_ = self.store().parameter_list[argName]
            # Case 1.
            if (parameter_["role"] == "control"):

                dName = self.store().getRelatedField(argName)
                if dName == None:
                    continue

                #for each queried value (e.g. Clip1 : [0, -0.1]) add a set of fbo components of the field
                for value in query[argName]:
                    layer = self._QueryTranslator_SpecB__createLayer(query, baseLayer, {argName : value}, dName)
                    layer.setLayerName(argName + "/" + str(value))
                    createdLayers.append(layer)

            # Case 2.   (field arguments need to be added alone only if there is no parameter related to it)
            elif (parameter_["role"] == "field") and  \
                 not self.store().hasRelatedParameter(argName):

                #for each queried value (e.g. Calculator1 : [coord_x, ..] add a set of fbo components.
                for value in query[argName]:
                    layer = self._QueryTranslator_SpecB__createLayer(query, baseLayer, {argName: value}, argName)
                    layer.setLayerName(argName + "/" + str(value))
                    createdLayers.append(layer)

        return createdLayers

    def translateQuery(self, query, colorDefinitions = {}):
        baseLayer, otherQueriedNames = self._QueryTranslator__createBaseLayerFromQuery(query)
        allLayers = self._QueryTranslator_SpecB__generateQueriedLayers(query, baseLayer, otherQueriedNames, colorDefinitions)
        self._QueryTranslator__loadLayers(allLayers)

        return allLayers
