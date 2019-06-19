"""
Module defining classes and methods for managing cinema data storage.
"""
from __future__ import absolute_import

import json
import itertools
try:
    from itertools import izip as zip
except ImportError: #use py 3's native zip
    pass

def py23iteritems(d):
    myit = None
    try:
        myit = d.iteritems()
    except AttributeError:
        myit = d.items()
    return myit

import re
from . import raster_wrangler


class Document(object):
    """
    This refers to a document in the cinema data storage. A document is
    uniquely identified by a 'descriptor'. A descriptor is a dictionary
    with key-value pairs, where key is a parameter name and value is the
    value for that particular parameter.
    TODO:
    A document can have arbitrary data (as 'data') and meta-data (as
    'attributes') associated with it. At the moment we are ignoring the
    attributes.
    """
    def __init__(self, descriptor, data=None):
        self.__descriptor = descriptor
        self.__data = data
        self.__attributes = None

    @property
    def descriptor(self):
        """
        A document descriptor is a unique identifier for the document. It is
        a dict with key value pairs. The descriptor cannot be changed once the
        document has been instantiated.
        """
        return self.__descriptor

    @property
    def data(self):
        """Data associated with the document."""
        return self.__data

    @data.setter
    def data(self, val):
        self.__data = val

    @property
    def attributes(self):
        """Attributes are arbitrary meta-data associated with the document.
        If no attributes are present, it is set to None. When present,
        attributes are a dict with arbitrary meta-data relevant to the
        application.
        """
        return self.__attributes

    @attributes.setter
    def attributes(self, attrs):
        self.__attributes = attrs


