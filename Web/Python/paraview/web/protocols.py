r"""paraviewweb_protocols is a module that contains a set of ParaViewWeb related
protocols that can be combined together to provide a flexible way to define
very specific web application.
"""

from __future__ import absolute_import, division, print_function

import os, sys, types, inspect, traceback, logging, re, json, fnmatch, time

# import Twisted reactor for later callback
from twisted.internet import reactor

# import RPC annotation
from wslink import register as exportRpc

# import paraview modules.
import paraview

from paraview import simple, servermanager
from paraview.servermanager import ProxyProperty, InputProperty
from paraview.web import helper
from vtkmodules.web import protocols as vtk_protocols
from vtkmodules.web import iteritems
from vtkmodules.web.render_window_serializer import SynchronizationContext, initializeSerializers, serializeInstance, getReferenceId
from paraview.web.decorators import *

from vtkmodules.vtkCommonDataModel import vtkImageData, vtkDataObject
from vtkmodules.vtkCommonCore      import vtkUnsignedCharArray, vtkCollection
from vtkmodules.vtkWebCore         import vtkDataEncoder, vtkWebInteractionEvent

from paraview.servermanager import vtkSMPVRepresentationProxy, \
        vtkSMTransferFunctionProxy, vtkSMTransferFunctionManager, \
        vtkSMProxyManager, vtkProcessModule, vtkPVRenderView

if sys.version_info >= (3,):
    xrange = range

# =============================================================================
# Helper methods
# =============================================================================

def tryint(s):
    try:
        return int(s)
    except:
        return s


def alphanum_key(s):
    """ Turn a string into a list of string and number chunks.
        "z23a" -> ["z", 23, "a"]
    """
    return [ tryint(c) for c in re.split('([0-9]+)', s) ]


def sanitizeKeys(mapObj):
    output = {}
    for key in mapObj:
        sanitizeKey = servermanager._make_name_valid(key)
        output[sanitizeKey] = mapObj[key]

    return output


# =============================================================================
#
# Base class for any ParaView based protocol
#
# =============================================================================

class ParaViewWebProtocol(vtk_protocols.vtkWebProtocol):

    def __init__(self):
        # self.Application = None
        self.coreServer = None
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
            print (msg)

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
            self.baseDirectory = os.path.normpath(self.baseDirectory)
        else:
            baseDirs = basePath.split('|')
            for baseDir in baseDirs:
                basePair = baseDir.split('=')
                if os.path.exists(basePair[1]):
                    self.baseDirectoryMap[basePair[0]] = os.path.normpath(basePair[1])

            # Check if we ended up with just a single directory
            bdKeys = list(self.baseDirectoryMap)
            if len(bdKeys) == 1:
                self.baseDirectory = os.path.normpath(self.baseDirectoryMap[bdKeys[0]])
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

        cleanedPath = os.path.normpath(absolutePath)

        # Make sure the cleanedPath is part of the allowed ones
        if self.multiRoot:
            for key, value in iteritems(self.baseDirectoryMap):
                if cleanedPath.startswith(value):
                    return cleanedPath
        elif cleanedPath.startswith(self.baseDirectory):
            return cleanedPath

        return None

    def updateScalarBars(self, view=None, mode=1):
        """
        Manage scalarbar state

            view:
                A view proxy or the current active view will be used.

            mode:
                HIDE_UNUSED_SCALAR_BARS = 0x01,
                SHOW_USED_SCALAR_BARS = 0x02
        """
        v = view or self.getView(-1)
        lutMgr = vtkSMTransferFunctionManager()
        lutMgr.UpdateScalarBars(v.SMProxy, mode)

    def publish(self, topic, event):
        if self.coreServer:
            self.coreServer.publish(topic, event)

# =============================================================================
#
# Handle Mouse interaction on any type of view
#
# =============================================================================

class ParaViewWebMouseHandler(ParaViewWebProtocol):

    def __init__(self, **kwargs):
        super(ParaViewWebMouseHandler, self).__init__()
        self.lastAction = 'up'

    # RpcName: mouseInteraction => viewport.mouse.interaction
    @exportRpc("viewport.mouse.interaction")
    def mouseInteraction(self, event):
        """
        RPC Callback for mouse interactions.
        """
        view = self.getView(event['view'])
        realViewId = view.GetGlobalIDAsString()

        if hasattr(view, 'UseInteractiveRenderingForScreenshots'):
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

        if event["action"] == 'down' and self.lastAction != event["action"]:
            self.getApplication().InvokeEvent('StartInteractionEvent')

        if event["action"] == 'up' and self.lastAction != event["action"]:
            self.getApplication().InvokeEvent('EndInteractionEvent')

        if retVal:
            self.getApplication().InvokeEvent('UpdateEvent')

        self.lastAction = event["action"]

        return retVal

# =============================================================================
#
# Basic 3D Viewport API (Camera + Orientation + CenterOfRotation
#
# =============================================================================

class ParaViewWebViewPort(ParaViewWebProtocol):

    def __init__(self, scale=1.0, maxWidth=2560, maxHeight=1440, **kwargs):
        super(ParaViewWebViewPort, self).__init__()
        self.scale = scale
        self.maxWidth = maxWidth
        self.maxHeight = maxHeight

    # RpcName: resetCamera => viewport.camera.reset
    @exportRpc("viewport.camera.reset")
    def resetCamera(self, viewId):
        """
        RPC callback to reset camera.
        """
        view = self.getView(viewId)
        simple.Render(view)
        simple.ResetCamera(view)
        try:
            view.CenterOfRotation = view.CameraFocalPoint
        except:
            pass

        self.getApplication().InvalidateCache(view.SMProxy)
        self.getApplication().InvokeEvent('UpdateEvent')

        return view.GetGlobalIDAsString()

    # RpcName: updateOrientationAxesVisibility => viewport.axes.orientation.visibility.update
    @exportRpc("viewport.axes.orientation.visibility.update")
    def updateOrientationAxesVisibility(self, viewId, showAxis):
        """
        RPC callback to show/hide OrientationAxis.
        """
        view = self.getView(viewId)
        view.OrientationAxesVisibility = (showAxis if 1 else 0);

        self.getApplication().InvalidateCache(view.SMProxy)
        self.getApplication().InvokeEvent('UpdateEvent')

        return view.GetGlobalIDAsString()

    # RpcName: updateCenterAxesVisibility => viewport.axes.center.visibility.update
    @exportRpc("viewport.axes.center.visibility.update")
    def updateCenterAxesVisibility(self, viewId, showAxis):
        """
        RPC callback to show/hide CenterAxesVisibility.
        """
        view = self.getView(viewId)
        view.CenterAxesVisibility = (showAxis if 1 else 0);

        self.getApplication().InvalidateCache(view.SMProxy)
        self.getApplication().InvokeEvent('UpdateEvent')

        return view.GetGlobalIDAsString()

    # RpcName: updateCamera => viewport.camera.update
    @exportRpc("viewport.camera.update")
    def updateCamera(self, view_id, focal_point, view_up, position, forceUpdate = True):
        view = self.getView(view_id)

        view.CameraFocalPoint = focal_point
        view.CameraViewUp = view_up
        view.CameraPosition = position

        if forceUpdate:
            self.getApplication().InvalidateCache(view.SMProxy)
            self.getApplication().InvokeEvent('UpdateEvent')


    @exportRpc("viewport.camera.get")
    def getCamera(self, view_id):
        view = self.getView(view_id)
        bounds = [-1, 1, -1, 1, -1, 1]

        if view and view.GetClientSideView().GetClassName() == 'vtkPVRenderView':
            rr = view.GetClientSideView().GetRenderer()
            bounds = rr.ComputeVisiblePropBounds()

        return {
            'bounds': bounds,
            'center': list(view.CenterOfRotation),
            'focal': list(view.CameraFocalPoint),
            'up': list(view.CameraViewUp),
            'position': list(view.CameraPosition)
        }

    @exportRpc("viewport.size.update")
    def updateSize(self, view_id, width, height):
        view = self.getView(view_id)
        w = width * self.scale
        h = height * self.scale
        if w > self.maxWidth:
            s = float(self.maxWidth) / float(w)
            w *= s
            h *= s
        elif h > self.maxHeight:
            s = float(self.maxHeight) / float(h)
            w *= s
            h *= s
        view.ViewSize = [ int(w), int(h) ]
        self.getApplication().InvokeEvent('UpdateEvent')

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
        beginTime = int(round(time.time() * 1000))
        view = self.getView(options["view"])
        size = view.ViewSize[0:2]
        resize = size != options.get("size", size)
        if resize:
            size = options["size"]
            view.ViewSize = size
        t = 0
        if options and "mtime" in options:
            t = options["mtime"]
        quality = 100
        if options and "quality" in options:
            quality = options["quality"]
        localTime = 0
        if options and "localTime" in options:
            localTime = options["localTime"]
        reply = {}
        app = self.getApplication()
        reply["image"] = app.StillRenderToString(view.SMProxy, t, quality)

        # Check that we are getting image size we have set if not wait until we
        # do.
        tries = 10;
        while resize and list(app.GetLastStillRenderImageSize()) != size \
              and size != [0, 0] and tries > 0:
            app.InvalidateCache(view.SMProxy)
            reply["image"] = app.StillRenderToString(view.SMProxy, t, quality)
            tries -= 1

        if not resize and options and ("clearCache" in options) and options["clearCache"]:
            app.InvalidateCache(view.SMProxy)
            reply["image"] = app.StillRenderToString(view.SMProxy, t, quality)

        reply["stale"] = app.GetHasImagesBeingProcessed(view.SMProxy)
        reply["mtime"] = app.GetLastStillRenderToMTime()
        reply["size"] = view.ViewSize[0:2]
        reply["format"] = "jpeg;base64"
        reply["global_id"] = view.GetGlobalIDAsString()
        reply["localTime"] = localTime

        endTime = int(round(time.time() * 1000))
        reply["workTime"] = (endTime - beginTime)

        return reply

# =============================================================================
#
# Provide Image publish-based delivery mechanism
#
# =============================================================================

CAMERA_PROP_NAMES = [
  'CameraFocalPoint',
  'CameraParallelProjection',
  'CameraParallelScale',
  'CameraPosition',
  'CameraViewAngle',
  'CameraViewUp',
]

def _pushCameraLink(viewSrc, viewDstList):
  props = {}
  for name in CAMERA_PROP_NAMES:
    props[name] = getattr(viewSrc, name)
  for v in viewDstList:
    for name in CAMERA_PROP_NAMES:
      v.__setattr__(name, props[name])
  return props

