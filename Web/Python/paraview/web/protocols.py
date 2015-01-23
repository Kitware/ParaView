r"""paraviewweb_protocols is a module that contains a set of ParaViewWeb related
protocols that can be combined together to provide a flexible way to define
very specific web application.
"""

import os, sys, logging, types, inspect, traceback, logging, re, json
from time import time

# import RPC annotation
from autobahn.wamp import register as exportRpc

# import paraview modules.
import paraview

from paraview import simple, servermanager
from paraview.servermanager import ProxyProperty, InputProperty
from paraview.web import helper
from vtk.web import protocols as vtk_protocols
from decorators import *

from vtkWebCorePython import vtkWebInteractionEvent

# Needed for:
#    vtkSMPVRepresentationProxy
#    vtkSMTransferFunctionProxy
#    vtkSMTransferFunctionManager
from vtkPVServerManagerRenderingPython import *

# Needed for:
#    vtkSMProxyManager
from vtkPVServerManagerCorePython import *

# Needed for:
#    vtkDataObject
from vtkCommonDataModelPython import *


# =============================================================================
#
# Base class for any ParaView based protocol
#
# =============================================================================

class ParaViewWebProtocol(vtk_protocols.vtkWebProtocol):

    def __init__(self):
        self.Application = None
        self.multiRoot = False
        self.baseDirectory = ''
        self.baseDirectoryMap = {}

    def mapIdToProxy(self, id):
        """
        Maps global-id for a proxy to the proxy instance. May return None if the
        id is not valid.
        """
        try:
            id = int(id)
        except:
            return None
        if id <= 0:
            return None
        return simple.servermanager._getPyProxy(\
                simple.servermanager.ActiveConnection.Session.GetRemoteObject(id))

    def getView(self, vid):
        """
        Returns the view for a given view ID, if vid is None then return the
        current active view.
        :param vid: The view ID
        :type vid: str
        """
        view = self.mapIdToProxy(vid)
        if not view:
            # Use active view is none provided.
            view = simple.GetActiveView()

        if not view:
            raise Exception("no view provided: " + str(vid))

        return view

    def debug(self, msg):
        if self.debugMode == True:
            print msg

    def setBaseDirectory(self, basePath):
        self.overrideDataDirKey = None
        self.baseDirectory = ''
        self.baseDirectoryMap = {}
        self.multiRoot = False

        if basePath.find('|') < 0:
            if basePath.find('=') >= 0:
                basePair = basePath.split('=')
                if os.path.exists(basePair[1]):
                    self.baseDirectory = basePair[1]
                    self.overrideDataDirKey = basePair[0]
            else:
                self.baseDirectory = basePath
        else:
            baseDirs = basePath.split('|')
            for baseDir in baseDirs:
                basePair = baseDir.split('=')
                if os.path.exists(basePair[1]):
                    self.baseDirectoryMap[basePair[0]] = basePair[1]

            # Check if we ended up with just a single directory
            bdKeys = self.baseDirectoryMap.keys()
            if len(bdKeys) == 1:
                self.baseDirectory = self.baseDirectoryMap[bdKeys[0]]
                self.overrideDataDirKey = bdKeys[0]
                self.baseDirectoryMap = {}
            elif len(bdKeys) > 1:
                self.multiRoot = True

    def getAbsolutePath(self, relativePath):
        absolutePath = None

        if self.multiRoot == True:
            relPathParts = relativePath.replace('\\', '/').split('/')
            realBasePath = self.baseDirectoryMap[relPathParts[0]]
            absolutePath = os.path.join(realBasePath, *relPathParts[1:])
        else:
            absolutePath = os.path.join(self.baseDirectory, relativePath)

        return os.path.normpath(absolutePath)

# =============================================================================
#
# Handle Mouse interaction on any type of view
#
# =============================================================================

class ParaViewWebMouseHandler(ParaViewWebProtocol):

    # RpcName: mouseInteraction => viewport.mouse.interaction
    @exportRpc("viewport.mouse.interaction")
    def mouseInteraction(self, event):
        """
        RPC Callback for mouse interactions.
        """
        view = self.getView(event['view'])

        if event["action"] == 'down':
            view.UseInteractiveRenderingForScreenshots = 1
        elif event["action"] == 'up':
            view.UseInteractiveRenderingForScreenshots = 0

        buttons = 0
        if event["buttonLeft"]:
            buttons |= vtkWebInteractionEvent.LEFT_BUTTON;
        if event["buttonMiddle"]:
            buttons |= vtkWebInteractionEvent.MIDDLE_BUTTON;
        if event["buttonRight"]:
            buttons |= vtkWebInteractionEvent.RIGHT_BUTTON;

        modifiers = 0
        if event["shiftKey"]:
            modifiers |= vtkWebInteractionEvent.SHIFT_KEY
        if event["ctrlKey"]:
            modifiers |= vtkWebInteractionEvent.CTRL_KEY
        if event["altKey"]:
            modifiers |= vtkWebInteractionEvent.ALT_KEY
        if event["metaKey"]:
            modifiers |= vtkWebInteractionEvent.META_KEY

        pvevent = vtkWebInteractionEvent()
        pvevent.SetButtons(buttons)
        pvevent.SetModifiers(modifiers)
        pvevent.SetX(event["x"])
        pvevent.SetY(event["y"])
        #pvevent.SetKeyCode(event["charCode"])
        retVal = self.getApplication().HandleInteractionEvent(view.SMProxy, pvevent)
        del pvevent
        return retVal

# =============================================================================
#
# Basic 3D Viewport API (Camera + Orientation + CenterOfRotation
#
# =============================================================================

class ParaViewWebViewPort(ParaViewWebProtocol):

    # RpcName: resetCamera => viewport.camera.reset
    @exportRpc("viewport.camera.reset")
    def resetCamera(self, view):
        """
        RPC callback to reset camera.
        """
        view = self.getView(view)
        simple.Render(view)
        simple.ResetCamera(view)
        try:
            view.CenterOfRotation = view.CameraFocalPoint
        except:
            pass

        self.getApplication().InvalidateCache(view.SMProxy)
        return view.GetGlobalIDAsString()

    # RpcName: updateOrientationAxesVisibility => viewport.axes.orientation.visibility.update
    @exportRpc("viewport.axes.orientation.visibility.update")
    def updateOrientationAxesVisibility(self, view, showAxis):
        """
        RPC callback to show/hide OrientationAxis.
        """
        view = self.getView(view)
        view.OrientationAxesVisibility = (showAxis if 1 else 0);

        self.getApplication().InvalidateCache(view.SMProxy)
        return view.GetGlobalIDAsString()

    # RpcName: updateCenterAxesVisibility => viewport.axes.center.visibility.update
    @exportRpc("viewport.axes.center.visibility.update")
    def updateCenterAxesVisibility(self, view, showAxis):
        """
        RPC callback to show/hide CenterAxesVisibility.
        """
        view = self.getView(view)
        view.CenterAxesVisibility = (showAxis if 1 else 0);

        self.getApplication().InvalidateCache(view.SMProxy)
        return view.GetGlobalIDAsString()

    # RpcName: updateCamera => viewport.camera.update
    @exportRpc("viewport.camera.update")
    def updateCamera(self, view_id, focal_point, view_up, position):
        view = self.getView(view_id)

        view.CameraFocalPoint = focal_point
        view.CameraViewUp = view_up
        view.CameraPosition = position
        self.getApplication().InvalidateCache(view.SMProxy)

# =============================================================================
#
# Provide Image delivery mechanism
#
# =============================================================================

class ParaViewWebViewPortImageDelivery(ParaViewWebProtocol):

    # RpcName: stillRender => viewport.image.render
    @exportRpc("viewport.image.render")
    def stillRender(self, options):
        """
        RPC Callback to render a view and obtain the rendered image.
        """
        beginTime = int(round(time() * 1000))
        view = self.getView(options["view"])
        size = [view.ViewSize[0], view.ViewSize[1]]
        if options and options.has_key("size"):
            size = options["size"]
            view.ViewSize = size
        t = 0
        if options and options.has_key("mtime"):
            t = options["mtime"]
        quality = 100
        if options and options.has_key("quality"):
            quality = options["quality"]
        localTime = 0
        if options and options.has_key("localTime"):
            localTime = options["localTime"]
        reply = {}
        app = self.getApplication()
        reply["image"] = app.StillRenderToString(view.SMProxy, t, quality)
        reply["stale"] = app.GetHasImagesBeingProcessed(view.SMProxy)
        reply["mtime"] = app.GetLastStillRenderToStringMTime()
        reply["size"] = [view.ViewSize[0], view.ViewSize[1]]
        reply["format"] = "jpeg;base64"
        reply["global_id"] = view.GetGlobalIDAsString()
        reply["localTime"] = localTime

        endTime = int(round(time() * 1000))
        reply["workTime"] = (endTime - beginTime)

        return reply


# =============================================================================
#
# Provide Geometry delivery mechanism (WebGL)
#
# =============================================================================

class ParaViewWebViewPortGeometryDelivery(ParaViewWebProtocol):

    # RpcName: getSceneMetaData => viewport.webgl.metadata
    @exportRpc("viewport.webgl.metadata")
    def getSceneMetaData(self, view_id):
        view  = self.getView(view_id);
        data = self.getApplication().GetWebGLSceneMetaData(view.SMProxy)
        return data

    # RpcName: getWebGLData => viewport.webgl.data
    @exportRpc("viewport.webgl.data")
    def getWebGLData(self, view_id, object_id, part):
        view  = self.getView(view_id)
        data = self.getApplication().GetWebGLBinaryData(view.SMProxy, str(object_id), part-1)
        return data

# =============================================================================
#
# Time management
#
# =============================================================================

class ParaViewWebTimeHandler(ParaViewWebProtocol):

    def __init__(self):
        super(ParaViewWebTimeHandler, self).__init__()
        # setup animation scene
        self.scene = simple.GetAnimationScene()
        simple.GetTimeTrack()
        self.scene.PlayMode = "Snap To TimeSteps"

    # RpcName: updateTime => pv.vcr.action
    @exportRpc("pv.vcr.action")
    def updateTime(self,action):
        view = simple.GetRenderView()
        animationScene = simple.GetAnimationScene()
        currentTime = view.ViewTime

        if action == "next":
            animationScene.GoToNext()
            if currentTime == view.ViewTime:
                animationScene.GoToFirst()
        if action == "prev":
            animationScene.GoToPrevious()
            if currentTime == view.ViewTime:
                animationScene.GoToLast()
        if action == "first":
            animationScene.GoToFirst()
        if action == "last":
            animationScene.GoToLast()

        return view.ViewTime

