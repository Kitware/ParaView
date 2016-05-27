#==============================================================================
# Copyright (c) 2015,  Kitware Inc., Los Alamos National Laboratory
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this
# list of conditions and the following disclaimer in the documentation and/or other
# materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may
# be used to endorse or promote products derived from this software without specific
# prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#==============================================================================
"""
    Module defining classes and methods for managing cinema data storage.
"""

import sys
import json
import os.path
import re
import itertools
import weakref
import numpy as np
import copy
import raster_wrangler

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
        """A document descriptor is a unique
        identifier for the document. It is a dict with key value pairs. The
        descriptor cannot be changed once the document has been instantiated."""
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
        attributes are a dict with arbitrary meta-data relevant to the application.
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

    @property
    def parameter_list(self):
        """
        The parameter list is the set of variables and their values that the
        documents in the store vary over. """
        return self.__parameter_list

    def _parse_parameter_type(self, name, properties):
        #look for hints about document type relations
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

        if 'types' in properties:
            for x in range(0, len(properties['types'])):
                if properties['types'][x] == 'depth':
                    value = properties['values'][x]
                    newentry = [name, value]
                    Zs.append(newentry)
                if properties['types'][x] == 'luminance':
                    value = properties['values'][x]
                    newentry = [name, value]
                    Ls.append(newentry)
                # Mark value renders
                if properties['types'][x] == 'value':
                    value = properties['values'][x]
                    newentry = [name, value]
                    Vs.append(newentry)
        if len(Zs) > 0:
            self.__type_specs['Z'] = Zs
        if len(Ls) > 0:
            self.__type_specs['LUMINANCE'] = Ls
        if len(Vs) > 0:
            self.__type_specs['VALUE'] = Vs

    def _set_parameter_list(self, val):
        """For use by subclasses alone"""
        self.__parameter_list = val
        for name in self.__parameter_list:
            self._parse_parameter_type(name, self.__parameter_list[name])

    def add_parameter(self, name, properties):
        """Add a parameter.
        :param name: Name for the parameter.
        :param properties: Keyword arguments can be used to associate miscellaneous
        meta-data with this parameter.
        """
        #if self.__loaded:
        #    raise RuntimeError("Updating parameters after loading/creating a store is not supported.")
        # TODO: Err, except when it is, in the important case of adding new time steps to a collection.
        # I postulate it is always OK to add safely to outermost parameter (loop).
        self.cached_searches = {}
        self.__parameter_list[name] = properties
        self._parse_parameter_type(name, properties)

    def get_parameter(self, name):
        return self.__parameter_list[name]

    def get_complete_descriptor(self, partial_desc):
        """
        Convenience method that expands an incomplete list of parameters into
        the full set using default values for the missing variables.
        TODO: doesn't make sense with bifurcation (dependencies), when SFS supports them remove
        """
        full_desc = dict()
        for name, properties in self.parameter_list.items():
            if properties.has_key("default"):
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
        #try any assigned mappings (for example color='depth' then 'Z')
        for typename, checks in self.__type_specs.items():
            for check in checks:
                name = check[0]
                conditions = check[1]
                if name in desc and desc[name] in conditions:
                    return typename
        #no takers, use the default for this store
        typename = self.get_default_type()
        return typename


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
        Auxiliary data about the store itself. An example is hints that help the
        viewer app know how to interpret this particular store.
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
        if self.metadata == None or not 'type' in self.metadata:
            return -1
        if self.metadata['type'] == 'parametric-image-stack':
            return 0
        elif self.metadata['type'] == 'composite-image-stack':
            return 1

    def get_version_minor(self):
        """ minor version information corresponds to larger changes in a store type """
        if self.metadata == None or not 'version' in self.metadata:
            return 0
        return int(self.metadata['version'].split('.')[0])

    def get_version_patch(self):
        """ patch version information corresponds to slight changes in a store type """
        if self.metadata == None or not 'version' in self.metadata:
            return 0
        return int(self.metadata['version'].split('.')[1])

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
            print doc.data
        for doc in store.find({'phi': 0, 'theta': 100}):
            print doc.data

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
        for depender, dependees in self.parameter_associations.iteritems():
            if name in dependees:
                return True
        return False

    def getDependeeValue(self, depender, dependee):
        """ Return the required value of a dependee to fulfill a dependency. """
        try:
            value = self.parameter_associations[depender][dependee]
        except KeyError:
            raise KeyError("Invalid dependency! ", depender, ", ", dependee)

        return value

    def getdependers(self, name):
        """ return a list of all the parameters that depend on the given one """
        result = []
        for depender, dependees in self.parameter_associations.iteritems():
            if name in dependees["vis"]:
                result.append(depender)
        return result

    def getdependees(self, depender):
        """ return a list of all the parameters that 'depender' depends on """
        try:
            result =  self.parameter_associations[depender]
        except KeyError:
            #This is a valid state, it only means there is no dependees
            result = {}

        return result

    def getRelatedField(self, parameter):
        ''' Returns the 'field' argument related to a 'parameter'. '''
        for depender, dependees in self.parameter_associations.iteritems():
            if parameter in dependees["vis"] and \
               self.isfield(depender):
                return depender

        return None

    def hasRelatedParameter(self, fieldName):
        ''' Predicate to know if a field has a related 'parameter' argument. '''
        paramName = self.parameter_associations[fieldName]["vis"][0]
        return  (paramName in self.parameter_list)

    def dependencies_satisfied(self, dep_param, descriptor):
        """
        Check if the values in decriptor satisfy all of the dependencies
        of dep_param.
        Return true if no dependencies to satisfy.
        Return false if dependency of dependency fails.
        """
        if not dep_param in self.__parameter_associations:
            return True
        for dep in self.__parameter_associations[dep_param]:
            if not dep in descriptor:
                #something dep_param needs is not in the descriptor at all
                return False
            if not descriptor[dep] in self.__parameter_associations[dep_param][dep]:
                #something dep_param needs doesn't have an accepted value in the descriptor
                return False
            if not self.dependencies_satisfied(dep, descriptor):
                #recurse to check deps of dep_param themselves
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
        return (self.parameter_list[name]['role'] == 'layer') if (name in self.parameter_list and 'role' in self.parameter_list[name]) else False

    def add_control(self, name, properties):
        """
        A control is a togglable parameter for a filter. Examples include:
        isovalue, offset.
        """
        properties['role'] = 'control'
        self.add_parameter(name, properties)

    def iscontrol(self, name):
        return (self.parameter_list[name]['role'] == 'control') if (name in self.parameter_list and 'role' in self.parameter_list[name]) else False

    def add_field(self, name, properties, parent_layer, parents_values):
        """
        A field is a component of the final color for a layer. Examples include:
        depth, normal, color, scalar values.
        """
        properties['type'] = 'hidden'
        properties['role'] = 'field'
        self.add_parameter(name, properties)
        self.assign_parameter_dependence(name, parent_layer, parents_values)

    def isfield(self, name):
        return (self.parameter_list[name]['role'] == 'field') if (name in self.parameter_list and 'role' in self.parameter_list[name]) else False

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

        return (independent_parameters,field,controls)

    def iterate(self, parameters=None, fixedargs=None):
        """
        Run through all combinations of parameter/value pairs without visiting
        any combinations that do not satisfy dependencies among them.
        Parameters, if supplied, is a list of parameter names to enforce an ordering.
        Fixed arguments, if supplied, are parameter/value pairs that we want
        to hold constant in the exploration.
        """

        #optimization - cache and reuse to avoid expensive search
        argstr = json.dumps((parameters,fixedargs), sort_keys=True)
        if argstr in self.cached_searches:
            for x in self.cached_searches[argstr]:
                yield x
            return

        #prepare to iterate through all the possibilities, in order if one is given
        #param_names = parameters if parameters else sorted(self.parameter_list.keys())
        param_names = parameters if parameters else self.parameter_list.keys()
        #print "PARAMETERS", param_names
        params = []
        values = []
        dep_params = []
        for name in param_names:
            vals = self.get_parameter(name)['values']
            if fixedargs and name in fixedargs:
                continue
            params.append(name)
            values.append(vals)

        #the algorithm is to iterate through all combinations, and remove
        #the impossible ones. I use a set to avoid redundant combinations.
        #In order to use the set I serialize to make something hashable.
        #Then I insert into a list to preserve the (hopefully optimized) order.
        ok_descs = set()
        ordered_descs = []
        for element in itertools.product(*values):
            descriptor = dict(itertools.izip(params, element))

            if fixedargs != None:
                descriptor.update(fixedargs)
            ok_params = []
            ok_vals = []

            ok_desc = {}
            for param, value in descriptor.iteritems():
                if self.dependencies_satisfied(param, descriptor):
                    ok_desc.update({param:value})

            OK = True
            if fixedargs:
                for k,v in fixedargs.iteritems():
                    if not (k in ok_desc and ok_desc[k] == v):
                        OK = False
            if OK:
                strval = "{ "
                for name in sorted(ok_desc.keys()):
                    strval = strval + '"' + name + '": "' + str(ok_desc[name]) + '", '
                strval = strval[0:-2] + "}"
                #strval = json.dumps(ok_desc, sort_keys=True) #slower than hand rolled above
                if not strval in ok_descs:
                    ok_descs.add(strval)
                    ordered_descs.append(ok_desc)
                    yield ok_desc

        self.cached_searches[argstr] = ordered_descs

