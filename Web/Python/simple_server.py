
import sys
import types
from threading import Timer

from twisted.python import log
from twisted.internet import reactor, defer
from twisted.web.server import Site
from twisted.web.static import File

from autobahn.wamp import WampServerFactory
from autobahn.resource import WebSocketResource

from paraview import simple

def processArgs():
    config = { 'port': 8080, 'content': None, 'js': None, 'module': "web",
              'debug': False, 'timeout': 300}

    for arg in sys.argv:
        if arg.startswith("--debug"):
            config['debug'] = True
        if arg.startswith("--content"):
            config['content'] = arg.split("=")[1]
        if arg.startswith("--js"):
            config['js'] = arg.split("=")[1]
        if arg.startswith("--module"):
            config['module'] = arg.split("=")[1]
        if arg.startswith("--port"):
            config['port'] = int(arg.split("=")[1])
        if arg.startswith("--timeout"):
            config['timeout'] = int(arg.split("=")[1])

    return config

class ReapingWampServerFactory(WampServerFactory):
    """
    Server factory that allow protocol instances to call back when a connection
    is made or lost. If the connection count drops to zero, then the reap timer
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
            log.msg("Client has reconnected, cancelling reaper")
            self._reaper.cancel()
            self._reaper = None

        self._connection_count += 1
        log.msg("on_connect: connection count = %s" % self._connection_count)

    def connection_lost(self):
        """
        Called when a connection is lost.
        """
        self._connection_count -= 1
        log.msg("connection_lost: connection count = %s" % self._connection_count)
        if self._connection_count == 0 and not self._reaper:
            log.msg("Starting timer, process will terminate in: %ssec" % self._timeout)
            self._reaper = Timer(self._timeout, lambda: reactor.stop())
            self._reaper.daemon=True
            self._reaper.start()

if __name__ == '__main__':
    config = processArgs()
    log.startLogging(sys.stdout)

    # Import module if needed
    exec('import ' + config['module'])
    exec('module = ' + config['module'])

    # Handle static file distribution
    if config['content']:
        root = File(config['content'])
    else:
        root = File('.')

    # Handle javascript files
    if config['js']:
        root.putChild("js",  File(config['js']))

    # Setup factory
    wampFactory = ReapingWampServerFactory("ws://localhost:" + str(config['port']),
                      config['debug'], config['timeout'])
    wampFactory.protocol = module.getProtocol()
    wsResource = WebSocketResource(wampFactory)
    root.putChild("ws", wsResource)

    # setup pipeline
    module.initializePipeline()

    # both under one Twisted Web Site
    site = Site(root)
    reactor.listenTCP(config['port'], site)

    wampFactory.startFactory()
    reactor.run()
    wampFactory.stopFactory()
