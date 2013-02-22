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
timekeeper = servermanager.ProxyManager().GetProxy("timekeeper", "TimeKeeper")
pipeline = web_helper.Pipeline('Kitware')
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
        'name': 'Stream Tracer',
        'icon': 'stream',
        'category': 'filter'
    },{
        'name': 'Warp',
        'icon': 'filter',
        'category': 'filter'
    }]

def initializePipeline():
    global view
    view = simple.GetRenderView()
    simple.Render()
    view.ViewSize = [800,800]

# Define ParaView Protocol
class PipelineManager(web.ParaViewServerProtocol):

    @exportRpc("getPipeline")
    def getPipeline(self):
        return pipeline.getRootNode()

    @exportRpc("addSource")
    def addSource(self, algo_name, parent):
        global pipeline
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

        # Return the newly created proxy pipeline node
        return web_helper.getProxyAsPipelineNode(newProxy.GetGlobalIDAsString())

    @exportRpc("deleteSource")
    def deleteSource(self, proxy_id):
        pipeline.removeNode(proxy_id)
        proxy = web_helper.idToProxy(proxy_id)
        simple.Delete(proxy)
        simple.Render()

    @exportRpc("updateDisplayProperty")
    def updateDisplayProperty(self, options):
        proxy = web_helper.idToProxy(options['proxy_id']);
        rep = simple.GetDisplayProperties(proxy)
        web_helper.updateProxyProperties(rep, options)
        simple.Render()

    @exportRpc("openFile")
    def openFile(self, path):
        reader = simple.OpenDataFile(path)
        simple.RenameSource( path.split("/")[-1], reader)
        simple.Show()
        simple.Render()
        simple.ResetCamera()

        # Add node to pipeline
        pipeline.addNode('0', reader.GetGlobalIDAsString())

        return web_helper.getProxyAsPipelineNode(reader.GetGlobalIDAsString())

    @exportRpc("listFiles")
    def listFiles(self):
        return fileList

    @exportRpc("listFilters")
    def listFilters(self):
        return filterList


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="ParaView/Web Pipeline Manager web-application")
    web.add_arguments(parser)
    parser.add_argument("--path-to-list", default=os.getcwd(),
        help="path to data directory to list", dest="path")
    args = parser.parse_args()

    fileList = web_helper.listFiles(args.path)
    initializePipeline()
    web.start_webserver(options=args, protocol=PipelineManager)