class FileStore(Store):
    """Implementation of a store based on named files and directories."""

    def __init__(self, dbfilename=None):
        super(FileStore, self).__init__()
        self.__filename_pattern = None
        tmpfname = dbfilename if dbfilename \
                else os.path.join(os.getcwd(), "info.json")
        if not tmpfname.endswith("info.json"):
            tmpfname = os.path.join(tmpfname, "info.json")
        self.__dbfilename = tmpfname
        self.cached_searches = {}
        self.cached_files = {}

    def create(self):
        """creates a new file store"""
        super(FileStore, self).create()
        self.save()

    def load(self):
        """loads an existing filestore"""
        super(FileStore, self).load()
        with open(self.__dbfilename, mode="rb") as file:
            info_json = json.load(file)
            if 'arguments' in info_json:
                self._set_parameter_list(info_json['arguments'])
            elif 'parameter_list' in info_json:
                self._set_parameter_list(info_json['parameter_list'])
            else:
                print "Error I can't read that file"
                exit()
            self.metadata = info_json['metadata']
            self.filename_pattern = info_json['name_pattern']
            a = {}
            if 'associations' in info_json:
                a = info_json['associations']
            elif 'constraints' in info_json:
                a = info_json['constraints']
            self._set_parameter_associations(a)

    def save(self):
        """ writes out a modified file store """
        info_json = None
        if (self.get_version_major()<1 or
            (self.get_version_minor()==0 and self.get_version_patch()==0)):
            info_json = dict(
                arguments = self.parameter_list,
                name_pattern = self.filename_pattern,
                metadata = self.metadata,
                associations = self.parameter_associations
            )
        else:
            info_json = dict(
                parameter_list = self.parameter_list,
                name_pattern = self.filename_pattern,
                metadata = self.metadata,
                constraints = self.parameter_associations
            )

        dirname = os.path.dirname(self.__dbfilename)
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        with open(self.__dbfilename, mode="wb") as file:
            json.dump(info_json, file)

    @property
    def filename_pattern(self):
        """
        Files corresponding to Documents are arranged on disk
        according the the directory and filename structure described
        by the filename_pattern. The format is a regular expression
        consisting of parameter names enclosed in '{' and '}' and
        separated by spacers. "/" spacer characters produce sub
        directories.

        Composite type stores ignore the file name pattern other than
        the extension.
        """
        return self.__filename_pattern

    @filename_pattern.setter
    def filename_pattern(self, val):
        self.__filename_pattern = val

        #choose default data type in the store based on file extension
        self._default_type = 'RGB'
        if val[val.rfind("."):] == '.txt':
            self._default_type = 'TXT'

    def get_default_type(self):
        """ overridden to use the filename pattern to determine default type """
        return self._default_type

    def _get_filename(self, desc):
        dirname = os.path.dirname(self.__dbfilename)

        #find filename modulo any dependent parameters
        fixed = self.filename_pattern.format(**desc)
        base, ext = os.path.splitext(fixed)

        if (self.get_version_major()>0 and
            (self.get_version_minor()>0 or self.get_version_patch()>0)):
            base = ""

