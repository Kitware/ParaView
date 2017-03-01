from vtk.web import camera
from paraview.web import data_writer
from paraview.web import data_converter

from vtk.web.query_data_model import *
from paraview.web.camera import *
from vtk.web import iteritems
from paraview import simple
from paraview import servermanager
from vtk import *

import json, os, math, gzip, shutil, hashlib

# Global helper variables
encode_codes = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
arrayTypesMapping = '  bBhHiIlLfd'
jsMapping = {
    'b': 'Int8Array',
    'B': 'Uint8Array',
    'h': 'Int16Array',
    'H': 'Int16Array',
    'i': 'Int32Array',
    'I': 'Uint32Array',
    'l': 'Int32Array',
    'L': 'Uint32Array',
    'f': 'Float32Array',
    'd': 'Float64Array'
}

# -----------------------------------------------------------------------------
# Basic Dataset Builder
# -----------------------------------------------------------------------------

class DataSetBuilder(object):
    def __init__(self, location, camera_data, metadata={}, sections={}):
        self.dataHandler = DataHandler(location)
        self.cameraDescription = camera_data
        self.camera = None

        for key, value in iteritems(metadata):
            self.dataHandler.addMetaData(key, value)

        for key, value in iteritems(sections):
            self.dataHandler.addSection(key, value)

        # Update the can_write flag for MPI
        self.dataHandler.can_write = (servermanager.vtkProcessModule.GetProcessModule().GetPartitionId() == 0)

    def getDataHandler(self):
        return self.dataHandler

    def getCamera(self):
        return self.camera

    def updateCamera(self, camera):
        update_camera(self.view, camera)

    def start(self, view=None):
        if view:
            # Keep track of the view
            self.view = view

            # Handle camera if any
            if self.cameraDescription:
                if self.cameraDescription['type'] == 'spherical':
                    self.camera = camera.SphericalCamera(self.dataHandler, view.CenterOfRotation, view.CameraPosition, view.CameraViewUp, self.cameraDescription['phi'], self.cameraDescription['theta'])
                elif self.cameraDescription['type'] == 'cylindrical':
                    self.camera = camera.CylindricalCamera(self.dataHandler, view.CenterOfRotation, view.CameraPosition, view.CameraViewUp, self.cameraDescription['phi'], self.cameraDescription['translation'])

            # Update background color
            bgColor = view.Background
            bgColorString = 'rgb(%d, %d, %d)' % tuple(int(bgColor[i]*255) for i in range(3))

            if view.UseGradientBackground:
                bgColor2 = view.Background2
                bgColor2String = 'rgb(%d, %d, %d)' % tuple(int(bgColor2[i]*255) for i in range(3))
                self.dataHandler.addMetaData('backgroundColor', 'linear-gradient(%s,%s)' % (bgColor2String, bgColorString))
            else:
                self.dataHandler.addMetaData('backgroundColor', bgColorString)

        # Update file patterns
        self.dataHandler.updateBasePattern()

    def stop(self):
        self.dataHandler.writeDataDescriptor()

# -----------------------------------------------------------------------------
# Image Dataset Builder
# -----------------------------------------------------------------------------

class ImageDataSetBuilder(DataSetBuilder):
    def __init__(self, location, imageMimeType, cameraInfo, metadata={}):
        DataSetBuilder.__init__(self, location, cameraInfo, metadata)
        imageExtenstion = '.' + imageMimeType.split('/')[1]
        self.dataHandler.registerData(name='image', type='blob', mimeType=imageMimeType, fileName=imageExtenstion)

    def writeImages(self):
        for cam in self.camera:
            update_camera(self.view, cam)
            simple.WriteImage(self.dataHandler.getDataAbsoluteFilePath('image'))

