r"""web is a module that enables using ParaView through a web-server. This
module implments a WampServerProtocol that provides the core RPC-API needed to
place interactive visualization in web-pages. Developers can extent
ParaViewServerProtocol to provide additional RPC callbacks for their web-applications.

This module can be used as the entry point to the application. In that case, it
sets up a Twisted web-server that can generate visualizations as well as serve
web-pages are determines by the command line arguments passed in.
Use "--help" to list the supported arguments.

"""

import types
import logging
from threading import Timer

from twisted.python import log
from twisted.internet import reactor
from autobahn.websocket import listenWS
from autobahn.wamp import exportRpc, \
                          WampServerProtocol
from autobahn.resource import WebSocketResource
from autobahn.wamp import WampServerFactory

# import paraview modules.
from paraview import simple
from vtkParaViewWebCorePython import vtkPVWebApplication,\
                                     vtkPVWebInteractionEvent
from webgl import WebGLResource

# Utility functions
def mapIdToProxy(id):
    """
    Maps global-id for a proxy to the proxy instance. May return None if the
    id is not valid.
    """
    id = int(id)
    if id <= 0:
        return None
    return simple.servermanager._getPyProxy(\
            simple.servermanager.ActiveConnection.Session.GetRemoteObject(id))

class ServerProtocol(WampServerProtocol):
    """
    Defines the core server protocol for ParaView/Web. Adds support to
    marshall/unmarshall RPC callbacks that involve ServerManager proxies as
    arguments or return values.

    Applications typically don't use this class directly, since it doesn't
    register any RPC callbacks that are required for basic web-applications with
    interactive visualizations. For that, use ParaViewServerProtocol.
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
        log.msg("onAfterCallSuccess", callid, logLevel=logging.DEBUG)
        result = self.marshall(result)
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
        log.msg("onBeforeCall", callid, uri, args, isRegistered,
            logLevel=logging.DEBUG)
        args = self.unmarshall(args)
        log.msg("onBeforeCall -- done", logLevel=logging.DEBUG)
        return uri, args

    def marshall(self, argument):
        """
        Marshall the argument to JSON serialization object.
        """
        if isinstance(argument, simple.servermanager.Proxy):
            return { "__jsonclass__": "Proxy",
                     "__selfid__": argument.GetGlobalIDAsString()}
        return argument

    def unmarshall(self, argument):
        """
        Demarshalls the "argument".
        """
        if isinstance(argument, types.ListType):
            # for lists, unmarshall each argument in the list.
            result = []
            for arg in argument:
                arg = self.unmarshall(arg)
                result.append(arg)
            return result
        elif isinstance(argument, types.DictType) and \
            argument.has_key("__jsonclass__") and \
            argument["__jsonclass__"] == "Proxy":
            id = int(argument["__selfid__"])
            return mapIdToProxy(id)
        return argument

    def onConnect(self, connection_request):
        """
        Callback  fired during WebSocket opening handshake when new WebSocket
        client connection is about to be established.

        Call the factory to increment the connection count.
        """
        try:
            self.factory.on_connect()
        except AttributeError:
            pass
        return WampServerProtocol.onConnect(self, connection_request)

    def connectionLost(self, reason):
        """
        Called by Twisted when established TCP connection from client was lost.

        Call the factory to decrement the connection count and start a reaper if
        necessary.
        """
        try:
            self.factory.connection_lost()
        except AttributeError:
            pass
        WampServerProtocol.connectionLost(self, reason)

class ParaViewServerProtocol(ServerProtocol):
    """
    Extends ServerProtocol to add RPC calls for interacting and rendering views.
    Applications can extend ParaViewServerProtocol to add additional RPC
    callbacks updating visualization pipelines, etc.
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
        """
        RPC Callback to render a view and obtain the rendered image.
        """
        beginTime = int(round(ParaViewServerProtocol.time.time() * 1000))
        view = mapIdToProxy(options['view'])
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
        """
        RPC Callback for mouse interactions.
        """
        view = mapIdToProxy(event['view'])
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
        """
        RPC callback to reset camera.
        """
        view = mapIdToProxy(view)
        if not view:
            view = simple.GetActiveView()
        if not view:
            raise Exception("no view provided to resetCamera")
        simple.ResetCamera(view)
        ParaViewServerProtocol.WebApplication.InvalidateCache(view.SMProxy)
        return view.GetGlobalIDAsString()

    @exportRpc("exit")
    def exit(self):
        """RPC callback to exit"""
        reactor.stop()