# =============================================================================
#
# Color management
#
# =============================================================================

class ParaViewWebColorManager(ParaViewWebProtocol):

    def __init__(self, pathToColorMaps=None):
        super(ParaViewWebColorManager, self).__init__()
        self.colorMapNames = []
        if pathToColorMaps is not None:
            self.pathToLuts = pathToColorMaps
        else:
            module_path = os.path.abspath(__file__)
            path = os.path.dirname(module_path)
            self.pathToLuts = os.path.join(path, '..', 'ColorMaps.xml')

    # Rather than keeping a map of these xml strings in memory, it's very
    # quick to just scan the file when we need a color map.  This is a
    # convenience function to read the colormaps file and find the one
    # associated with a particular name, then return it as xml text.  Also
    # builds a list of preset color names and stores them in an instance
    # variable.
    def findColorMapText(self, colorMapName):
        content = None
        self.colorMapNames = []
        with open(self.pathToLuts, 'r') as fd:
            content = fd.read()
            if content is not None:
                colorMapMatcher = re.compile('(<ColorMap.+?(?=</ColorMap>)</ColorMap>)', re.DOTALL)
                nameMatcher = re.compile('name="([^"]+)"')
                iterator = colorMapMatcher.finditer(content)
                for match in iterator:
                    colorMap = match.group(1)
                    m = nameMatcher.search(colorMap)
                    if m:
                        mapName = m.group(1)
                        self.colorMapNames.append(mapName)
                        if mapName == colorMapName:
                            return colorMap

        return None

    # RpcName: getScalarBarVisibilities => pv.color.manager.scalarbar.visibility.get
    @exportRpc("pv.color.manager.scalarbar.visibility.get")
    def getScalarBarVisibilities(self, proxyIdList):
        """
        Returns whether or not each specified scalar bar is visible.
        """
        visibilities = {}
        for proxyId in proxyIdList:
            proxy = self.mapIdToProxy(proxyId)
            if proxy is not None:
                rep = simple.GetRepresentation(proxy)
                view = self.getView(-1)
                visibilities[proxyId] = vtkSMPVRepresentationProxy.IsScalarBarVisible(rep.SMProxy, view.SMProxy)

        return visibilities

    # RpcName: setScalarBarVisibilities => pv.color.manager.scalarbar.visibility.set
    @exportRpc("pv.color.manager.scalarbar.visibility.set")
    def setScalarBarVisibilities(self, proxyIdMap):
        """
        Sets the visibility of the scalar bar corresponding to each specified
        proxy.  The representation for each proxy is found using the
        filter/source proxy id and the current view.
        """
        visibilities = {}
        for proxyId in proxyIdMap:
            proxy = self.mapIdToProxy(proxyId)
            if proxy is not None:
                rep = simple.GetDisplayProperties(proxy)
                view = self.getView(-1)
                vtkSMPVRepresentationProxy.SetScalarBarVisibility(rep.SMProxy,
                                                                  view.SMProxy,
                                                                  proxyIdMap[proxyId])
                visibilities[proxyId] = vtkSMPVRepresentationProxy.IsScalarBarVisible(rep.SMProxy,
                                                                                      view.SMProxy)

        # Render to get scalar bars in correct position when doing local rendering (webgl)
        simple.Render()

        return visibilities

    # RpcName: rescaleTransferFunction => pv.color.manager.rescale.transfer.function
    @exportRpc("pv.color.manager.rescale.transfer.function")
    def rescaleTransferFunction(self, options):
        """
        Rescale the color transfer function to fit either the data range,
        the data range over time, or to a custom range, for the array by
        which the representation is currently being colored.
        """
        type = options['type']
        proxyId = options['proxyId']
        proxy = self.mapIdToProxy(proxyId)
        rep = simple.GetRepresentation(proxy)

        status = { 'success': False }

        if type == 'time':
            status['success'] = \
                vtkSMPVRepresentationProxy.RescaleTransferFunctionToDataRangeOverTime(rep.SMProxy)
        elif type == 'data':
            extend = False
            if 'extend' in options:
                extend = options['extend']
            status['success'] = \
                vtkSMPVRepresentationProxy.RescaleTransferFunctionToDataRange(rep.SMProxy, extend)
        elif type == 'custom':
            rangemin = float(options['min'])
            rangemax = float(options['max'])
            extend = False
            if 'extend' in options:
                extend = options['extend']
            lookupTable = rep.LookupTable
            if lookupTable is not None:
                status['success'] = \
                    vtkSMTransferFunctionProxy.RescaleTransferFunction(lookupTable.SMProxy,
                                                                       rangemin,
                                                                       rangemax,
                                                                       extend)

        if status['success']:
            currentRange = self.getCurrentScalarRange(proxyId)
            status['range'] = currentRange

        return status

    # RpcName: getCurrentScalarRange => pv.color.manager.scalar.range.get
    @exportRpc("pv.color.manager.scalar.range.get")
    def getCurrentScalarRange(self, proxyId):
        proxy = self.mapIdToProxy(proxyId)
        rep = simple.GetRepresentation(proxy)

        lookupTable = rep.LookupTable
        cMin = lookupTable.RGBPoints[0]
        cMax = lookupTable.RGBPoints[-4]

        return { 'min': cMin, 'max': cMax }

    # RpcName: colorBy => pv.color.manager.color.by
    @exportRpc("pv.color.manager.color.by")
    def colorBy(self, representation, colorMode, arrayLocation='POINTS', arrayName='', vectorMode='Magnitude', vectorComponent=0, rescale=False):
        """
        Choose the array to color by, and optionally specify magnitude or a
        vector component in the case of vector array.
        """
        locationMap = { 'POINTS': vtkDataObject.POINT, 'CELLS': vtkDataObject.CELL }
        repProxy = self.mapIdToProxy(representation)
        lutProxy = repProxy.LookupTable

        if colorMode == 'SOLID':
            # No array just diffuse color
            repProxy.ColorArrayName = ''
        else:
            # Select data array
            vtkSMPVRepresentationProxy.SetScalarColoring(repProxy.SMProxy, arrayName, locationMap[arrayLocation])
            lut = repProxy.LookupTable
            lut.VectorMode = str(vectorMode)
            lut.VectorComponent = int(vectorComponent)

            # FIXME: This should happen once at the time the lookup table is created
            # Also, the False is the default, but we need it here to avoid the
            # ambiguous call error
            if rescale or (lut != lutProxy):
                vtkSMPVRepresentationProxy.RescaleTransferFunctionToDataRange(repProxy.SMProxy, arrayName, locationMap[arrayLocation], False)

        simple.Render()

    # RpcName: setOpacityFunctionPoints => pv.color.manager.opacity.points.set
    @exportRpc("pv.color.manager.opacity.points.set")
    def setOpacityFunctionPoints(self, arrayName, pointArray):
        lutProxy = simple.GetColorTransferFunction(arrayName)
        pwfProxy = simple.GetOpacityTransferFunction(arrayName)

        # Use whatever the current scalar range is for this array
        cMin = lutProxy.RGBPoints[0]
        cMax = lutProxy.RGBPoints[-4]

        # Scale and bias the x values, which come in between 0.0 and 1.0, to the
        # current scalar range
        for i in range(len(pointArray) / 4):
            idx = i * 4
            x = pointArray[idx]
            pointArray[idx] = (x * (cMax - cMin)) + cMin

        # Set the Points property to scaled and biased points array
        pwfProxy.Points = pointArray

        simple.Render()

    # RpcName: setSurfaceOpacity => pv.color.manager.surface.opacity.set
    @exportRpc("pv.color.manager.surface.opacity.set")
    def setSurfaceOpacity(self, representation, enabled):
        repProxy = self.mapIdToProxy(representation)
        lutProxy = repProxy.LookupTable

        lutProxy.EnableOpacityMapping = enabled

        simple.Render()

    # RpcName: getSurfaceOpacity => pv.color.manager.surface.opacity.get
    @exportRpc("pv.color.manager.surface.opacity.get")
    def getSurfaceOpacity(self, representation):
        repProxy = self.mapIdToProxy(representation)
        lutProxy = repProxy.LookupTable

        return lutProxy.EnableOpacityMapping

    # RpcName: selectColorMap => pv.color.manager.select.preset
    @exportRpc("pv.color.manager.select.preset")
    def selectColorMap(self, representation, paletteName):
        """
        Choose the color map preset to use when coloring by an array.
        """
        repProxy = self.mapIdToProxy(representation)
        colorMapText = self.findColorMapText(paletteName)

        if colorMapText is not None:
            lutProxy = repProxy.LookupTable
            if lutProxy is not None:
                vtkSMTransferFunctionProxy.ApplyColorMap(lutProxy.SMProxy, colorMapText)
                simple.Render()
                return { 'result': 'success' }
            else:
                return { 'result': 'Representation proxy ' + representation + ' is missing lookup table' }

        return { 'result': 'preset ' + paletteName + ' not found' }

    # RpcName: listColorMapNames => pv.color.manager.list.preset
    @exportRpc("pv.color.manager.list.preset")
    def listColorMapNames(self):
        """
        List the names of all color map presets available on the server.  This
        list will contain the names of any presets you provided in the file you
        supplied to the constructor of this protocol.
        """
        self.findColorMapText('')
        return self.colorMapNames

