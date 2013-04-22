r"""paraviewweb_wamp is a module that provide classes that extend any
WAMP related class for the purpose of ParaViewWeb.

"""

import types
import logging
import time
from threading import Timer

from twisted.python import log
from twisted.internet import reactor
from autobahn.websocket import listenWS
from autobahn.wamp import exportRpc, \
                          WampServerProtocol
from autobahn.resource import WebSocketResource
from autobahn.wamp import WampServerFactory

from paraview import simple
from vtkParaViewWebCorePython import vtkPVWebApplication

# =============================================================================
#
# Base class for ParaViewWeb WampServerProtocol
#
# =============================================================================

class ServerProtocol(WampServerProtocol):
    """
    Defines the core server protocol for ParaView/Web. Adds support to
    marshall/unmarshall RPC callbacks that involve ServerManager proxies as
    arguments or return values.

    Applications typically don't use this class directly, since it doesn't
    register any RPC callbacks that are required for basic web-applications with
    interactive visualizations. For that, use ParaViewServerProtocol.
    """

    def __init__(self):
        self.Application = vtkPVWebApplication()
        self.ParaViewWebProtocols = []
        self.initialize();

    def initialize(self):
        """
        Let the sub class define what they need to do to properly initialize
        themselves.
        """
        pass

    def registerParaViewWebProtocol(self, protocol):
        protocol.setApplication(self.Application)
        self.ParaViewWebProtocols.append(protocol)

    def getParaViewWebProtocols(self):
        return self.ParaViewWebProtocols

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
        return self.marshall(result)

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
        return uri, self.unmarshall(args)

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
            return simple.servermanager._getPyProxy(\
                simple.servermanager.ActiveConnection.Session.GetRemoteObject(id))
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

    def onSessionOpen(self):
        """
        Callback fired when WAMP session was fully established.
        """
        self.registerForPubSub("http://paraview.org/event#", True)
        self.registerForRpc(self, "http://paraview.org/pv#")
        for protocol in self.ParaViewWebProtocols:
            self.registerForRpc(protocol, "http://paraview.org/pv#")

    @exportRpc("exit")
    def exit(self):
        """RPC callback to exit"""
        reactor.stop()

# =============================================================================
#
# Base class for ParaViewWeb WampServerFactory
#
# =============================================================================

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