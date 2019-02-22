"""
An implementation of the database API stored in one VTK .vti format file.
"""
from __future__ import absolute_import

from . import store
import numpy as np
import os
import json
import itertools


class VTIFileStore(store.Store):
    """Implementation of a store based on a single volume file (image stack).
    NOTE: This class is limited to parametric-image-stack type stores,
    currently unmaintained and may go away in the near future."""

    def __init__(self, dbfilename=None):
        super(VTIFileStore, self).__init__()
        if dbfilename:
            self.__dbfilename = dbfilename
        else:
            self.__dbfilename = os.path.join(os.getcwd(), "info.json")
        self._volume = None
        self._needWrite = False
        self.add_metadata({"store_type": "SFS"})

    def __del__(self):
        if self._needWrite:
            import vtk
            vw = vtk.vtkXMLImageDataWriter()
            vw.SetFileName(self._vol_file)
            vw.SetInputData(self._volume)
            vw.Write()

    def create(self):
        """creates a new file store"""
        super(VTIFileStore, self).create()
        self.save()

    def load(self):
        """loads an existing filestore"""
        super(VTIFileStore, self).load()
        with open(self.__dbfilename, mode="rb") as file:
            info_json = json.load(file)
            self._set_parameter_list(info_json['arguments'])
            self.metadata = info_json['metadata']

    def save(self):
        """ writes out a modified file store """
        info_json = dict(arguments=self.parameter_list,
                         metadata=self.metadata)
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
        """
        find position of descriptor within the set of slices
        TODO: algorithm is dumb, but consisent with find
        (which is also dumb)
        """
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
            for k, v in descriptor.items():
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
        oid = volume.ComputePointId([0, 0, index])
        nparray = image.PointData[0]
        nparray[oid:oid+(width*height)] = imageslice

        self._needWrite = True

    def insert(self, document):
        """
        overridden to store data within a volume after parent
        makes a note of it
        """
        super(VTIFileStore, self).insert(document)

        index = self.get_sliceindex(document)

        if document.data is not None:
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
        imageslice = np.reshape(
            nparray[oid:oid+width*height], (width, height, 3))

        doc = store.Document(desc, imageslice)
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
            for k, v in q.items():
                if desc[k] != v:
                    fail = True
            if not fail:
                yield self._load_slice(q, index, desc)
            index = index + 1