# =============================================================================
#
# Proxy management protocol
#
# =============================================================================
class ParaViewWebProxyManager(ParaViewWebProtocol):

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
                       'signed_char' ]    # 15

    def __init__(self, allowedProxiesFile=None, baseDir=None, allowUnconfiguredReaders=True):
        """
        - basePath: specify the base directory (or directories) that we should start with, if this
         parameter takes the form: "name1=path1|name2=path2|...", then we will treat this as the
         case where multiple data directories are required.  In this case, each top-level directory
         will be given the name associated with the directory in the argument.
        """
        super(ParaViewWebProxyManager, self).__init__()
        self.debugMode = False
        self.domainFunctionMap = { "vtkSMBooleanDomain": booleanDomainDecorator,
                                   "vtkSMProxyListDomain": proxyListDomainDecorator,
                                   "vtkSMIntRangeDomain": numberRangeDomainDecorator,
                                   "vtkSMDoubleRangeDomain": numberRangeDomainDecorator,
                                   "vtkSMArrayRangeDomain": numberRangeDomainDecorator,
                                   "vtkSMRepresentationTypeDomain": stringListDomainDecorator,
                                   "vtkSMStringListDomain": stringListDomainDecorator,
                                   "vtkSMArrayListDomain": arrayListDomainDecorator,
                                   "vtkSMArraySelectionDomain": arraySelectionDomainDecorator,
                                   "vtkSMEnumerationDomain": enumerationDomainDecorator,
                                   "vtkSMCompositeTreeDomain": treeDomainDecorator }
        self.alwaysIncludeProperties = [ 'CubeAxesVisibility' ]
        self.alwaysExcludeProperties = [ 'LookupTable', 'Texture', 'ColorArrayName',
                                         'Representations', 'HiddenRepresentations',
                                         'HiddenProps', 'UseTexturedBackground',
                                         'BackgroundTexture', 'KeyLightWarmth',
                                         'KeyLightIntensity', 'KeyLightElevation',
                                         'KeyLightAzimuth', 'FileName' ]
        self.propertyTypeMap = { 'vtkSMIntVectorProperty': 'int',
                                 'vtkSMDoubleVectorProperty': 'float',
                                 'vtkSMStringVectorProperty': 'str',
                                 'vtkSMProxyProperty': 'proxy',
                                 'vtkSMInputProperty': 'proxy' }
        self.allowedProxies = {}
        self.hintsMap = { 'PropertyWidgetDecorator': { 'ClipScalarsDecorator': clipScalarDecorator,
                                                       'GenericDecorator': genericDecorator },
                          'Widget': { 'multi_line': multiLineDecorator } }

        self.setBaseDirectory(baseDir)
        self.allowUnconfiguredReaders = allowUnconfiguredReaders

        # If there was a proxy list file, use it, otherwise use the default
        if allowedProxiesFile:
            self.readAllowedProxies(allowedProxiesFile)
        else:
            module_path = os.path.abspath(__file__)
            path = os.path.dirname(module_path)
            proxyFile = os.path.join(path, 'defaultProxies.json')
            self.readAllowedProxies(proxyFile)

        self.simpleTypes = [int, float, list, str]
        self.view = simple.GetRenderView()
        simple.SetActiveView(self.view)
        simple.Render()

    #--------------------------------------------------------------------------
    # Read the configured proxies json file into a dictionary and process.
    #--------------------------------------------------------------------------
    def readAllowedProxies(self, filepath):
        self.availableList = {}
        self.allowedProxies = {}
        with open(filepath, 'r') as fd:
            configurationData = json.load(fd)
            self.configureFiltersOrSources('sources', configurationData['sources'])
            self.configureFiltersOrSources('filters', configurationData['filters'])
            self.configureReaders(configurationData['readers'])

    #--------------------------------------------------------------------------
    # Handle filters and sources sections of the proxies config file.
    #--------------------------------------------------------------------------
    def configureFiltersOrSources(self, key, flist):
        list = []
        self.availableList[key] = list
        for item in flist:
            if 'label' in item:
                self.allowedProxies[item['label']] = item['name']
                list.append(item['label'])
            else:
                self.allowedProxies[item['name']] = item['name']
                list.append(item['name'])

    #--------------------------------------------------------------------------
    # Handle the readers section of the proxies config file.
    #--------------------------------------------------------------------------
    def configureReaders(self, rlist):
        self.readerFactoryMap = {}
        for config in rlist:
            readerName = config['name']
            readerMethod = 'FileName'
            if 'method' in config:
                readerMethod = config['method']
            for ext in config['extensions']:
                self.readerFactoryMap[ext] = [ readerName, readerMethod ]

    #--------------------------------------------------------------------------
    # Look higher up in XML heirarchy for attributes on a property (useful if
    # a property is an exposed property).
    #--------------------------------------------------------------------------
    def extractParentAttributes(self, property, attributes):
        proxy = None
        name = ''
        try:
            proxy = property.SMProperty.GetParent()
            name = proxy.GetPropertyName(property.SMProperty)
        except:
            try:
                proxy = property.GetParent()
                name = proxy.GetPropertyName(property)
            except:
                print 'ERROR: unable to get property parent for property ' + property.Name
                return {}

        xmlElement = servermanager.ActiveConnection.Session.GetProxyDefinitionManager().GetCollapsedProxyDefinition(proxy.GetXMLGroup(), proxy.GetXMLName(), None)
        attrMap = {}

        nbChildren = xmlElement.GetNumberOfNestedElements()
        for i in range(nbChildren):
            xmlChild = xmlElement.GetNestedElement(i)
            elementName = xmlChild.GetName()
            if elementName.endswith('Property'):
                propName = xmlChild.GetAttribute('name')
                if propName == name:
                    for attrName in attributes:
                        if xmlChild.GetAttribute(attrName):
                            attrMap[attrName] = xmlChild.GetAttribute(attrName)
        return attrMap

    #--------------------------------------------------------------------------
    # If we have a proxy property, gather up the xml information from the
    # possible proxy values it can take on.
    #--------------------------------------------------------------------------
    def getProxyListFromProperty(self, proxy, proxyPropElement):
        propPropName = proxyPropElement.GetAttribute('name')
        propInstance = proxy.GetProperty(propPropName)
        nbChildren = proxyPropElement.GetNumberOfNestedElements()
        foundPLDChild = False
        proxyDefMgr = servermanager.ActiveConnection.Session.GetProxyDefinitionManager()
        for i in range(nbChildren):
            child = proxyPropElement.GetNestedElement(i)
            if child.GetName() == 'ProxyListDomain':
                domain = propInstance.GetDomain(child.GetAttribute('name'))
                foundPLDChild = True
                for j in range(domain.GetNumberOfProxies()):
                    subProxy = domain.GetProxy(j)
                    pelt = proxyDefMgr.GetCollapsedProxyDefinition(subProxy.GetXMLGroup(),
                                                                   subProxy.GetXMLName(),
                                                                   None)
                    self.processXmlElement(subProxy, pelt)
        return foundPLDChild

    #--------------------------------------------------------------------------
    # Gather information from the xml associated with a proxy and properties.
    #--------------------------------------------------------------------------
    def processXmlElement(self, proxy, xmlElement):
        proxyId = proxy.GetGlobalIDAsString()
        nbChildren = xmlElement.GetNumberOfNestedElements()
        for i in range(nbChildren):
            xmlChild = xmlElement.GetNestedElement(i)
            name = xmlChild.GetName()
            nameAttr = xmlChild.GetAttribute('name')
            detailsKey = proxyId + ':' + str(nameAttr)

            infoOnly = xmlChild.GetAttribute('information_only')
            isInternal = xmlChild.GetAttribute('is_internal')
            panelVis = xmlChild.GetAttribute('panel_visibility')
            numElts = xmlChild.GetAttribute('number_of_elements')

            size = -1
            informationOnly = 0
            internal = 0

            # Check for attributes that might only exist on parent xml
            parentQueryList = []
            if not numElts:
                parentQueryList.append('number_of_elements')
            else:
                size = int(numElts)

            if not infoOnly:
                parentQueryList.append('information_only')
            else:
                informationOnly = int(infoOnly)

            if not isInternal:
                parentQueryList.append('is_internal')
            else:
                internal = int(isInternal)

            # Try to retrieve those attributes from parent xml
            parentAttrs = {}
            if len(parentQueryList) > 0:
                propInstance = proxy.GetProperty(nameAttr)
                if propInstance:
                    parentAttrs = self.extractParentAttributes(propInstance,
                                                               parentQueryList)

            if 'number_of_elements' in parentAttrs and parentAttrs['number_of_elements']:
                size = int(parentAttrs['number_of_elements'])
            if 'information_only' in parentAttrs and parentAttrs['information_only']:
                informationOnly = int(parentAttrs['information_only'])
            if 'is_internal' in parentAttrs and parentAttrs['is_internal']:
                internal = int(parentAttrs['is_internal'])

            # Now decide whether we should filter out this property
            if ((panelVis == 'never' or informationOnly == 1 or internal == 1) and nameAttr not in self.alwaysIncludeProperties) or nameAttr in self.alwaysExcludeProperties:
                self.debug('Filtering out property ' + str(nameAttr) + ' because panelVis is never, infoOnly is 1, isInternal is 1, or ' + str(nameAttr) + ' is an ALWAYS EXCLUDE property')
                continue

            if name == 'Hints':
                self.debug("    ((((((((((Got the hints))))))))) ")
                for j in range(xmlChild.GetNumberOfNestedElements()):
                    hintChild = xmlChild.GetNestedElement(j)
                    if hintChild.GetName() == 'Visibility':
                        replaceInputAttr = hintChild.GetAttribute('replace_input')
                        if replaceInputAttr:
                            self.propertyDetailsMap['specialHints'] = { 'replaceInput': int(replaceInputAttr) }
                            self.debug("          Replace input value: " + str(replaceInputAttr))
            elif name == 'ProxyProperty' or name == 'InputProperty':
                self.debug(str(nameAttr) + ' is a proxy property')
                if detailsKey not in self.propertyDetailsMap:
                    self.propertyDetailsMap[detailsKey] = {'type': name, 'panelVis': panelVis, 'size': size}
                    self.orderedNameList.append(detailsKey)
                foundProxyListDomain = self.getProxyListFromProperty(proxy, xmlChild)
                if foundProxyListDomain == True:
                    self.propertyDetailsMap[detailsKey]['size'] = 1
            elif name.endswith('Property'):
                # Add this property to the list that will be used both to
                # order as well as to filter the properties retrieved earlier.
                if detailsKey not in self.propertyDetailsMap:
                    self.propertyDetailsMap[detailsKey] = {'type': name, 'panelVis': panelVis, 'size': size}
                    self.orderedNameList.append(detailsKey)
            else:
                # Don't recurse on properties that might be within a
                # PropertyGroup unless the PropertyGroup is itself within an
                # ExposedProperties. Later we might want to make a note in the
                # ui properties that the properties listed within here should be
                # grouped
                if name != 'PropertyGroup' or xmlChild.GetParent().GetName() == "ExposedProperties":
                    self.processXmlElement(proxy, xmlChild)

    #--------------------------------------------------------------------------
    # Entry point for the xml processing methods.
    #--------------------------------------------------------------------------
    def getProxyXmlDefinitions(self, proxy):
        self.orderedNameList = []
        self.propertyDetailsMap = {}
        self.propertyDetailsList = []
        proxyDefMgr = servermanager.ActiveConnection.Session.GetProxyDefinitionManager()
        xmlElement = proxyDefMgr.GetCollapsedProxyDefinition(proxy.GetXMLGroup(),
                                                             proxy.GetXMLName(),
                                                             None)
        self.processXmlElement(proxy, xmlElement)

        self.debug('Length of final ordered property list: ' + str(len(self.orderedNameList)))
        for propName in self.orderedNameList:
            propRec = self.propertyDetailsMap[propName]
            self.debug('Property ' + str(propName) + ', type = ' + str(propRec['type']) + ', pvis = ' + str(propRec['panelVis']) + ', size = ' + str(propRec['size']))

    #--------------------------------------------------------------------------
    # Helper function to fill in all the properties of a proxy.  Delegates
    # properties with specific domain types to other helpers.
    #--------------------------------------------------------------------------
    def fillPropertyList(self, proxy_id, propertyList, dependency=None):
        proxy = self.mapIdToProxy(proxy_id)
        if proxy:
            for property in proxy.ListProperties():
                propertyName = proxy.GetProperty(property).Name
                displayName = proxy.GetProperty(property).GetXMLLabel()
                if propertyName in ["Refresh", "Input"] or propertyName.__contains__("Info"):
                    continue

                prop = proxy.GetProperty(property)
                pythonProp = servermanager._wrap_property(proxy, prop)

                propJson = { 'name': propertyName,
                             'id': proxy_id,
                             'label': displayName }

                if dependency is not None:
                    propJson['dependency'] = dependency

                if type(prop) == ProxyProperty or type(prop) == InputProperty:
                    proxyListDomain = prop.FindDomain('vtkSMProxyListDomain')
                    if proxyListDomain:
                        # Set the value of this ProxyProperty
                        propJson['value'] = prop.GetProxy(0).GetXMLLabel()

                        # Now recursively fill in properties of this proxy property
                        for i in range(proxyListDomain.GetNumberOfProxies()):
                            subProxy = proxyListDomain.GetProxy(i)
                            subProxyId = subProxy.GetGlobalIDAsString()
                            depStr = proxy_id + ':' + propertyName + ':' + subProxy.GetXMLLabel() + ':1'
                            self.fillPropertyList(subProxyId, propertyList, depStr)
                    else:
                        # The value of this ProxyProperty is the list of proxy ids
                        value = []
                        for i in range(prop.GetNumberOfProxies()):
                            p = prop.GetProxy(i)
                            value.append(p.GetGlobalIDAsString())
                        propJson['value'] = value
                else:
                    # For everything but ProxyProperty, we should just be able to call GetData()
                    try:
                        propJson['value'] = proxy.GetProperty(property).GetData()
                    except AttributeError as attrErr:
                        print 'Property ' + propertyName + ' has no GetData() method, skipping'
                        continue

                self.debug('Adding a property to the pre-sorted list: ' + str(propJson))
                propertyList.append(propJson)

    #--------------------------------------------------------------------------
    # Make a first pass over the properties, building up a couple of data
    # structures we can use to reorder the properties to match the xml order.
    #--------------------------------------------------------------------------
    def reorderProperties(self, rootProxyId, proxyProperties):
        self.getProxyXmlDefinitions(self.mapIdToProxy(rootProxyId))

        propMap = {}
        for prop in proxyProperties:
            propMap[prop['id'] + ':' + prop['name']] = prop

        orderedList = []
        for name in self.orderedNameList:
            if name in propMap:
                orderedList.append(propMap[name])

        return orderedList

    #--------------------------------------------------------------------------
    # Convenience function to set a property value.  Can be extended with other
    # special cases as the need arises.
    #--------------------------------------------------------------------------
    def setProperty(self, property, propertyValue):
        if propertyValue == 'vtkProcessId':
            property.SetElement(0, propertyValue)
        else:
            property.SetData(propertyValue)

    #--------------------------------------------------------------------------
    # Taken directly from helper.py, applies domains so we can see real values.
    #--------------------------------------------------------------------------
    def applyDomains(self, parentProxy, proxy_id):
        """
        This function is used to apply the domains so that queries for
        specific values like range min and max retrieve something useful.
        """
        proxy = self.mapIdToProxy(proxy_id)

        # Call recursively on each sub-proxy if any
        for property_name in proxy.ListProperties():
            prop = proxy.GetProperty(property_name)
            if prop.IsA('vtkSMProxyProperty'):
                try:
                    if len(prop.Available) and prop.GetNumberOfProxies() == 1:
                        listdomain = prop.GetDomain('proxy_list')
                        if listdomain:
                            for i in xrange(listdomain.GetNumberOfProxies()):
                                internal_proxy = listdomain.GetProxy(i)
                                self.applyDomains(parentProxy, internal_proxy.GetGlobalIDAsString())
                except:
                    exc_type, exc_obj, exc_tb = sys.exc_info()
                    print "Unexpected error:", exc_type, " line: " , exc_tb.tb_lineno

        # Reset all properties to leverage domain capabilities
        for prop_name in proxy.ListProperties():
            if prop_name == 'Input':
                continue
            try:
                prop = proxy.GetProperty(prop_name)
                iter = prop.NewDomainIterator()
                iter.Begin()
                while not iter.IsAtEnd():
                    domain = iter.GetDomain()
                    iter.Next()

                    try:
                        if domain.IsA('vtkSMBoundsDomain'):
                            domain.SetDomainValues(parentProxy.GetDataInformation().GetBounds())
                    except AttributeError as attrErr:
                        print 'Caught exception setting domain values in apply_domains:'
                        print attrErr

                prop.ResetToDefault()

                # Need to UnRegister to handle the ref count from the NewDomainIterator
                iter.UnRegister(None)
            except:
                exc_type, exc_obj, exc_tb = sys.exc_info()
                print "Unexpected error:", exc_type, " line: " , exc_tb.tb_lineno

        proxy.UpdateVTKObjects()

    #--------------------------------------------------------------------------
    # Helper function to gather information about the bounds, point, and cell
    # data.
    #--------------------------------------------------------------------------
    def getDataInformation(self, proxy):
        # The stuff in here works, but only after you have created the
        # representation.
        dataInfo = proxy.GetDataInformation()

        # Get some basic statistics about the data
        info = { 'bounds': dataInfo.DataInformation.GetBounds(),
                 'points': dataInfo.DataInformation.GetNumberOfPoints(),
                 'cells': dataInfo.DataInformation.GetNumberOfCells(),
                 'type': dataInfo.DataInformation.GetPrettyDataTypeString(),
                 'memory': dataInfo.DataInformation.GetMemorySize() }

        arrayData = []

        # Get information about point data arrays
        pdInfo = proxy.GetPointDataInformation()
        numberOfPointArrays = pdInfo.GetNumberOfArrays()
        for idx in xrange(numberOfPointArrays):
            array = pdInfo.GetArray(idx)
            numComps = array.GetNumberOfComponents()
            typeStr = ParaViewWebProxyManager.VTK_DATA_TYPES[array.GetDataType()]
            data = { 'name': array.Name, 'size': numComps, 'location': 'POINTS', 'type': typeStr }
            magRange = array.GetRange(-1)
            rangeList = []
            if numComps > 1:
                rangeList.append({ 'name': 'Magnitude', 'min': magRange[0], 'max': magRange[1] })
                for i in range(numComps):
                    ithrange = array.GetRange(i)
                    rangeList.append({ 'name': array.GetComponentName(i),
                                       'min': ithrange[0],
                                       'max': ithrange[1] })
            else:
                rangeList.append({ 'name': '', 'min': magRange[0], 'max': magRange[1] })

            data['range'] = rangeList
            arrayData.append(data)

        # Get information about cell data arrays
        cdInfo = proxy.GetCellDataInformation()
        numberOfCellArrays = cdInfo.GetNumberOfArrays()
        for idx in xrange(numberOfCellArrays):
            array = cdInfo.GetArray(idx)
            numComps = array.GetNumberOfComponents()
            typeStr = ParaViewWebProxyManager.VTK_DATA_TYPES[array.GetDataType()]
            data = { 'name': array.Name, 'size': numComps, 'location': 'CELLS', 'type': typeStr }
            magRange = array.GetRange(-1)
            rangeList = []
            if numComps > 1:
                rangeList.append({ 'name': 'Magnitude', 'min': magRange[0], 'max': magRange[1] })
                for i in range(numComps):
                    ithrange = array.GetRange(i)
                    rangeList.append({ 'name': array.GetComponentName(i),
                                       'min': ithrange[0],
                                       'max': ithrange[1] })
            else:
                rangeList.append({ 'name': '', 'min': magRange[1], 'max': magRange[1] })

            data['range'] = rangeList
            arrayData.append(data)

        # Get field data information
        fdInfo = dataInfo.DataInformation.GetFieldDataInformation()
        numFieldDataArrays = fdInfo.GetNumberOfArrays()
        for idx in xrange(numFieldDataArrays):
            array = fdInfo.GetArrayInformation(idx)
            numComps = array.GetNumberOfComponents()
            typeStr = ParaViewWebProxyManager.VTK_DATA_TYPES[array.GetDataType()]
            data = { 'name': array.GetName(), 'size': numComps, 'location': 'FIELDS', 'type': typeStr }
            rangeList = []
            for i in range(numComps):
                ithrange = array.GetComponentRange(i)
                rangeList.append({ 'name': array.GetComponentName(i),
                                   'min': ithrange[0],
                                   'max': ithrange[1] })
            data['range'] = rangeList
            arrayData.append(data)

        # Time data
        timeKeeper = servermanager.ProxyManager().GetProxiesInGroup("timekeeper").values()[0]
        tsVals = timeKeeper.TimestepValues
        if tsVals:
            tValStrings = []
            for tsVal in tsVals:
                tValStrings.append(str(tsVal))
            info['time'] = tValStrings
        else:
            info['time'] = []

        info['arrays'] = arrayData
        return info

    #--------------------------------------------------------------------------
    # Helper function to gather information about the coloring used by this
    # representation proxy.
    #--------------------------------------------------------------------------
    def getColorInformation(self, proxy):
        """
        Generates a block of color information, given a representation proxy.

            colorBy: {
                'representation': '652',
                'scalarBar': 0,
                'mode': 'color'    # 'array'
                'color': [ 0.5, 1.0, 1.0 ],
                'array': [ 'POINTS', 'RTData', -1 ],
                'colorMap': 'Blue to Red'
            }
        """
        colorInfo = { 'representation': proxy.GetGlobalIDAsString() }
        if proxy.GetProperty('DiffuseColor'):
            colorInfo['color'] = proxy.GetProperty('DiffuseColor').GetData()

        if proxy.GetProperty('ColorArrayName'):
            colorInfo['mode'] = 'array'
            view = self.getView(-1)
            sbVisible = vtkSMPVRepresentationProxy.IsScalarBarVisible(proxy.SMProxy,
                                                                      view.SMProxy);
            colorInfo['scalarBar'] = 1 if sbVisible else 0
            lut = proxy.GetProperty('LookupTable').GetData()
            component = -1

            if lut and lut.GetProperty('VectorMode').GetData() == 'Component':
                component = lut.GetProperty('VectorComponent').GetData()

            colorArrayProp = proxy.GetProperty('ColorArrayName')
            colorInfo['array'] = [colorArrayProp.GetAssociation(),
                                  colorArrayProp.GetArrayName(),
                                  component]
        else:
            colorInfo['mode'] = 'color'
            colorInfo['scalarBar'] = 0

        return colorInfo

    #--------------------------------------------------------------------------
    # Helper function populate the ui structures list, which corresponds with
    # with the properties list, element by element, and provides information
    # about the ui elements needed to represent each property.
    #--------------------------------------------------------------------------
    def getUiProperties(self, proxyId, proxyProperties):
        """
        Generates an array of objects, parallel to the properties, containing
        the information needed to render ui for each property.  Not all of the
        following attributes will always be present.

        ui : [ {
                 'name': 'Clip Type',
                 'advanced': 1,                 # 0
                 'doc': 'Documentation string for the property',
                 'dependency': '498:ClipFunction:Sphere:1',
                 'values': { 'Plane': '456', 'Box': '457', 'Scalar': '458', 'Sphere': '459' },
                 'type': 'int',                    # 'float', 'int', 'str', 'proxy', 'input', ...
                 'widget': 'textfield',            # 'checkbox', 'textarea', 'list-1', 'list-n'
                 'size': -1,                       # -1, 0, 2, 3, 6
                 'range': [ { 'min': 0, 'max': 1 }, { 'min': 4, 'max': 7 }, ... ]
               }, ...
             ]
        """
        uiProps = []

        for proxyProp in proxyProperties:
            uiElt = {}

            proxyId = proxyProp['id']
            propertyName = proxyProp['name']
            proxy = self.mapIdToProxy(proxyId)
            prop = proxy.GetProperty(propertyName)

            # Get the xml details we already parsed out for this property
            xmlProps = self.propertyDetailsMap[proxyId + ':' + propertyName]

            # Set a few defaults for the ui elements, which may be overridden
            uiElt['size'] = xmlProps['size']
            uiElt['widget'] = 'textfield'
            uiElt['type'] = self.propertyTypeMap[prop.GetClassName()]

            doc = prop.GetDocumentation()
            if doc:
                uiElt['doc'] = doc.GetDescription()

            uiElt['name'] = proxyProp['label']
            proxyProp.pop('label', None)

            if 'dependency' in proxyProp:
                uiElt['depends'] = proxyProp['dependency']
                proxyProp.pop('dependency', None)
            if xmlProps['panelVis'] == 'advanced':
                uiElt['advanced'] = 1
            else:
                uiElt['advanced'] = 0

            # Iterate over the property domains until we find the interesting one
            domainIter = prop.NewDomainIterator()
            domainIter.Begin()
            foundDomain = False
            while domainIter.IsAtEnd() == 0 and foundDomain == False:
                nextDomain = domainIter.GetDomain()
                domainName = nextDomain.GetClassName()
                if domainName in self.domainFunctionMap:
                    domainFunction = self.domainFunctionMap[domainName]
                    foundDomain = domainFunction(proxyProp, xmlProps, uiElt, nextDomain)
                    self.debug('Processed ' + str(propertyName) + ' as domain ' + str(domainName))
                domainIter.Next()

            if foundDomain == False:
                self.debug('  ~~~|~~~ ' + str(propertyName) + ' did not have recognized domain')

            domainIter.FastDelete()

            # Now get hints for the property and provide an opportunity to decorate
            hints = prop.GetHints()
            if hints:
                numHints = hints.GetNumberOfNestedElements()
                for idx in range(numHints):
                    hint = hints.GetNestedElement(idx)
                    if hint.GetName() in self.hintsMap:
                        hmap = self.hintsMap[hint.GetName()]
                        hintType = hint.GetAttribute('type')
                        if hintType and hintType in hmap:
                            # We're intereseted in decorating based on this hint
                            hintFunction = hmap[hintType]
                            hintFunction(prop, uiElt, hint)

            uiProps.append(uiElt)

        return uiProps

    #--------------------------------------------------------------------------
    # Helper function validates the string we get from the client to make sure
    # it is one of the allowed proxies that has been set up on the server side.
    #--------------------------------------------------------------------------
    def validate(self, functionName):
        if functionName not in self.allowedProxies:
            return None
        else:
            return functionName.strip()

    @exportRpc("pv.proxy.manager.create")
    def create(self, functionName, parentId):
        """
        Creates a new filter/source proxy as a child of the specified
        parent proxy.  Returns the proxy state for the newly created
        proxy as a JSON object.
        """
        name = self.validate(functionName)

        if not name:
            return { 'success': False,
                     'reason': '"' + functionName + '" was not valid and could not be evaluated' }

        pid = parentId
        parentProxy = self.mapIdToProxy(parentId)
        if parentProxy:
            simple.SetActiveSource(parentProxy)
        else:
            pid = '0'

        # Create new source/filter
        allowed = self.allowedProxies[name]
        newProxy = paraview.simple.__dict__[allowed]()

        # To make WebGL export work
        simple.Show()
        simple.Render()

        try:
            self.applyDomains(parentProxy, newProxy.GetGlobalIDAsString())
        except Exception as inst:
            print 'Caught exception applying domains:'
            print inst

        return self.get(newProxy.GetGlobalIDAsString())

    @exportRpc("pv.proxy.manager.create.reader")
    def open(self, relativePath):
        """
        Open relative file paths, attempting to use the file extension to select
        from the configured readers.
        """
        fileToLoad = []
        if type(relativePath) == list:
            for file in relativePath:
               fileToLoad.append(self.getAbsolutePath(file))
        else:
            fileToLoad.append(self.getAbsolutePath(relativePath))

        # Get file extension and look for configured reader
        idx = fileToLoad[0].rfind('.')
        extension = fileToLoad[0][idx+1:]
        readerName = None
        if extension in self.readerFactoryMap:
            readerName = self.readerFactoryMap[extension][0]
            customMethod = self.readerFactoryMap[extension][1]

        # Open the file(s)
        reader = None
        if readerName:
            kw = { customMethod: fileToLoad }
            reader = paraview.simple.__dict__[readerName](**kw)
        else:
            if self.allowUnconfiguredReaders:
                reader = simple.OpenDataFile(fileToLoad)
            else:
                return { 'success': False,
                         'reason': 'No configured reader found for ' + extension + ' files, and unconfigured readers are not enabled.' }

        # Rename the reader proxy
        name = fileToLoad[0].split("/")[-1]
        if len(name) > 15:
            name = name[:15] + '*'
        simple.RenameSource(name, reader)

        # Representation, view, and camera setup
        simple.Show()
        simple.Render()
        simple.ResetCamera()

        return { "id": reader.GetGlobalIDAsString() }

    @exportRpc("pv.proxy.manager.get")
    def get(self, proxyId):
        """
        Returns the proxy state for the given proxyId as a JSON object.
        """
        proxyProperties = []
        proxyId = str(proxyId)
        self.fillPropertyList(proxyId, proxyProperties)
        proxyProperties = self.reorderProperties(proxyId, proxyProperties)
        uiProperties = self.getUiProperties(proxyId, proxyProperties)
        proxyJson = { 'id': proxyId, 'properties': proxyProperties, 'ui': uiProperties }

        if 'specialHints' in self.propertyDetailsMap:
            proxyJson['hints'] = self.propertyDetailsMap['specialHints']

        proxy = self.mapIdToProxy(proxyId)

        if proxy.SMProxy.IsA('vtkSMRepresentationProxy') == 1:
            colorInfo = self.getColorInformation(proxy)
            proxyJson['colorBy'] = colorInfo
        elif proxy.SMProxy.IsA('vtkSMSourceProxy') == 1:
            dataInfo = self.getDataInformation(proxy)
            proxyJson['data'] = dataInfo

        return proxyJson

    @exportRpc("pv.proxy.manager.find.id")
    def findProxyId(self, groupName, proxyName):
        proxyMgr = servermanager.ProxyManager()
        proxy = proxyMgr.GetProxy(groupName, proxyName)
        proxyId = proxy.SMProxy.GetGlobalIDAsString()
        return proxyId

    @exportRpc("pv.proxy.manager.update")
    def update(self, propertiesList):
        """
        Takes a list of properties and updates the corresponding proxies.
        """
        failureList = []
        for newPropObj in propertiesList:
            try:
                proxyId = str(newPropObj['id'])
                proxy = self.mapIdToProxy(proxyId)
                propertyName = servermanager._make_name_valid(newPropObj['name'])
                propertyValue = helper.removeUnicode(newPropObj['value'])
                proxyProperty = servermanager._wrap_property(proxy, proxy.GetProperty(propertyName))
                self.setProperty(proxyProperty, propertyValue)
            except:
                failureList.append('Unable to set property for proxy ' + proxyId + ': ' + str(newPropObj))

        if len(failureList) > 0:
            return { 'success': False,
                     'errorList': failureList }

        return { 'success': True }

    @exportRpc("pv.proxy.manager.delete")
    def delete(self, proxyId):
        """
        Checks if the proxy can be deleted (it is not the input to another
        proxy), and then deletes it if so.  Returns True if the proxy was
        deleted, False otherwise.
        """
        proxy = self.mapIdToProxy(proxyId)
        canDelete = True
        pid = '0'
        for proxyJson in self.list()['sources']:
            if proxyJson['parent'] == proxyId:
                canDelete = False
            elif proxyJson['id'] == proxyId:
                pid = proxyJson['parent']
        if proxy is not None and canDelete is True:
            simple.Delete(proxy)
            return { 'success': 1, 'id': pid }
        return { 'success': 0, 'id': '0' }

    @exportRpc("pv.proxy.manager.list")
    def list(self, viewId=None):
        """
        Returns the current proxy list, specifying for each proxy it's
        name, id, and parent (input) proxy id.  A 'parent' of '0' means
        the proxy has no input.

            { 'view': '376',
              'sources': [
                {'name': 'disk_out_ref', 'id': '350', 'parent': '0', 'rep': '352', 'visible': 1 },
                {'name': 'Contour0', 'id': '455', 'parent': '350', 'rep': '319', 'visible': 0 },
                {'name': 'Clip0', 'id': '471', 'parent': '455', 'rep': '327', 'visible': 1 },
                {'name': 'Calc0', 'id': '221', 'multiparent': 2,
                 'parent': '471', 'parent_1': '455', 'rep': '756', 'visible': 1 }
              ]
            }
        """
        proxies = servermanager.ProxyManager().GetProxiesInGroup("sources")
        viewProxy = self.getView(viewId)
        proxyObj = { 'view': viewProxy.GetGlobalIDAsString() }
        proxyList = []
        for key in proxies:
            listElt = {}
            listElt['name'] = key[0]
            listElt['id'] = key[1]
            proxy = proxies[key]

            rep = simple.GetRepresentation(proxy=proxy, view=viewProxy)
            listElt['rep'] = rep.GetGlobalIDAsString()
            listElt['visible'] = rep.Visibility
            listElt['parent'] = '0'
            if hasattr(proxy, 'Input') and proxy.Input:
                inputProp = proxy.Input
                if hasattr(inputProp, 'GetNumberOfProxies'):
                    numProxies = inputProp.GetNumberOfProxies()
                    if numProxies > 1:
                        listElt['multiparent'] = numProxies
                        for inputIdx in range(numProxies):
                            proxyId = inputProp.GetProxy(inputIdx).GetGlobalIDAsString()
                            if inputIdx == 0:
                                listElt['parent'] = proxyId
                            else:
                                listElt['parent_%d' % inputIdx] = proxyId
                    elif numProxies == 1:
                        listElt['parent'] = inputProp.GetProxy(0).GetGlobalIDAsString()
                else:
                    listElt['parent'] = inputProp.GetGlobalIDAsString()

            proxyList.append(listElt)

        proxyObj['sources'] = proxyList
        return proxyObj

    @exportRpc("pv.proxy.manager.available")
    def available(self, typeOfProxy):
        """
        Returns a list of the available sources or filters, depending on the
        argument typeOfProxy, which can be either 'filters' or 'sources'.  If
        typeOfProxy is anything other than 'filter' or 'source', the empty
        list will be returned.

        Any attempt to create a source or filter not returned by this method
        will result in an error response.  The returned list has the following
        form:

        [ 'Wavelet', 'Cone', 'Sphere', ... ]

           or

        [ 'Contour', 'Clip', 'Slice', ... ]

        """
        return self.availableList[typeOfProxy]