# -----------------------------------------------------------------------------
# Data Prober Dataset Builder
# -----------------------------------------------------------------------------
class DataProberDataSetBuilder(DataSetBuilder):
    def __init__(self, input, location, sampling_dimesions, fields_to_keep, custom_probing_bounds = None, metadata={}):
        DataSetBuilder.__init__(self, location, None, metadata)
        self.fieldsToWrite = fields_to_keep
        self.resamplerFilter = simple.ResampleToImage(Input=input)
        self.resamplerFilter.SamplingDimensions = sampling_dimesions
        if custom_probing_bounds:
            self.resamplerFilter.UseInputBounds = 0
            self.resamplerFilter.SamplingBounds = custom_probing_bounds
        else:
            self.resamplerFilter.UseInputBounds = 1

        # Register all fields
        self.dataHandler.addTypes('data-prober', 'binary')
        self.DataProber = { 'types': {}, 'dimensions': sampling_dimesions, 'ranges': {}, 'spacing': [1,1,1] }
        for field in self.fieldsToWrite:
            self.dataHandler.registerData(name=field, type='array', rootFile=True, fileName='%s.array' % field)

    def writeData(self, time=0):
        if not self.dataHandler.can_write:
            return

        self.resamplerFilter.UpdatePipeline(time)
        imageData = self.resamplerFilter.GetClientSideObject().GetOutput()
        self.DataProber['spacing'] = imageData.GetSpacing()
        arrays = imageData.GetPointData()
        maskArray = arrays.GetArray(vtk.vtkDataSetAttributes.GhostArrayName())
        for field in self.fieldsToWrite:
            array = arrays.GetArray(field)
            if array:
                if array.GetNumberOfComponents() == 1:
                    # Push NaN when no value are present instead of 0
                    for idx in range(maskArray.GetNumberOfTuples()):
                        if maskArray.GetValue(idx) == 2: # Hidden point
                            array.SetValue(idx, float('NaN'))

                    with open(self.dataHandler.getDataAbsoluteFilePath(field), 'wb') as f:
                        f.write(buffer(array))

                    self.expandRange(array)
                else:
                    magarray = array.NewInstance()
                    magarray.SetNumberOfTuples(array.GetNumberOfTuples())
                    magarray.SetName(field)

                    for idx in range(magarray.GetNumberOfTuples()):
                        if maskArray.GetValue(idx) == 2: # Hidden point
                            # Push NaN when no value are present
                            magarray.SetValue(idx, float('NaN'))
                        else:
                            entry = array.GetTuple(idx)
                            mag = self.magnitude(entry)
                            magarray.SetValue(idx,mag)

                    with open(self.dataHandler.getDataAbsoluteFilePath(field), 'wb') as f:
                        f.write(buffer(magarray))

                    self.expandRange(magarray)
            else:
                print ('No array for', field)
                print (self.resamplerFilter.GetOutput())

    def magnitude(self, tuple):
        value = 0
        for item in tuple:
            value += item * item
        value = value**0.5

        return value

    def expandRange(self, array):
        field = array.GetName()
        self.DataProber['types'][field] = jsMapping[arrayTypesMapping[array.GetDataType()]]

        if field in self.DataProber['ranges']:
            dataRange = array.GetRange()
            if dataRange[0] < self.DataProber['ranges'][field][0]:
                self.DataProber['ranges'][field][0] = dataRange[0]
            if dataRange[1] > self.DataProber['ranges'][field][1]:
                self.DataProber['ranges'][field][1] = dataRange[1]
        else:
            self.DataProber['ranges'][field] = [array.GetRange()[0], array.GetRange()[1]]

    def stop(self, compress=True):
        # Rescale spacing to have the smaller value to be 1.0
        smallerValue = min(self.DataProber['spacing'])
        if smallerValue < 1.0:
            self.DataProber['spacing'] = tuple( i / smallerValue for i in self.DataProber['spacing'])

        # Push metadata
        self.dataHandler.addSection('DataProber', self.DataProber)

        # Write metadata
        DataSetBuilder.stop(self)

        if compress:
            for root, dirs, files in os.walk(self.dataHandler.getBasePath()):
                print ('Compress', root)
                for name in files:
                    if '.array' in name and '.gz' not in name:
                        with open(os.path.join(root, name), 'rb') as f_in:
                            with gzip.open(os.path.join(root, name + '.gz'), 'wb') as f_out:
                                shutil.copyfileobj(f_in, f_out)
                        os.remove(os.path.join(root, name))


# -----------------------------------------------------------------------------
# Float Image with Layer Dataset Builder
# -----------------------------------------------------------------------------

