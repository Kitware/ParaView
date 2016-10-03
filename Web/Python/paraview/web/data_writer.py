import os, math

from paraview import simple
from vtk import *

VTK_DATA_TYPES = [ 'void',            # 0
                   'bit',             # 1
                   'char',            # 2
                   'unsigned_char',   # 3
                   'short',           # 4
                   'unsigned_short',  # 5
                   'int',             # 6
                   'unsigned_int',    # 7
                   'long',            # 8
                   'unsigned_long',   # 9
                   'float',           # 10
                   'double',          # 11
                   'id_type',         # 12
                   'unspecified',     # 13
                   'unspecified',     # 14
                   'signed_char' ]

# -----------------------------------------------------------------------------
# Scalar Value to File
# -----------------------------------------------------------------------------

class ScalarRenderer(object):
    def __init__(self, isWriter=True, removePNG=True):
        self.view = simple.CreateView('RenderView')
        self.view.Background = [0.0, 0.0, 0.0]
        self.view.CenterAxesVisibility = 0
        self.view.OrientationAxesVisibility = 0

        self.reader = vtkPNGReader()
        self.cleanAfterMe = removePNG

        self.canWrite = isWriter

    def getView(self):
        return self.view

    def writeLightArray(self, path, source):
        rep = simple.Show(source, self.view)
        rep.Representation = 'Surface'
        rep.DiffuseColor = [1,1,1]
        simple.ColorBy(rep, ('POINTS', None))

        # Grab data
        tmpFileName = path + '__.png'
        self.view.LockBounds = 1
        simple.SaveScreenshot(tmpFileName, self.view)
        self.view.LockBounds = 0

        if self.canWrite:
            # Convert data
            self.reader.SetFileName(tmpFileName)
            self.reader.Update()

            rgbArray = self.reader.GetOutput().GetPointData().GetArray(0)
            arraySize = rgbArray.GetNumberOfTuples()

            rawArray = vtkUnsignedCharArray()
            rawArray.SetNumberOfTuples(arraySize)

            for idx in range(arraySize):
                light = rgbArray.GetTuple3(idx)[0]
                rawArray.SetTuple1(idx, light)

            with open(path, 'wb') as f:
                f.write(buffer(rawArray))

            # Delete temporary file
            if self.cleanAfterMe:
                os.remove(tmpFileName)

        simple.Hide(source, self.view)

    def writeMeshArray(self, path, source):
        rep = simple.Show(source, self.view)
        rep.Representation = 'Surface With Edges'
        rep.DiffuseColor = [0,0,0]
        rep.EdgeColor = [1.0, 1.0, 1.0]
        simple.ColorBy(rep, ('POINTS', None))

        # Grab data
        tmpFileName = path + '__.png'
        self.view.LockBounds = 1
        simple.SaveScreenshot(tmpFileName, self.view)
        self.view.LockBounds = 0

        if self.canWrite:
            # Convert data
            self.reader.SetFileName(tmpFileName)
            self.reader.Update()

            rgbArray = self.reader.GetOutput().GetPointData().GetArray(0)
            arraySize = rgbArray.GetNumberOfTuples()

            rawArray = vtkUnsignedCharArray()
            rawArray.SetNumberOfTuples(arraySize)

            for idx in range(arraySize):
                light = rgbArray.GetTuple3(idx)[0]
                rawArray.SetTuple1(idx, light)

            with open(path, 'wb') as f:
                f.write(buffer(rawArray))

            # Delete temporary file
            if self.cleanAfterMe:
                os.remove(tmpFileName)

        simple.Hide(source, self.view)

    def writeArray(self, path, source, name, component=0):
        rep = simple.Show(source, self.view)
        rep.Representation = 'Surface'
        rep.DiffuseColor = [1,1,1]

        dataRange = [0.0, 1.0]
        fieldToColorBy = ['POINTS', name]

        self.view.ArrayNameToDraw = name
        self.view.ArrayComponentToDraw = component

        pdi = source.GetPointDataInformation()
        cdi = source.GetCellDataInformation()

        if pdi.GetArray(name):
            self.view.DrawCells = 0
            dataRange = pdi.GetArray(name).GetRange(component)
            fieldToColorBy[0] = 'POINTS'
        elif cdi.GetArray(name):
            self.view.DrawCells = 1
            dataRange = cdi.GetArray(name).GetRange(component)
            fieldToColorBy[0] = 'CELLS'
        else:
            print ("No array with that name", name)
            return

        realRange = dataRange
        if dataRange[0] == dataRange[1]:
          dataRange = [dataRange[0] - 0.1, dataRange[1] + 0.1]

        simple.ColorBy(rep, fieldToColorBy)

        # Grab data
        tmpFileName = path + '__.png'
        self.view.ScalarRange = dataRange
        self.view.LockBounds = 1
        self.view.StartCaptureValues()
        simple.SaveScreenshot(tmpFileName, self.view)
        self.view.StopCaptureValues()
        self.view.LockBounds = 0

        if self.canWrite:
            # Convert data
            self.reader.SetFileName(tmpFileName)
            self.reader.Update()

            rgbArray = self.reader.GetOutput().GetPointData().GetArray(0)
            arraySize = rgbArray.GetNumberOfTuples()

            rawArray = vtkFloatArray()
            rawArray.SetNumberOfTuples(arraySize)

            minValue = 10000.0
            maxValue = -100000.0
            delta = (dataRange[1] - dataRange[0]) / 16777215.0 # 2^24 - 1 => 16,777,215
            for idx in range(arraySize):
                rgb = rgbArray.GetTuple3(idx)
                if rgb[0] != 0 or rgb[1] != 0 or rgb[2] != 0:
                    value = dataRange[0] + delta * float(rgb[0]*65536 + rgb[1]*256 + rgb[2] - 1)
                    rawArray.SetTuple1(idx, value)
                    minValue = min(value, minValue)
                    maxValue = max(value, maxValue)
                else:
                    rawArray.SetTuple1(idx, float('NaN'))

            # print ('Array bounds', minValue, maxValue, 'compare to', dataRange)

            with open(path, 'wb') as f:
                f.write(buffer(rawArray))

            # Delete temporary file
            if self.cleanAfterMe:
              os.remove(tmpFileName)

        # Remove representation from view
        simple.Hide(source, self.view)

        return realRange