# =============================================================================
#
# Pipeline manager
#
# =============================================================================

class ParaViewWebPipelineManager(ParaViewWebProtocol):

    def __init__(self, baseDir=None, fileToLoad=None):
        super(ParaViewWebPipelineManager, self).__init__()
        # Setup global variables
        self.pipeline = helper.Pipeline('Kitware')
        self.view = simple.GetRenderView()
        self.baseDir = baseDir;
        simple.SetActiveView(self.view)
        simple.Render()
        if fileToLoad and fileToLoad[-5:] != '.pvsm':
            try:
                self.openFile(fileToLoad)
            except:
                print "error loading..."

    def getColorTransferFunction(self, arrayName):
        proxyMgr = vtkSMProxyManager.GetProxyManager()
        sessionProxyMgr = proxyMgr.GetActiveSessionProxyManager()
        lutMgr = vtkSMTransferFunctionManager()
        return lutMgr.GetColorTransferFunction(arrayName, sessionProxyMgr)

    # RpcName: reloadPipeline => pv.pipeline.manager.reload
    @exportRpc("pv.pipeline.manager.reload")
    def reloadPipeline(self):
        self.pipeline.clear()
        pxm = simple.servermanager.ProxyManager()

        # Fill tree structure (order-less)
        for proxyInfo in pxm.GetProxiesInGroup("sources"):
            id = str(proxyInfo[1])
            proxy = helper.idToProxy(id)
            parentId = helper.getParentProxyId(proxy)
            self.pipeline.addNode(parentId, id)

        return self.pipeline.getRootNode(self.getView(-1))

    # RpcName: getPipeline => pv.pipeline.manager.pipeline.get
    @exportRpc("pv.pipeline.manager.pipeline.get")
    def getPipeline(self):
        if self.pipeline.isEmpty():
            return self.reloadPipeline()
        return self.pipeline.getRootNode(self.getView(-1))

    # RpcName: addSource => pv.pipeline.manager.proxy.add
    @exportRpc("pv.pipeline.manager.proxy.add")
    def addSource(self, algo_name, parent):
        pid = str(parent)
        parentProxy = helper.idToProxy(parent)
        if parentProxy:
            simple.SetActiveSource(parentProxy)
        else:
            pid = '0'

        # Create new source/filter
        cmdLine = 'simple.' + algo_name + '()'
        newProxy = eval(cmdLine)

        # Create its representation and render
        simple.Show()
        simple.Render()
        simple.ResetCamera()

        # Add node to pipeline
        self.pipeline.addNode(pid, newProxy.GetGlobalIDAsString())

        # Handle domains
        helper.apply_domains(parentProxy, newProxy.GetGlobalIDAsString())

        # Return the newly created proxy pipeline node
        return helper.getProxyAsPipelineNode(newProxy.GetGlobalIDAsString(), self.getView(-1))

    # RpcName: deleteSource => pv.pipeline.manager.proxy.delete
    @exportRpc("pv.pipeline.manager.proxy.delete")
    def deleteSource(self, proxy_id):
        self.pipeline.removeNode(proxy_id)
        proxy = helper.idToProxy(proxy_id)
        simple.Delete(proxy)
        simple.Render()

    # RpcName: updateDisplayProperty => pv.pipeline.manager.proxy.representation.update
    @exportRpc("pv.pipeline.manager.proxy.representation.update")
    def updateDisplayProperty(self, options):
        proxy = helper.idToProxy(options['proxy_id'])
        rep = simple.GetDisplayProperties(proxy)
        helper.updateProxyProperties(rep, options)

        if options.has_key('ColorArrayName') and len(options['ColorArrayName']) > 0:
            name = options['ColorArrayName']
            type = options['ColorAttributeType']

            if type == 'POINT_DATA':
                attr_type = vtkDataObject.POINT
            elif type == 'CELL_DATA':
                attr_type = vtkDataObject.CELL

            dataRepr = simple.GetRepresentation(proxy)

            vtkSMPVRepresentationProxy.SetScalarColoring(dataRepr.SMProxy, name, attr_type)
            vtkSMPVRepresentationProxy.RescaleTransferFunctionToDataRange(dataRepr.SMProxy, name, attr_type, True)

        simple.Render()

    # RpcName: pushState => pv.pipeline.manager.proxy.update
    @exportRpc("pv.pipeline.manager.proxy.update")
    def pushState(self, state):
        proxy_type = None
        for proxy_id in state:
            if proxy_id in ['proxy', 'widget_source']:
                proxy_type = proxy_id
                continue
            proxy = helper.idToProxy(proxy_id);
            helper.updateProxyProperties(proxy, state[proxy_id])
            simple.Render()

        if proxy_type == 'proxy':
            return helper.getProxyAsPipelineNode(state['proxy'], self.getView(-1))
        elif proxy_type == 'widget_source':
            proxy.UpdateWidget(proxy.Observed)

    # RpcName: openFile => pv.pipeline.manager.file.open
    @exportRpc("pv.pipeline.manager.file.open")
    def openFile(self, path):
        reader = simple.OpenDataFile(path)
        simple.RenameSource( path.split("/")[-1], reader)
        simple.Show()
        simple.Render()
        simple.ResetCamera()

        # Add node to pipeline
        self.pipeline.addNode('0', reader.GetGlobalIDAsString())

        return helper.getProxyAsPipelineNode(reader.GetGlobalIDAsString(), self.getView(-1))

    # RpcName: openRelativeFile => pv.pipeline.manager.file.ropen
    @exportRpc("pv.pipeline.manager.file.ropen")
    def openRelativeFile(self, relativePath):
        fileToLoad = []
        if type(relativePath) == list:
            for file in relativePath:
               fileToLoad.append(os.path.join(self.baseDir, file))
        else:
            fileToLoad.append(os.path.join(self.baseDir, relativePath))

        reader = simple.OpenDataFile(fileToLoad)
        name = fileToLoad[0].split("/")[-1]
        if len(name) > 15:
            name = name[:15] + '*'
        simple.RenameSource(name, reader)
        simple.Show()
        simple.Render()
        simple.ResetCamera()

        # Add node to pipeline
        self.pipeline.addNode('0', reader.GetGlobalIDAsString())

        return helper.getProxyAsPipelineNode(reader.GetGlobalIDAsString(), self.getView(-1))

    # RpcName: updateScalarbarVisibility => pv.pipeline.manager.scalarbar.visibility.update
    @exportRpc("pv.pipeline.manager.scalarbar.visibility.update")
    def updateScalarbarVisibility(self, options):
        lutMgr = vtkSMTransferFunctionManager()
        lutMap = {}
        view = self.getView(-1)
        if options:
            for key, lut in options.iteritems():
                visibility = lut['enabled']
                if type(lut['name']) == unicode:
                    lut['name'] = str(lut['name'])
                parts = key.split('_')
                arrayName = parts[0]
                numComps = int(parts[1])

                lutProxy = self.getColorTransferFunction(arrayName)
                barRep = servermanager._getPyProxy(lutMgr.GetScalarBarRepresentation(lutProxy, view.SMProxy))

                if visibility == 1:
                    barRep.Visibility = 1
                    barRep.Enabled = 1
                    barRep.Title = arrayName
                    if numComps > 1:
                        barRep.ComponentTitle = 'Magnitude'
                    else:
                        barRep.ComponentTitle = ''
                    vtkSMScalarBarWidgetRepresentationProxy.PlaceInView(barRep.SMProxy, view.SMProxy)
                else:
                    barRep.Visibility = 0
                    barRep.Enabled = 0

                lutMap[key] = { 'lutId': lut['name'],
                                        'name': arrayName,
                                        'size': numComps,
                                        'enabled': visibility }
        return lutMap

    # RpcName: updateScalarRange => pv.pipeline.manager.scalar.range.rescale
    @exportRpc("pv.pipeline.manager.scalar.range.rescale")
    def updateScalarRange(self, proxyId):
        proxy = self.mapIdToProxy(proxyId);
        dataRepr = simple.GetRepresentation(proxy)
        vtkSMPVRepresentationProxy.RescaleTransferFunctionToDataRange(dataRepr.SMProxy, False)

    # RpcName: setLutDataRange => pv.pipeline.manager.lut.range.update
    @exportRpc("pv.pipeline.manager.lut.range.update")
    def setLutDataRange(self, name, number_of_components, customRange):
        lut = self.getColorTransferFunction(name)
        vtkSMTransferFunctionProxy.RescaleTransferFunction(lut, customRange[0],
                                                           customRange[1], False)

    # RpcName: getLutDataRange => pv.pipeline.manager.lut.range.get
    @exportRpc("pv.pipeline.manager.lut.range.get")
    def getLutDataRange(self, name, number_of_components):
        lut = self.getColorTransferFunction(name)
        rgbPoints = lut.GetProperty('RGBPoints')
        return [ rgbPoints.GetElement(0),
                 rgbPoints.GetElement(rgbPoints.GetNumberOfElements() - 4) ]


