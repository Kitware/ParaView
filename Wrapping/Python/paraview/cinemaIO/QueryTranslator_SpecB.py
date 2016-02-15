from QueryTranslator import QueryTranslator
import cinema_store
from LayerSpec import *


class QueryTranslator_SpecB(QueryTranslator):
    ''' This class translates queries coming from a selection in the
    viewer (only supports Viewer 2.0) into a set of layers to be composed. '''
    def __init__(self):
        super(QueryTranslator_SpecB, self).__init__()

    def _QueryTranslator__createBaseLayerFromQuery(self, query):
        ''' Creates a base query (a base LayerSpec used to create all the Layers).
        This base layer holds the static parameters (e.g. current time and camera).'''
        baseLayer = LayerSpec()
        otherLayers = []
        for name in query.keys():

            if (name in self.store().parameter_list and
                not self.store().isdepender(name) and
                not self.store().islayer(name)):

                values = query[name]
                v = list(iter(values))[0] #no options in query, so only 1 result not many
                baseLayer.addToBaseQuery({name:v})

            else:
                otherLayers.append(name) #not static, thus maybe a top level

        return baseLayer, otherLayers

    def __generateQueriedLayers(self, query, baseLayer, otherQueriedNames):
        ''' Generates a set of LayerSpecs depending on the queried parameters and sets
        its name 'parameter/value'. It currently supports two different cases:

        1. When the queried parameter name is a layer with values (e.g. Clip1, Slice1, etc...;
        layers with numerical values). In this case it finds the corresponding field and creates
        LayersSpecs for it.

        2. When the queried parameter name is a field itself with 'value type' values (e.g. colorCalculator1;
        fields with value arrays). In this case the field is known, so  it follows the association
        chain recursively to add these as base queries.
        '''
        createdLayers = []
        for argName in otherQueriedNames:
            # Case 1.
            if (self.store().islayer(argName)):
                dependerNames = self.store().getdependers(argName)
                for dName in dependerNames:
                    if self.store().isfield(dName):

                        #for each queried value (e.g. Clip1 : [0, -0.1]) add a set of fbo components of the field
                        for value in query[argName]:
                            layer = self.__createLayer(query, baseLayer, {argName : value}, dName)
                            layer.setLayerName(argName + "/" + str(value))
                            createdLayers.append(layer)

            # Case 2.
            elif (self.store().isfield(argName)):

                otherBaseQueries = self.__generateAdditionalBaseQueries(argName)
                #for each queried value (e.g. Calculator1 : [coord_x, ..] add a set of fbo components.
                for value in query[argName]:
                    layer = self.__createLayer(query, baseLayer, otherBaseQueries, argName)
                    layer.setLayerName(argName + "/" + str(value))
                    createdLayers.append(layer)

        return createdLayers

    def __generateAdditionalBaseQueries(self, argName):
        ''' Generates additional base queries required to display what a user specified.
        It generates them by exploring the dependees (associations) recursively.'''
        dependeesNames = self.store().getdependees(argName)
        foundBaseQueries = {}

        def recursivelyAddDependencies(dependerName, currentDependees, result):
            for dName in currentDependees:
                requiredValue = self.store().getDependeeValue(dependerName, dName)
                result[dName] = requiredValue
                #print "->>> recAddDep | current level: ", dependerName, " -> ", dName

                nextDependees = self.store().getdependees(dName)
                if len(nextDependees) > 0:  # TODO watch for the maximum recursion level
                    recursivelyAddDependencies(dName, nextDependees, result)

        recursivelyAddDependencies(argName, dependeesNames, foundBaseQueries)

        return foundBaseQueries

    def __createLayer(self, query, baseLayer, otherBaseQueries, fieldName):
        ''' Creates a new layer by copying a 'baseLayer' and then including additional
        base queries ('otherBaseQueries'). It then adds all the fbo components specified in
        a field parameter ('fieldName') to this layer for later compositing.
        '''
        newLayer = copy.deepcopy(baseLayer)
        newLayer.addToBaseQuery(otherBaseQueries)

        try:
            fieldParameter = self.store().parameter_list[fieldName]
            fieldValues = fieldParameter["values"]
            fieldTypes = fieldParameter["types"]

            for index, value in enumerate(fieldValues):

                # TODO: when "rgb" a hack might be needed to add a single color when several available
                # Note 4th condition: includes a query[] value if the fieldName has been queried (necessary
                # for compatibility SpecB-testcase1
                if fieldTypes[index] == "depth" or \
                   fieldTypes[index] == "luminance" or \
                   fieldTypes[index] == "rgb" or \
                   ((value in query[fieldName]) if (fieldName in query) else False):

                    #print "->>> Adding query to layer: ", fieldName, ", ", value
                    imageType = self.store().determine_type({fieldName:value})
                    # TODO imageType parameter can be resolved  with the other two within ::addQuery()
                    newLayer.addQuery(imageType, fieldName, value)

        except KeyError:
            raise KeyError("The field parameter ", fieldName, " is not well defined!", \
             "\n Types: ", fieldTypes, "\n Values: ", fieldValues, "\n Query: ", query,\
             "\n Value: ", value, " INDEX: ", index)

        return newLayer

    def translateQuery(self, query):
        ''' Receives a query and returns a set of LayerSpecs consisting of all the
        different layers to composite. '''
        # resolve the base layer (time and camera). this also cleans up the query
        baseLayer, otherQueriedNames = self._QueryTranslator__createBaseLayerFromQuery(query)
        # create the layers  # TODO cache layers to not have to re-create them every time
        allLayers = self.__generateQueriedLayers(query, baseLayer, otherQueriedNames)
        self._QueryTranslator__loadLayers(allLayers)

        return allLayers

    def supportsLayering(self):
        return True
