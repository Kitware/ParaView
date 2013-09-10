r"""
    This module is a ParaViewWeb server application.
    The following command line illustrate how to use it::

        $ pvpython .../pv_web_parallel.py --ds-host localhost --ds-port 11111

        --ds-host localhost
             Host name where pvserver has been started

        --ds-port 11111
              Port number to use to connect to pvserver

        --rs-host localhost
              Host name where renderserver has been started

        --rs-port 22222
              Port number to use to connect to the renderserver

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
from paraview     import simple
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
# Create custom Parallel sphere class to handle clients requests
# =============================================================================

class _Sphere(pv_wamp.PVServerProtocol):

    dataDir = None
    authKey = "vtkweb-secret"
    dsHost = None
    dsPort = 11111
    rsHost = None
    rsPort = 11111
    text = ""

    def initialize(self):
        # Bring used components
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebMouseHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPort())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortImageDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortGeometryDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebStartupRemoteConnection(_Sphere.dsHost, _Sphere.dsPort, _Sphere.rsHost, _Sphere.rsPort))

        # Update authentication key to use
        self.updateSecret(_Sphere.authKey)

        # Create a sphere and color it by process Id to higlight the parallel
        # aspect of that example.
        simple.Sphere()
        representation = simple.Show()
        lut = simple.GetLookupTableForArray( "vtkProcessId", 1, RGBPoints=[0.0, 0.23, 0.299, 0.754, 10.0, 0.706, 0.016, 0.15], VectorMode='Magnitude', NanColor=[0.25, 0.0, 0.0], ColorSpace='Diverging', ScalarRangeInitialized=1.0 )
        representation.ColorArrayName = ('POINT_DATA', 'vtkProcessId')
        representation.LookupTable = lut

        text = simple.Text()
        text.Text = _Sphere.text

        simple.Show()
        simple.Render()
        simple.ResetCamera()

# =============================================================================
# Main: Parse args and start server
# =============================================================================

if __name__ == "__main__":
    # Create argument parser
    parser = argparse.ArgumentParser(description="ParaView/Web Parallel Sphere web-application")

    # Add default arguments
    server.add_arguments(parser)

    # Add local arguments
    parser.add_argument("--data-dir", default=os.getcwd(), help="path to data directory to list", dest="path")
    parser.add_argument("--ds-host", default=None, help="Hostname to connect to for DataServer", dest="dsHost")
    parser.add_argument("--ds-port", default=11111, type=int, help="Port number to connect to for DataServer", dest="dsPort")
    parser.add_argument("--rs-host", default=None, help="Hostname to connect to for RenderServer", dest="rsHost")
    parser.add_argument("--rs-port", default=11111, type=int, help="Port number to connect to for RenderServer", dest="rsPort")
    parser.add_argument("--text", default="", help="2D text to be shown", dest="text")

    # Exctract arguments
    args = parser.parse_args()

    # Configure our current application
    _Sphere.authKey = args.authKey
    _Sphere.dataDir = args.path
    _Sphere.dsHost = args.dsHost
    _Sphere.dsPort = args.dsPort
    _Sphere.rsHost = args.rsHost
    _Sphere.rsPort = args.rsPort
    _Sphere.text   = args.text

    # Start server
    server.start_webserver(options=args, protocol=_Sphere)