class Store(object):
    """
    API for cinema stores. A store is a collection of Documents,
    with API to add, find, and access them. This class is an abstract class
    defining the API and storage independent logic. Storage specific
    subclasses handle the 'database' access.

    The design of cinema store is based on the following principles:

    The store comprises of documents (Document instances). Each document has a
    unique set of parameters, aka a "descriptor" associated with it. This
    can be thought of as the 'unique key' in database terminology.

    One defines the parameters (contents of the descriptor) for documents
    on the store itself. The set of them is is referred to as 'parameter_list'.
    One uses 'add_parameter()' calls to add new parameter definitions for a new
    store instance.

    Users insert documents in the store using 'insert'. One can find
    document(s) using 'find' which returns a generator (or cursor) allow users
    to iterate over all match documents.
    """

    def __init__(self):
        self.__metadata = None
        self.__parameter_list = {}
        self.__loaded = False
        self.__parameter_associations = {}
        self.__type_specs = {}
        self.cached_searches = {}
        self.raster_wrangler = raster_wrangler.RasterWrangler()
        self.vector_regex = re.compile('[0-9xyzXYZ]')

    @property
    def parameter_list(self):
        """
        The parameter list is the set of variables and their values that the
        documents in the store vary over. """
        return self.__parameter_list

    def _parse_parameter_type(self, name, properties):
        # look for hints about document type relations
        if 'Z' in self.__type_specs:
            Zs = self.__type_specs['Z']
        else:
            Zs = []

        if 'LUMINANCE' in self.__type_specs:
            Ls = self.__type_specs['LUMINANCE']
        else:
            Ls = []

        if 'VALUE' in self.__type_specs:
            Vs = self.__type_specs['VALUE']
        else:
            Vs = []

        if 'MAGNITUDE' in self.__type_specs:
            Ms = self.__type_specs['MAGNITUDE']
        else:
            Ms = []

        potential_vectors = {}
        if 'types' in properties:
            for x in range(0, len(properties['types'])):
                if list(properties['types'])[x] == 'depth':
                    value = list(properties['values'])[x]
                    newentry = [name, value]
                    Zs.append(newentry)
                if list(properties['types'])[x] == 'luminance':
                    value = list(properties['values'])[x]
                    newentry = [name, value]
                    Ls.append(newentry)
                # Mark value renders
                if list(properties['types'])[x] == 'value':
                    value = list(properties['values'])[x]
                    newentry = [name, value]
                    Vs.append(newentry)
                    # Check if vector component
                    vecname = value.split('_')
                    if (len(vecname) > 1 and len(vecname[-1]) == 1 and
                            self.vector_regex.match(vecname[-1])):
                        if vecname[0] in potential_vectors:
                            potential_vectors[vecname[0]] += 1
                        else:
                            potential_vectors[vecname[0]] = 1
                if list(properties['types'])[x] == 'magnitude':
                    value = list(properties['values'])[x]
                    newentry = [name, value]
                    Ms.append(newentry)

            # Add the magnitudes if they haven't been added
            for vec in potential_vectors:
                if potential_vectors[vec] > 1:
                    newvalue = vec + '_magnitude'
                    if newvalue not in properties['values']:
                        newentry = [name, newvalue]
                        Ms.append(newentry)
                        properties['values'].append(newvalue)
                        properties['types'].append('magnitude')

        if len(Zs) > 0:
            self.__type_specs['Z'] = Zs
        if len(Ls) > 0:
            self.__type_specs['LUMINANCE'] = Ls
        if len(Vs) > 0:
            self.__type_specs['VALUE'] = Vs
        if len(Ms) > 0:
            self.__type_specs['MAGNITUDE'] = Ms

    def _set_parameter_list(self, val):
        """For use by subclasses alone"""
        self.__parameter_list = val
        for name in self.__parameter_list:
            self._parse_parameter_type(name, self.__parameter_list[name])

    def add_parameter(self, name, properties):
        """
        Add a parameter.
        :param name: Name for the parameter.
        :param properties: Keyword arguments can be used to associate
        miscellaneous meta-data with this parameter.
        """
        self.cached_searches = {}
        self.__parameter_list[name] = properties
        self._parse_parameter_type(name, properties)

    def get_parameter(self, name):
        return self.__parameter_list[name]

    def get_parameter_values(self, name):
        """
        Get all values of type value
        :param name: Name for the parameter.
        """
        values = []
        if ('values' in self.__parameter_list[name] and
                'types' in self.__parameter_list[name]):
            for val, typ in zip(self.__parameter_list[name]['values'],
                                self.__parameter_list[name]['types']):
                if typ == 'value':
                    values.append(val)
        return values

    def get_complete_descriptor(self, partial_desc):
        """
        Convenience method that expands an incomplete list of parameters into
        the full set using default values for the missing variables.
        TODO: doesn't make sense with bifurcation (dependencies) remove
        """
        full_desc = dict()
        for name, properties in self.parameter_list.items():
            if "default" in properties:
                full_desc[name] = properties["default"]
        full_desc.update(partial_desc)
        return full_desc

    def get_default_type(self):
        """ subclasses override this if they know more """
        return "RGB"

    def determine_type(self, desc):
        """
        Given a descriptor this figures out what type of data it holds.
        It works by first looking into the __type_specs for registered
        relationships and if that fails returning the registered default
        type for the store.
        """
        # try any assigned mappings (for example color='depth' then 'Z')
        for typename, checks in self.__type_specs.items():
            for check in checks:
                name = check[0]
                conditions = check[1]
                if name in desc and str(desc[name]) in str(conditions):
                    return typename
        # no takers, use the default for this store
        typename = self.get_default_type()
        return typename

    def find_field_key(self, desc):
        """
        Given a descriptor this finds the field value if any in the
        descriptor.
        """
        for k in desc.keys():
            params = self.parameter_list[k]
            if 'role' in params:
                if params['role'] == 'field':
                    return k
        return None

    @property
    def parameter_associations(self):
        """ paremeter associations establish a dependency relationship between
        different parameters. """
        return self.__parameter_associations

    def _set_parameter_associations(self, val):
        """For use by subclasses alone"""
        self.__parameter_associations = val

    @property
    def metadata(self):
        """
        Auxiliary data about the store itself. An example is hints that help
        the viewer app know how to interpret this particular store.
        """
        return self.__metadata

    @metadata.setter
    def metadata(self, val):
        self.__metadata = val

    def add_metadata(self, keyval):
        if not self.__metadata:
            self.__metadata = {}
        self.__metadata.update(keyval)

    def get_version_major(self):
        """ major version information corresponds with store type """
        if self.metadata is None or 'type' not in self.metadata:
            return -1
        if self.metadata['type'] == 'parametric-image-stack':
            return 0
        elif self.metadata['type'] == 'composite-image-stack':
            return 1

    def get_version_minor(self):
        """
        minor version information corresponds to larger changes in a store
        type
        """
        if self.metadata is None or 'version' not in self.metadata:
            return 0
        return int(self.metadata['version'].split('.')[0])

    def get_version_patch(self):
        """
        patch version information corresponds to slight changes in a store
        type
        """
        if self.metadata is None or 'version' not in self.metadata:
            return 0
        return int(self.metadata['version'].split('.')[1])

    def get_camera_model(self):
        """
        return the camera model that the raster images in the store
        correspond to
        """
        if self.metadata is None or 'camera_model' not in self.metadata:
            # figure out something appropriate for pre-spec C stores
            if 'phi' in self.parameter_list or 'theta' in self.parameter_list:
                # camera is represented by phi aka 'azimuth' and theta
                # aka 'elevation' tracks)
                return 'phi-theta'
            # camera does not vary
            return 'static'
        if self.metadata['camera_model'] == 'static':
            # camera does not vary
            return self.metadata['camera_model']
        elif self.metadata['camera_model'] == 'phi-theta':
            # camera is represented by phi aka 'azimuth' and theta
            # aka 'elevation' tracks)
            return self.metadata['camera_model']
        elif self.metadata['camera_model'] == 'azimuth-elevation-roll':
            # camera is represented by a pose (Matrix) track, azimuth,
            # elevation and roll vary
            return self.metadata['camera_model']
        elif self.metadata['camera_model'] == 'yaw-pitch-roll':
            # camera is represented by a pose (Matrix) track, yaw, pitch and
            # roll vary
            return self.metadata['camera_model']
        return 'static'

    def create(self):
        """
        Creates an empty store.
        Subclasses must extend this.
        """
        assert not self.__loaded
        self.__loaded = True

    def load(self):
        """
        Loads contents on the store (but not the documents).
        Subclasses must extend this.
        """
        assert not self.__loaded
        self.__loaded = True

    def find(self, q=None):
        """
        Return iterator to all documents that match query q.
        Should support empty query or direct values queries e.g.

        for doc in store.find({'phi': 0}):
            print (doc.data)
        for doc in store.find({'phi': 0, 'theta': 100}):
            print (doc.data)

        """
        raise RuntimeError("Subclasses must define this method")

    def insert(self, document):
        """
        Inserts a new document.
        Subclasses must extend this.
        """
        if not self.__loaded:
            self.create()

    def assign_parameter_dependence(self, dep_param, param, on_values):
        """
        mark a particular parameter as being explorable only for a subset
        of the possible values of another.

        For example given parameter 'appendage type' which might have
        value 'foot' or 'flipper', a dependent parameter might be 'shoe type'
        which only makes sense for 'feet'. More to the point we use this
        for 'layers' and 'fields' in composite rendering of objects in a scene
        and the color settings that each object is allowed to take.
        """
        self.cached_searches = {}
        self.__parameter_associations.setdefault(dep_param, {}).update(
            {param: on_values})

    def isdepender(self, name):
        """ check if the named parameter depends on any others """
        if name in self.parameter_associations.keys():
            return True
        return False

    def isdependee(self, name):
        """ check if the named parameter has others that depend on it """
        for depender, dependees in py23iteritems(self.parameter_associations):
            if name in dependees:
                return True
        return False

    def getDependeeValue(self, depender, dependee):
        """
        Return the required value of a dependee to fulfill a dependency.
        """
        try:
            value = self.parameter_associations[depender][dependee]
        except KeyError:
            raise KeyError("Invalid dependency! ", depender, ", ", dependee)

        return value

    def getdependers(self, name):
        """
        return a list of all the parameters that depend on the given one
        """
        result = []
        for depender, dependees in py23iteritems(self.parameter_associations):
            if name in dependees["vis"]:
                result.append(depender)
        return result

    def getdependees(self, depender):
        """ return a list of all the parameters that 'depender' depends on """
        try:
            result = self.parameter_associations[depender]
        except KeyError:
            # This is a valid state, it only means there is no dependees
            result = {}

        return result

    def getRelatedField(self, parameter):
        ''' Returns the 'field' argument related to a 'parameter'. '''
        for depender, dependees in py23iteritems(self.parameter_associations):
            if parameter in dependees["vis"] and \
               self.isfield(depender):
                return depender

        return None

    def hasRelatedParameter(self, fieldName):
        '''
        Predicate to know if a field has a related 'parameter' argument.
        '''
        paramName = self.parameter_associations[fieldName]["vis"][0]
        return (paramName in self.parameter_list)

    def dependencies_satisfied(self, dep_param, descriptor):
        """
        Check if the values in decriptor satisfy all of the dependencies
        of dep_param.
        Return true if no dependencies to satisfy.
        Return false if dependency of dependency fails.
        """
        if dep_param not in self.__parameter_associations:
            return True
        for dep in self.__parameter_associations[dep_param]:
            if dep not in descriptor:
                # something dep_param needs is not in the descriptor at all
                return False
            if (descriptor[dep] not in self.__parameter_associations[
                    dep_param][dep]):
                # something dep_param needs doesn't have an accepted value
                # in the descriptor
                return False
            if not self.dependencies_satisfied(dep, descriptor):
                # recurse to check deps of dep_param themselves
                return False
        return True

    def add_layer(self, name, properties):
        """
        A Layer boils down to an image of something in the scene, and only
        that thing, along with the depth at each pixel. Layers (note the
        plural) can be composited back together by a viewer.
        """
        properties['type'] = 'option'
        properties['role'] = 'layer'
        self.add_parameter(name, properties)

    def islayer(self, name):
        if (name in self.parameter_list and
                'role' in self.parameter_list[name]):
            return self.parameter_list[name]['role'] == 'layer'
        return False

    def add_control(self, name, properties):
        """
        A control is a togglable parameter for a filter. Examples include:
        isovalue, offset.
        """
        properties['role'] = 'control'
        self.add_parameter(name, properties)

    def iscontrol(self, name):
        if (name in self.parameter_list and
                'role' in self.parameter_list[name]):
            return self.parameter_list[name]['role'] == 'control'
        return False

    def add_field(self, name, properties, parent_layer, parents_values):
        """
        A field is a component of the final color for a layer. Examples
        include: depth, normal, color, scalar values.
        """
        properties['type'] = 'hidden'
        properties['role'] = 'field'
        self.add_parameter(name, properties)
        self.assign_parameter_dependence(name, parent_layer, parents_values)

    def isfield(self, name):
        if (name in self.parameter_list and
                'role' in self.parameter_list[name]):
            return self.parameter_list[name]['role'] == 'field'
        return False

    def parameters_for_object(self, obj):
        """
        Given <obj>, an element of the layer <vis>, this method returns:
        1. the names of independent parameters (t, theta, etc) that affect it
        2. the name of its associated field
        3. the names of the controls that affect it
        """
        independent_parameters = [par for par in self.__parameter_list.keys()
                                  if 'role' not in self.__parameter_list[par]]

        fields = [x for x in self.parameter_associations.keys()
                  if obj in self.parameter_associations[x]['vis'] and
                  self.isfield(x)]

        field = fields[0] if fields else None

        controls = [x for x in self.parameter_associations.keys()
                    if obj in self.parameter_associations[x]['vis'] and
                    self.iscontrol(x)]

        return (independent_parameters, field, controls)

    def iterate(self, parameters=None, fixedargs=None, progressObject=None):
        """
        Run through all combinations of parameter/value pairs without visiting
        any combinations that do not satisfy dependencies among them.
        Parameters, if supplied, is a list of parameter names to enforce an
        ordering. Fixed arguments, if supplied, are parameter/value pairs that
        we want to hold constant in the exploration.
        """

        # optimization - cache and reuse to avoid expensive search
        argstr = json.dumps((parameters, fixedargs), sort_keys=True)
        if argstr in self.cached_searches:
            for x in self.cached_searches[argstr]:
                yield x
            return

        # prepare to iterate through all the possibilities, in order if one is
        # given
        param_names = parameters if parameters else self.parameter_list.keys()
        # print ("PARAMETERS", param_names)
        params = []
        values = []
        total_elem = 1.0
        for name in param_names:
            vals = self.get_parameter(name)['values']
            if fixedargs and name in fixedargs:
                continue

            total_elem *= len(vals)
            params.append(name)
            values.append(vals)

        # The algorithm is to iterate through all combinations, and remove
        # the impossible ones. I use a set to avoid redundant combinations.
        # In order to use the set I serialize to make something hashable.
        # Then I insert into a list to preserve the (hopefully optimized)
        # order.
        ok_descs = set()
        ordered_descs = []

        elem_accum = 0.0
        for element in itertools.product(*values):
            descriptor = dict(zip(params, element))

            if progressObject:
                elem_accum += 1.0
                progressObject.UpdateProgress(elem_accum / total_elem)

            if fixedargs is not None:
                descriptor.update(fixedargs)

            ok_desc = {}
            for param, value in py23iteritems(descriptor):
                if self.dependencies_satisfied(param, descriptor):
                    ok_desc.update({param: value})

            OK = True
            if fixedargs:
                for k, v in py23iteritems(fixedargs):
                    if not (k in ok_desc and ok_desc[k] == v):
                        OK = False
            if OK:
                strval = "{ "
                for name in sorted(ok_desc.keys()):
                    strval = strval + '"' + name + '": "' + str(
                        ok_desc[name]) + '", '
                strval = strval[0:-2] + "}"
                # strval = json.dumps(ok_desc, sort_keys=True)  # slower
                if strval not in ok_descs:
                    ok_descs.add(strval)
                    ordered_descs.append(ok_desc)
                    yield ok_desc

        self.cached_searches[argstr] = ordered_descs