class LayerDataSetBuilder(DataSetBuilder):
    def __init__(self, input, location, cameraInfo, imageSize=[500,500], metadata={}):
        DataSetBuilder.__init__(self, location, cameraInfo, metadata)
        self.dataRenderer = data_writer.ScalarRenderer(isWriter=self.dataHandler.can_write)
        self.view = self.dataRenderer.getView()
        self.view.ViewSize = imageSize
        self.floatImage = {'dimensions': imageSize, 'layers': [], 'ranges': {}}
        self.layerMap = {}
        self.input = input
        self.activeLayer = None
        self.activeField = None
        self.layerChanged = False
        self.lastTime = -1

        # Update data type
        self.dataHandler.addTypes('float-image')

    def getView(self):
        return self.view

    def setActiveLayer(self, layer, field, hasMesh=False, activeSource=None):
        if activeSource:
            self.activeSource = activeSource
        else:
            self.activeSource = self.input
        needDataRegistration = False
        if layer not in self.layerMap:
            layerObj = { 'name': layer, 'array': field, 'arrays': [ field ], 'active': True, 'type': 'Float32Array', 'hasMesh': hasMesh }
            self.layerMap[layer] = layerObj
            self.floatImage['layers'].append(layerObj)
            needDataRegistration = True

            # Register layer lighting
            self.dataHandler.registerData(name='%s__light' % layer, type='array', rootFile=True, fileName='%s__light.array' % layer, categories=[ '%s__light' % layer ])

            # Register layer mesh
            if hasMesh:
                self.dataHandler.registerData(name='%s__mesh' % layer, type='array', rootFile=True, fileName='%s__mesh.array' % layer, categories=[ '%s__mesh' % layer ])

        elif field not in self.layerMap[layer]['arrays']:
            self.layerMap[layer]['arrays'].append(field)
            needDataRegistration = True

        # Keep track of the active data
        if self.activeLayer != layer:
            self.layerChanged = True
        self.activeLayer = layer
        self.activeField = field

        if needDataRegistration:
            self.dataHandler.registerData(name='%s_%s' % (layer, field), type='array', rootFile=True, fileName='%s_%s.array' % (layer, field), categories=[ '%s_%s' % (layer, field) ])

    def writeLayerData(self, time=0):
        dataRange = [0, 1]
        self.activeSource.UpdatePipeline(time)

        if self.activeField and self.activeLayer:

            if self.layerChanged or self.lastTime != time:
                self.layerChanged = False
                self.lastTime = time

                # Capture lighting information
                for camPos in self.getCamera():
                    self.view.CameraFocalPoint = camPos['focalPoint']
                    self.view.CameraPosition = camPos['position']
                    self.view.CameraViewUp = camPos['viewUp']
                    self.dataRenderer.writeLightArray(self.dataHandler.getDataAbsoluteFilePath('%s__light'%self.activeLayer), self.activeSource)

                # Capture mesh information
                if self.layerMap[self.activeLayer]['hasMesh']:
                    for camPos in self.getCamera():
                        self.view.CameraFocalPoint = camPos['focalPoint']
                        self.view.CameraPosition = camPos['position']
                        self.view.CameraViewUp = camPos['viewUp']
                        self.dataRenderer.writeMeshArray(self.dataHandler.getDataAbsoluteFilePath('%s__mesh'%self.activeLayer), self.activeSource)

            for camPos in self.getCamera():
                self.view.CameraFocalPoint = camPos['focalPoint']
                self.view.CameraPosition = camPos['position']
                self.view.CameraViewUp = camPos['viewUp']
                dataName = ('%s_%s' % (self.activeLayer, self.activeField))
                dataRange = self.dataRenderer.writeArray(self.dataHandler.getDataAbsoluteFilePath(dataName), self.activeSource, self.activeField)

            if self.activeField not in self.floatImage['ranges']:
                self.floatImage['ranges'][self.activeField] = [ dataRange[0], dataRange[1] ]
            else:
                # Expand the ranges
                if dataRange[0] < self.floatImage['ranges'][self.activeField][0]:
                    self.floatImage['ranges'][self.activeField][0] = dataRange[0]
                if dataRange[1] > self.floatImage['ranges'][self.activeField][1]:
                    self.floatImage['ranges'][self.activeField][1] = dataRange[1]

    def start(self):
        DataSetBuilder.start(self, self.view)

    def stop(self, compress=True):
        if not self.dataHandler.can_write:
            return

        # Push metadata
        self.dataHandler.addSection('FloatImage', self.floatImage)

        # Write metadata
        DataSetBuilder.stop(self)

        if compress:
            for root, dirs, files in os.walk(self.dataHandler.getBasePath()):
                print ('Compress', root)
                for name in files:
                    if '.array' in name and '.gz' not in name:
                        with open(os.path.join(root, name), 'rb') as f_in:
                            with gzip.open(os.path.join(root, name + '.gz'), 'wb') as f_out:
                                shutil.copyfileobj(f_in, f_out)
                        os.remove(os.path.join(root, name))


# -----------------------------------------------------------------------------
# Composite Dataset Builder
# -----------------------------------------------------------------------------

