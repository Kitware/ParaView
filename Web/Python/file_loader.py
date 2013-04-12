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
from paraview import simple, web, servermanager

# find out the path of the file to load
reader = None
fileToLoad = None
pathToList = "."
fileList = {}

def listFiles(pathToList):
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

view = None
def initializePipeline():
    """
    Called by the default server and serves as a demonstration purpose.
    This should be overriden by application protocols to setup the
    application specific pipeline
    """
    global view, reader
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

    # setup animation scene
    scene = simple.GetAnimationScene()
    simple.GetTimeTrack()
    scene.PlayMode = "Snap To TimeSteps"

class FileOpener(web.ParaViewServerProtocol):

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

    @exportRpc("vcr")
    def updateTime(self,action):
        animationScene = simple.GetAnimationScene()
        currentTime = view.ViewTime

        if action == "next":
            animationScene.GoToNext()
            if currentTime == view.ViewTime:
                animationScene.GoToFirst()
        if action == "prev":
            animationScene.GoToPrevious()
            if currentTime == view.ViewTime:
                animationScene.GoToLast()
        if action == "first":
            animationScene.GoToFirst()
        if action == "last":
            animationScene.GoToLast()

        return view.ViewTime

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
    initializePipeline()
    web.start_webserver(options=args, protocol=FileOpener)