class ParaViewWebPublishImageDelivery(ParaViewWebProtocol):
    def __init__(self, decode=True, **kwargs):
        ParaViewWebProtocol.__init__(self)
        self.trackingViews = {}
        self.lastStaleTime = {}
        self.staleHandlerCount = {}
        self.deltaStaleTimeBeforeRender = 0.5 # 0.5s
        self.decode = decode
        self.viewsInAnimations = []
        self.targetFrameRate = 30.0
        self.minFrameRate = 12.0
        self.maxFrameRate = 30.0

        # Camera link handling
        self.linkedViews = []
        self.linkNames = []
        self.onLinkChange = None

        # Mouse handling
        self.lastAction = 'up'
        self.activeViewId = None

    # In case some external protocol wants to monitor when link views change
    def setLinkChangeCallback(self, fn):
      self.onLinkChange = fn

    def pushRender(self, vId, ignoreAnimation = False, staleCount=0):
        if vId not in self.trackingViews:
            return

        if not self.trackingViews[vId]["enabled"]:
            return

        if not ignoreAnimation and len(self.viewsInAnimations) > 0:
            return

        if "originalSize" not in self.trackingViews[vId]:
            view = self.getView(vId)
            self.trackingViews[vId]["originalSize"] = (int(view.ViewSize[0]), int(view.ViewSize[1]))

        if "ratio" not in self.trackingViews[vId]:
            self.trackingViews[vId]["ratio"] = 1

        ratio = self.trackingViews[vId]["ratio"]
        mtime = self.trackingViews[vId]["mtime"]
        quality = self.trackingViews[vId]["quality"]
        size = [int(s * ratio) for s in self.trackingViews[vId]["originalSize"]]

        reply = self.stillRender({ "view": vId, "mtime": mtime, "quality": quality, "size": size })

        # View might have been deleted
        if not reply:
          return

        stale = reply["stale"]
        if reply["image"]:
            # depending on whether the app has encoding enabled:
            if self.decode:
                reply["image"] = base64.standard_b64decode(reply["image"]);

            reply["image"] = self.addAttachment(reply["image"]);
            reply["format"] = "jpeg"
            # save mtime for next call.
            self.trackingViews[vId]["mtime"] = reply["mtime"]
            # echo back real ID, instead of -1 for 'active'
            reply["id"] = vId
            self.publish('viewport.image.push.subscription', reply)
        if stale:
            self.lastStaleTime[vId] = time.time()
            if self.staleHandlerCount[vId] == 0:
                self.staleHandlerCount[vId] += 1
                reactor.callLater(self.deltaStaleTimeBeforeRender, lambda: self.renderStaleImage(vId, staleCount))
        else:
            self.lastStaleTime[vId] = 0


    def renderStaleImage(self, vId, staleCount=0):
        if vId in self.staleHandlerCount and self.staleHandlerCount[vId] > 0:
            self.staleHandlerCount[vId] -= 1

            if self.lastStaleTime[vId] != 0:
                delta = (time.time() - self.lastStaleTime[vId])
                # Break on staleCount otherwise linked view will always report to be stale
                # And loop forever
                if delta >= self.deltaStaleTimeBeforeRender and staleCount < 3:
                    self.pushRender(vId, False, staleCount + 1)
                elif delta < self.deltaStaleTimeBeforeRender:
                    self.staleHandlerCount[vId] += 1
                    reactor.callLater(self.deltaStaleTimeBeforeRender - delta + 0.001, lambda: self.renderStaleImage(vId, staleCount))


    def animate(self, renderAllViews=True):
        if len(self.viewsInAnimations) == 0:
            return

        nextAnimateTime = time.time() + 1.0 /  self.targetFrameRate

        # Handle the rendering of the views
        if self.activeViewId:
          self.pushRender(self.activeViewId, True)

        if renderAllViews:
          for vId in set(self.viewsInAnimations):
              if vId != self.activeViewId:
                self.pushRender(vId, True)

        nextAnimateTime -= time.time()

        if self.targetFrameRate > self.maxFrameRate:
            self.targetFrameRate = self.maxFrameRate

        if nextAnimateTime < 0:
            if nextAnimateTime < -1.0:
                self.targetFrameRate = 1
            if self.targetFrameRate > self.minFrameRate:
                self.targetFrameRate -= 1.0
            if self.activeViewId:
                # If active view, prioritize that one over the others
                # -> Divide by 2 the refresh rate of the other views
                reactor.callLater(0.001, lambda: self.animate(not renderAllViews))
            else:
                # Keep animating at the best rate we can
                reactor.callLater(0.001, lambda: self.animate())
        else:
            # We have time so let's render all
            if self.targetFrameRate < self.maxFrameRate and nextAnimateTime > 0.005:
                self.targetFrameRate += 1.0
            reactor.callLater(nextAnimateTime, lambda: self.animate())


    @exportRpc("viewport.image.animation.fps.max")
    def setMaxFrameRate(self, fps = 30):
        self.maxFrameRate = fps


    @exportRpc("viewport.image.animation.fps.get")
    def getCurrentFrameRate(self):
        return self.targetFrameRate


    @exportRpc("viewport.image.animation.start")
    def startViewAnimation(self, viewId = '-1'):
        sView = self.getView(viewId)
        realViewId = sView.GetGlobalIDAsString()

        self.viewsInAnimations.append(realViewId)
        if len(self.viewsInAnimations) == 1:
            self.animate()


    @exportRpc("viewport.image.animation.stop")
    def stopViewAnimation(self, viewId = '-1'):
        sView = self.getView(viewId)
        realViewId = sView.GetGlobalIDAsString()

        if realViewId in self.viewsInAnimations and realViewId in self.trackingViews:
            progressRendering = self.trackingViews[realViewId]['streaming']
            self.viewsInAnimations.remove(realViewId)
            if progressRendering:
                self.progressiveRender(realViewId)


    def progressiveRender(self, viewId = '-1'):
        sView = self.getView(viewId)
        realViewId = sView.GetGlobalIDAsString()

        if realViewId in self.viewsInAnimations:
            return

        if sView.GetSession().GetPendingProgress():
            reactor.callLater(self.deltaStaleTimeBeforeRender, lambda: self.progressiveRender(viewId))
        else:
            again = sView.StreamingUpdate(True)
            self.pushRender(realViewId, True)

            if again:
                reactor.callLater(0.001, lambda: self.progressiveRender(viewId))


    @exportRpc("viewport.image.push")
    def imagePush(self, options):
        view = self.getView(options["view"])
        viewId = view.GetGlobalIDAsString()

        # Make sure an image is pushed
        self.getApplication().InvalidateCache(view.SMProxy)

        self.pushRender(viewId)


    # Internal function since the reply[image] is not
    # JSON(serializable) it can not be an RPC one
    def stillRender(self, options):
        """
        RPC Callback to render a view and obtain the rendered image.
        """
        beginTime = int(round(time.time() * 1000))
        viewId = str(options["view"])
        view = self.getView(viewId)

        # If no view id provided, skip rendering
        if not viewId:
          print('No view')
          print(options)
          return None

        # Make sure request match our selected view
        if viewId != '-1' and view.GetGlobalIDAsString() != viewId:
          # We got active view rather than our request
          view = None

        # No view to render => need some cleanup
        if not view:
          # The view has been deleted, we can not render it...
          # Clean up old view state
          if viewId in self.viewsInAnimations:
            self.viewsInAnimations.remove(viewId)

          if viewId in self.trackingViews:
            del self.trackingViews[viewId]

          if viewId in self.staleHandlerCount:
            del self.staleHandlerCount[viewId]

          # the view does not exist anymore, skip rendering
          return None

        # We are in business to render our view...

        # Make sure our view size match our request
        size = view.ViewSize[0:2]
        resize = size != options.get("size", size)
        if resize:
            size = options["size"]
            if size[0] > 10 and size[1] > 10:
              view.ViewSize = size

        # Rendering options
        t = 0
        if options and "mtime" in options:
            t = options["mtime"]
        quality = 100
        if options and "quality" in options:
            quality = options["quality"]
        localTime = 0
        if options and "localTime" in options:
            localTime = options["localTime"]
        reply = {}
        app = self.getApplication()
        if t == 0:
            app.InvalidateCache(view.SMProxy)
        if self.decode:
            stillRender = app.StillRenderToString
        else:
            stillRender = app.StillRenderToBuffer
        reply_image = stillRender(view.SMProxy, t, quality)

        # Check that we are getting image size we have set if not wait until we
        # do. The render call will set the actual window size.
        tries = 10;
        while resize and list(app.GetLastStillRenderImageSize()) != size \
              and size != [0, 0] and tries > 0:
            app.InvalidateCache(view.SMProxy)
            reply_image = stillRender(view.SMProxy, t, quality)
            tries -= 1

        if not resize and options and ("clearCache" in options) and options["clearCache"]:
            app.InvalidateCache(view.SMProxy)
            reply_image = stillRender(view.SMProxy, t, quality)

        # Pack the result
        reply["stale"] = app.GetHasImagesBeingProcessed(view.SMProxy)
        reply["mtime"] = app.GetLastStillRenderToMTime()
        reply["size"] = view.ViewSize[0:2]
        reply["memsize"] = reply_image.GetDataSize() if reply_image else 0
        reply["format"] = "jpeg;base64" if self.decode else "jpeg"
        reply["global_id"] = view.GetGlobalIDAsString()
        reply["localTime"] = localTime
        if self.decode:
            reply["image"] = reply_image
        else:
            # Convert the vtkUnsignedCharArray into a bytes object, required by Autobahn websockets
            reply["image"] = memoryview(reply_image).tobytes() if reply_image else None

        endTime = int(round(time.time() * 1000))
        reply["workTime"] = (endTime - beginTime)

        return reply


    @exportRpc("viewport.image.push.observer.add")
    def addRenderObserver(self, viewId):
        sView = self.getView(viewId)
        if not sView:
            return { 'error': 'Unable to get view with id %s' % viewId }

        realViewId = sView.GetGlobalIDAsString()

        if not realViewId in self.trackingViews:
            observerCallback = lambda *args, **kwargs: self.pushRender(realViewId)
            startCallback = lambda *args, **kwargs: self.startViewAnimation(realViewId)
            stopCallback = lambda *args, **kwargs: self.stopViewAnimation(realViewId)
            tag = self.getApplication().AddObserver('UpdateEvent', observerCallback)
            tagStart = self.getApplication().AddObserver('StartInteractionEvent', startCallback)
            tagStop = self.getApplication().AddObserver('EndInteractionEvent', stopCallback)
            # TODO do we need self.getApplication().AddObserver('ResetActiveView', resetActiveView())
            self.trackingViews[realViewId] = { 'tags': [tag, tagStart, tagStop], 'observerCount': 1, 'mtime': 0, 'enabled': True, 'quality': 100, 'streaming': sView.GetClientSideObject().GetEnableStreaming() }
            self.staleHandlerCount[realViewId] = 0
        else:
            # There is an observer on this view already
            self.trackingViews[realViewId]['observerCount'] += 1

        self.pushRender(realViewId)
        return { 'success': True, 'viewId': realViewId }


    @exportRpc("viewport.image.push.observer.remove")
    def removeRenderObserver(self, viewId):
        sView = None
        try:
            sView = self.getView(viewId)
        except:
            print('no view with ID %s available in removeRenderObserver' % viewId)

        realViewId = sView.GetGlobalIDAsString() if sView else viewId

        observerInfo = None
        if realViewId in self.trackingViews:
            observerInfo = self.trackingViews[realViewId]

        if not observerInfo:
            return { 'error': 'Unable to find subscription for view %s' % realViewId }

        observerInfo['observerCount'] -= 1

        if observerInfo['observerCount'] <= 0:
            for tag in observerInfo['tags']:
                self.getApplication().RemoveObserver(tag)
            del self.trackingViews[realViewId]
            del self.staleHandlerCount[realViewId]

        return { 'result': 'success' }


    @exportRpc("viewport.image.push.quality")
    def setViewQuality(self, viewId, quality, ratio=1, updateLinkedView=True):
        sView = self.getView(viewId)
        if not sView:
            return { 'error': 'Unable to get view with id %s' % viewId }

        realViewId = sView.GetGlobalIDAsString()
        observerInfo = None
        if realViewId in self.trackingViews:
            observerInfo = self.trackingViews[realViewId]

        if not observerInfo:
            return { 'error': 'Unable to find subscription for view %s' % realViewId }

        observerInfo['quality'] = quality
        observerInfo['ratio'] = ratio

        # Handle linked view quality/ratio synch
        if updateLinkedView and realViewId in self.linkedViews:
          for vid in self.linkedViews:
            self.setViewQuality(vid, quality, ratio, False)

        # Update image size right now!
        if "originalSize" in self.trackingViews[realViewId]:
            size = [int(s * ratio) for s in self.trackingViews[realViewId]["originalSize"]]
            if 'SetSize' in sView:
                sView.SetSize(size)
            else:
                sView.ViewSize = size

        return { 'result': 'success' }


    @exportRpc("viewport.image.push.original.size")
    def setViewSize(self, viewId, width, height):
        sView = self.getView(viewId)
        if not sView:
            return { 'error': 'Unable to get view with id %s' % viewId }

        realViewId = sView.GetGlobalIDAsString()
        observerInfo = None
        if realViewId in self.trackingViews:
            observerInfo = self.trackingViews[realViewId]

        if not observerInfo:
            return { 'error': 'Unable to find subscription for view %s' % realViewId }

        observerInfo['originalSize'] = (int(width), int(height))

        return { 'result': 'success' }


    @exportRpc("viewport.image.push.enabled")
    def enableView(self, viewId, enabled):
        sView = self.getView(viewId)
        if not sView:
            return { 'error': 'Unable to get view with id %s' % viewId }

        realViewId = sView.GetGlobalIDAsString()
        observerInfo = None
        if realViewId in self.trackingViews:
            observerInfo = self.trackingViews[realViewId]

        if not observerInfo:
            return { 'error': 'Unable to find subscription for view %s' % realViewId }

        observerInfo['enabled'] = enabled

        return { 'result': 'success' }


    @exportRpc("viewport.image.push.invalidate.cache")
    def invalidateCache(self, viewId):
        sView = self.getView(viewId)
        if not sView:
            return { 'error': 'Unable to get view with id %s' % viewId }

        self.getApplication().InvalidateCache(sView.SMProxy)
        self.getApplication().InvokeEvent('UpdateEvent')
        return { 'result': 'success' }

    # -------------------------------------------------------------------------
    # View linked
    # -------------------------------------------------------------------------

    def validateViewLinks(self):
      for linkName in self.linkNames:
        simple.RemoveCameraLink(linkName)
      self.linkNames = []

      if len(self.linkedViews) > 1:
        viewList = [self.getView(vid) for vid in self.linkedViews]
        refView = viewList.pop(0)
        for view in viewList:
          linkName = '%s_%s' % (refView.GetGlobalIDAsString(), view.GetGlobalIDAsString())
          simple.AddCameraLink(refView, view, linkName)
          self.linkNames.append(linkName)

        # Synch camera state
        srcView = viewList[0]
        dstViews = viewList[1:]
        _pushCameraLink(srcView, dstViews)


    @exportRpc("viewport.view.link")
    def updateViewLink(self, viewId = None, linkState = False):
      if viewId:
        if linkState:
          self.linkedViews.append(viewId)
        else:
          try:
            self.linkedViews.remove(viewId)
          except:
            pass
        #self.validateViewLinks()

      if len(self.linkedViews) > 1:
        allViews = [self.getView(vid) for vid in self.linkedViews]
        _pushCameraLink(allViews[0], allViews[1:])

      if self.onLinkChange:
        self.onLinkChange(self.linkedViews)

      if linkState:
        self.getApplication().InvokeEvent('UpdateEvent')

      return self.linkedViews


    # -------------------------------------------------------------------------
    # Mouse handling
    # -------------------------------------------------------------------------

    @exportRpc("viewport.mouse.interaction")
    def mouseInteraction(self, event):
        """
        RPC Callback for mouse interactions.
        """
        view = self.getView(event['view'])

        if hasattr(view, 'UseInteractiveRenderingForScreenshots'):
            if event["action"] == 'down':
                view.UseInteractiveRenderingForScreenshots = 1
            elif event["action"] == 'up':
                view.UseInteractiveRenderingForScreenshots = 0

        buttons = 0
        if event["buttonLeft"]:
            buttons |= vtkWebInteractionEvent.LEFT_BUTTON
        if event["buttonMiddle"]:
            buttons |= vtkWebInteractionEvent.MIDDLE_BUTTON
        if event["buttonRight"]:
            buttons |= vtkWebInteractionEvent.RIGHT_BUTTON

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

        self.activeViewId = view.GetGlobalIDAsString()

        if event["action"] == 'down' and self.lastAction != event["action"]:
            self.getApplication().InvokeEvent('StartInteractionEvent')

        if event["action"] == 'up' and self.lastAction != event["action"]:
            self.getApplication().InvokeEvent('EndInteractionEvent')

        #if retVal :
        #  self.getApplication().InvokeEvent('UpdateEvent')

        if self.activeViewId in self.linkedViews:
          dstViews = [self.getView(vid) for vid in self.linkedViews]
          _pushCameraLink(view, dstViews)

        self.lastAction = event["action"]

        return retVal


    @exportRpc("viewport.mouse.zoom.wheel")
    def updateZoomFromWheel(self, event):
      if 'Start' in event["type"]:
        self.getApplication().InvokeEvent('StartInteractionEvent')

      viewProxy = self.getView(event['view'])
      if viewProxy and 'spinY' in event:
        rootId = viewProxy.GetGlobalIDAsString()
        zoomFactor = 1.0 - event['spinY'] / 10.0

        if rootId in self.linkedViews:
          fp = viewProxy.CameraFocalPoint
          pos = viewProxy.CameraPosition
          delta = [fp[i] - pos[i] for i in range(3)]
          viewProxy.GetActiveCamera().Zoom(zoomFactor)
          viewProxy.UpdatePropertyInformation()
          pos2 = viewProxy.CameraPosition
          viewProxy.CameraFocalPoint = [pos2[i] + delta[i] for i in range(3)]
          dstViews = [self.getView(vid) for vid in self.linkedViews]
          _pushCameraLink(viewProxy, dstViews)
        else:
          fp = viewProxy.CameraFocalPoint
          pos = viewProxy.CameraPosition
          delta = [fp[i] - pos[i] for i in range(3)]
          viewProxy.GetActiveCamera().Zoom(zoomFactor)
          viewProxy.UpdatePropertyInformation()
          pos2 = viewProxy.CameraPosition
          viewProxy.CameraFocalPoint = [pos2[i] + delta[i] for i in range(3)]

      if 'End' in event["type"]:
        self.getApplication().InvokeEvent('EndInteractionEvent')

