"""
An implementation of the database API based on named files and directories.
"""
from __future__ import absolute_import

from . import store
import json
import os
import sys
import copy
import numpy as np


class FileStore(store.Store):
    """Implementation of a store based on named files and directories."""

    def __init__(self, dbfilename=None):
        super(FileStore, self).__init__()
        self.__filename_pattern = None
        if dbfilename:
            tmpfname = dbfilename
        else:
            tmpfname = os.path.join(os.getcwd(), "info.json")
        if not tmpfname.endswith("info.json"):
            tmpfname = os.path.join(tmpfname, "info.json")
        self.__dbfilename = tmpfname
        self.cached_searches = {}
        self.cached_files = {}
        self.metadata = {}
        self.__new_files = []

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
                print("Error I can't read that file")
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
        if (self.get_version_major() < 1 or
            (self.get_version_minor() == 0 and
             self.get_version_patch() == 0)):
            info_json = dict(
                arguments=self.parameter_list,
                name_pattern=self.filename_pattern,
                metadata=self.metadata,
                associations=self.parameter_associations
            )
        else:
            info_json = dict(
                parameter_list=self.parameter_list,
                name_pattern=self.filename_pattern,
                metadata=self.metadata,
                constraints=self.parameter_associations
            )

        dirname = os.path.dirname(self.__dbfilename)
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        with open(self.__dbfilename, mode="wb") as file:
            json.dump(info_json, file, sort_keys=True, indent=4)

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

        # choose default data type in the store based on file extension
        self._default_type = 'RGB'
        if val[val.rfind("."):] == '.txt':
            self._default_type = 'TXT'

    def get_default_type(self):
        """
        overridden to use the filename pattern to determine default type
        """
        return self._default_type

    def _get_filename(self, desc, readingFile=True):
        dirname = os.path.dirname(self.__dbfilename)

        # find filename modulo any dependent parameters
        fixed = self.filename_pattern.format(**desc)
        base, ext = os.path.splitext(fixed)

        if (self.get_version_major() > 0 and
                (self.get_version_minor() > 0 or
                 self.get_version_patch() > 0)):
            base = ""

        # Add any dependent parameters
        # for dep in sorted(self.parameter_associations.keys()):
        #     if dep in desc:
        #         #print ("    ->>> base /// dep: ", base, " /// ", dep)
        #         base = base + "/" + dep + "=" + str(desc[dep])

        # a more intuitive layout than the above alphanumeric sort
        # this one follows the graph and thus keeps related things, like
        # all the rasters (fields) for a particular object (layer), close
        # to one another
        keys = [k for k in sorted(desc)]
        ordered_keys = []
        while len(keys):
            k = keys.pop(0)
            if (self.get_version_major() < 1 or
                (self.get_version_minor() == 0 and
                 self.get_version_patch() == 0)):
                if not self.isdepender(k) and not self.isdependee(k):
                    continue
            parents = self.getdependees(k)
            ready = True
            for p in parents:
                if not (p in ordered_keys):
                    ready = False
                    # this is the crux- haven't seen a parent yet, so try later
                    keys.append(k)
                    break
            if ready:
                ordered_keys.append(k)
        if (self.get_version_major() < 1 or
            (self.get_version_minor() == 0 and
             self.get_version_patch() == 0)):
            for k in ordered_keys:
                base = base + "/" + k + "=" + str(desc[k])
        else:
            sep = ""
            for k in ordered_keys:
                index = self.get_parameter(k)['values'].index(desc[k])
                base = base + sep + k + "=" + str(index)
                sep = "/"

        # determine file type for this document
        doctype = self.determine_type(desc)
        if doctype == "Z":
            # Depth images are by default float buffers
            ext = self.raster_wrangler.floatExtension()
        elif (doctype == "VALUE" and
              (not os.path.exists(os.path.join(dirname, base + ext)) and
               readingFile)):
            # Value images could be RGB buffers (INVERTIBLE_LUT) or float
            # buffers. When reading a file, if the default extension (provided
            # in the json file) is not found then it is assumed to be a float
            # buffer.
            # TODO Split this function to handle these cases separately.
            ext = self.raster_wrangler.floatExtension()

        fullpath = os.path.join(dirname, base + ext)
        fullpath = fullpath.replace("*", "")  # avoid wildcard character
        return fullpath

    def insert(self, document):
        """overridden to write file for the document after parent
        makes a record of it in the store"""
        super(FileStore, self).insert(document)

        fname = self._get_filename(document.descriptor, readingFile=False)
        if not os.path.exists(fname):
            self.__new_files.append((document.descriptor,fname))

        dirname = os.path.dirname(fname)
        if not os.path.exists(dirname):
            # In batch mode '-sym', the dir might be created by a different
            # rank.
            try:
                os.makedirs(dirname)
            except OSError:
                print("OSError: Could not make dirs! " +
                      "This is expected if running in batch mode with '-sym'")
                pass

        if document.data is not None:
            for parname, parvalue in document.descriptor.iteritems():
                params = self.get_parameter(parname)
                if 'role' in params:
                    if params['role'] == 'field':
                        if 'image_size' not in self.metadata:
                            self.add_metadata(
                                {'image_size': document.data.shape[:2]})
                            self.add_metadata({'endian': sys.byteorder})

            doctype = self.determine_type(document.descriptor)
            if doctype == 'RGB' or doctype == 'LUMINANCE':
                self.raster_wrangler.rgbwriter(document.data, fname)
            elif doctype == 'VALUE':
                # find the range for the value that this raster shows
                vrange = [0, 1]
                for parname, parvalue in document.descriptor.iteritems():
                    param = self.get_parameter(parname)
                    if 'valueRanges' in param:
                        # we now have a color parameter, look for the range
                        # for the specific array we have a raster for
                        vr = param['valueRanges']
                        if parvalue in vr:
                            vrange = vr[parvalue]
                self.raster_wrangler.valuewriter(document.data, fname, vrange)
            elif doctype == 'Z':
                self.raster_wrangler.zwriter(document.data, fname)
            elif doctype == 'MAGNITUDE':
                pass
            else:
                self.raster_wrangler.genericwriter(document.data, fname)

    def _load_data(self, descriptor):
        doctype = self.determine_type(descriptor)
        doc_file = self._get_filename(descriptor)
        if 'image_size' in self.metadata:
            shape = self.metadata['image_size']
        else:
            shape = None

        # Magnitude is special case
        if doctype == 'MAGNITUDE':
            direct_format = False
            if ('value_mode' in self.metadata and
                    self.metadata['value_mode'] == 2):
                direct_format = True

            k = self.find_field_key(descriptor)
            prefix = ''.join(descriptor[k].split('_')[:-1])
            components = []
            for v in self.get_parameter_values(k):
                if prefix in v:
                    desc = copy.copy(descriptor)
                    desc[k] = v
                    vrange = [0., 1.]
                    if 'valueRanges' in self.parameter_list[k]:
                        if v in self.parameter_list[k]['valueRanges']:
                            vrange = self.parameter_list[k]['valueRanges'][v]

                    fname = self._get_filename(desc)
                    comp = self._load_file_data(fname, doctype, shape)
                    if direct_format:
                        scaled = comp
                    else:
                        scaled = vrange[0] + (vrange[1]-vrange[0])*comp/256
                    components.append(scaled)

            mag = np.linalg.norm(np.array(components), axis=0)
            mrange = [mag.min(), mag.max()]
            if direct_format:
                data = mag
            else:
                data = 256*(mag - mrange[0])/(mrange[1]-mrange[0])
        else:
            data = self._load_file_data(doc_file, doctype, shape)

        doc = store.Document(descriptor, data)
        doc.attributes = None
        return doc

    def _load_file_data(self, fname, doctype, shape):
        if fname in self.cached_files:
            return self.cached_files[fname]
        else:
            try:
                if doctype == 'RGB' or doctype == 'LUMINANCE':
                    data = self.raster_wrangler.rgbreader(fname)
                elif doctype == 'VALUE' or doctype == 'MAGNITUDE':
                    data = self.raster_wrangler.valuereader(fname, shape)
                elif doctype == 'Z':
                    data = self.raster_wrangler.zreader(fname, shape)
                else:
                    data = self.raster_wrangler.genericreader(fname)
            except IOError:
                data = None
                raise

            self.cached_files[fname] = data
            return data

    def find(self, q=None):
        """ overridden to implement parent API with files"""
        q = q if q else dict()
        target_desc = q

        for possible_desc in self.iterate(fixedargs=target_desc):
            if possible_desc == {}:
                yield None
            yield self._load_data(possible_desc)

    def get(self, q):
        """ optimization of find()[0] for an important case where caller
        knows exactly what to retrieve."""
        return self._load_data(q)

    def get_new_files(self):
        return self.__new_files
