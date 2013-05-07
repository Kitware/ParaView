r"""paraviewweb_protocols is a module that contains a set of ParaViewWeb related
protocols that can be combined together to provide a flexible way to define
very specific web application.
"""

import types
import logging
import inspect
from time import time

# import RPC annotation
from autobahn.wamp import exportRpc

# import paraview modules.
from paraview import simple, web, servermanager, web_helper
from vtkParaViewWebCorePython import vtkPVWebInteractionEvent

# =============================================================================
#
# Base class for any ParaView based protocol
#
# =============================================================================

class ParaViewWebProtocol(object):

    def __init__(self):
        self.Application = None

    def mapIdToProxy(self, id):
        """
        Maps global-id for a proxy to the proxy instance. May return None if the
        id is not valid.
        """
        id = int(id)
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
            raise Exception("no view provided: " + vid)

        return view

    def setApplication(self, app):
        self.Application = app

    def getApplication(self):
        return self.Application

# =============================================================================
#
# Handle Mouse interaction on any type of view
#
# =============================================================================

class ParaViewWebMouseHandler(ParaViewWebProtocol):

    @exportRpc("mouseInteraction")
    def mouseInteraction(self, event):
        """
        RPC Callback for mouse interactions.
        """
        view = self.getView(event['view'])

        buttons = 0
        if event["buttonLeft"]:
            buttons |= vtkPVWebInteractionEvent.LEFT_BUTTON;
        if event["buttonMiddle"]:
            buttons |= vtkPVWebInteractionEvent.MIDDLE_BUTTON;
        if event["buttonRight"]:
            buttons |= vtkPVWebInteractionEvent.RIGHT_BUTTON;

        modifiers = 0
        if event["shiftKey"]:
            modifiers |= vtkPVWebInteractionEvent.SHIFT_KEY
        if event["ctrlKey"]:
            modifiers |= vtkPVWebInteractionEvent.CTRL_KEY
        if event["altKey"]:
            modifiers |= vtkPVWebInteractionEvent.ALT_KEY
        if event["metaKey"]:
            modifiers |= vtkPVWebInteractionEvent.META_KEY

        pvevent = vtkPVWebInteractionEvent()
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

    @exportRpc("resetCamera")
    def resetCamera(self, view):
        """
        RPC callback to reset camera.
        """
        view = self.getView(view)
        simple.ResetCamera(view)
        try:
            view.CenterOfRotation = view.CameraFocalPoint
        except:
            pass

        self.getApplication().InvalidateCache(view.SMProxy)
        return view.GetGlobalIDAsString()

    @exportRpc("updateOrientationAxesVisibility")
    def updateOrientationAxesVisibility(self, view, showAxis):
        """
        RPC callback to show/hide OrientationAxis.
        """
        view = self.getView(view)
        view.OrientationAxesVisibility = (showAxis if 1 else 0);

        self.getApplication().InvalidateCache(view.SMProxy)
        return view.GetGlobalIDAsString()

    @exportRpc("updateCenterAxesVisibility")
    def updateCenterAxesVisibility(self, view, showAxis):
        """
        RPC callback to show/hide CenterAxesVisibility.
        """
        view = self.getView(view)
        view.CenterAxesVisibility = (showAxis if 1 else 0);

        self.getApplication().InvalidateCache(view.SMProxy)
        return view.GetGlobalIDAsString()

    @exportRpc("updateCamera")
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

    @exportRpc("stillRender")
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

    @exportRpc("getSceneMetaData")
    def getSceneMetaData(self, view_id):
        view  = self.getView(view_id);
        data = self.getApplication().GetWebGLSceneMetaData(view.SMProxy)
        return data

    @exportRpc("getWebGLData")
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

    @exportRpc("updateTime")
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
# Pipeline manager
#
# =============================================================================