# =============================================================================
#
# Provide Progress support
#
# =============================================================================

class ParaViewWebProgressUpdate(ParaViewWebProtocol):
    progressObserverTag = None

    def __init__(self, **kwargs):
        super(ParaViewWebProgressUpdate, self).__init__()
        self.listenToProgress()

    def listenToProgress(self):
        progressHandler = simple.servermanager.ActiveConnection.Session.GetProgressHandler()
        if not ParaViewWebProgressUpdate.progressObserverTag:
            ParaViewWebProgressUpdate.progressObserverTag = progressHandler.AddObserver(
                'ProgressEvent',
                lambda handler, event: self.updateProgress(handler))

    def updateProgress(self, caller):
        txt = caller.GetLastProgressText()
        progress = caller.GetLastProgress()
        self.publish('paraview.progress', { 'text': txt, 'progress': progress })

# =============================================================================
#
# Provide Geometry delivery mechanism (WebGL)
#
# =============================================================================

class ParaViewWebViewPortGeometryDelivery(ParaViewWebProtocol):

    def __init__(self, **kwargs):
        super(ParaViewWebViewPortGeometryDelivery, self).__init__()

        self.dataCache = {}

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

    # RpcName: getCachedWebGLData => viewport.webgl.cached.data
    @exportRpc("viewport.webgl.cached.data")
    def getCachedWebGLData(self, sha):
        if sha not in self.dataCache:
            return { 'success': False, 'reason': 'Key %s not in data cache' % sha }
        return { 'success': True, 'data': self.dataCache[sha] }

    # RpcName: getSceneMetaDataAllTimesteps => viewport.webgl.metadata.alltimesteps
    @exportRpc("viewport.webgl.metadata.alltimesteps")
    def getSceneMetaDataAllTimesteps(self, view_id=-1):
        animationScene = simple.GetAnimationScene()
        timeKeeper = animationScene.TimeKeeper
        tsVals = timeKeeper.TimestepValues.GetData()
        currentTime = timeKeeper.Time

        oldCache = self.dataCache
        self.dataCache = {}
        returnToClient = {}

        view  = self.getView(view_id);
        animationScene.GoToFirst()

        # Iterate over all the timesteps, building up a list of unique shas
        for i in xrange(len(tsVals)):
            simple.Render()

            mdString = self.getApplication().GetWebGLSceneMetaData(view.SMProxy)
            timestepMetaData = json.loads(mdString)
            objects = timestepMetaData['Objects']

            # Iterate over the objects in the scene
            for obj in objects:
                sha = obj['md5']
                objId = obj['id']
                numParts = obj['parts']

                if sha not in self.dataCache:
                    if sha in oldCache:
                        self.dataCache[sha] = oldCache[sha]
                    else:
                        transparency = obj['transparency']
                        layer = obj['layer']
                        partData = []

                        # Ask for the binary data for each part of this object
                        for part in xrange(numParts):
                            partNumber = part
                            data = self.getApplication().GetWebGLBinaryData(view.SMProxy, str(objId), partNumber)
                            partData.append(data)

                        # Now add object, with all its binary parts, to the cache
                        self.dataCache[sha] = { 'md5': sha,
                                                'id': objId,
                                                'numParts': numParts,
                                                'transparency': transparency,
                                                'layer': layer,
                                                'partsList': partData }

                returnToClient[sha] = { 'id': objId, 'numParts': numParts }

            # Now move time forward
            animationScene.GoToNext()

        # Set the time back to where it was when all timesteps were requested
        timeKeeper.Time = currentTime
        animationScene.AnimationTime = currentTime
        simple.Render()

        return { 'success': True, 'metaDataList': returnToClient }

