"""
    Module defining classes and methods for managing cinema data storage.
"""

import sys
import json
import os.path
import re
import itertools
import weakref

class Document(object):
    """
    This refers to a document in the cinema data storage. A document is
    uniquely identified by a 'descriptor'. A descriptor is a dictionary with
    key-value pairs, where key is a parameter name and value is the value for
    that particular parameter.

    A document can have arbitrary meta-data (as 'attributes') and data (as
    'data') associated with it.
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
    def attributes(self):
        """Attributes are arbitrary meta-data associated with the document.
        If no attributes are present, it is set to None. When present,
        attributes are a dict with arbitrary meta-data relevant to the application.
        """
        return self.__attributes

    @attributes.setter
    def attributes(self, attrs):
        self.__attributes = attrs

    @property
    def data(self):
        """Data associated with the document."""
        return self.__data

    @data.setter
    def data(self, val):
        self.__data = val

class Store(object):
    """Base class for a cinema store. A store is a collection of Documents,
    with API to add, find, and access them.

    This class is an abstract class defining the API and storage independent
    logic. Storage specific subclasses handle the 'database' access.

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
        self.__metadata = None #better name is view hints
        self.__parameter_list = {}
        self.__loaded = False

    @property
    def parameter_list(self):
        return self.__parameter_list

    def _set_parameter_list(self, val):
        """For use by subclasses alone"""
        self.__parameter_list = val

    @property
    def metadata(self):
        return self.__metadata

    @metadata.setter
    def metadata(self, val):
        self.__metadata = val

    def add_metadata(self, keyval):
        if not self.__metadata:
            self.__metadata = {}
        self.__metadata.update(keyval)

    def get_complete_descriptor(self, partial_desc):
        full_desc = dict()
        for name, properties in self.parameter_list.items():
            if properties.has_key("default"):
                full_desc[name] = properties["default"]
        full_desc.update(partial_desc)
        return full_desc

    def add_parameter(self, name, properties):
        """Add a parameter.

        :param name: Name for the parameter.

        :param properties: Keyword arguments can be used to associate miscellaneous
        meta-data with this parameter.
        """
        #if self.__loaded:
        #    raise RuntimeError("Updating parameters after loading/creating a store is not supported.")
        # TODO: except when it is, in the important case of adding new time steps to a collection
        # probably can only add safely to outermost parameter (loop)
        properties = self.validate_parameter(name, properties)
        self.__parameter_list[name] = properties

    def get_parameter(self, name):
        return self.__parameter_list[name]

    def validate_parameter(self, name, properties):
        """Validates a  new parameter and return updated parameter properties.
        Subclasses should override this as needed.
        """
        return properties

    def insert(self, document):
        """Inserts a new document"""
        if not self.__loaded:
            self.create()

    def load(self):
        assert not self.__loaded
        self.__loaded = True

    def create(self):
        assert not self.__loaded
        self.__loaded = True

    def find(self, q=None):
        raise RuntimeError("Subclasses must define this method")

    def get_image_type(self):
        return None