class ReapingWampServerFactory(WampServerFactory):
    """
    ReapingWampServerFactory is WampServerFactory subclass that adds support to
    close the web-server after a timeout when the last connected client drops.

    Currently, the protocol must call on_connect() and connection_lost() methods
    to notify the factory that the connection was started/closed.

    If the connection count drops to zero, then the reap timer
    is started which will end the process if no other connections are made in
    the timeout interval.
    """

    def __init__(self, url, debugWamp, timeout):
        self._reaper = None
        self._connection_count = 0
        self._timeout = timeout
        WampServerFactory.__init__(self, url, debugWamp)

    def on_connect(self):
        """
        Called when a new connection is made.
        """
        if self._reaper:
            log.msg("Client has reconnected, cancelling reaper",
                logLevel=logging.DEBUG)
            self._reaper.cancel()
            self._reaper = None

        self._connection_count += 1
        log.msg("on_connect: connection count = %s" % self._connection_count,
            logLevel=logging.DEBUG)

    def connection_lost(self):
        """
        Called when a connection is lost.
        """
        self._connection_count -= 1
        log.msg("connection_lost: connection count = %s" % self._connection_count,
            logLevel=logging.DEBUG)
        if self._connection_count == 0 and not self._reaper:
            log.msg("Starting timer, process will terminate in: %ssec" % self._timeout,
                logLevel=logging.DEBUG)
            self._reaper = Timer(self._timeout, lambda: reactor.stop())
            self._reaper.daemon=True
            self._reaper.start()

def add_arguments(parser):
    """
    Add arguments processed know to this module. parser must be
    argparse.ArgumentParser instance.
    """
    import os
    parser.add_argument("-d", "--debug",
        help="log debugging messages to stdout",
        action="store_true")
    parser.add_argument("-p", "--port", type=int, default=8080,
        help="port number for the web-server to listen on (default: 8080)")
    parser.add_argument("-t", "--timeout", type=int, default=300,
        help="timeout for reaping process on idle in seconds (default: 300s)")
    parser.add_argument("-c", "--content", default=os.getcwd(),
        help="root for web-pages to serve (default: current-working-directory)")
    return parser

def start(argv=None,
        protocol=ParaViewServerProtocol,
        description="ParaView/Web web-server based on Twisted."):
    """
    Sets up the web-server using with __name__ == '__main__'. This can also be
    called directly. Pass the opational protocol to override the protocol used.
    Default is ParaViewServerProtocol.
    """
    try:
        import argparse
    except ImportError:
        # since  Python 2.6 and earlier don't have argparse, we simply provide
        # the source for the same as _argparse and we use it instead.
        import _argparse as argparse

    parser = argparse.ArgumentParser(description=description)
    add_arguments(parser)
    args = parser.parse_args(argv)
    start_webserver(options=args, protocol=protocol)

def start_webserver(options, protocol=ParaViewServerProtocol, disableLogging=False):
    """
    Starts the web-server with the given protocol. Options must be an object
    with the following members:
        options.port : port number for the web-server to listen on
        options.timeout : timeout for reaping process on idle in seconds
        options.content : root for web-pages to serve.
    """
    from twisted.internet import reactor
    from twisted.web.server import Site
    from twisted.web.static import File
    import sys

    if not disableLogging:
        log.startLogging(sys.stdout)

    # setup the server-factory
    wampFactory = ReapingWampServerFactory(
        "ws://localhost:%d" % options.port, options.debug, options.timeout)
    wampFactory.protocol = protocol
    wsResource = WebSocketResource(wampFactory)

    root = File(options.content)
    root.putChild("ws", wsResource)


    webgl = WebGLResource()
    root.putChild("WebGL", webgl);

    site = Site(root)

    reactor.listenTCP(options.port, site)

    wampFactory.startFactory()
    reactor.run()
    wampFactory.stopFactory()

if __name__ == "__main__":
    start()
