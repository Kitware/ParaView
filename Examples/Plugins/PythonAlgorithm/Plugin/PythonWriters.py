"""This module demonstrates how to implement a writer using
VTKPythonAlgorithmBase in ParaView"""


# This is module to import. It provides VTKPythonAlgorithmBase, the base class
# for all python-based vtkAlgorithm subclasses in VTK and decorators used to
# 'register' the algorithm with ParaView along with information about UI.
from paraview.util.vtkAlgorithm import *


#------------------------------------------------------------------------------
# A writer example.
#------------------------------------------------------------------------------
@smproxy.writer(extensions="npz", file_description="NumPy Compressed Arrays", support_reload=False)
@smproperty.input(name="Input", port_index=0)
@smdomain.datatype(dataTypes=["vtkTable"], composite_data_supported=False)
class NumpyWriter(VTKPythonAlgorithmBase):
    def __init__(self):
        VTKPythonAlgorithmBase.__init__(self, nInputPorts=1, nOutputPorts=0, inputType='vtkTable')
        self._filename = None

    @smproperty.stringvector(name="FileName", panel_visibility="never")
    @smdomain.filelist()
    def SetFileName(self, fname):
        """Specify filename for the file to write."""
        if self._filename != fname:
            self._filename = fname
            self.Modified()

    def RequestData(self, request, inInfoVec, outInfoVec):
        from vtkmodules.vtkCommonDataModel import vtkTable
        from vtkmodules.numpy_interface import dataset_adapter as dsa

        table = dsa.WrapDataObject(vtkTable.GetData(inInfoVec[0], 0))
        kwargs = {}
        for aname in table.RowData.keys():
            kwargs[aname] = table.RowData[aname]

        import numpy
        numpy.savez_compressed(self._filename, **kwargs)
        return 1

    def Write(self):
        self.Modified()
        self.Update()