class FileStore(Store):
    """Implementation of a store based on files and directories"""

    def __init__(self, dbfilename=None):
        super(FileStore, self).__init__()
        self.__filename_pattern = None
        self.__dbfilename = dbfilename if dbfilename \
                else os.path.join(os.getcwd(), "info.json")

    def load(self):
        """loads an existing filestore"""
        super(FileStore, self).load()
        with open(self.__dbfilename, mode="rb") as file:
            info_json = json.load(file)
            #for legacy reasons, the parameters are called
            #arguments" in the files
            self._set_parameter_list(info_json['arguments'])
            self.metadata = info_json['metadata']
            self.filename_pattern = info_json['name_pattern']

    def save(self):
        """ writes out a modified file store """
        info_json = dict(
                arguments = self.parameter_list,
                name_pattern = self.filename_pattern,
                metadata = self.metadata
                )
        dirname = os.path.dirname(self.__dbfilename)
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        with open(self.__dbfilename, mode="wb") as file:
            json.dump(info_json, file)

    def create(self):
        """creates a new file store"""
        super(FileStore, self).create()
        self.save()

    @property
    def filename_pattern(self):
        """
        Files corresponding to Documents are arranged on disk
        according the the directory and filename structure described
        by the filename_pattern. The format is a regular expression
        consisting of parameter names enclosed in '{' and '}' and
        separated by spacers. "/" spacer characters produce sub
        directories.
        """
        return self.__filename_pattern

    @filename_pattern.setter
    def filename_pattern(self, val):
        self.__filename_pattern = val
        #Now set up to be able to convert filenames into descriptors automatically
        #break filename pattern up into an ordered list of parameter names
        cp = re.sub("{[^}]+}", "(\S+)", self.__filename_pattern) #convert to a RE
        #extract names
        keyargs = re.match(cp, self.__filename_pattern).groups()
        self.__fn_keys = list(x[1:-1] for x in keyargs) #strip "{" and "}"
        #make an RE to get the values from full pathname, igoring leading directories
        self.__fn_vals_RE = '(\S+)/'+cp

    def get_image_type(self):
        return self.filename_pattern[self.filename_pattern.rfind("."):]

    def insert(self, document):
        super(FileStore, self).insert(document)

        fname = self.get_filename(document)
        dirname = os.path.dirname(fname)
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        if not document.data == None:
            with open(fname, mode='w') as file:
                file.write(document.data)

        #with open(fname + ".__data__", mode="w") as file:
        #    info_json = dict(
        #            descriptor = document.descriptor,
        #            attributes = document.attributes)
        #    json.dump(info_json, file)


    def get_filename(self, document):
        desc = self.get_complete_descriptor(document.descriptor)
        suffix = self.filename_pattern.format(**desc)
        dirname = os.path.dirname(self.__dbfilename)
        return os.path.join(dirname, suffix)

    def find(self, q=None):
        """
        Currently support empty query or direct values queries e.g.
        for doc in store.find({'phi': 0}):
            print doc.data
        for doc in store.find({'phi': 0, 'theta': 100}):
            print doc.data
        """
        q = q if q else dict()
        p = q

        # build a file name match pattern based on the query.
        for name, properties in self.parameter_list.items():
            if not name in q:
                p[name] = "*"
        dirname = os.path.dirname(self.__dbfilename)
        match_pattern = os.path.join(dirname, self.filename_pattern.format(**p))

        from fnmatch import fnmatch
        from os import walk
        for root, dirs, files in walk(os.path.dirname(self.__dbfilename)):
            for fn in files:
                doc_file = os.path.join(root, fn)
                #if file.find("__data__") == -1 and fnmatch(doc_file, match_pattern):
                #    yield self.load_document(doc_file)
                if fnmatch(doc_file, match_pattern):
                    yield self.load_image(doc_file)

    # def load_document(self, doc_file):
    #    with open(doc_file + ".__data__", "r") as file:
    #        info_json = json.load(file)
    #    with open(doc_file, "r") as file:
    #        data = file.read()
    #    doc = Document(info_json["descriptor"], data)
    #    doc.attributes = info_json["attributes"]
    #    return doc

    def load_image(self, doc_file):
        #with open(doc_file + ".__data__", "r") as file:
        #    info_json = json.load(file)
        with open(doc_file, "r") as file:
            data = file.read()
        # convert filename into a list of values
        vals = re.match(self.__fn_vals_RE, doc_file).groups()[1:]
        descriptor = dict(zip(self.__fn_keys, vals))
        doc = Document(descriptor, data)
        doc.attributes = None
        return doc


def make_parameter(name, values, **kwargs):
    default = kwargs['default'] if 'default' in kwargs else values[0]
    typechoice = kwargs['typechoice'] if 'typechoice' in kwargs else 'range'
    label = kwargs['label'] if 'label' in kwargs else name

    types = ['list','range']
    if not typechoice in types:
        raise RuntimeError, "Invalid typechoice, must be on of %s" % str(types)
    if not default in values:
        raise RuntimeError, "Invalid default, must be one of %s" % str(values)
    properties = dict()
    properties['type'] = typechoice
    properties['label'] = label
    properties['values'] = values
    properties['default'] = default
    return properties