# =============================================================================
#
# Provide an updated geometry delivery mechanism which better matches the
# client-side rendering capability we have in vtk.js
#
# =============================================================================

class ParaViewWebLocalRendering(ParaViewWebProtocol):
    def __init__(self, **kwargs):
        super(ParaViewWebLocalRendering, self).__init__()
        self.context = SynchronizationContext()
        self.trackingViews = {}
        self.mtime = 0

        initializeSerializers()

    # RpcName: getArray => viewport.geometry.array.get
    @exportRpc("viewport.geometry.array.get")
    def getArray(self, dataHash, binary = False):
        if binary:
            return self.addAttachment(self.context.getCachedDataArray(dataHash, binary))
        return self.context.getCachedDataArray(dataHash, binary)

    # RpcName: addViewObserver => viewport.geometry.view.observer.add
    @exportRpc("viewport.geometry.view.observer.add")
    def addViewObserver(self, viewId):
        sView = self.getView(viewId)
        if not sView:
            return { 'error': 'Unable to get view with id %s' % viewId }

        realViewId = sView.GetGlobalIDAsString()

        def pushGeometry(newSubscription=False):
            simple.Render(sView)
            stateToReturn = self.getViewState(realViewId, newSubscription)
            stateToReturn['mtime'] = 0 if newSubscription else self.mtime
            self.mtime += 1
            return stateToReturn

        if not realViewId in self.trackingViews:
            observerCallback = lambda *args, **kwargs: self.publish('viewport.geometry.view.subscription', pushGeometry())
            tag = self.getApplication().AddObserver('UpdateEvent', observerCallback)
            self.trackingViews[realViewId] = { 'tags': [tag], 'observerCount': 1 }
        else:
            # There is an observer on this view already
            self.trackingViews[realViewId]['observerCount'] += 1

        self.publish('viewport.geometry.view.subscription', pushGeometry(True))
        return { 'success': True, 'viewId': realViewId }

    # RpcName: removeViewObserver => viewport.geometry.view.observer.remove
    @exportRpc("viewport.geometry.view.observer.remove")
    def removeViewObserver(self, viewId):
        sView = self.getView(viewId)
        if not sView:
            return { 'error': 'Unable to get view with id %s' % viewId }

        realViewId = sView.GetGlobalIDAsString()

        observerInfo = None
        if realViewId in self.trackingViews:
            observerInfo = self.trackingViews[realViewId]

        if not observerInfo:
            return { 'error': 'Unable to find subscription for view %s' % realViewId }

        observerInfo['observerCount'] -= 1

        if observerInfo['observerCount'] <= 0:
            for tag in observerInfo['tags']:
                self.getApplication().RemoveObserver(tag)
            del self.trackingViews[realViewId]

        return { 'result': 'success' }

    # RpcName: getViewState => viewport.geometry.view.get.state
    @exportRpc("viewport.geometry.view.get.state")
    def getViewState(self, viewId, newSubscription=False):
        sView = self.getView(viewId)
        if not sView:
            return { 'error': 'Unable to get view with id %s' % viewId }

        self.context.setIgnoreLastDependencies(newSubscription)

        # Get the active view and render window, use it to iterate over renderers
        renderWindow = sView.GetRenderWindow()
        renderWindowId = sView.GetGlobalIDAsString()
        viewInstance = serializeInstance(None, renderWindow, renderWindowId, self.context, 1)
        viewInstance['extra'] = {
            'vtkRefId': getReferenceId(renderWindow),
            'centerOfRotation': sView.CenterOfRotation.GetData(),
            'camera': getReferenceId(sView.GetActiveCamera())
        }

        self.context.setIgnoreLastDependencies(False)
        self.context.checkForArraysToRelease()

        if viewInstance:
            return viewInstance

        return None

# =============================================================================
#
# Time management
#
# =============================================================================

class ParaViewWebTimeHandler(ParaViewWebProtocol):

    def __init__(self, **kwargs):
        super(ParaViewWebTimeHandler, self).__init__()
        # setup animation scene
        self.scene = simple.GetAnimationScene()
        simple.GetTimeTrack()
        self.scene.PlayMode = "Snap To TimeSteps"
        self.playing = False
        self.playTime = 0.1 # Time in second

    def nextPlay(self):
        self.updateTime('next')
        if self.playing:
            reactor.callLater(self.playTime, self.nextPlay)

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

        timestep = list(animationScene.TimeKeeper.TimestepValues).index(animationScene.TimeKeeper.Time)
        self.publish("pv.time.change", { 'time': float(view.ViewTime), 'timeStep': timestep } )

        self.getApplication().InvokeEvent('UpdateEvent')

        return view.ViewTime

    @exportRpc("pv.time.index.set")
    def setTimeStep(self, timeIdx):
        anim = simple.GetAnimationScene()
        anim.TimeKeeper.Time = anim.TimeKeeper.TimestepValues[timeIdx]

        self.getApplication().InvokeEvent('UpdateEvent')

        return anim.TimeKeeper.Time

    @exportRpc("pv.time.index.get")
    def getTimeStep(self):
        anim = simple.GetAnimationScene()
        try:
            return list(anim.TimeKeeper.TimestepValues).index(anim.TimeKeeper.Time)
        except:
            return 0

    @exportRpc("pv.time.value.set")
    def setTimeValue(self, t):
        anim = simple.GetAnimationScene()

        try:
            step = list(anim.TimeKeeper.TimestepValues).index(t)
            anim.TimeKeeper.Time = anim.TimeKeeper.TimestepValues[step]
            self.getApplication().InvokeEvent('UpdateEvent')
        except:
            print ('Try to update time with', t, 'but value not found in the list')

        return anim.TimeKeeper.Time

    @exportRpc("pv.time.value.get")
    def getTimeValue(self):
        anim = simple.GetAnimationScene()
        return anim.TimeKeeper.Time

    @exportRpc("pv.time.values")
    def getTimeValues(self):
        return list(simple.GetAnimationScene().TimeKeeper.TimestepValues)

    @exportRpc("pv.time.play")
    def play(self, deltaT=0.1):
        if not self.playing:
            self.playTime = deltaT
            self.playing = True
            self.getApplication().InvokeEvent('StartInteractionEvent')
            self.nextPlay()

    @exportRpc("pv.time.stop")
    def stop(self):
        self.getApplication().InvokeEvent('EndInteractionEvent')
        self.playing = False

# =============================================================================
#
# Color management
#
# =============================================================================

