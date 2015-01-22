r"""
    This module is a ParaViewWeb server application.
    The following command line illustrates how to use it::

        $ pvpython .../pv_web_data_prober.py --data-dir /.../path-to-your-data-directory

        --data-dir
            Path used to list that directory on the server and let the client choose a
            file to load.  You may also specify multiple directories, each with a name
            that should be displayed as the top-level name of the directory in the UI.
            If this parameter takes the form: "name1=path1|name2=path2|...",
            then we will treat this as the case where multiple data directories are
            required.  In this case, each top-level directory will be given the name
            associated with the directory in the argument.

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

# Application to probe datasets.

import sys
import os
import os.path

# import paraview modules.
from paraview import simple, servermanager, vtk
from paraview.web import wamp      as pv_wamp
from paraview.web import protocols as pv_protocols

from vtk.web import server
from vtkWebCorePython import *

# import annotations
from autobahn.wamp import register as exportRpc

from twisted.python import log
import logging

try:
    import argparse
except ImportError:
    # since  Python 2.6 and earlier don't have argparse, we simply provide
    # the source for the same as _argparse and we use it instead.
    import _argparse as argparse

# =============================================================================
# Create custom Data Prober class to handle clients requests
# =============================================================================

class _DataProber(pv_wamp.PVServerProtocol):
    """
    DataProber extends paraview.web.PVServerProtocol to add API for loading
    datasets add probing them.
    """

    DataPath = "."
    PipelineObjects = []
    Database = ""
    Widget = None
    View = None
    authKey = "vtkweb-secret"

    def initialize(self):
        global directoryToList
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebMouseHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPort())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortImageDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortGeometryDelivery())

        # Update authentication key to use
        self.updateSecret(_DataProber.authKey)

    @classmethod
    def setupApplication(cls):
        """Setups the default application state."""
        # read data directory.

        root = { "name": "ROOT", "dirs" : [], "files" : []}
        directory_map = {}
        directory_map[_DataProber.DataPath] = root
        for path, dirs, files in os.walk(_DataProber.DataPath):
            element = directory_map[path]

            for name in dirs:
                item = { "name": name, "dirs" : [], "files" : []}
                item["name"] = name
                directory_map[os.path.join(path, name)] = item
                element["dirs"].append(item)
            element["files"] = []
            for name in files:
                relpath = os.path.relpath(os.path.join(path, name),
                    _DataProber.DataPath)
                item = { "name" : name, "itemValue" : relpath}
                element["files"].append(item)
        cls.Database = root
        cls.View = simple.CreateRenderView()
        simple.Render()

        # setup animation scene
        scene = simple.GetAnimationScene()
        simple.GetTimeTrack()
        scene.PlayMode = "Snap To TimeSteps"

    @classmethod
    def endInteractionCallback(cls, self):
        def callback(caller, event):
            caller.GetProperty("Point1WorldPosition").Copy(
                caller.GetProperty("Point1WorldPositionInfo"))
            caller.GetProperty("Point2WorldPosition").Copy(
                caller.GetProperty("Point2WorldPositionInfo"))
            self.publish("vtk.event.probe.data.changed", True)
            print 'publish callback'
        return callback

    def update3DWidget(self):
        cls = self.__class__
        if not cls.Widget:
            widget = simple.servermanager.rendering.LineWidgetRepresentation()
            widget.Point1WorldPosition = [-1, -1, -1]
            widget.Point2WorldPosition = [1, 1, 1]
            cls.View.Representations.append(widget)
            widget.Enabled = 1
            cls.Widget = widget
            widget.SMProxy.AddObserver(vtk.vtkCommand.EndInteractionEvent,
                cls.endInteractionCallback(self))

        if cls.PipelineObjects:
            # compute bounds for all pipeline objects.
            total_bounds = [ vtk.VTK_DOUBLE_MAX, vtk.VTK_DOUBLE_MIN,
                             vtk.VTK_DOUBLE_MAX, vtk.VTK_DOUBLE_MIN,
                             vtk.VTK_DOUBLE_MAX, vtk.VTK_DOUBLE_MIN]
            for item in cls.PipelineObjects:
                reader = item["Reader"]
                probe = item["Probe"]
                bounds = reader.GetDataInformation().GetBounds()
                if vtk.vtkMath.AreBoundsInitialized(bounds):
                    if total_bounds[0] > bounds[0]:
                        total_bounds[0] = bounds[0]
                    if total_bounds[1] < bounds[1]:
                        total_bounds[1] = bounds[1]
                    if total_bounds[2] > bounds[2]:
                        total_bounds[2] = bounds[2]
                    if total_bounds[3] < bounds[3]:
                        total_bounds[3] = bounds[3]
                    if total_bounds[4] > bounds[4]:
                        total_bounds[4] = bounds[4]
                    if total_bounds[5] < bounds[5]:
                        total_bounds[5] = bounds[5]
            if total_bounds[0] <= total_bounds[1]:
                cls.Widget.Point1WorldPosition = [bounds[0], bounds[2], bounds[4]]
                cls.Widget.Point2WorldPosition = [bounds[1], bounds[3], bounds[5]]
        minpos = cls.Widget.Point1WorldPosition
        maxpos = cls.Widget.Point2WorldPosition
        return (minpos[0], maxpos[0], minpos[1], maxpos[1], minpos[2], maxpos[2])

    @classmethod
    def toHTML(cls, element):
        if element.has_key("itemValue"):
            return '<li itemValue="%s">%s</li>' % (element["itemValue"],
                element["name"])

        if element["name"] != "ROOT":
            text = "<li>"
            text += element["name"]
            text += "<ul>"
        else:
            text = ""

        for item in element["dirs"]:
            text += cls.toHTML(item)
        for item in element["files"]:
            text += cls.toHTML(item)

        if element["name"] != "ROOT":
            text += "</ul></li>"
        return text

    # RpcName: loadData => pv.data.prober.load.data
    @exportRpc("pv.data.prober.load.data")
    def loadData(self, datafile):
        """Load a data file. The argument is a path relative to the DataPath
        pointing to the dataset to load.

        Returns True if the dataset was loaded successfully, otherwise returns
        False.

        If the dataset is loaded, this methods setups the visualization
        pipelines for interactive probing all loaded datasets.
        """

        datafile = os.path.join(_DataProber.DataPath, datafile)
        log.msg("Loading data-file", datafile, logLevel=logging.DEBUG)
        reader = simple.OpenDataFile(datafile)
        if not reader:
            return False
        rep = simple.Show(reader, Representation="Wireframe")
        probe = simple.PlotOverLine(Source = "High Resolution Line Source")

        item = {}
        item["Reader"] = reader
        item["ReaderRepresentation"] = rep
        item["Probe"] = probe
        item["name"] = os.path.split(datafile)[1]
        _DataProber.PipelineObjects.append(item)

    # RpcName: loadDatasets => pv.data.prober.load.dataset
    @exportRpc("pv.data.prober.load.dataset")
    def loadDatasets(self, datafiles):
        # initially, we'll only support loading 1 dataset.
        for item in _DataProber.PipelineObjects:
            simple.Delete(item["Probe"])
            simple.Delete(item["ReaderRepresentation"])
            simple.Delete(item["Reader"])
        _DataProber.PipelineObjects = []

        for path in datafiles:
            self.loadData(path)
        bounds = self.update3DWidget()
        self.resetCameraWithBounds(bounds)
        simple.Render()
        return True

    # RpcName: getProbeData => pv.data.prober.probe.data
    @exportRpc("pv.data.prober.probe.data")
    def getProbeData(self):
        """Returns probe-data from all readers. The returned datastructure has
            the following syntax.
            [
                {
                    "name"      : "<name>",
                    "headers"   : [ "foo", "time", "bar" ],
                    "data"      : [ [3, "2009-11-04", 1], [...], ...]
                },
                { ... }, ...
            ]
        """
        retVal = []
        for item in _DataProber.PipelineObjects:
            name = item["name"]
            probe = item["Probe"]
            probe.Source.Point1 = _DataProber.Widget.Point1WorldPosition
            probe.Source.Point2 = _DataProber.Widget.Point2WorldPosition
            print "Probing ", probe.Source.Point1, probe.Source.Point2
            simple.UpdatePipeline(time=_DataProber.View.ViewTime, proxy=probe)

            # fetch probe result from root node.
            do = simple.servermanager.Fetch(probe, 0)
            data = vtkWebUtilities.WriteAttributesToJavaScript(
                vtk.vtkDataObject.POINT, do)
            headers = vtkWebUtilities.WriteAttributeHeadersToJavaScript(
                vtk.vtkDataObject.POINT, do)
            # process the strings returned by vtkPVWebUtilities to generate
            # Python objects
            nan = "_nan_"
            data = eval(data)
            headers = eval(headers)
            retVal.append({ "name": name,
                            "headers": headers,
                            "data" : data })
        return retVal

    def resetCameraWithBounds(self, bounds):
        if vtk.vtkMath.AreBoundsInitialized(bounds):
            _DataProber.View.SMProxy.ResetCamera(bounds)
            _DataProber.View.CenterOfRotation = [
                (bounds[0] + bounds[1]) * 0.5,
                (bounds[2] + bounds[3]) * 0.5,
                (bounds[4] + bounds[5]) * 0.5]

    # RpcName: getDatabase => pv.data.prober.database.json
    @exportRpc("pv.data.prober.database.json")
    def getDatabase(self):
        return _DataProber.Database

    # RpcName: getDatabaseAsHTML => pv.data.prober.database.html
    @exportRpc("pv.data.prober.database.html")
    def getDatabaseAsHTML(self):
        return _DataProber.toHTML(_DataProber.Database)

    # RpcName: goToNext => pv.data.prober.time.next
    @exportRpc("pv.data.prober.time.next")
    def goToNext(self):
        oldTime = self.View.ViewTime
        simple.GetAnimationScene().GoToNext()
        if oldTime != self.View.ViewTime:
            self.publish("vtk.event.probe.data.changed", True)
            print 'publish a'
            return True
        return False

    # RpcName: goToPrev => pv.data.prober.time.previous
    @exportRpc("pv.data.prober.time.previous")
    def goToPrev(self):
        oldTime = self.View.ViewTime
        simple.GetAnimationScene().GoToPrevious()
        if oldTime != self.View.ViewTime:
            self.publish("vtk.event.probe.data.changed", True)
            print 'publish b'
            return True
        return False

# =============================================================================
# Main: Parse args and start server
# =============================================================================

if __name__ == "__main__":
    # Create argument parser
    parser = argparse.ArgumentParser(description="ParaView Web data.prober")

    # Add default arguments
    server.add_arguments(parser)

    # Add local arguments
    parser.add_argument("--data-dir", help="path to data directory", dest="path")

    # Exctract arguments
    args = parser.parse_args()

    # Configure our current application
    _DataProber.DataPath = args.path
    _DataProber.setupApplication()
    _DataProber.authKey = args.authKey

    # Start server
    server.start_webserver(options=args, protocol=_DataProber)
