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
from paraview import simple, web, servermanager, web_helper

# Setup global variables
timesteps = []
currentTimeIndex = 0
timekeeper = servermanager.ProxyManager().GetProxy("timekeeper", "TimeKeeper")
pipeline = web_helper.Pipeline('Kitware')
lutManager = web_helper.LookupTableManager()
view = None
fileList = []
filterList = [{
        'name': 'Cone',
        'icon': 'dataset',
        'category': 'source'
    },{
        'name': 'Sphere',
        'icon': 'dataset',
        'category': 'source'
    },{
        'name': 'Wavelet',
        'icon': 'dataset',
        'category': 'source'
    },{
        'name': 'Clip',
        'icon': 'clip',
        'category': 'filter'
    },{
        'name': 'Slice',
        'icon': 'slice',
        'category': 'filter'
    },{
        'name': 'Contour',
        'icon': 'contour',
        'category': 'filter'
    },{
        'name': 'Threshold',
        'icon': 'threshold',
        'category': 'filter'
    },{
        'name': 'StreamTracer',
        'icon': 'stream',
        'category': 'filter'
    },{
        'name': 'WarpByScalar',
        'icon': 'filter',
        'category': 'filter'
    }]

def initializePipeline():
    global view, lutManager
    view = simple.GetRenderView()
    simple.Render()
    view.ViewSize = [800,800]
    lutManager.setView(view)

# Define ParaView Protocol
class PipelineManager(web.ParaViewServerProtocol):

    @exportRpc("getPipeline")
    def getPipeline(self):
        return pipeline.getRootNode(lutManager)

    @exportRpc("addSource")
    def addSource(self, algo_name, parent):
        global pipeline, lutManager
        pid = str(parent)
        parentProxy = web_helper.idToProxy(parent)
        if parentProxy:
            simple.SetActiveSource(parentProxy)
        else:
            pid = '0'

        # Create new source/filter
        cmdLine = 'simple.' + algo_name + '()'
        newProxy = eval(cmdLine)

        # Create its representation and render
        simple.Show()
        simple.Render()
        simple.ResetCamera()

        # Add node to pipeline
        pipeline.addNode(pid, newProxy.GetGlobalIDAsString())

        # Create LUT if need be
        if pid == '0':
            lutManager.registerFieldData(newProxy.GetPointDataInformation())
            lutManager.registerFieldData(newProxy.GetCellDataInformation())

        # Return the newly created proxy pipeline node
        return web_helper.getProxyAsPipelineNode(newProxy.GetGlobalIDAsString(), lutManager)

    @exportRpc("deleteSource")
    def deleteSource(self, proxy_id):
        pipeline.removeNode(proxy_id)
        proxy = web_helper.idToProxy(proxy_id)
        simple.Delete(proxy)
        simple.Render()

    @exportRpc("updateDisplayProperty")
    def updateDisplayProperty(self, options):
        global lutManager;
        proxy = web_helper.idToProxy(options['proxy_id']);
        rep = simple.GetDisplayProperties(proxy)
        web_helper.updateProxyProperties(rep, options)

        # Try to bind the proper lookup table if need be
        if options.has_key('ColorArrayName') and len(options['ColorArrayName']) > 0:
            name = options['ColorArrayName']
            type = options['ColorAttributeType']
            nbComp = 1

            if type == 'POINT_DATA':
                data = proxy.GetPointDataInformation()
                for i in range(data.GetNumberOfArrays()):
                    array = data.GetArray(i)
                    if array.Name == name:
                        nbComp = array.GetNumberOfComponents()
            elif type == 'CELL_DATA':
                data = proxy.GetPointDataInformation()
                for i in range(data.GetNumberOfArrays()):
                    array = data.GetArray(i)
                    if array.Name == name:
                        nbComp = array.GetNumberOfComponents()
            lut = lutManager.getLookupTable(name, nbComp)
            rep.LookupTable = lut

        simple.Render()

    @exportRpc("pushState")
    def pushState(self, state):
        for proxy_id in state:
            if proxy_id == 'proxy':
                continue
            proxy = web_helper.idToProxy(proxy_id);
            web_helper.updateProxyProperties(proxy, state[proxy_id])
            simple.Render()
        return web_helper.getProxyAsPipelineNode(state['proxy'], lutManager)

    @exportRpc("openFile")
    def openFile(self, path):
        global timesteps;
        reader = simple.OpenDataFile(path)
        simple.RenameSource( path.split("/")[-1], reader)
        simple.Show()
        simple.Render()
        simple.ResetCamera()

        # Add node to pipeline
        pipeline.addNode('0', reader.GetGlobalIDAsString())

        # Create LUT if need be
        lutManager.registerFieldData(reader.GetPointDataInformation())
        lutManager.registerFieldData(reader.GetCellDataInformation())

        try:
            if len(timesteps) == 0:
                timesteps = reader.TimestepValues
        except:
            pass

        return web_helper.getProxyAsPipelineNode(reader.GetGlobalIDAsString(), lutManager)

    @exportRpc("listFiles")
    def listFiles(self):
        return fileList

    @exportRpc("listFilters")
    def listFilters(self):
        return filterList

    @exportRpc("updateScalarbarVisibility")
    def updateScalarbarVisibility(self, options):
        global lutManager;
        # Remove unicode
        if options:
            for lut in options.values():
                if type(lut['name']) == unicode:
                    lut['name'] = str(lut['name'])
                lutManager.enableScalarBar(lut['name'], lut['size'], lut['enabled'])
        return lutManager.getScalarbarVisibility()

    @exportRpc("vcr")
    def updateTime(self,action):
        global currentTimeIndex, timekeeper, timesteps, reader
        if len(timesteps) == 0:
            print 'No time information for ', action
            return 'No time information'
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
        description="ParaView/Web Pipeline Manager web-application")
    web.add_arguments(parser)
    parser.add_argument("--data-dir", default=os.getcwd(),
        help="path to data directory to list", dest="path")
    args = parser.parse_args()

    fileList = web_helper.listFiles(args.path)
    initializePipeline()
    web.start_webserver(options=args, protocol=PipelineManager)
