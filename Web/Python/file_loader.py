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

class FileOpener(paraviewweb_wamp.ServerProtocol):

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
        self.updateSecret(FileOpener.authKey)

        # Create default pipeline
        if FileOpener.fileToLoad:
            FileOpener.reader = simple.OpenDataFile(FileOpener.fileToLoad)
            simple.Show()

            FileOpener.view = simple.Render()
            FileOpener.view.ViewSize = [800,800]
            # If this is running on a Mac DO NOT use Offscreen Rendering
            #view.UseOffscreenRendering = 1
            simple.ResetCamera()
        else:
            FileOpener.view = simple.GetRenderView()
            simple.Render()
            FileOpener.view.ViewSize = [800,800]
        simple.SetActiveView(FileOpener.view)

    @exportRpc("openFile")
    def openFile(self, file):
        id = ""
        if FileOpener.reader:
            try:
                simple.Delete(FileOpener.reader)
            except:
                FileOpener.reader = None
        try:
            FileOpener.reader = simple.OpenDataFile(file)
            simple.Show()
            simple.Render()
            simple.ResetCamera()
            id = FileOpener.reader.GetGlobalIDAsString()
        except:
            FileOpener.reader = None
        return id

    @exportRpc("openFileFromPath")
    def openFileFromPath(self, file):
        file = os.path.join(FileOpener.pathToList, file)
        return self.openFile(file)

    @exportRpc("listFiles")
    def listFiles(self):
        nodeTree = {}
        rootNode = { "data": "/", "children": [] , "state" : "open"}
        nodeTree[FileOpener.pathToList] = rootNode
        for path, directories, files in os.walk(FileOpener.pathToList):
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
    FileOpener.fileToLoad = args.data
    FileOpener.pathToList = args.path
    FileOpener.authKey    = args.authKey

    # Start server
    web.start_webserver(options=args, protocol=FileOpener)