# =============================================================================
#
# Key/Value Store Protocol
#
# =============================================================================

class ParaViewWebKeyValuePairStore(ParaViewWebProtocol):

    def __init__(self):
        super(ParaViewWebKeyValuePairStore, self).__init__()
        self.keyValStore = {}

    # RpcName: storeKeyPair => 'pv.keyvaluepair.store'
    @exportRpc("pv.keyvaluepair.store")
    def storeKeyPair(self, key, value):
        self.keyValStore[key] = value

    # RpcName: retrieveKeyPair => 'pv.keyvaluepair.retrieve'
    @exportRpc("pv.keyvaluepair.retrieve")
    def retrieveKeyPair(self, key):
        if key in self.keyValStore:
            return self.keyValStore[key]
        else:
            return None


# =============================================================================
#
# Filter list
#
# =============================================================================

class ParaViewWebFilterList(ParaViewWebProtocol):

    def __init__(self, filtersFile=None):
        super(ParaViewWebFilterList, self).__init__()
        self.filterFile = filtersFile

    # RpcName: listFilters => pv.filters.list
    @exportRpc("pv.filters.list")
    def listFilters(self):
        filterSet = []
        if self.filterFile is None :
            filterSet = [{
                        'name': 'Cone',
                        'icon': 'dataset',
                        'category': 'source'
                    },{
                        'name': 'Sphere',
                        'icon': 'dataset',
                        'category': 'source'
                    },{
                        'name': 'Wavelet',
                        'icon': 'dataset',
                        'category': 'source'
                    },{
                        'name': 'Clip',
                        'icon': 'clip',
                        'category': 'filter'
                    },{
                        'name': 'Slice',
                        'icon': 'slice',
                        'category': 'filter'
                    },{
                        'name': 'Contour',
                        'icon': 'contour',
                        'category': 'filter'
                    },{
                        'name': 'Threshold',
                        'icon': 'threshold',
                        'category': 'filter'
                    },{
                        'name': 'StreamTracer',
                        'icon': 'stream',
                        'category': 'filter'
                    },{
                        'name': 'WarpByScalar',
                        'icon': 'filter',
                        'category': 'filter'
                    }]
        else :
            with open(self.filterFile, 'r') as fd:
                filterSet = json.loads(fd.read())

        if servermanager.ActiveConnection.GetNumberOfDataPartitions() > 1:
            filterSet.append({ 'name': 'D3', 'icon': 'filter', 'category': 'filter' })

        return filterSet


