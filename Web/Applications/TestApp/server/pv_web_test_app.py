r"""
    This module is a ParaViewWeb server application, designed simply to have an
    application to be used in web-based testing.

    The following command line illustrate how to use it::

        $ pvpython .../pv_web_test_app.py --data-dir /.../path-to-your-data-directory

    --data-dir is used to list that directory on the server and let the client
               choose a file to load.

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
import sys
import os

# import paraview modules.
from paraview import simple
from paraview.web import wamp      as pv_wamp
from paraview.web import protocols as pv_protocols

from vtk.web import server
from vtkWebCorePython import *

try:
    import argparse
except ImportError:
    # since  Python 2.6 and earlier don't have argparse, we simply provide
    # the source for the same as _argparse and we use it instead.
    import _argparse as argparse

# =============================================================================
# Create custom class to handle clients requests
# =============================================================================

class _TestApp(pv_wamp.PVServerProtocol):

    # Application configuration
    dataDir = None
    authKey = "vtkweb-secret"
    echoStr = ""
    colorMapsPath = None
    proxyFile = None

    def initialize(self):
        # Bring used components
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebMouseHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPort())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortImageDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortGeometryDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebTimeHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebPipelineManager(baseDir=_TestApp.dataDir))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebProxyManager(allowedProxiesFile=_TestApp.proxyFile,
                                                                         baseDir=_TestApp.dataDir,
                                                                         allowUnconfiguredReaders=True))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebColorManager(pathToColorMaps=_TestApp.colorMapsPath));
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebFileManager(_TestApp.dataDir))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebRemoteConnection())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebStartupRemoteConnection())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebStateLoader())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebTestProtocols())

        # Update authentication key to use
        self.updateSecret(_TestApp.authKey)


# =============================================================================
# Main: Parse args and start server
# =============================================================================

if __name__ == "__main__":
    # Create argument parser
    parser = argparse.ArgumentParser(description="ParaView/Web Testing-application")

    # Add default arguments
    server.add_arguments(parser)

    # Add local arguments
    parser.add_argument("--data-dir",
                        default=os.getcwd(),
                        help="path to ParaView data directory",
                        dest="path")
    parser.add_argument("--echo",
                        default="",
                        help="A string that will be available through echo protocol",
                        dest="echoString")
    parser.add_argument("--proxy-file",
                        default=None,
                        type=str,
                        help="Path to a file containing the list of allowed filter and source proxies",
                        dest="proxyFile")

    # Exctract arguments
    args = parser.parse_args()

    # Configure our current application
    _TestApp.dataDir    = args.path
    _TestApp.authKey    = args.authKey
    _TestApp.echoStr    = args.echoString
    _TestApp.proxyFile  = args.proxyFile

    # Start server
    server.start_webserver(options=args, protocol=_TestApp)
