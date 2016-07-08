from vtk import *

from vtk.web.camera import *

import json, os, math, gzip, shutil

# -----------------------------------------------------------------------------
# Helper function
# -----------------------------------------------------------------------------

def getScalarFromRGB(rgb, scalarRange=[-1.0, 1.0]):
    delta = (scalarRange[1] - scalarRange[0]) / 16777215.0 # 2^24 - 1 => 16,777,215
    if rgb[0] != 0 or rgb[1] != 0 or rgb[2] != 0:
        # Decode encoded value
        return scalarRange[0] + delta * float(rgb[0]*65536 + rgb[1]*256 + rgb[2] - 1)
    else:
        # No value
        return float('NaN')

def convertImageToFloat(srcPngImage, destFile, scalarRange=[0.0, 1.0]):
    reader = vtkPNGReader()
    reader.SetFileName(srcPngImage)
    reader.Update()
    rgbArray = reader.GetOutput().GetPointData().GetArray(0)
    stackSize = rgbArray.GetNumberOfTuples()
    size = reader.GetOutput().GetDimensions()


    outputArray = vtkFloatArray()
    outputArray.SetNumberOfComponents(1)
    outputArray.SetNumberOfTuples(stackSize)

    for idx in range(stackSize):
        outputArray.SetTuple1(
            idx,
            getScalarFromRGB(rgbArray.GetTuple(idx), scalarRange)
        )

    # Write float file
    with open(destFile, 'wb') as f:
        f.write(buffer(outputArray))

    return size


# -----------------------------------------------------------------------------
# Composite.json To order.array
# -----------------------------------------------------------------------------
class CompositeJSON(object):
    def __init__(self, numberOfLayers):
        self.nbLayers = numberOfLayers
        self.encoding = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'

    def load(self, file):
        with open(file, "r") as f:
            composite = json.load(f)
            self.width = composite["dimensions"][0]
            self.height = composite["dimensions"][1]
            self.pixels = composite["pixel-order"].split('+')
            self.imageSize = self.width * self.height
            self.stackSize = self.imageSize * self.nbLayers

    def getImageSize(self):
        return self.imageSize

    def getStackSize(self):
        return self.stackSize

    def writeOrderSprite(self,path):
        ds = vtkImageData()
        ds.SetDimensions(self.width, self.height, self.nbLayers)
        ds.GetPointData().AddArray(self.getSortedOrderArray())

        writer = vtkDataSetWriter()
        writer.SetInputData(ds)
        writer.SetFileName(path)
        writer.Update()


    def getSortedOrderArray(self):
        sortedOrder = vtkUnsignedCharArray()
        sortedOrder.SetName('layerIdx');
        sortedOrder.SetNumberOfTuples(self.stackSize)

        # Reset content
        for idx in range(self.stackSize):
            sortedOrder.SetValue(idx, 255)

        idx = 0
        for pixel in self.pixels:
            x = (idx % self.width)
            y = (idx / self.width)
            flipYIdx = self.width * (self.height - y - 1) + x
            if '@' in pixel:
                idx += int(pixel[1:])
            else:
                # Need to decode the order
                layerIdx = 0
                for layer in pixel:
                    sortedOrder.SetValue(flipYIdx + self.imageSize * layerIdx, self.encoding.index(layer))
                    layerIdx += 1

                # Move to next pixel
                idx += 1

        return sortedOrder