# =============================================================================
#
# Remote file list @DEPRECATED
#
# =============================================================================

class ParaViewWebFileManager(ParaViewWebProtocol):

    def __init__(self, defaultDirectoryToList):
        super(ParaViewWebFileManager, self).__init__()
        self.directory = defaultDirectoryToList
        self.dirCache = None

    # RpcName: listFiles => pv.files.list
    @exportRpc("pv.files.list")
    def listFiles(self):
        if not self.dirCache:
            self.dirCache = helper.listFiles(self.directory)
        return self.dirCache


# =============================================================================
#
# Handle remote Connection
#
# =============================================================================

class ParaViewWebRemoteConnection(ParaViewWebProtocol):

    # RpcName: connect => pv.remote.connect
    @exportRpc("pv.remote.connect")
    def connect(self, options):
        """
        Creates a connection to a remote pvserver.
        Expect an option argument which should override any of
        those default properties::

            {
            'host': 'localhost',
            'port': 11111,
            'rs_host': None,
            'rs_port': 11111
            }

        """
        ds_host = "localhost"
        ds_port = 11111
        rs_host = None
        rs_port = 11111


        if options:
            if options.has_key("host"):
                ds_host = options["host"]
            if options.has_key("port"):
                ds_port = options["port"]
            if options.has_key("rs_host"):
                rs_host = options["rs_host"]
            if options.has_key("rs_port"):
                rs_host = options["rs_port"]

        simple.Connect(ds_host, ds_port, rs_host, rs_port)

    # RpcName: reverseConnect => pv.remote.reverse.connect
    @exportRpc("pv.remote.reverse.connect")
    def reverseConnect(self, port=11111):
        """
        Create a reverse connection to a server.  Listens on port and waits for
        an incoming connection from the server.
        """
        simple.ReverseConnect(port)

    # RpcName: pvDisconnect => pv.remote.disconnect
    @exportRpc("pv.remote.disconnect")
    def pvDisconnect(self, message):
        """Free the current active session"""
        simple.Disconnect()