class CompositeDataSetBuilder(DataSetBuilder):
    def __init__(self, location, sceneConfig, cameraInfo, metadata={}, sections={}, view=None):
        DataSetBuilder.__init__(self, location, cameraInfo, metadata, sections)

        if view:
            self.view = view
        else:
            self.view = simple.CreateView('RenderView')

        self.view.ViewSize = sceneConfig['size']
        self.view.CenterAxesVisibility = 0
        self.view.OrientationAxesVisibility = 0
        self.view.UpdatePropertyInformation()
        if 'camera' in sceneConfig and 'CameraFocalPoint' in sceneConfig['camera']:
            self.view.CenterOfRotation = sceneConfig['camera']['CameraFocalPoint']

        # Initialize camera
        for key, value in iteritems(sceneConfig['camera']):
            self.view.GetProperty(key).SetData(value)

        # Create a representation for all scene sources
        self.config = sceneConfig
        self.representations = []
        for data in self.config['scene']:
            rep = simple.Show(data['source'], self.view)
            self.representations.append(rep)
            if 'representation' in data:
                for key in data['representation']:
                    rep.GetProperty(key).SetData(data['representation'][key])

        # Add directory path
        self.dataHandler.registerData(name='directory', rootFile=True, fileName='file.txt', categories=['trash'])

    def start(self):
        DataSetBuilder.start(self, self.view)

    def stop(self, compress=True, clean=True):
        DataSetBuilder.stop(self)

        if not self.dataHandler.can_write:
            return

        # Make the config serializable
        for item in self.config['scene']:
            del item['source']

        # Write the scene to disk
        with open(os.path.join(self.dataHandler.getBasePath(), "config.json"), 'w') as f:
            f.write(json.dumps(self.config))

        dataConverter = data_converter.ConvertCompositeDataToSortedStack(self.dataHandler.getBasePath())
        dataConverter.convert()

        # Remove tmp files
        os.remove(os.path.join(self.dataHandler.getBasePath(), "index.json"))
        os.remove(os.path.join(self.dataHandler.getBasePath(), "config.json"))

        # Composite pipeline meta description
        compositePipeline = {
            'default_pipeline': '',
            'layers': [],
            'fields': {},
            'layer_fields': {},
            'pipeline': []
        }
        rootItems = {}
        fieldNameMapping = {}

        # Clean scene in config and gather ranges
        dataRanges = {}
        layerIdx = 0
        for layer in self.config['scene']:
            # Create group node if any
            if 'parent' in layer and layer['parent'] not in rootItems:
                rootItems[layer['parent']] = { 'name': layer['parent'], 'ids': [], 'children': [] }
                compositePipeline['pipeline'].append(rootItems[layer['parent']])

            # Create layer entry
            layerCode = encode_codes[layerIdx]
            layerItem = { 'name': layer['name'], 'ids': [ layerCode ]}
            compositePipeline['layers'].append(layerCode)
            compositePipeline['layer_fields'][layerCode] = []
            compositePipeline['default_pipeline'] += layerCode

            # Register layer entry in pipeline
            if 'parent' in layer:
                rootItems[layer['parent']]['children'].append(layerItem)
                rootItems[layer['parent']]['ids'].append(layerCode)
            else:
                compositePipeline['pipeline'].append(layerItem)

            # Handle color / field
            colorByList = []
            for color in layer['colors']:
                # Find color code
                if color not in fieldNameMapping:
                    colorCode = encode_codes[len(fieldNameMapping)]
                    fieldNameMapping[color] = colorCode
                    compositePipeline['fields'][colorCode] = color
                else:
                    colorCode = fieldNameMapping[color]

                # Register color code
                compositePipeline['layer_fields'][layerCode].append(colorCode)
                if len(colorByList) == 0:
                    compositePipeline['default_pipeline'] += colorCode

                values = None
                if 'constant' in layer['colors'][color]:
                    value = layer['colors'][color]['constant']
                    values = [ value, value ]
                    colorByList.append({'name': color, 'type': 'const', 'value': value})
                elif 'range' in layer['colors'][color]:
                    values = layer['colors'][color]['range']
                    colorByList.append({'name': color, 'type': 'field'})

                if values:
                    if color not in dataRanges:
                        dataRanges[color] = values
                    else:
                        dataRanges[color][0] = min(dataRanges[color][0], values[0], values[1])
                        dataRanges[color][1] = max(dataRanges[color][1], values[0], values[1])

            layer['colorBy'] = colorByList
            del layer['colors']
            layerIdx += 1

        sortedCompositeSection = {
            'dimensions': self.config['size'],
            'pipeline': self.config['scene'],
            'ranges': dataRanges,
            'layers': len(self.config['scene']),
            'light': self.config['light']
        }
        self.dataHandler.addSection('SortedComposite', sortedCompositeSection)
        self.dataHandler.addSection('CompositePipeline', compositePipeline)
        self.dataHandler.addTypes('sorted-composite', 'multi-color-by')

        self.dataHandler.removeData('directory')
        for dataToRegister in dataConverter.listData():
            self.dataHandler.registerData(**dataToRegister)

        self.dataHandler.writeDataDescriptor()

        if clean:
          for root, dirs, files in os.walk(self.dataHandler.getBasePath()):
              print ('Clean', root)
              for name in files:
                  if name in ['camera.json']:
                      os.remove(os.path.join(root, name))

        if compress:
            for root, dirs, files in os.walk(self.dataHandler.getBasePath()):
                print ('Compress', root)
                for name in files:
                    if ('.float32' in name or '.uint8' in name) and '.gz' not in name:
                        with open(os.path.join(root, name), 'rb') as f_in:
                            with gzip.open(os.path.join(root, name + '.gz'), 'wb') as f_out:
                                shutil.copyfileobj(f_in, f_out)
                        os.remove(os.path.join(root, name))

    def writeData(self):
        composite_size = len(self.representations)

        self.view.UpdatePropertyInformation()
        self.view.Background = [0,0,0]
        imageSize = self.view.ViewSize[0] * self.view.ViewSize[1]

        # Generate the heavy data
        for camPos in self.getCamera():
            self.view.CameraFocalPoint = camPos['focalPoint']
            self.view.CameraPosition = camPos['position']
            self.view.CameraViewUp = camPos['viewUp']

            # Show all representations
            for compositeIdx in range(composite_size):
                rep = self.representations[compositeIdx]
                rep.Visibility = 1

            # Fix camera bounds
            self.view.LockBounds = 0
            simple.Render(self.view)
            self.view.LockBounds = 1

            # Update destination directory
            dest_path = os.path.dirname(self.dataHandler.getDataAbsoluteFilePath('directory'))

            # Write camera informations
            if self.dataHandler.can_write:
                with open(os.path.join(dest_path, "camera.json"), 'w') as f:
                    f.write(json.dumps(camPos))

            # Hide all representations
            for compositeIdx in range(composite_size):
                rep = self.representations[compositeIdx]
                rep.Visibility = 0

            # Show only active Representation
            # Extract images for each fields
            for compositeIdx in range(composite_size):
                rep = self.representations[compositeIdx]
                if compositeIdx > 0:
                    self.representations[compositeIdx - 1].Visibility = 0
                rep.Visibility = 1

                # capture Z
                simple.Render()
                zBuffer = self.view.CaptureDepthBuffer()
                with open(os.path.join(dest_path, 'depth_%d.float32' % compositeIdx), 'wb') as f:
                    f.write(buffer(zBuffer))

                # Prevent color interference
                rep.DiffuseColor = [1,1,1]

                # Handle light
                for lightType in self.config['light']:
                    if lightType == 'intensity':
                        rep.AmbientColor  = [1,1,1]
                        rep.SpecularColor = [1,1,1]

                        self.view.StartCaptureLuminance()
                        image = self.view.CaptureWindow(1)
                        imagescalars = image.GetPointData().GetScalars()
                        self.view.StopCaptureLuminance()

                        # Extract specular information
                        specularOffset = 1 # [diffuse, specular, ?]
                        imageSize = imagescalars.GetNumberOfTuples()
                        specularComponent = vtkUnsignedCharArray()
                        specularComponent.SetNumberOfComponents(1)
                        specularComponent.SetNumberOfTuples(imageSize)

                        for idx in range(imageSize):
                            specularComponent.SetValue(idx, imagescalars.GetValue(idx * 3 + specularOffset))

                        with open(os.path.join(dest_path, 'intensity_%d.uint8' % compositeIdx), 'wb') as f:
                            f.write(buffer(specularComponent))

                        # Free memory
                        image.UnRegister(None)

                    if lightType == 'normal':
                        if rep.Representation in ['Point Gaussian', 'Points', 'Outline', 'Wireframe']:
                            uniqNormal = [(camPos['position'][i] - camPos['focalPoint'][i]) for i in range(3)]
                            tmpNormalArray = vtkFloatArray()
                            tmpNormalArray.SetNumberOfComponents(1)
                            tmpNormalArray.SetNumberOfTuples(imageSize)

                            for comp in range(3):
                                tmpNormalArray.FillComponent(0, uniqNormal[comp])
                                with open(os.path.join(dest_path, 'normal_%d_%d.float32' % (compositeIdx, comp)), 'wb') as f:
                                    f.write(buffer(tmpNormalArray))
                        else:
                            for comp in range(3):
                                # Configure view to handle POINT_DATA / CELL_DATA
                                self.view.DrawCells = 0
                                self.view.ArrayNameToDraw = 'Normals'
                                self.view.ArrayComponentToDraw = comp
                                self.view.ScalarRange = [-1.0, 1.0]
                                self.view.StartCaptureValues()
                                image = self.view.CaptureWindow(1)
                                imagescalars = image.GetPointData().GetScalars()
                                self.view.StopCaptureValues()

                                # Convert RGB => Float => Write
                                floatArray = data_converter.convertRGBArrayToFloatArray(imagescalars, [-1.0, 1.0])
                                with open(os.path.join(dest_path, 'normal_%d_%d.float32' % (compositeIdx, comp)), 'wb') as f:
                                    f.write(buffer(floatArray))

                                # Free memory
                                image.UnRegister(None)

                # Handle color by
                for fieldName, fieldConfig in iteritems(self.config['scene'][compositeIdx]['colors']):
                    if 'constant' in fieldConfig:
                        # Skip nothing to render
                        continue

                    # Configure view to handle POINT_DATA / CELL_DATA
                    if fieldConfig['location'] == 'POINT_DATA':
                        self.view.DrawCells = 0
                    else:
                        self.view.DrawCells = 1

                    self.view.ArrayNameToDraw = fieldName
                    self.view.ArrayComponentToDraw = 0
                    self.view.ScalarRange = fieldConfig['range']
                    self.view.StartCaptureValues()
                    image = self.view.CaptureWindow(1)
                    imagescalars = image.GetPointData().GetScalars()
                    self.view.StopCaptureValues()

                    floatArray = data_converter.convertRGBArrayToFloatArray(imagescalars, fieldConfig['range'])
                    with open(os.path.join(dest_path, '%d_%s.float32' % (compositeIdx, fieldName)), 'wb') as f:
                        f.write(buffer(floatArray))
                        self.dataHandler.registerData(name='%d_%s' % (compositeIdx, fieldName), fileName='/%d_%s.float32' % (compositeIdx, fieldName), type='array', categories=['%d_%s' % (compositeIdx, fieldName)])

                    # Free memory
                    image.UnRegister(None)

