# FIXME: This will be placed under "paraview" module, hence users will import as
# follows:
#   from paraview import web


import sys
import types
from threading import Timer

from twisted.python import log
from twisted.internet import reactor
from autobahn.websocket import listenWS
from autobahn.wamp import exportRpc, \
                          WampServerProtocol
from autobahn.resource import WebSocketResource

# import paraview modules.
from paraview import simple
from vtkParaViewWebPython import vtkPVWebApplication,\
                                 vtkPVWebInteractionEvent

def globalIdToProxy(id):
    if id == -1:
        return None
    return simple.servermanager._getPyProxy(\
                simple.servermanager.ActiveConnection.Session.GetRemoteObject(id))

def getProtocol():
    return ParaViewServerProtocol

def initializePipeline():
    """
    Called by the default server and serves as a demonstration purpose.
    This should be overriden by application protocols to setup the
    application specific pipeline
    """
    c = simple.Cone()
    c.Resolution = 80
    simple.Show()
    view = simple.Render()
    # If this is running on a Mac DO NOT use Offscreen Rendering
    #view.UseOffscreenRendering = 1
    pass

class ServerProtocol(WampServerProtocol):
    """
    Defines the server protocol for core ParaViewWeb
    """

    def onAfterCallSuccess(self, result, callid):
        """
        Callback fired after executing incoming RPC with success.

        The default implementation will just return `result` to the client.

        :param result: Result returned for executing the incoming RPC.
        :type result: Anything returned by the user code for the endpoint.
        :param callid: WAMP call ID for incoming RPC.
        :type callid: str
        :returns obj -- Result send back to client.
        """
        log.msg("onAfterCallSuccess", callid)
        result = self._mashall(result)
        return result
        # not sure why the below causes errors.
        #return super(RpcServer1Protocol, self).onAfterCallSuccess(result, callid)

    def onBeforeCall(self, callid, uri, args, isRegistered):
        """
        Callback fired before executing incoming RPC. This can be used for
        logging, statistics tracking or redirecting RPCs or argument mangling i.e.

        The default implementation just returns the incoming URI/args.

        :param uri: RPC endpoint URI (fully-qualified).
        :type uri: str
        :param args: RPC arguments array.
        :type args: list
        :param isRegistered: True, iff RPC endpoint URI is registered in this session.
        :type isRegistered: bool
        :returns pair -- Must return URI/Args pair.
        """
        log.msg("onBeforeCall", callid, uri, args, isRegistered)
        args = self._demarshall(args)
        log.msg("onBeforeCall -- done")
        return uri, args

    def _mashall(self, argument):
        """
        Marshall the argument to JSON serialization object.
        """
        if isinstance(argument, simple.servermanager.Proxy):
            return { "__jsonclass__": "Proxy",
                     "__selfid__": argument.GetGlobalIDAsString()}
        return argument

    def _demarshall(self, argument):
        """
        Demarshalls the "argument".
        """
        if isinstance(argument, types.ListType):
            # for lists, demarshall each argument in the list.
            result = []
            for arg in argument:
                arg = self._demarshall(arg)
                result.append(arg)
            return result
        elif isinstance(argument, types.DictType) and \
            argument.has_key("__jsonclass__") and \
            argument["__jsonclass__"] == "Proxy":
            id = int(argument["__selfid__"])
            return simple.servermanager._getPyProxy(\
                simple.servermanager.ActiveConnection.Session.GetRemoteObject(id))
        return argument

    def onConnect(self, connection_request):
      """
      Callback  fired during WebSocket opening handshake when new WebSocket
      client connection is about to be established.

      Call the factory to increment the connection count.
      """
      self.factory.on_connect()
      return WampServerProtocol.onConnect(self, connection_request)

    def connectionLost(self, reason):
      """
      Called by Twisted when established TCP connection from client was lost.

      Call the factory to decrement the connection count and start a reaper if
      necessary.
      """
      self.factory.connection_lost()
      WampServerProtocol.connectionLost(self, reason)

class ParaViewServerProtocol(ServerProtocol):
    """
    Extends ServerProtocol to add RPC calls for core ParaViewWeb API for
    rendering images, etc.
    """
    WebApplication = vtkPVWebApplication()
    import time

    def onSessionOpen(self):
        """
        Callback fired when WAMP session was fully established.
        """
        self.registerForRpc(self, "http://paraview.org/pv#")
        self.registerForPubSub("http://paraview.org/event#", True)

        pass

    @exportRpc("stillRender")
    def stillRender(self, options):
        beginTime = int(round(ParaViewServerProtocol.time.time() * 1000))
        view = globalIdToProxy(options['view'])
        if not view:
            # Use active view is none provided.
            view = simple.GetActiveView()
        if not view:
            raise Exception("no view provided to stillRender")
        size = [view.ViewSize[0], view.ViewSize[1]]
        if options and options.has_key("size"):
            size = options["size"]
            view.ViewSize = size
        time = 0
        if options and options.has_key("mtime"):
            time = options["mtime"]
        quality = 0
        if options and options.has_key("quality"):
            quality = options["quality"]
        localTime = 0
        if options and options.has_key("localTime"):
            localTime = options["localTime"]
        reply = {}
        app = ParaViewServerProtocol.WebApplication
        reply["image"] = app.StillRenderToString(view.SMProxy, time, quality)
        reply["stale"] = app.GetHasImagesBeingProcessed(view.SMProxy)
        reply["mtime"] = app.GetLastStillRenderToStringMTime()
        reply["size"] = [view.ViewSize[0], view.ViewSize[1]]
        reply["format"] = "jpeg;base64"
        reply["global_id"] = view.GetGlobalIDAsString()
        reply["localTime"] = localTime

        endTime = int(round(ParaViewServerProtocol.time.time() * 1000))
        reply["workTime"] = (endTime - beginTime)

        return reply

    @exportRpc("mouseInteraction")
    def mouseInteraction(self, event):
        view = globalIdToProxy(event['view'])
        if not view:
            view = simple.GetActiveView()
        if not view:
            raise Exception("no view provided to mouseInteraction")

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
        retVal = ParaViewServerProtocol.WebApplication.HandleInteractionEvent(view.SMProxy, pvevent)
        del pvevent
        return retVal

    @exportRpc("resetCamera")
    def resetCamera(self, view):
        view = globalIdToProxy(view)
        if not view:
            view = simple.GetActiveView()
        if not view:
            raise Exception("no view provided to resetCamera")
        simple.ResetCamera(view)
        ParaViewServerProtocol.WebApplication.InvalidateCache(view.SMProxy)
        return view.GetGlobalIDAsString()


    @exportRpc("exit")
    def exit(self):
        reactor.stop()