# -----------------------------------------------------------------------------
# Composite Sprite to Sorted Composite Dataset Builder
# -----------------------------------------------------------------------------
class ConvertCompositeSpriteToSortedStack(object):
    def __init__(self, directory):
        self.basePath = directory
        self.layers = []
        self.data = []
        self.imageReader = vtkPNGReader()

        # Load JSON metadata
        with open(os.path.join(directory, "config.json"), "r") as f:
            self.config = json.load(f)
            self.nbLayers = len(self.config['scene'])
            while len(self.layers) < self.nbLayers:
                self.layers.append({})

        with open(os.path.join(directory, "index.json"), "r") as f:
            self.info = json.load(f)

        with open(os.path.join(directory, "offset.json"), "r") as f:
            offsets = json.load(f)
            for key, value in offsets.iteritems():
                meta = key.split('|')
                if len(meta) == 2:
                    self.layers[int(meta[0])][meta[1]] = value
                elif meta[1] in self.layers[int(meta[0])]:
                    self.layers[int(meta[0])][meta[1]][int(meta[2])] = value
                else:
                    self.layers[int(meta[0])][meta[1]] = [value, value, value]

        self.composite = CompositeJSON(len(self.layers))

    def listData(self):
        return self.data

    def convert(self):
        for root, dirs, files in os.walk(self.basePath):
            if 'rgb.png' in files:
                print 'Process', root
                self.processDirectory(root)

    def processDirectory(self, directory):
        self.imageReader.SetFileName(os.path.join(directory, 'rgb.png'))
        self.imageReader.Update()
        rgbArray = self.imageReader.GetOutput().GetPointData().GetArray(0)

        self.composite.load(os.path.join(directory, 'composite.json'))
        orderArray = self.composite.getSortedOrderArray()

        imageSize = self.composite.getImageSize()
        stackSize = self.composite.getStackSize()

        # Write order (sorted order way)
        with open(os.path.join(directory, 'order.uint8'), 'wb') as f:
            f.write(buffer(orderArray))
            self.data.append({'name': 'order', 'type': 'array', 'fileName': '/order.uint8'})

        # Encode Normals (sorted order way)
        if 'normal' in self.layers[0]:
            sortedNormal = vtkUnsignedCharArray()
            sortedNormal.SetNumberOfComponents(3) # x,y,z
            sortedNormal.SetNumberOfTuples(stackSize)

            # Get Camera orientation and rotation information
            camDir = [0,0,0]
            worldUp = [0,0,0]
            with open(os.path.join(directory, "camera.json"), "r") as f:
                camera = json.load(f)
                camDir = normalize([ camera['position'][i] - camera['focalPoint'][i] for i in range(3) ])
                worldUp = normalize(camera['viewUp'])

            # [ camRight, camUp, camDir ] will be our new orthonormal basis for normals
            camRight = vectProduct(camDir, worldUp)
            camUp = vectProduct(camRight, camDir)

            # Tmp structure to capture (x,y,z) normal
            normalByLayer = vtkFloatArray()
            normalByLayer.SetNumberOfComponents(3)
            normalByLayer.SetNumberOfTuples(stackSize)

            # Capture all layer normals
            layerIdx = 0
            zPosCount = 0
            zNegCount = 0
            for layer in self.layers:
                normalOffset = layer['normal']
                for idx in range(imageSize):
                    normalByLayer.SetTuple3(
                        layerIdx * imageSize + idx,
                        getScalarFromRGB(rgbArray.GetTuple(idx + normalOffset[0] * imageSize)),
                        getScalarFromRGB(rgbArray.GetTuple(idx + normalOffset[1] * imageSize)),
                        getScalarFromRGB(rgbArray.GetTuple(idx + normalOffset[2] * imageSize))
                    )

                    # Re-orient normal to be view based
                    vect = normalByLayer.GetTuple3(layerIdx * imageSize + idx)
                    if not math.isnan(vect[0]):
                        # Express normal in new basis we computed above
                        rVect = normalize([ -dotProduct(vect, camRight), dotProduct(vect, camUp), dotProduct(vect, camDir)  ])

                        # Need to reverse vector ?
                        if rVect[2] < 0:
                            normalByLayer.SetTuple3(layerIdx * imageSize + idx, -rVect[0], -rVect[1], -rVect[2])
                        else:
                            normalByLayer.SetTuple3(layerIdx * imageSize + idx, rVect[0], rVect[1], rVect[2])

                layerIdx += 1

            # Sort normals and encode them as 3 bytes ( -1 < xy < 1 | 0 < z < 1)
            for idx in range(stackSize):
                layerIdx = int(orderArray.GetValue(idx))
                if layerIdx == 255:
                    # No normal => same as view direction
                    sortedNormal.SetTuple3(idx, 128, 128, 255)
                else:
                    offset = layerIdx * imageSize
                    imageIdx = idx % imageSize
                    vect = normalByLayer.GetTuple3(imageIdx + offset)
                    if not math.isnan(vect[0]) and not math.isnan(vect[1]) and not math.isnan(vect[2]):
                        sortedNormal.SetTuple3(idx, int(127.5 * (vect[0] + 1)), int(127.5 * (vect[1] + 1)), int(255 * vect[2]))
                    else:
                        print 'WARNING: encountered NaN in normal of layer ',layerIdx,': [',vect[0],',',vect[1],',',vect[2],']'
                        sortedNormal.SetTuple3(idx, 128, 128, 255)

            # Write the sorted data
            with open(os.path.join(directory, 'normal.uint8'), 'wb') as f:
                f.write(buffer(sortedNormal))
                self.data.append({'name': 'normal', 'type': 'array', 'fileName': '/normal.uint8', 'categories': ['normal']})

        # Encode Intensity (sorted order way)
        if 'intensity' in self.layers[0]:
            intensityOffsets = []
            sortedIntensity = vtkUnsignedCharArray()
            sortedIntensity.SetNumberOfTuples(stackSize)

            for layer in self.layers:
                intensityOffsets.append(layer['intensity'])

            for idx in range(stackSize):
                layerIdx = int(orderArray.GetValue(idx))
                if layerIdx == 255:
                    sortedIntensity.SetValue(idx, 255)
                else:
                    offset = 3 * intensityOffsets[layerIdx] * imageSize
                    imageIdx = idx % imageSize
                    sortedIntensity.SetValue(idx, rgbArray.GetValue(imageIdx * 3 + offset))

            with open(os.path.join(directory, 'intensity.uint8'), 'wb') as f:
                f.write(buffer(sortedIntensity))
                self.data.append({'name': 'intensity', 'type': 'array', 'fileName': '/intensity.uint8', 'categories': ['intensity']})

        # Encode Each layer Scalar
        layerIdx = 0
        for layer in self.layers:
            for scalar in layer:
                if scalar not in ['intensity', 'normal']:
                    offset = imageSize * layer[scalar]
                    scalarRange = self.config['scene'][layerIdx]['colors'][scalar]['range']
                    delta = (scalarRange[1] - scalarRange[0]) / 16777215.0 # 2^24 - 1 => 16,777,215

                    scalarArray = vtkFloatArray()
                    scalarArray.SetNumberOfTuples(imageSize)
                    for idx in range(imageSize):
                        rgb = rgbArray.GetTuple(idx + offset)
                        if rgb[0] != 0 or rgb[1] != 0 or rgb[2] != 0:
                            # Decode encoded value
                            value = scalarRange[0] + delta * float(rgb[0]*65536 + rgb[1]*256 + rgb[2] - 1)
                            scalarArray.SetValue(idx, value)
                        else:
                            # No value
                            scalarArray.SetValue(idx, float('NaN'))


                    with open(os.path.join(directory, '%d_%s.float32' % (layerIdx, scalar)), 'wb') as f:
                        f.write(buffer(scalarArray))
                        self.data.append({'name': '%d_%s' % (layerIdx, scalar), 'type': 'array', 'fileName': '/%d_%s.float32' % (layerIdx, scalar), 'categories': ['%d_%s' % (layerIdx, scalar)]})

            layerIdx += 1