#        #add any dependent parameters
#        for dep in sorted(self.parameter_associations.keys()):
#            if dep in desc:
#                #print "    ->>> base /// dep: ", base, " /// ", dep
#                base = base + "/" + dep + "=" + str(desc[dep])
        #a more intuitive layout than the above alphanumeric sort
        #this one follows the graph and thus keeps related things, like
        #all the rasters (fields) for a particular object (layer), close
        #to one another
        keys = [k for k in sorted(desc)]
        ordered_keys = []
        while len(keys):
            k = keys.pop(0)
            if (self.get_version_major()<1 or
                (self.get_version_minor()==0 and self.get_version_patch()==0)):
                if not self.isdepender(k) and not self.isdependee(k):
                    continue
            parents = self.getdependees(k)
            ready = True
            for p in parents:
                if not (p in ordered_keys):
                    ready = False
                    #this is the crux - haven't seen a parent yet, so try again later
                    keys.append(k)
                    break
            if ready:
                ordered_keys.append(k)
        if (self.get_version_major()<1 or
            (self.get_version_minor()==0 and self.get_version_patch()==0)):
            for k in ordered_keys:
                base = base + "/" + k + "=" + str(desc[k])
        else:
            sep = ""
            for k in ordered_keys:
                index = self.get_parameter(k)['values'].index(desc[k])
                base = base + sep + k + "=" + str(index)
                sep = "/"

        #determine file type for this document
        doctype = self.determine_type(desc)
        if doctype == "Z":
            ext = self.raster_wrangler.zfileextension()

        fullpath = os.path.join(dirname, base+ext)
        return fullpath

    def insert(self, document):
        """overridden to write file for the document after parent
        makes a record of it in the store"""
        super(FileStore, self).insert(document)

        fname = self._get_filename(document.descriptor)

        dirname = os.path.dirname(fname)
        if not os.path.exists(dirname):
            os.makedirs(dirname)

        if not document.data is None:
            doctype = self.determine_type(document.descriptor)
            if doctype == 'RGB' or doctype == 'VALUE' or doctype == 'LUMINANCE':
                self.raster_wrangler.rgbwriter(document.data, fname)
            elif doctype == 'Z':
                self.raster_wrangler.zwriter(document.data, fname)
            else:
                self.raster_wrangler.genericwriter(document.data, fname)

    def _load_data(self, doc_file, descriptor):
        doctype = self.determine_type(descriptor)
        try:
            if doctype == 'RGB' or doctype == 'VALUE':
                data = self.raster_wrangler.rgbreader(doc_file)
            elif doctype == 'LUMINANCE':
                data = self.raster_wrangler.rgbreader(doc_file)
            elif doctype == 'Z':
                data = self.raster_wrangler.zreader(doc_file)
            else:
                data = self.raster_wrangler.genericreader(doc_file)

        except IOError:
            data = None
            raise

        doc = Document(descriptor, data)
        doc.attributes = None
        return doc

    def find(self, q=None):
        """ overridden to implement parent API with files"""
        q = q if q else dict()
        target_desc = q

        for possible_desc in self.iterate(fixedargs=target_desc):
            if possible_desc == {}:
                yield None
            filename = self._get_filename(possible_desc)
            #optimization - cache and reuse to avoid file load
            if filename in self.cached_files:
                yield self.cached_files[filename]
                return
            fcontent = self._load_data(filename, possible_desc)
            #todo: shouldn't be unbounded size
            self.cached_files[filename] = fcontent
            yield fcontent

    def get(self, q):
        """ optimization of find()[0] for an important case where caller
        knows exactly what to retrieve."""
        filename = self._get_filename(q)
        #optimization - cache and reuse to avoid file load
        if filename in self.cached_files:
            return self.cached_files[filename]
        fcontent = self._load_data(filename, q)
        #todo: shouldn't be unbounded size
        self.cached_files[filename] = fcontent
        return fcontent