def make_parameter(name, values, **kwargs):
    """ define a new parameter that will be added to a store.
    Primarily takes a name and an array of potential values.
    May also be given a default value from inside the array.
    May also be given a typechoice to help the UI which is required to be
    one of 'list', 'range', 'option' or 'hidden'.
    May also bve given a user friendly label.
    """
    default = kwargs['default'] if 'default' in kwargs else values[0]
    if default not in values:
        raise RuntimeError("Invalid default, must be one of %s" % str(values))

    typechoice = kwargs['typechoice'] if 'typechoice' in kwargs else 'range'
    valid_types = ['list', 'range', 'option', 'hidden']
    if typechoice not in valid_types:
        raise RuntimeError(
            "Invalid typechoice, must be one of %s" % str(valid_types))

    label = kwargs['label'] if 'label' in kwargs else name

    properties = dict()
    properties['type'] = typechoice
    properties['label'] = label
    properties['values'] = values
    properties['default'] = default
    return properties


def make_field(name, _values, **kwargs):
    """
    specialization of make_parameters for parameters that define fields
    (aka color inputs). In this case the values is a list of name, type pairs
    where types must be one of 'rgb', 'lut', 'depth', 'value', or 'luminance'
    May also be given an set of valueRanges, which have min and max values for
    named 'value' type color selections.
    """

    values = list(_values.keys())
    img_types = list(_values.values())
    valid_itypes = ['rgb', 'lut', 'depth', 'value', 'luminance', 'normals']
    for i in img_types:
        if i not in valid_itypes:
            raise RuntimeError(
                "Invalid typechoice, must be one of %s" % str(valid_itypes))

    default = kwargs['default'] if 'default' in kwargs else values[0]
    if default not in values:
        raise RuntimeError("Invalid default, must be one of %s" % str(values))

    typechoice = 'hidden'

    label = kwargs['label'] if 'label' in kwargs else name

    properties = dict()
    properties['type'] = typechoice
    properties['label'] = label
    properties['values'] = values
    properties['default'] = default
    properties['types'] = img_types

    if 'valueRanges' in kwargs:
        properties['valueRanges'] = kwargs['valueRanges']

    return properties