class ParaViewWebColorManager(ParaViewWebProtocol):

    def __init__(self, pathToColorMaps=None, showBuiltin=True, **kwargs):
        super(ParaViewWebColorManager, self).__init__()
        if pathToColorMaps:
            simple.ImportPresets(filename=pathToColorMaps)
        self.presets = servermanager.vtkSMTransferFunctionPresets.GetInstance()
        self.colorMapNames = []
        for i in range(self.presets.GetNumberOfPresets()):
            if showBuiltin or not self.presets.IsPresetBuiltin(i):
                self.colorMapNames.append(self.presets.GetPresetName(i))

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
        self.getApplication().InvokeEvent('UpdateEvent')

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

        self.getApplication().InvokeEvent('UpdateEvent')

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
            simple.ColorBy(repProxy, (arrayLocation, arrayName))
            if repProxy.LookupTable:
                lut = repProxy.LookupTable
                lut.VectorMode = str(vectorMode)
                lut.VectorComponent = int(vectorComponent)
            if rescale:
                repProxy.RescaleTransferFunctionToDataRange(rescale, False)

        self.updateScalarBars()

        simple.Render()
        self.getApplication().InvokeEvent('UpdateEvent')

    # RpcName: setOpacityFunctionPoints => pv.color.manager.opacity.points.set
    @exportRpc("pv.color.manager.opacity.points.set")
    def setOpacityFunctionPoints(self, arrayName, pointArray, enableOpacityMapping=False):
        lutProxy = simple.GetColorTransferFunction(arrayName)
        pwfProxy = simple.GetOpacityTransferFunction(arrayName)

        # Use whatever the current scalar range is for this array
        cMin = lutProxy.RGBPoints[0]
        cMax = lutProxy.RGBPoints[-4]

        # Scale and bias the x values, which come in between 0.0 and 1.0, to the
        # current scalar range
        for i in range(len(pointArray) // 4):
            idx = i * 4
            x = pointArray[idx]
            pointArray[idx] = (x * (cMax - cMin)) + cMin

        # Set the Points property to scaled and biased points array
        pwfProxy.Points = pointArray

        lutProxy.EnableOpacityMapping = enableOpacityMapping

        simple.Render()
        self.getApplication().InvokeEvent('UpdateEvent')

    # RpcName: getOpacityFunctionPoints => pv.color.manager.opacity.points.get
    @exportRpc("pv.color.manager.opacity.points.get")
    def getOpacityFunctionPoints(self, arrayName):
        result = []
        lutProxy = simple.GetColorTransferFunction(arrayName)
        pwfProxy = simple.GetOpacityTransferFunction(arrayName)

        # Use whatever the current scalar range is for this array
        cMin = lutProxy.RGBPoints[0]
        cMax = lutProxy.RGBPoints[-4]
        pointArray = pwfProxy.Points

        # Scale and bias the x values, which come in between 0.0 and 1.0, to the
        # current scalar range
        for i in range(len(pointArray) // 4):
            idx = i * 4
            result.append({
                'x': (pointArray[idx] - cMin) / (cMax - cMin),
                'y': pointArray[idx + 1],
                'x2': pointArray[idx + 2],
                'y2': pointArray[idx + 3],
            });
        return result

    # RpcName: getRgbPoints => pv.color.manager.rgb.points.get
    @exportRpc("pv.color.manager.rgb.points.get")
    def getRgbPoints(self, arrayName):

        lutProxy = simple.GetColorTransferFunction(arrayName)

        # First, set the current coloring mode
        colorInfo = {}
        colorInfo['mode'] = 'categorical' if lutProxy.InterpretValuesAsCategories else 'continuous'

        # Now build up the continuous coloring information
        continuousInfo = {}

        l = lutProxy.RGBPoints.GetData()
        scalars = l[0:len(l):4]
        continuousInfo['scalars'] = [ float(s) for s in scalars ]

        reds = l[1:len(l):4]
        greens = l[2:len(l):4]
        blues = l[3:len(l):4]
        continuousInfo['colors'] = [ list(a) for a in zip(reds, greens, blues) ]

        colorInfo['continuous'] = continuousInfo

        # Finally, build up the categorical coloring information
        categoricalInfo = {}

        rgbs = lutProxy.IndexedColors
        reds = rgbs[0:len(rgbs):3]
        greens = rgbs[1:len(rgbs):3]
        blues = rgbs[2:len(rgbs):3]
        categoricalInfo['colors'] = [ list(a) for a in zip(reds, greens, blues) ]

        l = lutProxy.Annotations
        scalars = l[0:len(l):2]
        categoricalInfo['scalars'] = [ float(s) for s in scalars ]
        categoricalInfo['annotations'] = l[1:len(l):2]

        ### If the numbers of categorical colors and scalars do not match,
        ### then this is probably because you already had a categorical color
        ### scheme set up when you applied a new indexed preset.
        numColors = len(categoricalInfo['colors'])
        numScalars = len(categoricalInfo['scalars'])
        if numColors != numScalars:
            # More colors than scalars means we applied a preset colormap with
            # more colors than what we had set up already, so I want to add
            # scalars and annotations.
            if numColors > numScalars:
                nextValue = 0

                # Another special case here is that there were no categorical
                # scalars/colors before the preset was applied, and we will need
                # to insert a single space character (' ') annotation at the start
                # of the list to make the scalarbar appear.
                if numScalars == 0:
                    categoricalInfo['scalars'].append(nextValue)
                    categoricalInfo['annotations'].append(' ')

                nextValue = categoricalInfo['scalars'][-1] + 1

                for i in xrange((numColors - numScalars) - 1):
                    categoricalInfo['scalars'].append(nextValue)
                    categoricalInfo['annotations'].append('')
                    nextValue += 1

            # More scalars than colors means we applied a preset colormap with
            # *fewer* colors than what had already, so I want to add fake colors
            # for any non-empty annotations I have after the end of the color list
            elif numScalars > numColors:
                newScalars = categoricalInfo['scalars'][0:numColors]
                newAnnotations = categoricalInfo['annotations'][0:numColors]

                for i in xrange(numColors, numScalars):
                    if categoricalInfo['annotations'][i] != '':
                        newScalars.append(categoricalInfo['scalars'][i])
                        newAnnotations.append(categoricalInfo['annotations'][i])
                        categoricalInfo['colors'].append([0.5, 0.5, 0.5])

                categoricalInfo['scalars'] = newScalars
                categoricalInfo['annotations'] = newAnnotations

            # Now that we have made the number of indexed colors and associated
            # scalars match, the final step in this special case is to apply the
            # matched up props we just computed so that server and client ui are
            # in sync.
            idxColorsProperty = []
            annotationsProperty = []

            for aIdx in xrange(len(categoricalInfo['scalars'])):
                annotationsProperty.append(str(categoricalInfo['scalars'][aIdx]))
                annotationsProperty.append(str(categoricalInfo['annotations'][aIdx]))
                idxColorsProperty.extend(categoricalInfo['colors'][aIdx])

            lutProxy.Annotations = annotationsProperty
            lutProxy.IndexedColors = idxColorsProperty

            simple.Render

        colorInfo['categorical'] = categoricalInfo

        return colorInfo

    # RpcName: setRgbPoints => pv.color.manager.rgb.points.set
    @exportRpc("pv.color.manager.rgb.points.set")
    def setRgbPoints(self, arrayName, rgbInfo):
        lutProxy = simple.GetColorTransferFunction(arrayName)

        colorMode = rgbInfo['mode']
        continuousInfo = rgbInfo['continuous']
        categoricalInfo = rgbInfo['categorical']

        # First make sure the continuous mode properties are set
        continuousScalars = continuousInfo['scalars']
        continuousColors = continuousInfo['colors']

        rgbPoints = []
        for idx in xrange(len(continuousScalars)):
            scalar = continuousScalars[idx]
            rgb = continuousColors[idx]
            rgbPoints.append(float(scalar))
            rgbPoints.extend(rgb)
        lutProxy.RGBPoints = rgbPoints

        # Now make sure the categorical mode properties are set
        annotations = categoricalInfo['annotations']
        categoricalScalars = categoricalInfo['scalars']
        categoricalColors = categoricalInfo['colors']

        annotationsProperty = []
        idxColorsProperty = []
        for aIdx in xrange(len(annotations)):
            annotationsProperty.append(str(categoricalScalars[aIdx]))
            annotationsProperty.append(str(annotations[aIdx]))
            idxColorsProperty.extend(categoricalColors[aIdx])

        lutProxy.Annotations = annotationsProperty
        lutProxy.IndexedColors = idxColorsProperty

        # Finally, set the coloring mode property
        if colorMode == 'continuous':      # continuous coloring
            lutProxy.InterpretValuesAsCategories = 0
        else:                                    # categorical coloring
            lutProxy.InterpretValuesAsCategories = 1

        simple.Render();
        self.getApplication().InvokeEvent('UpdateEvent')

    # RpcName: getLutImage => pv.color.manager.lut.image.get
    @exportRpc("pv.color.manager.lut.image.get")
    def getLutImage(self, representation, numSamples, customRange=None):
        repProxy = self.mapIdToProxy(representation)
        lut = repProxy.LookupTable.GetClientSideObject()

        dataRange = customRange
        if not dataRange:
            dataRange = lut.GetRange()

        delta = (dataRange[1] - dataRange[0]) / float(numSamples)

        colorArray = vtkUnsignedCharArray()
        colorArray.SetNumberOfComponents(3)
        colorArray.SetNumberOfTuples(numSamples)

        rgb = [ 0, 0, 0 ]
        for i in range(numSamples):
            lut.GetColor(dataRange[0] + float(i) * delta, rgb)
            r = int(round(rgb[0] * 255))
            g = int(round(rgb[1] * 255))
            b = int(round(rgb[2] * 255))
            colorArray.SetTuple3(i, r, g, b)

        # Add the color array to an image data
        imgData = vtkImageData()
        imgData.SetDimensions(numSamples, 1, 1)
        aIdx = imgData.GetPointData().SetScalars(colorArray)

        # Use the vtk data encoder to base-64 encode the image as png, using no compression
        encoder = vtkDataEncoder()
        # two calls in a row crash on Windows - bald timing hack to avoid the crash.
        time.sleep(0.01);
        b64Str = encoder.EncodeAsBase64Png(imgData, 0)

        return { 'range': dataRange, 'image': b64Str }

    @exportRpc("pv.color.manager.lut.image.all")
    def getLutImages(self, numSamples):
        colorArray = vtkUnsignedCharArray()
        colorArray.SetNumberOfComponents(3)
        colorArray.SetNumberOfTuples(numSamples)

        pxm = simple.servermanager.ProxyManager()
        lutProxy = pxm.NewProxy('lookup_tables', 'PVLookupTable')
        lut = lutProxy.GetClientSideObject()
        dataRange = lut.GetRange()
        delta = (dataRange[1] - dataRange[0]) / float(numSamples)

        # Add the color array to an image data
        imgData = vtkImageData()
        imgData.SetDimensions(numSamples, 1, 1)
        imgData.GetPointData().SetScalars(colorArray)

        # Use the vtk data encoder to base-64 encode the image as png, using no compression
        encoder = vtkDataEncoder()

        # Result container
        result = {}

        # Loop over all presets
        for name in self.colorMapNames:
            lutProxy.ApplyPreset(name, True)
            rgb = [ 0, 0, 0 ]
            for i in range(numSamples):
                lut.GetColor(dataRange[0] + float(i) * delta, rgb)
                r = int(round(rgb[0] * 255))
                g = int(round(rgb[1] * 255))
                b = int(round(rgb[2] * 255))
                colorArray.SetTuple3(i, r, g, b)

            result[name] = encoder.EncodeAsBase64Png(imgData, 0)

        simple.Delete(lutProxy)

        return result

    # RpcName: setSurfaceOpacity => pv.color.manager.surface.opacity.set
    @exportRpc("pv.color.manager.surface.opacity.set")
    def setSurfaceOpacity(self, representation, enabled):
        repProxy = self.mapIdToProxy(representation)
        lutProxy = repProxy.LookupTable

        lutProxy.EnableOpacityMapping = enabled

        simple.Render()
        self.getApplication().InvokeEvent('UpdateEvent')

    # RpcName: getSurfaceOpacity => pv.color.manager.surface.opacity.get
    @exportRpc("pv.color.manager.surface.opacity.get")
    def getSurfaceOpacity(self, representation):
        repProxy = self.mapIdToProxy(representation)
        lutProxy = repProxy.LookupTable

        return lutProxy.EnableOpacityMapping

    # RpcName: setSurfaceOpacityByArray => pv.color.manager.surface.opacity.by.array.set
    @exportRpc("pv.color.manager.surface.opacity.by.array.set")
    def setSurfaceOpacityByArray(self, arrayName, enabled):
        lutProxy = simple.GetColorTransferFunction(arrayName)

        lutProxy.EnableOpacityMapping = enabled

        simple.Render()
        self.getApplication().InvokeEvent('UpdateEvent')

    # RpcName: getSurfaceOpacityByArray => pv.color.manager.surface.opacity.by.array.get
    @exportRpc("pv.color.manager.surface.opacity.by.array.get")
    def getSurfaceOpacityByArray(self, arrayName):
        lutProxy = simple.GetColorTransferFunction(arrayName)
        return lutProxy.EnableOpacityMapping

    # RpcName: selectColorMap => pv.color.manager.select.preset
    @exportRpc("pv.color.manager.select.preset")
    def selectColorMap(self, representation, paletteName):
        """
        Choose the color map preset to use when coloring by an array.
        """
        repProxy = self.mapIdToProxy(representation)
        lutProxy = repProxy.LookupTable

        if lutProxy is not None:
            lutProxy.ApplyPreset(paletteName, True)
            simple.Render()
            self.getApplication().InvokeEvent('UpdateEvent')
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

    def __init__(self, allowedProxiesFile=None, baseDir=None, fileToLoad=None, allowUnconfiguredReaders=True, groupProxyEditorWidgets=True, respectPropertyGroups=True, **kwargs):
        """
        basePath: specify the base directory (or directories) that we should start with, if this
        parameter takes the form: "name1=path1|name2=path2|...", then we will treat this as the
        case where multiple data directories are required.  In this case, each top-level directory
        will be given the name associated with the directory in the argument.
        """
        super(ParaViewWebProxyManager, self).__init__()
        self.debugMode = False
        self.groupProxyEditorPropertyWidget = groupProxyEditorWidgets
        self.respectPropertyGroups = respectPropertyGroups
        self.proxyDefinitionCache = {}
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
        self.alwaysIncludeProperties = [ 'GridAxesVisibility' ]
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
                                 'vtkSMInputProperty': 'proxy',
                                 'vtkSMDoubleMapProperty': 'map' }
        self.allowedProxies = {}
        self.hintsMap = { 'PropertyWidgetDecorator': { 'ClipScalarsDecorator': clipScalarDecorator,
                                                       'GenericDecorator': genericDecorator },
                          'Widget': { 'multi_line': multiLineDecorator },
                          'ProxyEditorPropertyWidget': { 'default': proxyEditorPropertyWidgetDecorator } }

        self.setBaseDirectory(baseDir)
        self.allowUnconfiguredReaders = allowUnconfiguredReaders

        # If there was a proxy list file, use it, otherwise use the default
        if allowedProxiesFile:
            self.readAllowedProxies(allowedProxiesFile)
        else:
            from ._default_proxies import getDefaultProxies
            self.readAllowedProxies(getDefaultProxies())

        if fileToLoad:
            if '*' in fileToLoad:
                fullBasePathForGroup = os.path.dirname(self.getAbsolutePath(fileToLoad))
                fileNamePattern = os.path.basename(fileToLoad)
                groupToLoad = []

                for fileName in os.listdir(fullBasePathForGroup):
                    if fnmatch.fnmatch(fileName, fileNamePattern):
                        groupToLoad.append(os.path.join(fullBasePathForGroup, fileName))

                groupToLoad.sort(key=alphanum_key)
                self.open(groupToLoad)
            else:
                self.open(fileToLoad)

        self.simpleTypes = [int, float, list, str]
        self.view = simple.GetRenderView()
        simple.SetActiveView(self.view)
        simple.Render()

    #--------------------------------------------------------------------------
    # Read the configured proxies json file into a dictionary and process.
    #--------------------------------------------------------------------------
    def readAllowedProxies(self, filepathOrConfig):
        self.availableList = {}
        self.allowedProxies = {}
        configurationData = {}
        if type(filepathOrConfig) == dict:
            configurationData = filepathOrConfig
        else:
            with open(filepathOrConfig, 'r') as fd:
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
            useDirectory = False
            if 'method' in config:
                readerMethod = config['method']
            if 'useDirectory' in config:
                useDirectory = config['useDirectory']
            for ext in config['extensions']:
                self.readerFactoryMap[ext] = [ readerName, readerMethod, useDirectory ]

    #--------------------------------------------------------------------------
    # Convenience method to get proxy defs, cached if available
    #--------------------------------------------------------------------------
    def getProxyDefinition(self, group, name):
        cacheKey = '%s:%s' % (group, name)
        if cacheKey in self.proxyDefinitionCache:
            return self.proxyDefinitionCache[cacheKey]

        xmlElement = servermanager.ActiveConnection.Session.GetProxyDefinitionManager().GetCollapsedProxyDefinition(group, name, None)
        # print('\n\n\n (%s, %s): \n\n' % (group, name))
        # xmlElement.PrintXML()
        self.proxyDefinitionCache[cacheKey] = xmlElement
        return xmlElement

    #--------------------------------------------------------------------------
    # Look higher up in XML hierarchy for attributes on a property (useful if
    # a property is an exposed property).  From the documentation of vtkSMProperty,
    # the GetParent method will access the sub-proxy to which the property
    # actually belongs, if that is the case.
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
                print ('ERROR: unable to get property parent for property ' + property.Name)
                return {}

        xmlElement = self.getProxyDefinition(proxy.GetXMLGroup(), proxy.GetXMLName())
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
    def getProxyListFromProperty(self, proxy, proxyPropElement, inPropGroup, group, parentGroup, detailsKey):
        propPropName = proxyPropElement.GetAttribute('name')
        propVisibility = proxyPropElement.GetAttribute('panel_visibility')
        propInstance = proxy.GetProperty(propPropName)
        nbChildren = proxyPropElement.GetNumberOfNestedElements()
        foundPLDChild = False
        subProxiesToProcess = []

        domain = propInstance.FindDomain("vtkSMProxyListDomain")

        if domain:
            foundPLDChild = True
            for j in range(domain.GetNumberOfProxies()):
                subProxy = domain.GetProxy(j)
                pelt = self.getProxyDefinition(subProxy.GetXMLGroup(), subProxy.GetXMLName())
                subProxiesToProcess.append([subProxy, pelt])

        if len(subProxiesToProcess) > 0:
            newGroup = group
            newParentGroup = parentGroup
            if self.groupProxyEditorPropertyWidget:
                hintChild = proxyPropElement.FindNestedElementByName('Hints')
                if hintChild:
                    pepwChild = hintChild.FindNestedElementByName('ProxyEditorPropertyWidget')
                    if pepwChild:
                        newGroup = '%s:%s' % (proxy.GetGlobalIDAsString(), propPropName)
                        self.groupDetailsMap[newGroup] = {
                            'groupName': propPropName,
                            'groupWidget': 'ProxyEditorPropertyWidget',
                            'groupVisibilityProperty': pepwChild.GetAttribute('property'),
                            'groupVisibility': propVisibility if propVisibility is not None else 'default',
                            'groupType': 'ProxyEditorPropertyWidget'
                        }
                        newParentGroup = group
                        self.propertyDetailsMap[detailsKey]['group'] = newGroup
                        self.propertyDetailsMap[detailsKey]['parentGroup'] = newParentGroup
            for sub in subProxiesToProcess:
                self.processXmlElement(sub[0], sub[1], inPropGroup, newGroup, newParentGroup, detailsKey)

        return foundPLDChild

    #--------------------------------------------------------------------------
    # Decide whether to update our internal map of property details, or just the
    # ordered list of properties, or both.
    #
    # detailsKey  => uniquely identifies a property via concatenation of proxy
    #                id and property name, e.g. '476:Opacity'
    # name        => property name
    # panelVis    => value of 'panel_visibilty' attribute on element
    # size        => number of elements to set for this property
    # inPropGroup => whether or not this property is inside a 'PropertyGroup' element
    # group       => name of group this property belongs to (could be other than 'root',
    #                even if 'inPropGroup' is false, due to the grouping we impose
    #                when we encounter a ProxyProperty with Hints: 'ProxyEditorPropertyWidget')
    # parentGroup => name of parent group (could be None for root level property)
    #--------------------------------------------------------------------------
    def trackProperty(self, detailsKey, name, panelVis, panelVisQualifier, size, inPropGroup, group, parentGroup, belongsToProxyProperty=None):
        if detailsKey not in self.propertyDetailsMap:
            self.propertyDetailsMap[detailsKey] = {
                'type': name,
                'panelVis': panelVis,
                'panelVisQualifier': panelVisQualifier,
                'size': size,
                'group': group,
                'parentGroup': parentGroup
            }

        if belongsToProxyProperty:
            if belongsToProxyProperty not in self.proxyPropertyDependents:
                self.proxyPropertyDependents[belongsToProxyProperty] = []
            self.proxyPropertyDependents[belongsToProxyProperty].append(detailsKey)

            # If this property belongs to a proxy property, and if that property is marked
            # as advanced, then this property should be marked advanced too.
            if self.propertyDetailsMap[belongsToProxyProperty]['panelVis'] == 'advanced':
                self.propertyDetailsMap[detailsKey]['panelVis'] = 'advanced'


        if inPropGroup:
            self.propertyDetailsMap[detailsKey]['group'] = group
            self.propertyDetailsMap[detailsKey]['parentGroup'] = parentGroup
            if panelVis is not None:
                self.propertyDetailsMap[detailsKey]['panelVis'] = panelVis
            if panelVisQualifier is not None:
                self.propertyDetailsMap[detailsKey]['panelVisQualifier'] = panelVisQualifier
        else:
            if panelVis is not None and self.propertyDetailsMap[detailsKey]['panelVis'] is None:
                self.propertyDetailsMap[detailsKey]['panelVis'] = panelVis
            if panelVisQualifier is not None and self.propertyDetailsMap[detailsKey]['panelVisQualifier'] is None:
                self.propertyDetailsMap[detailsKey]['panelVisQualifier'] = panelVisQualifier

        if detailsKey not in self.orderedNameList:
            self.orderedNameList.append(detailsKey)
        else:
            # Do already have property in ordered list, but because we're in a
            # PropertyGroup, we want to override the previous order
            if inPropGroup:
                idx = self.orderedNameList.index(detailsKey)
                self.orderedNameList.pop(idx)
                self.orderedNameList.append(detailsKey)

                # Additionally, if the property we are now re-ordering is a ProxyProperty, then
                # we also need to grab any properties of the sub proxies of it's proxy list domain
                # and move all of those down as well
                if detailsKey in self.proxyPropertyDependents:
                    subProxiesProps = self.proxyPropertyDependents[detailsKey]
                    for subProxyPropKey in subProxiesProps:
                        idx = self.orderedNameList.index(subProxyPropKey)
                        self.orderedNameList.pop(idx)
                        self.orderedNameList.append(subProxyPropKey)

    #--------------------------------------------------------------------------
    # Gather information from the xml associated with a proxy and properties.
    #--------------------------------------------------------------------------
    def processXmlElement(self, proxy, xmlElement, inPropGroup, group, parentGroup, belongsToProxyProperty=None):
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

            panelVisQualifier = None
            if self.proxyIsRepresentation:
                panelVisQualifier = xmlChild.GetAttribute('panel_visibility_default_for_representation')

            size = -1
            informationOnly = 0
            internal = 0

            # Check for attributes that might only exist on parent xml
            parentQueryList = []
            if numElts is None:
                parentQueryList.append('number_of_elements')
            else:
                size = int(numElts)

            if infoOnly is None:
                parentQueryList.append('information_only')
            else:
                informationOnly = int(infoOnly)

            if isInternal is None:
                parentQueryList.append('is_internal')
            else:
                internal = int(isInternal)

            if panelVis is None:
                parentQueryList.append('panel_visibility')

            if self.proxyIsRepresentation:
                if panelVisQualifier is None:
                    parentQueryList.append('panel_visibility_default_for_representation')

            # Try to retrieve those attributes from parent xml (implicit assumption is this is a property
            # which actually belongs to a subproxy)
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
            if 'panel_visibility' in parentAttrs:
                panelVis = parentAttrs['panel_visibility']
            if 'panel_visibility_default_for_representation' in parentAttrs:
                panelVisQualifier = parentAttrs['panel_visibility_default_for_representation']

            # Now decide whether we should filter out this property
            if (((name.endswith('Property') and panelVis == 'never') or informationOnly == 1 or internal == 1) and nameAttr not in self.alwaysIncludeProperties) or nameAttr in self.alwaysExcludeProperties:
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
                self.trackProperty(detailsKey, name, panelVis, panelVisQualifier, size, inPropGroup, group, parentGroup, belongsToProxyProperty)
                foundProxyListDomain = self.getProxyListFromProperty(proxy, xmlChild, inPropGroup, group, parentGroup, detailsKey)
                if foundProxyListDomain == True:
                    self.propertyDetailsMap[detailsKey]['size'] = 1
            elif name.endswith('Property'):
                self.trackProperty(detailsKey, name, panelVis, panelVisQualifier, size, inPropGroup, group, parentGroup, belongsToProxyProperty)
            else:
                # Anything else, we recursively process the element, with special handling
                # in the case that the element is a PropertyGroup
                if name == 'PropertyGroup':
                    newGroup = group
                    newParentGroup = parentGroup
                    if self.respectPropertyGroups:
                        typeAttr = xmlChild.GetAttribute('type')
                        labelAttr = xmlChild.GetAttribute('label')
                        panelWidgetAttr = None
                        if not labelAttr:
                            panelWidgetAttr = xmlChild.GetAttribute('panel_widget')
                            labelAttr = panelWidgetAttr
                            if not labelAttr:
                                labelAttr = 'Unlabeled Property Group (%s)' % proxyId
                        newGroup = '%s:%s' % (proxyId, labelAttr)
                        self.groupDetailsMap[newGroup] = {
                            'groupName': labelAttr,
                            'groupWidget': 'PropertyGroup',
                            'groupVisibility': panelVis,
                            'groupType': typeAttr
                        }
                        newParentGroup = group
                    self.processXmlElement(proxy, xmlChild, True, newGroup, newParentGroup, belongsToProxyProperty)
                else:
                    self.processXmlElement(proxy, xmlChild, inPropGroup, group, parentGroup, belongsToProxyProperty)

    #--------------------------------------------------------------------------
    # Entry point for the xml processing methods.
    #--------------------------------------------------------------------------
    def getProxyXmlDefinitions(self, proxy):
        self.orderedNameList = []
        self.propertyDetailsMap = {}
        self.groupDetailsMap = {}
        self.proxyPropertyDependents = {}
        self.proxyIsRepresentation = proxy.GetXMLGroup() == 'representations'
        self.groupDetailsMap['root'] = {
            'groupName': 'root'
        }

        xmlElement = self.getProxyDefinition(proxy.GetXMLGroup(), proxy.GetXMLName())
        self.processXmlElement(proxy, xmlElement, False, 'root', None)

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
                prop = proxy.GetProperty(property)
                propertyName = prop.Name
                displayName = prop.GetXMLLabel()
                if propertyName in ["Refresh", "Input"] or propertyName.__contains__("Info"):
                    continue

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
                        propJson['value'] = prop.GetData()
                    except AttributeError as attrErr:
                        self.debug('Property ' + propertyName + ' has no GetData() method, skipping')
                        continue

                    # One exception is properties which have enumeration domain, in which case we substitute
                    # the numeric value for the enum text value.
                    enumDomain = prop.FindDomain('vtkSMEnumerationDomain')
                    if enumDomain:
                        for entryNum in range(enumDomain.GetNumberOfEntries()):
                            if enumDomain.GetEntryText(entryNum) == propJson['value']:
                                propJson['value'] = enumDomain.GetEntryValue(entryNum)
                                break

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
        groupsInfo = []
        for name in self.orderedNameList:
            if name in propMap:
                orderedList.append(propMap[name])
                xmlDetails = self.propertyDetailsMap[name]
                groupsInfo.append({ 'group': xmlDetails['group'], 'parentGroup': xmlDetails['parentGroup'] })

        return orderedList, groupsInfo

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
                        listdomain = prop.FindDomain("vtkSMProxyListDomain")
                        if listdomain:
                            for i in xrange(listdomain.GetNumberOfProxies()):
                                internal_proxy = listdomain.GetProxy(i)
                                self.applyDomains(parentProxy, internal_proxy.GetGlobalIDAsString())
                except:
                    exc_type, exc_obj, exc_tb = sys.exc_info()
                    print ("Unexpected error:", exc_type, " line: " , exc_tb.tb_lineno)

        # Reset all properties to leverage domain capabilities
        for prop_name in proxy.ListProperties():
            if prop_name == 'Input':
                continue
            try:
                skipReset = False
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
                        skipReset = True
                        print ('Caught exception setting domain values in apply_domains:')
                        print (attrErr)

                if not skipReset:
                    prop.ResetToDefault()

                # Need to UnRegister to handle the ref count from the NewDomainIterator
                iter.UnRegister(None)
            except:
                exc_type, exc_obj, exc_tb = sys.exc_info()
                print ("Unexpected error:", exc_type, " line: " , exc_tb.tb_lineno)

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
                rangeList.append({ 'name': '', 'min': magRange[0], 'max': magRange[1] })

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
        timeKeeper = next(iter(servermanager.ProxyManager().GetProxiesInGroup("timekeeper").values()))
        tsVals = timeKeeper.TimestepValues
        if tsVals:
            if isinstance(tsVals, float):
                tsVals = [ tsVals ]
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

        if self.proxyIsRepresentation:
            reprProxy = self.mapIdToProxy(proxyId)
            reprProp = reprProxy.GetProperty('Representation')
            reprValue = reprProp.GetData()

        uiProps = []

        for proxyProp in proxyProperties:
            uiElt = {}

            pid = proxyProp['id']
            propertyName = proxyProp['name']
            proxy = self.mapIdToProxy(pid)
            prop = proxy.GetProperty(propertyName)

            # Get the xml details we already parsed out for this property
            xmlProps = self.propertyDetailsMap[pid + ':' + propertyName]

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
                if self.proxyIsRepresentation and 'panelVisQualifier' in xmlProps and xmlProps['panelVisQualifier'] and xmlProps['panelVisQualifier'].lower() == reprValue.lower():
                    uiElt['advanced'] = 0
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
                        hmapKey = None
                        if hintType and hintType in hmap:
                            hmapKey = hintType
                        elif 'default' in hmap:
                            hmapKey = 'default'

                        if hmapKey:
                            # We're interested in decorating based on this hint
                            hintFunction = hmap[hmapKey]
                            hintFunction(prop, uiElt, hint)

            uiProps.append(uiElt)

        return uiProps

    #--------------------------------------------------------------------------
    # Helper function to restructure the properties so they reflect the grouping
    # encountered when processing the xml.
    #--------------------------------------------------------------------------
    def restructureProperties(self, groupList, propList, uiList):
        props = []
        uis = [] if uiList else None
        groupMap = { 'root': { 'proxy': props, 'ui': uis } }

        for idx in range(len(groupList)):
            groupInfo = groupList[idx]
            group = groupInfo['group']
            parentGroup = groupInfo['parentGroup']
            if group != 'root' and self.groupDetailsMap[group]['groupVisibility'] == 'never':
                self.debug('Culling property (%s) in group (%s) because group has visibility never' % (propList[idx]['name'], group))
                continue
            if not group in groupMap:
                groupMap[group] = { 'proxy': [], 'ui': [] if uiList else None }
                groupMap[parentGroup]['proxy'].append({
                    'id': group,
                    'name': self.groupDetailsMap[group]['groupName'],
                    'value': False,
                    'children': groupMap[group]['proxy']
                })
                if uiList:
                    groupMap[parentGroup]['ui'].append({
                        'widget': self.groupDetailsMap[group]['groupWidget'],
                        'name': self.groupDetailsMap[group]['groupName'],
                        'type': self.groupDetailsMap[group]['groupType'],
                        'advanced': 1 if self.groupDetailsMap[group]['groupVisibility'] == 'advanced' else 0,
                        'children': groupMap[group]['ui']
                    })
                    # Make sure we can find the group ui item we just appended
                    groupMap[group]['groupItem'] = groupMap[parentGroup]['ui'][-1]

            pushToFront = False
            if uiList:
                if 'groupVisibilityProperty' in self.groupDetailsMap[group] and propList[idx]['name'] == self.groupDetailsMap[group]['groupVisibilityProperty']:
                    # If a proxy property claimed this property as its visibilty property, we should not
                    # allow it to remain marked as 'advanced'.
                    uiList[idx]['advanced'] = 0
                    pushToFront = True

                if pushToFront:
                    groupMap[group]['ui'].insert(0, uiList[idx])
                else:
                    groupMap[group]['ui'].append(uiList[idx])

            if pushToFront:
                groupMap[group]['proxy'].insert(0, propList[idx])
            else:
                groupMap[group]['proxy'].append(propList[idx])

        # Before we're done we need to check for groups of properties where every
        # property in the group has a panel_visibilty of 'advanced', in which
        # case, the group should be marked the same.
        if uiList:
            for groupName in groupMap:
                if groupName != 'root' and 'ui' in groupMap[groupName]:
                    groupUiList = groupMap[groupName]['ui']
                    firstUiElt = groupUiList[0]
                    groupDependency = False
                    if 'depends' in firstUiElt and firstUiElt['depends']:
                        groupDependency = True
                    advancedGroup = True
                    for uiProp in groupUiList:
                        if uiProp['advanced'] == 0:
                            advancedGroup = False
                        if groupDependency and ('depends' not in uiProp or uiProp['depends'] != firstUiElt['depends']):
                            groupDependency = False

                    if advancedGroup:
                        groupMap[groupName]['groupItem']['advanced'] = 1

                    if groupDependency:
                        groupMap[groupName]['groupItem']['depends'] = firstUiElt['depends']

        return { 'proxy': props, 'ui': uis }

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
    def create(self, functionName, parentId, initialValues = {}, skipDomain = False, subProxyValues = {}):
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
        sanitizedInitialValues = sanitizeKeys(initialValues)
        allowed = self.allowedProxies[name]
        newProxy = paraview.simple.__dict__[allowed](**sanitizedInitialValues)

        # Update subproxy values
        if newProxy:
            for subProxyName in subProxyValues:
                subProxy = newProxy.GetProperty(subProxyName).GetData()
                if subProxy:
                    for propName in subProxyValues[subProxyName]:
                        prop = subProxy.SMProxy.GetProperty(propName)
                        value = subProxyValues[subProxyName][propName]
                        if isinstance(value, list):
                            prop.SetElements(value)
                        else:
                            prop.SetElements([value])
                    subProxy.UpdateVTKObjects()

        # To make WebGL export work
        simple.Show()
        simple.Render()
        self.getApplication().InvokeEvent('UpdateEvent')

        try:
            if not skipDomain:
                self.applyDomains(parentProxy, newProxy.GetGlobalIDAsString())
        except Exception as inst:
            print ('Caught exception applying domains:')
            print (inst)

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
                validPath = self.getAbsolutePath(file)
                if validPath:
                    fileToLoad.append(validPath)
        else:
            validPath = self.getAbsolutePath(relativePath)
            if validPath:
                fileToLoad.append(validPath)

        if len(fileToLoad) == 0:
            return { 'success': False, 'reason': 'No valid path name ' + str(relativePath) }

        # Get file extension and look for configured reader
        idx = fileToLoad[0].rfind('.')
        extension = fileToLoad[0][idx+1:]

        # Check if we were asked to load a state file
        if extension == 'pvsm':
            simple.LoadState(fileToLoad[0])
            newView = simple.Render()
            simple.SetActiveView(newView)
            if self.getApplication():
                self.getApplication().InvokeEvent('ResetActiveView')
                self.getApplication().InvokeEvent('UpdateEvent')

            return { 'success': True, 'view': newView.GetGlobalIDAsString() }

        readerName = None
        if extension in self.readerFactoryMap:
            readerName = self.readerFactoryMap[extension][0]
            customMethod = self.readerFactoryMap[extension][1]
            useDirectory = self.readerFactoryMap[extension][2]

        # Open the file(s)
        reader = None
        if readerName:
            if useDirectory:
                kw = { customMethod: os.path.dirname(fileToLoad[0]) }
            else:
                kw = { customMethod: fileToLoad }
            reader = paraview.simple.__dict__[readerName](**kw)
        else:
            if self.allowUnconfiguredReaders:
                reader = simple.OpenDataFile(fileToLoad)
            else:
                return { 'success': False,
                         'reason': 'No configured reader found for ' + extension + ' files, and unconfigured readers are not enabled.' }

        # Rename the reader proxy
        name = fileToLoad[0].split(os.path.sep)[-1]
        if len(name) > 15:
            name = name[:15] + '*'
        simple.RenameSource(name, reader)

        # Representation, view, and camera setup
        simple.Show()
        simple.Render()
        simple.ResetCamera()
        if self.getApplication():
            self.getApplication().InvokeEvent('UpdateEvent')

        return { 'success': True, 'id': reader.GetGlobalIDAsString() }

    @exportRpc("pv.proxy.manager.get")
    def get(self, proxyId, ui=True):
        """
        Returns the proxy state for the given proxyId as a JSON object.
        """
        proxyProperties = []
        proxyId = str(proxyId)
        self.fillPropertyList(proxyId, proxyProperties)

        proxyProperties, groupsInfo = self.reorderProperties(proxyId, proxyProperties)
        proxyJson = { 'id': proxyId }

        # Perform costly request only when needed
        uiProperties = None
        if ui:
            uiProperties = self.getUiProperties(proxyId, proxyProperties)

        # Now restructure the flat, ordered lists to reflect grouping
        restructuredProperties = self.restructureProperties(groupsInfo, proxyProperties, uiProperties)

        proxyJson['properties'] = restructuredProperties['proxy']
        if ui:
            proxyJson['ui'] = restructuredProperties['ui']

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

        self.getApplication().InvokeEvent('UpdateEvent')

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
            self.updateScalarBars()
            self.getApplication().InvokeEvent('UpdateEvent')
            return { 'success': 1, 'id': pid }

        self.getApplication().InvokeEvent('UpdateEvent')
        return { 'success': 0, 'id': '0' }

    @exportRpc("pv.proxy.manager.list")
    def list(self, viewId=None):
        """Returns the current proxy list, specifying for each proxy it's
        name, id, and parent (input) proxy id.  A 'parent' of '0' means
        the proxy has no input.
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
        """Returns a list of the available sources or filters, depending on the
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
# Key/Value Store Protocol
#
# =============================================================================

class ParaViewWebKeyValuePairStore(ParaViewWebProtocol):

    def __init__(self, **kwargs):
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
# Save Data Protocol
#
# =============================================================================

class ParaViewWebSaveData(ParaViewWebProtocol):

    def __init__(self, baseSavePath='', **kwargs):
        super(ParaViewWebSaveData, self).__init__()

        savePath = baseSavePath

        if baseSavePath.find('=') >= 0:
            parts = baseSavePath.split('=')
            savePath = parts[-1]

        self.baseSavePath = savePath

        self.imageExtensions = [ 'jpg', 'png' ]
        self.dataExtensions = [ 'vtk', 'ex2', 'vtp', 'vti', 'vtu', 'vtm', 'vts', 'vtr', 'csv' ]
        self.stateExtensions = [ 'pvsm' ]

    # RpcName: saveData => 'pv.data.save'
    @exportRpc("pv.data.save")
    def saveData(self, filePath, options=None):
        """
        Save some data on the server side.  Can save data from a proxy, a
        screenshot, or else the current state.  Options may be different
        depending on the type of data to be save.  Saving a screenshot
        can take an optional size array indicating the desired width and
        height of the saved image.  Saving data can take an optional proxy
        id specifying which filter or source to save output from.

            options = {
                'proxyId': '426',
                'size': [ <imgWidth>, <imgHeight> ]
            }

        """

        # make sure file path exists
        fullPath = os.path.join(self.baseSavePath, filePath)
        if not os.path.exists(os.path.dirname(fullPath)):
            os.makedirs(os.path.dirname(fullPath))

        # Get file extension and look for configured reader
        idx = filePath.rfind('.')
        extension = filePath[idx+1:]

        # Now find out what kind of save it is, and do it
        if extension in self.imageExtensions:
            if options and 'size' in options:
                # Get the active view
                v = self.getView(-1)

                # Keep track of current size
                cw = v.ViewSize[0]
                ch = v.ViewSize[1]

                # Resize to desired screenshot size
                v.ViewSize = [ int(str(options['size'][0])), int(str(options['size'][1])) ]
                simple.Render()

                # Save actual screenshot
                simple.SaveScreenshot(fullPath)

                # Put the size back the way we found it
                v.ViewSize = [cw, ch]
                simple.Render()
            else:
                simple.SaveScreenshot(fullPath)
        elif extension in self.dataExtensions:
            proxy = None
            if options and 'proxyId' in options:
                proxyId = str(options['proxyId'])
                proxy = self.mapIdToProxy(proxyId)
            simple.SaveData(fullPath, proxy)
        elif extension in self.stateExtensions:
            # simple.SaveState(fullPath) # FIXME: Fixed after 4.3.1
            servermanager.SaveState(fullPath)
        else:
            msg = 'ERROR: Unrecognized extension (%s) in relative path: %s' % (extension, filePath)
            print (msg)
            return { 'success': False, 'message': msg }

        return { 'success': True }

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
            if "host" in options:
                ds_host = options["host"]
            if "port" in options:
                ds_port = options["port"]
            if "rs_host" in options:
                rs_host = options["rs_host"]
            if "rs_port" in options:
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

    def __init__(self, dsHost = None, dsPort = 11111, rsHost=None, rsPort=22221, rcPort=-1, **kwargs):
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

    def __init__(self, plugins=None, pathSeparator=':', **kwargs):
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

    def __init__(self, state_path = None, **kwargs):
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

        self.getApplication().InvokeEvent('UpdateEvent')

        return ids

# =============================================================================
#
# Handle Server File Listing
#
# =============================================================================

class ParaViewWebFileListing(ParaViewWebProtocol):

    def __init__(self, basePath, name, excludeRegex=r"^\.|~$|^\$", groupRegex=r"[0-9]+\.", **kwargs):
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

        currentPath = os.path.normpath(os.path.join(baseDirectory, relativeDir))
        normBase = os.path.normpath(baseDirectory)

        if not currentPath.startswith(normBase):
            print ("### CAUTION ==========================================")
            print (" Attempt to get to another root path ###")
            print ("  => Requested:", relativeDir)
            print ("  => BaseDir:", normBase)
            print ("  => Computed path:", currentPath)
            print ("### CAUTION ==========================================")
            currentPath = normBase

        self.directory_proxy.List(currentPath)
        self.directory_proxy.UpdatePropertyInformation()

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

        # Filter files to create groups. Dicts are not orderable in Py3 - supply a key function.
        files.sort(key=lambda x: x["label"])
        groups = result['groups']
        groupIdx = {}
        filesToRemove = []
        for file in files:
            fileSplit = re.split(self.gPattern, file['label'])
            if len(fileSplit) == 2:
                filesToRemove.append(file)
                gName = '*.'.join(fileSplit)
                if gName in groupIdx:
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
            return { 'label': self.rootName, 'files': [], 'dirs': list(self.baseDirectoryMap), 'groups': [], 'path': [ self.rootName ] }

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
from paraview.modules.vtkRemotingCore import *
from vtkmodules.vtkCommonCore import *

class ParaViewWebSelectionHandler(ParaViewWebProtocol):

    def __init__(self, **kwargs):
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
                    self.getApplication().InvokeEvent('UpdateEvent')
                else:
                    rep.Input.SMProxy.SetSelectionInput(0, selection.SMProxy, 0)

# =============================================================================
#
# Handle Data Export
#
# =============================================================================

class ParaViewWebExportData(ParaViewWebProtocol):

    def __init__(self, basePath, **kwargs):
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
        self.getApplication().InvokeEvent('UpdateEvent')

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
            print ("No widget representation for %s" % proxy.__class__.__name__)

        return widgetProxy.GetGlobalIDAsString()

    def CreateWidgetRepresentation(self, view, name):
        proxy = simple.servermanager.CreateProxy("representations", name, None)
        pythonWrap = simple.servermanager.rendering.__dict__[proxy.GetXMLName()]()
        pythonWrap.UpdateVTKObjects()
        view.Representations.append(pythonWrap)
        pythonWrap.Visibility = 1
        pythonWrap.Enabled = 1
        return pythonWrap