# -----------------------------------------------------------------------------
# VTKGeometryDataSetBuilder Dataset Builder
# -----------------------------------------------------------------------------

def writeCellArray(dataHandler, currentData, cellName, inputCellArray):
    nbValues = inputCellArray.GetNumberOfTuples()
    if nbValues == 0:
        return

    outputCells = vtkTypeUInt32Array()
    outputCells.SetNumberOfTuples(nbValues)

    for valueIdx in range(nbValues):
        outputCells.SetValue(valueIdx, inputCellArray.GetValue(valueIdx))

    iBuffer = buffer(outputCells)
    iMd5 = hashlib.md5(iBuffer).hexdigest()
    iPath = os.path.join(dataHandler.getBasePath(), 'data',"%s.Uint32Array" % iMd5)
    currentData['polys'] = 'data/%s.Uint32Array' % iMd5
    with open(iPath, 'wb') as f:
        f.write(iBuffer)

class VTKGeometryDataSetBuilder(DataSetBuilder):
    def __init__(self, location, sceneConfig, metadata={}, sections={}):
        DataSetBuilder.__init__(self, location, None, metadata, sections)

        # Update data type
        self.dataHandler.addTypes('vtk-geometry');

        # Create a representation for all scene sources
        self.config = sceneConfig

        # Processing pipeline
        self.surfaceExtract = None

        # Add directory path
        self.dataHandler.registerData(priority=0, name='scene', rootFile=True, fileName='scene.json', type='json')

        # Create directory containers
        dataPath = os.path.join(location, 'data')
        for p in [dataPath]:
            if not os.path.exists(p):
                os.makedirs(p)

        # Create metadata structure
        colorToCodeMap = {}
        parentNodes = {}
        pipelineMeta = {
            'layers': [],
            'pipeline': [],
            'layer_fields': {},
            'fields': {}
        }
        geometryMeta = {
            'ranges': {},
            'layer_map': {},
        }
        self.ranges = geometryMeta['ranges']
        for item in sceneConfig['scene']:
            # Handle layer
            layerCode = encode_codes[len(pipelineMeta['layers'])]
            pipelineMeta['layers'].append(layerCode)
            geometryMeta['layer_map'][layerCode] = item['name']

            # Handle colors
            pipelineMeta['layer_fields'][layerCode] = []
            for fieldName in item['colors']:
                colorCode = None
                if fieldName in colorToCodeMap:
                    colorCode = colorToCodeMap[fieldName]
                else:
                    colorCode = encode_codes[len(colorToCodeMap)]
                    colorToCodeMap[fieldName] = colorCode
                    geometryMeta['ranges'][fieldName] = [1, -1] # FIXME we don't know the range

                pipelineMeta['layer_fields'][layerCode].append(colorCode)
                pipelineMeta['fields'][colorCode] = fieldName

            # Handle pipeline
            if 'parent' in item:
                # Need to handle hierarchy
                if item['parent'] in parentNodes:
                    # Fill children
                    rootNode = parentNodes[item['parent']]
                    rootNode['ids'].append(layerCode)
                    rootNode['children'].append({
                        'name': item['name'],
                        'ids': [layerCode]
                    })
                else:
                    # Create root + register
                    rootNode = {
                        'name': item['parent'],
                        'ids': [ layerCode ],
                        'children': [
                            {
                                'name': item['name'],
                                'ids': [ layerCode ]
                            }
                        ]
                    }
                    parentNodes[item['parent']] = rootNode
                    pipelineMeta['pipeline'].append(rootNode)
            else:
                # Add item info as a new pipeline node
                pipelineMeta['pipeline'].append({
                    'name': item['name'],
                    'ids': [layerCode]
                })

        # Register metadata to be written in index.json
        self.dataHandler.addSection('Geometry', geometryMeta)
        self.dataHandler.addSection('CompositePipeline', pipelineMeta)

    def writeData(self, time=0):
        if not self.dataHandler.can_write:
            return

        currentScene = [];
        for data in self.config['scene']:
            currentData = {
                'name': data['name'],
                'fields': {},
                'cells': {}
            }
            currentScene.append(currentData)
            if self.surfaceExtract:
                self.merge.Input = data['source']
            else:
                self.merge = simple.MergeBlocks(Input=data['source'], MergePoints=0)
                self.surfaceExtract = simple.ExtractSurface(Input=self.merge)


            # Extract surface
            self.surfaceExtract.UpdatePipeline(time)
            ds = self.surfaceExtract.SMProxy.GetClientSideObject().GetOutputDataObject(0)
            originalDS = data['source'].SMProxy.GetClientSideObject().GetOutputDataObject(0)

            originalPoints = ds.GetPoints()

            # Points
            points = vtkFloatArray()
            nbPoints = originalPoints.GetNumberOfPoints()
            points.SetNumberOfComponents(3)
            points.SetNumberOfTuples(nbPoints)
            for idx in range(nbPoints):
                coord = originalPoints.GetPoint(idx)
                points.SetTuple3(idx, coord[0], coord[1], coord[2])

            pBuffer = buffer(points)
            pMd5 = hashlib.md5(pBuffer).hexdigest()
            pPath = os.path.join(self.dataHandler.getBasePath(), 'data',"%s.Float32Array" % pMd5)
            currentData['points'] = 'data/%s.Float32Array' % pMd5
            with open(pPath, 'wb') as f:
                f.write(pBuffer)

            # Handle cells
            writeCellArray(self.dataHandler, currentData['cells'], 'verts', ds.GetVerts().GetData())
            writeCellArray(self.dataHandler, currentData['cells'], 'lines', ds.GetLines().GetData())
            writeCellArray(self.dataHandler, currentData['cells'], 'polys', ds.GetPolys().GetData())
            writeCellArray(self.dataHandler, currentData['cells'], 'strips', ds.GetStrips().GetData())

            # Fields
            for fieldName, fieldInfo in iteritems(data['colors']):
                array = None
                if 'constant' in fieldInfo:
                    currentData['fields'][fieldName] = fieldInfo
                    continue
                elif 'POINT_DATA' in fieldInfo['location']:
                    array = ds.GetPointData().GetArray(fieldName)
                elif 'CELL_DATA' in fieldInfo['location']:
                    array = ds.GetCellData().GetArray(fieldName)
                jsType = jsMapping[arrayTypesMapping[array.GetDataType()]]
                arrayRange = array.GetRange(-1)
                tupleSize = array.GetNumberOfComponents()
                arraySize = array.GetNumberOfTuples()
                if tupleSize == 1:
                    outputField = array
                else:
                    # compute magnitude
                    outputField = array.NewInstance()
                    outputField.SetNumberOfTuples(arraySize)
                    tupleIdxs = range(tupleSize)
                    for i in range(arraySize):
                        magnitude = 0
                        for j in tupleIdxs:
                            magnitude += math.pow(array.GetValue(i * tupleSize + j), 2)

                        outputField.SetValue(i, math.sqrt(magnitude))

                fBuffer = buffer(outputField)
                fMd5 = hashlib.md5(fBuffer).hexdigest()
                fPath = os.path.join(self.dataHandler.getBasePath(), 'data',"%s_%s.%s" % (fieldName, fMd5, jsType))
                with open(fPath, 'wb') as f:
                    f.write(fBuffer)

                currentRange = self.ranges[fieldName]
                if currentRange[1] < currentRange[0]:
                    currentRange[0] = arrayRange[0];
                    currentRange[1] = arrayRange[1];
                else:
                    currentRange[0] = arrayRange[0] if arrayRange[0] < currentRange[0] else currentRange[0];
                    currentRange[1] = arrayRange[1] if arrayRange[1] > currentRange[1] else currentRange[1];

                currentData['fields'][fieldName] = {
                    'array' : 'data/%s_%s.%s' % (fieldName, fMd5, jsType),
                    'location': fieldInfo['location'],
                    'range': outputField.GetRange()
                }

        # Write scene
        with open(self.dataHandler.getDataAbsoluteFilePath('scene'), 'w') as f:
            f.write(json.dumps(currentScene, indent=4))


    def stop(self, compress=True, clean=True):
        if not self.dataHandler.can_write:
            return

        DataSetBuilder.stop(self)

        if compress:
            for dirName in ['fields', 'index', 'points']:
                for root, dirs, files in os.walk(os.path.join(self.dataHandler.getBasePath(), dirName)):
                    print ('Compress', root)
                    for name in files:
                        if 'Array' in name and '.gz' not in name:
                            with open(os.path.join(root, name), 'rb') as f_in:
                                with gzip.open(os.path.join(root, name + '.gz'), 'wb') as f_out:
                                    shutil.copyfileobj(f_in, f_out)
                            os.remove(os.path.join(root, name))