class ParaViewWebPipelineManager(ParaViewWebProtocol):

    def __init__(self):
        super(ParaViewWebPipelineManager, self).__init__()
        # Setup global variables
        self.pipeline = web_helper.Pipeline('Kitware')
        self.lutManager = web_helper.LookupTableManager()
        self.view = simple.GetRenderView()
        simple.SetActiveView(self.view)
        simple.Render()
        self.view.ViewSize = [800,800]
        self.lutManager.setView(self.view)


    @exportRpc("getPipeline")
    def getPipeline(self):
        return self.pipeline.getRootNode(self.lutManager)

    @exportRpc("addSource")
    def addSource(self, algo_name, parent):
        pid = str(parent)
        parentProxy = web_helper.idToProxy(parent)
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

        # Create LUT if need be
        if pid == '0':
            self.lutManager.registerFieldData(newProxy.GetPointDataInformation())
            self.lutManager.registerFieldData(newProxy.GetCellDataInformation())

        # Return the newly created proxy pipeline node
        return web_helper.getProxyAsPipelineNode(newProxy.GetGlobalIDAsString(), self.lutManager)

    @exportRpc("deleteSource")
    def deleteSource(self, proxy_id):
        self.pipeline.removeNode(proxy_id)
        proxy = web_helper.idToProxy(proxy_id)
        simple.Delete(proxy)
        simple.Render()

    @exportRpc("updateDisplayProperty")
    def updateDisplayProperty(self, options):
        proxy = web_helper.idToProxy(options['proxy_id']);
        rep = simple.GetDisplayProperties(proxy)
        web_helper.updateProxyProperties(rep, options)

        # Try to bind the proper lookup table if need be
        if options.has_key('ColorArrayName') and len(options['ColorArrayName']) > 0:
            name = options['ColorArrayName']
            type = options['ColorAttributeType']
            nbComp = 1

            if type == 'POINT_DATA':
                data = proxy.GetPointDataInformation()
                for i in range(data.GetNumberOfArrays()):
                    array = data.GetArray(i)
                    if array.Name == name:
                        nbComp = array.GetNumberOfComponents()
            elif type == 'CELL_DATA':
                data = proxy.GetCellDataInformation()
                for i in range(data.GetNumberOfArrays()):
                    array = data.GetArray(i)
                    if array.Name == name:
                        nbComp = array.GetNumberOfComponents()
            lut = self.lutManager.getLookupTable(name, nbComp)
            rep.LookupTable = lut

        simple.Render()

    @exportRpc("pushState")
    def pushState(self, state):
        for proxy_id in state:
            if proxy_id == 'proxy':
                continue
            proxy = web_helper.idToProxy(proxy_id);
            web_helper.updateProxyProperties(proxy, state[proxy_id])
            simple.Render()
        return web_helper.getProxyAsPipelineNode(state['proxy'], self.lutManager)

    @exportRpc("openFile")
    def openFile(self, path):
        reader = simple.OpenDataFile(path)
        simple.RenameSource( path.split("/")[-1], reader)
        simple.Show()
        simple.Render()
        simple.ResetCamera()

        # Add node to pipeline
        self.pipeline.addNode('0', reader.GetGlobalIDAsString())

        # Create LUT if need be
        self.lutManager.registerFieldData(reader.GetPointDataInformation())
        self.lutManager.registerFieldData(reader.GetCellDataInformation())

        return web_helper.getProxyAsPipelineNode(reader.GetGlobalIDAsString(), self.lutManager)

    @exportRpc("updateScalarbarVisibility")
    def updateScalarbarVisibility(self, options):
        # Remove unicode
        if options:
            for lut in options.values():
                if type(lut['name']) == unicode:
                    lut['name'] = str(lut['name'])
                self.lutManager.enableScalarBar(lut['name'], lut['size'], lut['enabled'])
        return self.lutManager.getScalarbarVisibility()

    @exportRpc("updateScalarRange")
    def updateScalarRange(self, proxyId):
        proxy = self.mapIdToProxy(proxyId);
        self.lutManager.registerFieldData(proxy.GetPointDataInformation())
        self.lutManager.registerFieldData(proxy.GetCellDataInformation())

    @exportRpc("listFilters")
    def listFilters(self):
        return [{
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

# =============================================================================
#
# Remote file list
#
# =============================================================================

class ParaViewWebFileManager(ParaViewWebProtocol):

    def __init__(self, defaultDirectoryToList):
        super(ParaViewWebFileManager, self).__init__()
        self.directory = defaultDirectoryToList
        self.dirCache = None

    @exportRpc("listFiles")
    def listFiles(self):
        if not self.dirCache:
            self.dirCache = web_helper.listFiles(self.directory)
        return self.dirCache