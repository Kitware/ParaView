r"""
    This module is a ParaViewWeb server application.
    The following command line illustrates how to use it::

        $ pvpython .../pv_web_file_loader.py --data-dir /.../path-to-your-data-directory --file-to-load /.../any-vtk-friendly-file.vtk

        --file-to-load is optional and allow the user to pre-load a given dataset.

        --data-dir
            Path used to list that directory on the server and let the client choose a
            file to load.  You may also specify multiple directories, each with a name
            that should be displayed as the top-level name of the directory in the UI.
            If this parameter takes the form: "name1=path1|name2=path2|...",
            then we will treat this as the case where multiple data directories are
            required.  In this case, each top-level directory will be given the name
            associated with the directory in the argument.

        --ds-host None
             Host name where pvserver has been started

        --ds-port 11111
              Port number to use to connect to pvserver

        --rs-host None
              Host name where renderserver has been started

        --rs-port 22222
              Port number to use to connect to the renderserver

    Any ParaViewWeb executable script comes with a set of standard arguments that can be overriden if need be::

        --port 8080
             Port number on which the HTTP server will listen.

        --content /path-to-web-content/
             Directory that you want to serve as static web content.
             By default, this variable is empty which means that we rely on another
             server to deliver the static content and the current process only
             focuses on the WebSocket connectivity of clients.

        --authKey vtkweb-secret
             Secret key that should be provided by the client to allow it to make any
             WebSocket communication. The client will assume if none is given that the
             server expects "vtkweb-secret" as secret key.
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

# import annotations
from autobahn.wamp import register as exportRpc

try:
    import argparse
except ImportError:
    # since  Python 2.6 and earlier don't have argparse, we simply provide
    # the source for the same as _argparse and we use it instead.
    import _argparse as argparse

# =============================================================================
# Create custom File Opener class to handle clients requests
# =============================================================================

class _FileOpener(pv_wamp.PVServerProtocol):

    # Application configuration
    reader     = None
    fileToLoad = None
    pathToList = "."
    view       = None
    authKey    = "vtkweb-secret"
    dsHost = None
    dsPort = 11111
    rsHost = None
    rsPort = 11111

    def initialize(self):
        # Bring used components
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebStartupRemoteConnection(_FileOpener.dsHost, _FileOpener.dsPort, _FileOpener.rsHost, _FileOpener.rsPort))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebFileListing(_FileOpener.pathToList, "Home"))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebMouseHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPort())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortImageDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortGeometryDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebTimeHandler())

        # Update authentication key to use
        self.updateSecret(_FileOpener.authKey)

        # Create default pipeline
        if _FileOpener.fileToLoad:
            _FileOpener.reader = simple.OpenDataFile(_FileOpener.fileToLoad)
            simple.Show()

            _FileOpener.view = simple.Render()
            _FileOpener.view.ViewSize = [800,800]
            # If this is running on a Mac DO NOT use Offscreen Rendering
            #view.UseOffscreenRendering = 1
            simple.ResetCamera()
        else:
            _FileOpener.view = simple.GetRenderView()
            simple.Render()
            _FileOpener.view.ViewSize = [800,800]
        simple.SetActiveView(_FileOpener.view)

    def openFile(self, files):
        id = ""
        if _FileOpener.reader:
            try:
                simple.Delete(_FileOpener.reader)
            except:
                _FileOpener.reader = None
        try:
            _FileOpener.reader = simple.OpenDataFile(files)
            simple.Show()
            simple.Render()
            simple.ResetCamera()
            id = _FileOpener.reader.GetGlobalIDAsString()
        except:
            _FileOpener.reader = None
        return id

    # RpcName: openFileFromPath => pv.file-loader.open.file
    @exportRpc("pv.file.loader.open.file")
    def openFileFromPath(self, files):
        fileToLoad = []
        if type(files) == list:
            for file in files:
               fileToLoad.append(os.path.join(_FileOpener.pathToList, file))
        else:
            fileToLoad.append(os.path.join(_FileOpener.pathToList, files))
        return self.openFile(fileToLoad)

# =============================================================================
# Main: Parse args and start server
# =============================================================================

if __name__ == "__main__":
    # Create argument parser
    parser = argparse.ArgumentParser(description="ParaView/Web file loader web-application")

    # Add default arguments
    server.add_arguments(parser)

    # Add local arguments
    parser.add_argument("--file-to-load", help="relative file path to load based on --data-dir argument", dest="data")
    parser.add_argument("--data-dir", default=os.getcwd(), help="Base path directory", dest="path")
    parser.add_argument("--ds-host", default=None, help="Hostname to connect to for DataServer", dest="dsHost")
    parser.add_argument("--ds-port", default=11111, type=int, help="Port number to connect to for DataServer", dest="dsPort")
    parser.add_argument("--rs-host", default=None, help="Hostname to connect to for RenderServer", dest="rsHost")
    parser.add_argument("--rs-port", default=11111, type=int, help="Port number to connect to for RenderServer", dest="rsPort")


    # Exctract arguments
    args = parser.parse_args()

    # Configure our current application
    _FileOpener.fileToLoad = args.data
    _FileOpener.pathToList = args.path
    _FileOpener.authKey    = args.authKey
    _FileOpener.dsHost     = args.dsHost
    _FileOpener.dsPort     = args.dsPort
    _FileOpener.rsHost     = args.rsHost
    _FileOpener.rsPort     = args.rsPort

    # Start server
    server.start_webserver(options=args, protocol=_FileOpener)
