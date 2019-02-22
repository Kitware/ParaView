"""
Creating images that correspond to a set of queries by compositing the
results together. Used by viewer primarily.
"""
from __future__ import absolute_import
import itertools as it

from .querymaker import QueryMaker
from . import layer_rasters
import copy


class QueryMaker_SpecB(QueryMaker):
    '''
    This class translates UI queries into a set of Spec-B LayerRasters
    instances (multi-image).  This set of LayerRasters can be composited
    through the Compositor class (compositor.py).
    '''
    def __init__(self):
        super(QueryMaker_SpecB, self).__init__()

    def _QueryMaker__createBaseLayerFromQuery(self, query):
        '''
        Creates a base LayerRasters containing the values that are shared
        (global) by the set of user-selected layers (e.g. current time,
        camera, etc.). This base LayerRasters will be used as an initial
        template for all of the LayerRasters.

        These queries are assumed to have a single value. Queries containing
        multiple values need to be added separately in __generateQueriedLayers
        (...).
        '''
        baseLayer = layer_rasters.LayerRasters()
        parameter_list = self.store().parameter_list

        for name in query.keys():
            if (name in parameter_list) and (
                    "role" not in parameter_list[name]):
                values = query[name]
                if name == 'pose':
                    v = list(iter(values))
                else:
                    # assumed to have a single value
                    v = list(iter(values))[0]
                baseLayer.addToBaseQuery({name: v})

        return baseLayer

    def __generateQueriedLayers(self, query, globalBaseLayer):
        '''
        Creates all the LayerRasters instances, uses baseLayer as a common
        template (__createLayer(..) creates copies of it).

        Iterates over the "vis" parameters (assumes only those available items
        are to be displayed) and creates a LayerRasters for each based on all
        its necessary parameters. This method heavily relies on
        cinema_store::parameters_for_object() in order to resolve all the
        necessary parameters.

        visQuery is the top-level query (vis parameter), parQueries are
        queries of the mid-level parent filters wich control values affect the
        final parameter node, ownQueries account for the control
        values of the parameter itself.
        '''
        createdLayers = []

        # every visible pipeline item
        for paramName in (query["vis"] if ("vis" in query) else []):
            relatedParams = self.store().parameters_for_object(paramName)
            fieldName = relatedParams[1]
            dependeeControls = relatedParams[2]
            visQuery = {"vis": paramName}

            # pop its own name from the dependee list, create specific queries
            ownQueries = []
            if paramName in dependeeControls:
                dependeeControls.pop(dependeeControls.index(paramName))
                ownQueries = self.__generateSpecificBaseQueries(
                    query, paramName)

            # add the field to the list for the case when there is no controls
            # but only field value arrays
            if len(ownQueries) == 0:
                ownQueries += self.__generateSpecificBaseQueries(
                    query, fieldName)

            # set up each of the parent control queries (shared by each
            # target query)
            parQueries = []
            for control in dependeeControls:
                # TODO: make the [] brackets default to __genSpecificQueries
                parQueries += [
                    self.__generateSpecificBaseQueries(query, control)]

            if len(parQueries) > 0:
                allQueries = self.__traverseQueryTree(parQueries, ownQueries)
            else:
                allQueries = ownQueries

            for targetq in allQueries:
                totalBaseQueries = [visQuery] + (
                    [targetq] if isinstance(targetq, dict) else list(targetq))
                layer = self.__createLayer(
                    query, globalBaseLayer, totalBaseQueries, fieldName)
                layer.setCustomizationName(paramName)
                createdLayers.append(layer)

        return createdLayers

    def __traverseQueryTree(self, parQueries, ownQueries):
        '''
        Assembles iterable objects with all of the controls parameter
        combinations (including both the parent control values and its own
        control values). Uses map to apply the my_list customized list
        initializer and itertools.chain.from_iterable to chain together each
        layer's list of parameters to obtain a final list with all
        of the chained combinations.

        parQueries (parent control queries):
            [[{'Ctrl1' : a}, {'Ctrl1' : b}, {'Ctrl1' : c}, ...],
             [{'Ctrl2' : a}, {'Ctrl2' : b}, {'Ctrl2' : c}, ...], ...]

        ownQueries (parameter's own control values):
            [{'param' : a}, {'param' : b}, {'param' : c}, ...]

        output (all of the combinations)**:

            [[{'Ctrl1' : a}, {'Ctrl2' : a}, {'param' : a}],
             [{'Ctrl1' : a}, {'Ctrl2' : a}, {'param' : b}],
             [{'Ctrl1' : a}, {'Ctrl2' : a}, {'param' : c}], ...]

        Note(**): The elements of the output list are itertool objects which
        need to be evaluated by initializing a list (or any other iterable).

        Warning: this function assumes ownQueries and parQueries contain
        at least one element.  The caller should ensure this is always true.

        Note: See itertools docs for more information.
        (https://docs.python.org/3/library/itertools.html#itertools.product)'''

        def my_list(iterable_var):
            '''
            This is a customized list initializer which gives special
            treatment to dict types (e.g. queries). All iterables are
            initialized with list() while dictionaries are appended to a list.
            The intention is to be able to pass this function to map() and
            obtain a list of lists which is very simple to chain
            together with itertools.chain.from_iterable().'''
            if isinstance(iterable_var, dict):
                return [iterable_var]
            else:
                return list(iterable_var)

        # Compute the combinations between parent controls. it.product gives
        # the inner product between the two lists and returns sets which are
        # then converted back to lists. The resulting list-elements are
        # concatenated into a single list.
        current = map(my_list, parQueries.pop(0))
        for nextQuery in parQueries:
            current = map(
                my_list, it.product(current, map(my_list, nextQuery)))
            current = map(it.chain.from_iterable, current)
            # Force it.chain evaluation to avoid recursive referencing to a
            # single generator when doing the last product (ln. 170), as gens
            # evaluate only once.
            current = map(my_list, current)

        # Compute the combinations of the parent controls with the
        # parameter's own control values
        allQueries = map(
            my_list, it.product(current, map(my_list, ownQueries)))
        allQueries = map(it.chain.from_iterable, allQueries)

        return allQueries

    def __generateSpecificBaseQueries(self, query, control):
        '''
        Creates a set of queries based on its input parameters. These are all
        specific to a particular LayerRasters or group of LayerRasters.
        '''
        return [{control: value} for value in (
            query[control] if (control in query) else [])]

    def __createLayer(self, query, baseLayer, otherBaseQueries, fieldName):
        '''
        Creates a new layer by copying a 'baseLayer' and then including
        additional base queries ('otherBaseQueries'). It then adds all the fbo
        components specified in a field parameter ('fieldName') to this layer
        for later compositing.
        '''
        newLayer = copy.deepcopy(baseLayer)
        for bq in otherBaseQueries:
            newLayer.addToBaseQuery(bq)

        try:
            fieldParameter = self.store().parameter_list[fieldName]
            fieldValues = fieldParameter["values"]
            fieldTypes = fieldParameter["types"]
            fieldRanges = fieldParameter["valueRanges"] if (
                "valueRanges" in fieldParameter) else {}

            for index, value in enumerate(fieldValues):

                # TODO: when "rgb" a hack might be needed to add a single
                # color when several available
                # Note 4th condition: includes a query[] value if the
                # fieldName has been queried (necessary
                # for compatibility SpecB-testcase1
                if (fieldTypes[index] == "depth" or
                    fieldTypes[index] == "luminance" or
                    fieldTypes[index] == "rgb" or
                    ((value in query[fieldName])
                     if (fieldName in query) else False)):

                    # Actual value ranges will be used to normalize color lut
                    # if set

                    # Ensure value rasters are float and not invertible
                    if "value_mode" in self.store().metadata:
                        self.__floatValueRasters = self.store().metadata[
                            "value_mode"] == 2
                    else:
                        self.__floatValueRasters = False

                    if (value in fieldRanges) and self.__floatValueRasters:
                        newLayer.setValueRange(fieldRanges[value])

                    imageType = self.store().determine_type({fieldName: value})
                    # TODO imageType parameter can be resolved  with the
                    # other two within ::addQuery()
                    newLayer.addQuery(imageType, fieldName, value)

        except KeyError:
            raise KeyError(
                "The field parameter ", fieldName, " is not well defined!",
                "\n Types: ", fieldTypes,
                "\n Values: ", fieldValues,
                "\n Query: ", query,
                "\n Value: ", value, " INDEX: ", index)

        return newLayer

    def translateQuery(self, query, colorDefinitions={}):
        '''
        Receives a UI query and returns a set of LayerRasters consisting of all
        the different layers to composite. See the base class documentation
        for detail.
        '''
        # Create a common base LayerRasters (time and camera).
        baseLayer = self._QueryMaker__createBaseLayerFromQuery(query)
        # Create all the LayerRasters (TODO cache layers and other db info to
        # avoid re-creating them every time).
        allLayers = self.__generateQueriedLayers(query, baseLayer)
        self._QueryMaker__loadLayers(allLayers)

        return allLayers

    def supportsLayering(self):
        return True