# =============================================================================
#
# Handle remote Connection at startup
#
# =============================================================================

class ParaViewWebStartupRemoteConnection(ParaViewWebProtocol):

    connected = False

    def __init__(self, dsHost = None, dsPort = 11111, rsHost=None, rsPort=22222, rcPort=-1):
        super(ParaViewWebStartupRemoteConnection, self).__init__()
        if not ParaViewWebStartupRemoteConnection.connected and dsHost:
            ParaViewWebStartupRemoteConnection.connected = True
            simple.Connect(dsHost, dsPort, rsHost, rsPort)
        elif not ParaViewWebStartupRemoteConnection.connected and rcPort >= 0:
            ParaViewWebStartupRemoteConnection.connected = True
            simple.ReverseConnect(str(rcPort))


# =============================================================================
#
# Handle plugin loading at startup
#
# =============================================================================

class ParaViewWebStartupPluginLoader(ParaViewWebProtocol):

    loaded = False

    def __init__(self, plugins=None, pathSeparator=':'):
        super(ParaViewWebStartupPluginLoader, self).__init__()
        if not ParaViewWebStartupPluginLoader.loaded and plugins:
            ParaViewWebStartupPluginLoader.loaded = True
            for path in plugins.split(pathSeparator):
                simple.LoadPlugin(path, ns=globals())

# =============================================================================
#
# Handle State Loading
#
# =============================================================================

class ParaViewWebStateLoader(ParaViewWebProtocol):

    def __init__(self, state_path = None):
        super(ParaViewWebStateLoader, self).__init__()
        if state_path and state_path[-5:] == '.pvsm':
            self.loadState(state_path)

    # RpcName: loadState => pv.loader.state
    @exportRpc("pv.loader.state")
    def loadState(self, state_file):
        """
        Load a state file and return the list of view ids
        """
        simple.LoadState(state_file)
        ids = []
        for view in simple.GetRenderViews():
            ids.append(view.GetGlobalIDAsString())
        return ids

# =============================================================================
#
# Handle Server File Listing
#
# =============================================================================

class ParaViewWebFileListing(ParaViewWebProtocol):

    def __init__(self, basePath, name, excludeRegex=r"^\.|~$|^\$", groupRegex=r"[0-9]+\."):
        """
        Configure the way the WebFile browser will expose the server content.
         - basePath: specify the base directory (or directories) that we should start with, if this
         parameter takes the form: "name1=path1|name2=path2|...", then we will treat this as the
         case where multiple data directories are required.  In this case, each top-level directory
         will be given the name associated with the directory in the argument.
         - name: Name of that base directory that will show up on the web
         - excludeRegex: Regular expression of what should be excluded from the list of files/directories
        """
        self.setBaseDirectory(basePath)

        self.rootName = self.overrideDataDirKey or name
        self.pattern = re.compile(excludeRegex)
        self.gPattern = re.compile(groupRegex)
        pxm = simple.servermanager.ProxyManager()
        self.directory_proxy = pxm.NewProxy('misc', 'ListDirectory')
        self.fileList = simple.servermanager.VectorProperty(self.directory_proxy,self.directory_proxy.GetProperty('FileList'))
        self.directoryList = simple.servermanager.VectorProperty(self.directory_proxy,self.directory_proxy.GetProperty('DirectoryList'))

    def handleSingleRoot(self, baseDirectory, relativeDir, startPath=None):
        path = startPath or [ self.rootName ]
        if len(relativeDir) > len(self.rootName):
            relativeDir = relativeDir[len(self.rootName)+1:]
            path += relativeDir.replace('\\','/').split('/')

        currentPath = os.path.join(baseDirectory, relativeDir)

        self.directory_proxy.List(currentPath)

        # build file/dir lists
        files = []
        if len(self.fileList) > 1:
            for f in self.fileList.GetData():
                if not re.search(self.pattern, f):
                    files.append( { 'label': f })
        elif len(self.fileList) == 1 and not re.search(self.pattern, self.fileList.GetData()):
            files.append( { 'label': self.fileList.GetData() })

        dirs = []
        if len(self.directoryList) > 1:
            for d in self.directoryList.GetData():
                if not re.search(self.pattern, d):
                    dirs.append(d)
        elif len(self.directoryList) == 1 and not re.search(self.pattern, self.directoryList.GetData()):
            dirs.append(self.directoryList.GetData())

        result =  { 'label': relativeDir, 'files': files, 'dirs': dirs, 'groups': [], 'path': path }
        if relativeDir == '.':
            result['label'] = self.rootName

        # Filter files to create groups
        files.sort()
        groups = result['groups']
        groupIdx = {}
        filesToRemove = []
        for file in files:
            fileSplit = re.split(self.gPattern, file['label'])
            if len(fileSplit) == 2:
                filesToRemove.append(file)
                gName = '*.'.join(fileSplit)
                if groupIdx.has_key(gName):
                    groupIdx[gName]['files'].append(file['label'])
                else:
                    groupIdx[gName] = { 'files' : [file['label']], 'label': gName }
                    groups.append(groupIdx[gName])
        for file in filesToRemove:
            gName = '*.'.join(re.split(self.gPattern, file['label']))
            if len(groupIdx[gName]['files']) > 1:
                files.remove(file)
            else:
                groups.remove(groupIdx[gName])

        return result

    def handleMultiRoot(self, relativeDir):
        if relativeDir == '.':
            return { 'label': self.rootName, 'files': [], 'dirs': self.baseDirectoryMap.keys(), 'groups': [], 'path': [ self.rootName ] }

        pathList = relativeDir.replace('\\', '/').split('/')
        currentBaseDir = self.baseDirectoryMap[pathList[1]]
        if len(pathList) == 2:
            return self.handleSingleRoot(currentBaseDir, '.', pathList)
        else:  # must be greater than 2
            return self.handleSingleRoot(currentBaseDir, '/'.join([pathList[0]] + pathList[2:]), pathList[0:2])

    # RpcName: listServerDirectory => file.server.directory.list
    @exportRpc("file.server.directory.list")
    def listServerDirectory(self, relativeDir='.'):
        """
        RPC Callback to list a server directory relative to the basePath
        provided at start-up.
        """
        if self.multiRoot == True:
            return self.handleMultiRoot(relativeDir)
        else:
            return self.handleSingleRoot(self.baseDirectory, relativeDir)