# -----------------------------------------------------------------------------
# GeometryDataSetBuilder Dataset Builder
# -----------------------------------------------------------------------------

class GeometryDataSetBuilder(DataSetBuilder):
    def __init__(self, location, sceneConfig, metadata={}, sections={}):
        DataSetBuilder.__init__(self, location, None, metadata, sections)

        # Update data type
        self.dataHandler.addTypes('geometry');

        # Create a representation for all scene sources
        self.config = sceneConfig

        # Processing pipeline
        self.surfaceExtract = None

        # Add directory path
        self.dataHandler.registerData(priority=0, name='scene', rootFile=True, fileName='scene.json', type='json')

        # Create directory containers
        pointsPath = os.path.join(location, 'points')
        polyPath = os.path.join(location, 'index')
        colorPath = os.path.join(location, 'fields')
        for p in [pointsPath, polyPath, colorPath]:
            if not os.path.exists(p):
                os.makedirs(p)

        # Create metadata structure
        colorToCodeMap = {}
        parentNodes = {}
        pipelineMeta = {
            'layers': [],
            'pipeline': [],
            'layer_fields': {},
            'fields': {}
        }
        geometryMeta = {
            'ranges': {},
            'layer_map': {},
            'object_size': {}
        }
        self.objSize = geometryMeta['object_size']
        for item in sceneConfig['scene']:
            # Handle layer
            layerCode = encode_codes[len(pipelineMeta['layers'])]
            pipelineMeta['layers'].append(layerCode)
            geometryMeta['layer_map'][layerCode] = item['name']
            geometryMeta['object_size'][item['name']] = { 'points': 0, 'index': 0 }

            # Handle colors
            pipelineMeta['layer_fields'][layerCode] = []
            for fieldName in item['colors']:
                colorCode = None
                if fieldName in colorToCodeMap:
                    colorCode = colorToCodeMap[fieldName]
                else:
                    colorCode = encode_codes[len(colorToCodeMap)]
                    colorToCodeMap[fieldName] = colorCode
                    geometryMeta['ranges'][fieldName] = [0, 1] # FIXME we don't know the range

                pipelineMeta['layer_fields'][layerCode].append(colorCode)
                pipelineMeta['fields'][colorCode] = fieldName

            # Handle pipeline
            if 'parent' in item:
                # Need to handle hierarchy
                if item['parent'] in parentNodes:
                    # Fill children
                    rootNode = parentNodes[item['parent']]
                    rootNode['ids'].append(layerCode)
                    rootNode['children'].append({
                        'name': item['name'],
                        'ids': [layerCode]
                    })
                else:
                    # Create root + register
                    rootNode = {
                        'name': item['parent'],
                        'ids': [ layerCode ],
                        'children': [
                            {
                                'name': item['name'],
                                'ids': [ layerCode ]
                            }
                        ]
                    }
                    parentNodes[item['parent']] = rootNode
                    pipelineMeta['pipeline'].append(rootNode)
            else:
                # Add item info as a new pipeline node
                pipelineMeta['pipeline'].append({
                    'name': item['name'],
                    'ids': [layerCode]
                })

        # Register metadata to be written in index.json
        self.dataHandler.addSection('Geometry', geometryMeta)
        self.dataHandler.addSection('CompositePipeline', pipelineMeta)

    def writeData(self, time=0):
        if not self.dataHandler.can_write:
            return

        currentScene = [];
        for data in self.config['scene']:
            currentData = {
                'name': data['name'],
                'fields': {}
            }
            currentScene.append(currentData)
            if self.surfaceExtract:
                self.merge.Input = data['source']
            else:
                self.merge = simple.MergeBlocks(Input=data['source'], MergePoints=0)
                self.surfaceExtract = simple.ExtractSurface(Input=self.merge)


            # Extract surface
            self.surfaceExtract.UpdatePipeline(time)
            ds = self.surfaceExtract.SMProxy.GetClientSideObject().GetOutputDataObject(0)
            originalDS = data['source'].SMProxy.GetClientSideObject().GetOutputDataObject(0)

            originalPoints = ds.GetPoints()

            # Points
            points = vtkFloatArray()
            nbPoints = originalPoints.GetNumberOfPoints()
            points.SetNumberOfComponents(3)
            points.SetNumberOfTuples(nbPoints)
            for idx in range(nbPoints):
                coord = originalPoints.GetPoint(idx)
                points.SetTuple3(idx, coord[0], coord[1], coord[2])

            pBuffer = buffer(points)
            pMd5 = hashlib.md5(pBuffer).hexdigest()
            pPath = os.path.join(self.dataHandler.getBasePath(), 'points',"%s.Float32Array" % pMd5)
            currentData['points'] = 'points/%s.Float32Array' % pMd5
            with open(pPath, 'wb') as f:
                f.write(pBuffer)

            # Polys
            poly = ds.GetPolys()
            nbCells = poly.GetNumberOfCells()
            cellLocation = 0
            idList = vtkIdList()
            topo = vtkTypeUInt32Array()
            topo.Allocate(poly.GetData().GetNumberOfTuples())

            for cellIdx in range(nbCells):
                poly.GetCell(cellLocation, idList)
                cellSize = idList.GetNumberOfIds()
                cellLocation += cellSize + 1
                if cellSize == 3:
                    topo.InsertNextValue(idList.GetId(0))
                    topo.InsertNextValue(idList.GetId(1))
                    topo.InsertNextValue(idList.GetId(2))
                elif cellSize == 4:
                    topo.InsertNextValue(idList.GetId(0))
                    topo.InsertNextValue(idList.GetId(1))
                    topo.InsertNextValue(idList.GetId(3))
                    topo.InsertNextValue(idList.GetId(1))
                    topo.InsertNextValue(idList.GetId(2))
                    topo.InsertNextValue(idList.GetId(3))
                else:
                    print ("Cell size of", cellSize, "not supported")

            iBuffer = buffer(topo)
            iMd5 = hashlib.md5(iBuffer).hexdigest()
            iPath = os.path.join(self.dataHandler.getBasePath(), 'index',"%s.Uint32Array" % iMd5)
            currentData['index'] = 'index/%s.Uint32Array' % iMd5
            with open(iPath, 'wb') as f:
                f.write(iBuffer)

            # Grow object side
            self.objSize[data['name']]['points'] = max(self.objSize[data['name']]['points'], nbPoints)
            self.objSize[data['name']]['index'] = max(self.objSize[data['name']]['index'], topo.GetNumberOfTuples())

            # Colors / FIXME
            for fieldName, fieldInfo in iteritems(data['colors']):
                array = ds.GetPointData().GetArray(fieldName)
                tupleSize = array.GetNumberOfComponents()
                arraySize = array.GetNumberOfTuples()
                outputField = vtkFloatArray()
                outputField.SetNumberOfTuples(arraySize)
                if tupleSize == 1:
                    for i in range(arraySize):
                        outputField.SetValue(i, array.GetValue(i))
                else:
                    # compute magnitude
                    tupleIdxs = range(tupleSize)
                    for i in range(arraySize):
                        magnitude = 0
                        for j in tupleIdxs:
                            magnitude += math.pow(array.GetValue(i * tupleSize + j), 2)

                        outputField.SetValue(i, math.sqrt(magnitude))

                fBuffer = buffer(outputField)
                fMd5 = hashlib.md5(fBuffer).hexdigest()
                fPath = os.path.join(self.dataHandler.getBasePath(), 'fields',"%s_%s.Float32Array" % (fieldName, fMd5))
                with open(fPath, 'wb') as f:
                    f.write(fBuffer)

                currentData['fields'][fieldName] = 'fields/%s_%s.Float32Array' % (fieldName, fMd5)

        # Write scene
        with open(self.dataHandler.getDataAbsoluteFilePath('scene'), 'w') as f:
            f.write(json.dumps(currentScene, indent=2))


    def stop(self, compress=True, clean=True):
        if not self.dataHandler.can_write:
            return

        DataSetBuilder.stop(self)

        if compress:
            for dirName in ['fields', 'index', 'points']:
                for root, dirs, files in os.walk(os.path.join(self.dataHandler.getBasePath(), dirName)):
                    print ('Compress', root)
                    for name in files:
                        if 'Array' in name and '.gz' not in name:
                            with open(os.path.join(root, name), 'rb') as f_in:
                                with gzip.open(os.path.join(root, name + '.gz'), 'wb') as f_out:
                                    shutil.copyfileobj(f_in, f_out)
                            os.remove(os.path.join(root, name))
