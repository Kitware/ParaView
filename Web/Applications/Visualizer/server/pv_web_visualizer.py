r"""
    This module is a ParaViewWeb server application.
    The following command line illustrate how to use it::

        $ pvpython .../pv_web_visualizer.py --data-dir /.../path-to-your-data-directory

        --data-dir is used to list that directory on the server and let the client choose a file to load.

        --load-file try to load the file relative to data-dir if any.

        --ds-host None
             Host name where pvserver has been started

        --ds-port 11111
              Port number to use to connect to pvserver

        --rs-host None
              Host name where renderserver has been started

        --rs-port 22222
              Port number to use to connect to the renderserver

        --exclude-regex "[0-9]+\\."
              Regular expression used to filter out files in directory/file listing.

        --group-regex "^\\.|~$|^\\$"
              Regular expression used to group files into a single loadable entity.

        --plugins
            List of fully qualified path names to plugin objects to load

        --filters
            Path to a file with json text containing filters to load

    Any ParaViewWeb executable script come with a set of standard arguments that
    can be overriden if need be::

        --port 8080
             Port number on which the HTTP server will listen to.

        --content /path-to-web-content/
             Directory that you want to server as static web content.
             By default, this variable is empty which mean that we rely on another server
             to deliver the static content and the current process only focus on the
             WebSocket connectivity of clients.

        --authKey vtkweb-secret
             Secret key that should be provided by the client to allow it to make any
             WebSocket communication. The client will assume if none is given that the
             server expect "vtkweb-secret" as secret key.

"""

# import to process args
import os

# import paraview modules.
from paraview.web import wamp      as pv_wamp
from paraview.web import protocols as pv_protocols

from vtk.web import server

try:
    import argparse
except ImportError:
    # since  Python 2.6 and earlier don't have argparse, we simply provide
    # the source for the same as _argparse and we use it instead.
    import _argparse as argparse

# =============================================================================
# Create custom Pipeline Manager class to handle clients requests
# =============================================================================

class _PipelineManager(pv_wamp.PVServerProtocol):

    dataDir = None
    authKey = "vtkweb-secret"
    dsHost = None
    dsPort = 11111
    rsHost = None
    rsPort = 11111
    fileToLoad = None
    groupRegex = "[0-9]+\\."
    excludeRegex = "^\\.|~$|^\\$"
    plugins = None
    filterFile = None

    @staticmethod
    def add_arguments(parser):
        parser.add_argument("--data-dir", default=os.getcwd(), help="path to data directory to list", dest="path")
        parser.add_argument("--load-file", default=None, help="File to load if any based on data-dir base path", dest="file")
        parser.add_argument("--ds-host", default=None, help="Hostname to connect to for DataServer", dest="dsHost")
        parser.add_argument("--ds-port", default=11111, type=int, help="Port number to connect to for DataServer", dest="dsPort")
        parser.add_argument("--rs-host", default=None, help="Hostname to connect to for RenderServer", dest="rsHost")
        parser.add_argument("--rs-port", default=11111, type=int, help="Port number to connect to for RenderServer", dest="rsPort")
        parser.add_argument("--exclude-regex", default="^\\.|~$|^\\$", help="Regular expression for file filtering", dest="exclude")
        parser.add_argument("--group-regex", default="[0-9]+\\.", help="Regular expression for grouping files", dest="group")
        parser.add_argument("--plugins", default="", help="List of fully qualified path names to plugin objects to load", dest="plugins")
        parser.add_argument("--filters", default=None, help="Path to a file with json text containing filters to load", dest="filters")

    @staticmethod
    def configure(args):
        _PipelineManager.authKey      = args.authKey
        _PipelineManager.dataDir      = args.path
        _PipelineManager.dsHost       = args.dsHost
        _PipelineManager.dsPort       = args.dsPort
        _PipelineManager.rsHost       = args.rsHost
        _PipelineManager.rsPort       = args.rsPort
        _PipelineManager.excludeRegex = args.exclude
        _PipelineManager.groupRegex   = args.group
        _PipelineManager.plugins      = args.plugins
        _PipelineManager.filterFile   = args.filters

        if args.file:
            _PipelineManager.fileToLoad = args.path + '/' + args.file

    def initialize(self):
        # Bring used components
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebStartupRemoteConnection(_PipelineManager.dsHost, _PipelineManager.dsPort, _PipelineManager.rsHost, _PipelineManager.rsPort))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebStartupPluginLoader(_PipelineManager.plugins))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebStateLoader(_PipelineManager.fileToLoad))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebFileListing(_PipelineManager.dataDir, "Home", _PipelineManager.excludeRegex, _PipelineManager.groupRegex))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebPipelineManager(_PipelineManager.dataDir, _PipelineManager.fileToLoad))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebFilterList(_PipelineManager.filterFile))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebMouseHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPort())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortImageDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortGeometryDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebTimeHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebRemoteConnection())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebFileManager(_PipelineManager.dataDir))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebSelectionHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebWidgetManager())

        # Update authentication key to use
        self.updateSecret(_PipelineManager.authKey)

# =============================================================================
# Main: Parse args and start server
# =============================================================================

if __name__ == "__main__":
    # Create argument parser
    parser = argparse.ArgumentParser(description="ParaView/Web Pipeline Manager web-application")

    # Add arguments
    server.add_arguments(parser)
    _PipelineManager.add_arguments(parser)

    # Exctract arguments
    args = parser.parse_args()

    # Configure our current application
    _PipelineManager.configure(args)

    # Start server
    server.start_webserver(options=args, protocol=_PipelineManager)
