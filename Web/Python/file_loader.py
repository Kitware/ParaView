# import to process args
import sys
import os
import argparse

# import annotations
from autobahn.wamp import exportRpc

# import paraview modules.
from paraview import simple, web, servermanager

# find out the path of the file to load
reader = None
timesteps = []
currentTimeIndex = 0
timekeeper = servermanager.ProxyManager().GetProxy("timekeeper", "TimeKeeper")
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

    pass

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
        global currentTimeIndex, timekeeper, timesteps, reader
        if len(timesteps) == 0:
            timesteps = reader.TimestepValues
        updateTime = False
        if action == "next":
            currentTimeIndex = (currentTimeIndex + 1) % len(timesteps)
            updateTime = True
        if action == "prev":
            currentTimeIndex = (currentTimeIndex - 1 + len(timesteps)) % len(timesteps)
            updateTime = True
        if action == "first":
            currentTimeIndex = 0
            updateTime = True
        if action == "last":
            currentTimeIndex = len(timesteps) - 1
            updateTime = True
        if updateTime:
            timekeeper.Time = timesteps[currentTimeIndex]
            print "UpdateTime: ", str(timesteps[currentTimeIndex])

        return action

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="ParaView/Web file loader web-application")
    web.add_arguments(parser)
    parser.add_argument("--file-to-load", help="path to data file to load",
        dest=data)
    parser.add_argument("--path-to-list", default=os.getcwd(),
        help="path to data directory to list", dest=path)
    args = parser.parse_args()

    fileToLoad = args.data
    pathToList = args.path
    fileList = listFiles(pathToList)
    initializePipeline()
    web.start_webserver(options=args, protocol=FileOpener)
