r"""
    This module is a ParaViewWeb server application.
    The following command line illustrate how to use it::

        $ pvpython .../file_loader.py --data-dir /.../path-to-your-data-directory --file-to-load /.../any-vtk-friendly-file.vtk

    --file-to-load is optional and allow the user to pre-load a given dataset.
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

        --authKey paraviewweb-secret
             Secret key that should be provided by the client to allow it to make any
             WebSocket communication. The client will assume if none is given that the
             server expect "paraviewweb-secret" as secret key.
"""

# import to process args
import sys
import os

# import paraview modules.
from paraview import simple, web, paraviewweb_wamp, paraviewweb_protocols

# import annotations
from autobahn.wamp import exportRpc

try:
    import argparse
except ImportError:
    # since  Python 2.6 and earlier don't have argparse, we simply provide
    # the source for the same as _argparse and we use it instead.
    import _argparse as argparse

# =============================================================================
# Create custom File Opener class to handle clients requests
# =============================================================================

class _FileOpener(paraviewweb_wamp.ServerProtocol):

    # Application configuration
    reader     = None
    fileToLoad = None
    pathToList = "."
    view       = None
    authKey    = "paraviewweb-secret"

    def initialize(self):
        # Bring used components
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebMouseHandler())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebViewPort())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebViewPortImageDelivery())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebViewPortGeometryDelivery())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebTimeHandler())

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

    @exportRpc("openFile")
    def openFile(self, file):
        id = ""
        if _FileOpener.reader:
            try:
                simple.Delete(_FileOpener.reader)
            except:
                _FileOpener.reader = None
        try:
            _FileOpener.reader = simple.OpenDataFile(file)
            simple.Show()
            simple.Render()
            simple.ResetCamera()
            id = _FileOpener.reader.GetGlobalIDAsString()
        except:
            _FileOpener.reader = None
        return id

    @exportRpc("openFileFromPath")
    def openFileFromPath(self, file):
        file = os.path.join(_FileOpener.pathToList, file)
        return self.openFile(file)

    @exportRpc("listFiles")
    def listFiles(self):
        nodeTree = {}
        rootNode = { "data": "/", "children": [] , "state" : "open"}
        nodeTree[_FileOpener.pathToList] = rootNode
        for path, directories, files in os.walk(_FileOpener.pathToList):
            parent = nodeTree[path]
            for directory in directories:
                child = {'data': directory , 'children': [], "state" : "open", 'metadata': {'path': 'dir'}}
                nodeTree[path + '/' + directory] = child
                parent['children'].append(child)
                if directory == 'vtk':
                    child['state'] = 'closed'
            for filename in files:
                child = {'data': { 'title': filename, 'icon': '/'}, 'children': [], 'metadata': {'path': path + '/' + filename}}
                nodeTree[path + '/' + filename] = child
                parent['children'].append(child)
        return rootNode

# =============================================================================
# Main: Parse args and start server
# =============================================================================

if __name__ == "__main__":
    # Create argument parser
    parser = argparse.ArgumentParser(description="ParaView/Web file loader web-application")

    # Add default arguments
    web.add_arguments(parser)

    # Add local arguments
    parser.add_argument("--file-to-load", help="path to data file to load", dest="data")
    parser.add_argument("--data-dir", default=os.getcwd(), help="path to data directory to list", dest="path")

    # Exctract arguments
    args = parser.parse_args()

    # Configure our current application
    _FileOpener.fileToLoad = args.data
    _FileOpener.pathToList = args.path
    _FileOpener.authKey    = args.authKey

    # Start server
    web.start_webserver(options=args, protocol=_FileOpener)