# =============================================================================
#
# Handle Data Selection
#
# =============================================================================
from vtkPVClientServerCoreRenderingPython import *
from vtkCommonCorePython import *

class ParaViewWebSelectionHandler(ParaViewWebProtocol):

    def __init__(self):
        self.active_view = None
        self.previous_interaction = -1
        self.selection_type = -1

    # RpcName: startSelection => pv.selection.start
    @exportRpc("pv.selection.start")
    def startSelection(self, viewId, selectionType):
        """
        Method used to initialize an interactive selection
        """
        self.active_view = self.getView(viewId)
        if self.active_view.IsSelectionAvailable():
            self.previous_interaction = self.active_view.InteractionMode
            self.active_view.InteractionMode = vtkPVRenderView.INTERACTION_MODE_SELECTION
        else:
            self.active_view = None

    # RpcName: endSelection => pv.selection.end
    @exportRpc("pv.selection.end")
    def endSelection(self, area, extract):
        """
        Method used to finalize an interactive selection by providing
        the [ startPointX, startPointY, endPointX, endPointY ] area
        where (0,0) match the lower left corner of the pixel screen.
        """
        if self.active_view:
            self.active_view.InteractionMode = self.previous_interaction
            representations = vtkCollection()
            sources = vtkCollection()
            if self.selection_type == 0:
                self.active_view.SelectSurfacePoints(area, representations, sources, False)
            elif self.selection_type == 1:
                self.active_view.SelectSurfaceCells(area, representations, sources, False)
            elif self.selection_type == 2:
                self.active_view.SelectFrustumPoints(area, representations, sources, False)
            elif self.selection_type == 3:
                self.active_view.SelectFrustumCells(area, representations, sources, False)
            else:
                self.active_view.SelectSurfacePoints(area, representations, sources, False)
            # Don't know what to do if more than one representation/source
            if representations.GetNumberOfItems() == sources.GetNumberOfItems() and sources.GetNumberOfItems() == 1:
                # We are good for selection
                rep = servermanager._getPyProxy(representations.GetItemAsObject(0))
                selection = servermanager._getPyProxy(sources.GetItemAsObject(0))
                if extract:
                    extract = simple.ExtractSelection(Input=rep.Input, Selection=selection)
                    simple.Show(extract)
                    simple.Render()
                else:
                    rep.Input.SMProxy.SetSelectionInput(0, selection.SMProxy, 0)

# =============================================================================
#
# Handle Data Export
#
# =============================================================================

class ParaViewWebExportData(ParaViewWebProtocol):

    def __init__(self, basePath):
        self.base_export_path = basePath

    # RpcName: exportData => pv.export.data
    @exportRpc("pv.export.data")
    def exportData(self, proxy_id, path):
        proxy = self.mapIdToProxy(proxy_id)
        fullpath = str(os.path.join(self.base_export_path, str(path)))
        if fullpath.index('.vtk') == -1:
            fullpath += '.vtk'
        parentDir = os.path.dirname(fullpath)
        if not os.path.exists(parentDir):
            os.makedirs(parentDir)
        if proxy:
            writer = simple.DataSetWriter(Input=proxy, FileName=fullpath)
            writer.UpdatePipeline()
            del writer

# =============================================================================
#
# Protocols useful only for testing purposes
#
# =============================================================================

class ParaViewWebTestProtocols(ParaViewWebProtocol):

    # RpcName: clearAll => pv.test.reset
    @exportRpc("pv.test.reset")
    def clearAll(self):
        simple.Disconnect()
        simple.Connect()
        view = simple.GetRenderView()
        simple.SetActiveView(view)
        simple.Render()

    # RpcName: getColoringInfo => pv.test.color.info.get
    @exportRpc("pv.test.color.info.get")
    def getColoringInfo(self, proxyId):
        proxy = self.mapIdToProxy(proxyId)
        rep = simple.GetRepresentation(proxy)
        lut = rep.LookupTable
        arrayInfo = rep.GetArrayInformationForColorArray()
        arrayName = arrayInfo.GetName()

        return { 'arrayName': arrayName,
                 'vectorMode': lut.VectorMode,
                 'vectorComponent': lut.VectorComponent }

    @exportRpc("pv.test.repr.get")
    def getReprFromSource(self, srcProxyId):
        proxy = self.mapIdToProxy(srcProxyId)
        rep = simple.GetRepresentation(proxy)
        return { 'reprProxyId': rep.GetGlobalIDAsString() }


# =============================================================================
#
# Handle Widget Representation
#
# =============================================================================

def _line_update_widget(self, widget):
    widget.Point1WorldPosition = self.Point1;
    widget.Point2WorldPosition = self.Point2;

def _line_widget_update(self, obj, event):
    self.GetProperty('Point1').Copy(obj.GetProperty('Point1WorldPositionInfo'))
    self.GetProperty('Point2').Copy(obj.GetProperty('Point2WorldPositionInfo'))
    self.UpdateVTKObjects()

def _plane_update_widget(self, widget):
    widget.GetProperty('OriginInfo').SetData(self.Origin)
    widget.Origin = self.Origin
    widget.Normal = self.Normal
    widget.UpdateVTKObjects()

def _plane_widget_update(self, obj, event):
    self.GetProperty('Origin').Copy(obj.GetProperty('OriginInfo'))
    self.GetProperty('Normal').Copy(obj.GetProperty('NormalInfo'))
    self.UpdateVTKObjects()
    _hide_plane(obj)

def _draw_plane(obj,event):
    obj.GetProperty('DrawPlane').SetElement(0,1)
    obj.UpdateVTKObjects()

def _hide_plane(obj):
    obj.GetProperty('DrawPlane').SetElement(0,0)
    obj.UpdateVTKObjects()

class ParaViewWebWidgetManager(ParaViewWebProtocol):

    # RpcName: addRuler => pv.widgets.ruler.add
    @exportRpc("pv.widgets.ruler.add")
    def addRuler(self, view_id=-1):
        proxy = simple.Ruler(Point1=[-1.0, -1.0, -1.0], Point2=[1.0, 1.0, 1.0])
        self.createWidgetRepresentation(proxy.GetGlobalID(), view_id)
        return proxy.GetGlobalIDAsString()

    # RpcName: createWidgetRepresentation => pv.widgets.representation.create
    @exportRpc("pv.widgets.representation.create")
    def createWidgetRepresentation(self, proxy_id, view_id):
        proxy = self.mapIdToProxy(proxy_id)
        view = self.getView(view_id)
        widgetProxy = None
        # Find the corresponding widget representation
        if proxy.__class__.__name__ == 'Plane':
            widgetProxy = self.CreateWidgetRepresentation(view, 'ImplicitPlaneWidgetRepresentation')
            setattr(proxy.__class__, 'UpdateWidget', _plane_update_widget)
            setattr(proxy.__class__, 'WidgetUpdate', _plane_widget_update)
            widgetProxy.GetProperty('DrawPlane').SetElement(0, 0)
            widgetProxy.GetProperty('PlaceFactor').SetElement(0, 1.0)
            proxy.UpdateWidget(widgetProxy)
            widgetProxy.AddObserver("StartInteractionEvent", _draw_plane)
            proxy.Observed = widgetProxy
            proxy.ObserverTag = widgetProxy.AddObserver("EndInteractionEvent", proxy.WidgetUpdate)
        elif proxy.__class__.__name__ == 'Box':
            widgetProxy = self.CreateWidgetRepresentation(view, 'BoxWidgetRepresentation')
        elif proxy.__class__.__name__ == 'Handle':
            widgetProxy = self.CreateWidgetRepresentation(view, 'HandleWidgetRepresentation')
        elif proxy.__class__.__name__ == 'PointSource':
            widgetProxy = self.CreateWidgetRepresentation(view, 'PointSourceWidgetRepresentation')
        elif proxy.__class__.__name__ == 'LineSource' or proxy.__class__.__name__ == 'HighResolutionLineSource' :
            widgetProxy = self.CreateWidgetRepresentation(view, 'LineSourceWidgetRepresentation')
            setattr(proxy.__class__, 'UpdateWidget', _line_update_widget)
            setattr(proxy.__class__, 'WidgetUpdate', _line_widget_update)
            proxy.UpdateWidget(widgetProxy)
            proxy.Observed = widgetProxy
            proxy.ObserverTag = widgetProxy.AddObserver("EndInteractionEvent", proxy.WidgetUpdate)
        elif proxy.__class__.__name__ == 'Line':
            widgetProxy = self.CreateWidgetRepresentation(view, 'LineWidgetRepresentation')
            setattr(proxy.__class__, 'UpdateWidget', _line_update_widget)
            setattr(proxy.__class__, 'WidgetUpdate', _line_widget_update)
            proxy.UpdateWidget(widgetProxy)
            proxy.Observed = widgetProxy
            proxy.ObserverTag = widgetProxy.AddObserver("EndInteractionEvent", proxy.WidgetUpdate)
        elif proxy.__class__.__name__ in ['Distance', 'Ruler'] :
            widgetProxy = self.CreateWidgetRepresentation(view, 'DistanceWidgetRepresentation')
            setattr(proxy.__class__, 'UpdateWidget', _line_update_widget)
            setattr(proxy.__class__, 'WidgetUpdate', _line_widget_update)
            proxy.UpdateWidget(widgetProxy)
            proxy.Observed = widgetProxy
            proxy.ObserverTag = widgetProxy.AddObserver("EndInteractionEvent", proxy.WidgetUpdate)
        elif proxy.__class__.__name__ == 'Sphere':
            widgetProxy = self.CreateWidgetRepresentation(view, 'SphereWidgetRepresentation')
        elif proxy.__class__.__name__ == 'Spline':
            widgetProxy = self.CreateWidgetRepresentation(view, 'SplineWidgetRepresentation')
        else:
            print "No widget representation for %s" % proxy.__class__.__name__

        return widgetProxy.GetGlobalIDAsString()

    def CreateWidgetRepresentation(self, view, name):
        proxy = simple.servermanager.CreateProxy("representations", name, None)
        pythonWrap = simple.servermanager.rendering.__dict__[proxy.GetXMLName()]()
        pythonWrap.UpdateVTKObjects()
        view.Representations.append(pythonWrap)
        pythonWrap.Visibility = 1
        pythonWrap.Enabled = 1
        return pythonWrap