class SingleFileStore(Store):
    """Implementation of a store based on a single volume file (image stack).
    NOTE: This class is limited to parametric-image-stack type stores,
    currently unmaintained and may go away in the near future."""

    def __init__(self, dbfilename=None):
        super(SingleFileStore, self).__init__()
        self.__dbfilename = dbfilename if dbfilename \
                else os.path.join(os.getcwd(), "info.json")
        self._volume = None
        self._needWrite = False
        self.add_metadata({"store_type" : "SFS"})

    def __del__(self):
        if self._needWrite:
            import vtk
            vw = vtk.vtkXMLImageDataWriter()
            vw.SetFileName(self._vol_file)
            vw.SetInputData(self._volume)
            vw.Write()

    def create(self):
        """creates a new file store"""
        super(SingleFileStore, self).create()
        self.save()

    def load(self):
        """loads an existing filestore"""
        super(SingleFileStore, self).load()
        with open(self.__dbfilename, mode="rb") as file:
            info_json = json.load(file)
            self._set_parameter_list(info_json['arguments'])
            self.metadata = info_json['metadata']

    def save(self):
        """ writes out a modified file store """
        info_json = dict(
                arguments = self.parameter_list,
                metadata = self.metadata
                )
        dirname = os.path.dirname(self.__dbfilename)
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        with open(self.__dbfilename, mode="wb") as file:
            json.dump(info_json, file)

    def _get_numslices(self):
        slices = 0
        for name in sorted(self.parameter_list.keys()):
            numvals = len(self.get_parameter(name)['values'])
            if slices == 0:
                slices = numvals
            else:
                slices = slices * numvals
        return slices

    def _compute_sliceindex(self, descriptor):
        """find position of descriptor within the set of slices
        TODO: algorithm is dumb, but consisent with find (which is also dumb)"""
        args = []
        values = []
        ordered = sorted(self.parameter_list.keys())
        for name in ordered:
            vals = self.get_parameter(name)['values']
            args.append(name)
            values.append(vals)
        index = 0
        for element in itertools.product(*values):
            desc = dict(itertools.izip(args, element))
            fail = False
            for k,v in descriptor.items():
                if desc[k] != v:
                    fail = True
            if not fail:
                return index
            index = index + 1

    def get_sliceindex(self, document):
        """ returns the location of one document within the stack"""
        desc = self.get_complete_descriptor(document.descriptor)
        index = self._compute_sliceindex(desc)
        return index

    def _insertslice(self, vol_file, index, document):
        volume = self._volume
        width = document.data.shape[0]
        height = document.data.shape[1]
        if not volume:
            import vtk
            slices = self._get_numslices()
            volume = vtk.vtkImageData()
            volume.SetExtent(0, width-1, 0, height-1, 0, slices-1)
            volume.AllocateScalars(vtk.VTK_UNSIGNED_CHAR, 3)
            self._volume = volume
            self._vol_file = vol_file

        imageslice = document.data
        imageslice = imageslice.reshape(width*height, 3)

        from vtk.numpy_interface import dataset_adapter as dsa
        image = dsa.WrapDataObject(volume)
        oid = volume.ComputePointId([0,0,index])
        nparray = image.PointData[0]
        nparray[oid:oid+(width*height)] = imageslice

        self._needWrite = True

    def insert(self, document):
        """overridden to store data within a volume after parent makes a note of it"""
        super(SingleFileStore, self).insert(document)

        index = self.get_sliceindex(document)

        if not document.data is None:
            dirname = os.path.dirname(self.__dbfilename)
            if not os.path.exists(dirname):
                os.makedirs(dirname)
            vol_file = os.path.join(dirname, "cinema.vti")
            self._insertslice(vol_file, index, document)

    def _load_slice(self, q, index, desc):
        if not self._volume:
            import vtk
            dirname = os.path.dirname(self.__dbfilename)
            vol_file = os.path.join(dirname, "cinema.vti")
            vr = vtk.vtkXMLImageDataReader()
            vr.SetFileName(vol_file)
            vr.Update()
            volume = vr.GetOutput()
            self._volume = volume
            self._vol_file = vol_file
        else:
            volume = self._volume

        ext = volume.GetExtent()
        width = ext[1]-ext[0]
        height = ext[3]-ext[2]

        from vtk.numpy_interface import dataset_adapter as dsa
        image = dsa.WrapDataObject(volume)

        oid = volume.ComputePointId([0, 0, index])
        nparray = image.PointData[0]
        imageslice = np.reshape(nparray[oid:oid+width*height], (width,height,3))

        doc = Document(desc, imageslice)
        doc.attributes = None
        return doc

    def find(self, q=None):
        """Overridden to search for documentts withing the stack.
        TODO: algorithm is dumb, but consisent with compute_sliceindex
        (which is also dumb)"""
        q = q if q else dict()
        args = []
        values = []
        ordered = sorted(self.parameter_list.keys())
        for name in ordered:
            vals = self.get_parameter(name)['values']
            args.append(name)
            values.append(vals)

        index = 0
        for element in itertools.product(*values):
            desc = dict(itertools.izip(args, element))
            fail = False
            for k,v in q.items():
                if desc[k] != v:
                    fail = True
            if not fail:
                yield self._load_slice(q, index, desc)
            index = index + 1


