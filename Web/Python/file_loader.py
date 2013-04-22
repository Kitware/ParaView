# import to process args
import sys
import os

try:
    import argparse
except ImportError:
    # since  Python 2.6 and earlier don't have argparse, we simply provide
    # the source for the same as _argparse and we use it instead.
    import _argparse as argparse

# import annotations
from autobahn.wamp import exportRpc

# import paraview modules.
from paraview import simple, web, paraviewweb_wamp, paraviewweb_protocols

# find out the path of the file to load
reader = None
fileToLoad = None
pathToList = "."
fileList = {}
view = None

def listFiles(pathToList):
    """
    Custom server list file.
    """
    nodeTree = {}
    rootNode = { "data": "/", "children": [] , "state" : "open"}
    nodeTree[pathToList] = rootNode
    for path, directories, files in os.walk(pathToList):
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

class FileOpener(paraviewweb_wamp.ServerProtocol):

    def initialize(self):
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebMouseHandler())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebViewPort())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebViewPortImageDelivery())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebViewPortGeometryDelivery())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebTimeHandler())

        # Create default pipeline"""
        global view, reader, fileToLoad
        if fileToLoad:
            reader = simple.OpenDataFile(fileToLoad)
            simple.Show()

            view = simple.Render()
            view.ViewSize = [800,800]
            # If this is running on a Mac DO NOT use Offscreen Rendering
            #view.UseOffscreenRendering = 1
            simple.ResetCamera()
        else:
            view = simple.GetRenderView()
            simple.Render()
            view.ViewSize = [800,800]
        simple.SetActiveView(view)

    @exportRpc("openFile")
    def openFile(self, file):
        global reader
        id = ""
        if reader:
            try:
                simple.Delete(reader)
            except:
                reader = None
        try:
            reader = simple.OpenDataFile(file)
            simple.Show()
            simple.Render()
            simple.ResetCamera()
            id = reader.GetGlobalIDAsString()
        except:
            reader = None
        return id

    @exportRpc("openFileFromPath")
    def openFileFromPath(self, file):
        file = os.path.join(pathToList, file)
        return self.openFile(file)

    @exportRpc("listFiles")
    def listFiles(self):
        return fileList


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="ParaView/Web file loader web-application")
    web.add_arguments(parser)
    parser.add_argument("--file-to-load", help="path to data file to load",
        dest="data")
    parser.add_argument("--data-dir", default=os.getcwd(),
        help="path to data directory to list", dest="path")
    args = parser.parse_args()

    fileToLoad = args.data
    pathToList = args.path
    fileList = listFiles(pathToList)
    web.start_webserver(options=args, protocol=FileOpener)