def make_parameter(name, values, **kwargs):
    """ define a new parameter that will be added to a store.
    Primarily takes a name and an array of potential values.
    May also be given a default value from inside the array.
    May also be given a typechoice to help the UI which is required to be one of
    'list', 'range', 'option' or 'hidden'.
    May also bve given a user friendly label.
    """
    default = kwargs['default'] if 'default' in kwargs else values[0]
    if not default in values:
        raise RuntimeError, "Invalid default, must be one of %s" % str(values)

    typechoice = kwargs['typechoice'] if 'typechoice' in kwargs else 'range'
    valid_types = ['list','range','option','hidden']
    if not typechoice in valid_types:
        raise RuntimeError, "Invalid typechoice, must be one of %s" % str(valid_types)

    label = kwargs['label'] if 'label' in kwargs else name

    properties = dict()
    properties['type'] = typechoice
    properties['label'] = label
    properties['values'] = values
    properties['default'] = default
    return properties

def make_field(name, _values, **kwargs):
    """specialization of make_parameters for parameters that define
    fields(aka color inputs).
    In this case the values is a list of name, type pairs where types must be one
    of 'rgb', 'depth', 'value', or 'luminance'
    May also be given an set of valueRanges, which have min and max values for named
    'value' type color selections.
    """

    values = _values.keys()
    img_types = _values.values()
    valid_itypes = ['rgb','depth','value','luminance','normals']
    for i in img_types:
        if i not in valid_itypes:
            raise RuntimeError, "Invalid typechoice, must be one of %s" % str(valid_itypes)

    default = kwargs['default'] if 'default' in kwargs else values[0]
    if not default in values:
        raise RuntimeError, "Invalid default, must be one of %s" % str(values)

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
