# Application to probe datasets.

import sys
import os
import os.path
from twisted.python import log
import logging

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
from paraview import vtk

class DataProber(web.ParaViewServerProtocol):
    """DataProber extends web.ParaViewServerProtocol to add API for loading
        datasets add probing them."""

    DataPath = "."
    PipelineObjects = []
    Database = ""
    Widget = None
    View = None

    @classmethod
    def setupApplication(cls):
        """Setups the default application state."""
        # read data directory.

        root = { "name": "ROOT", "dirs" : [], "files" : []}
        directory_map = {}
        directory_map[DataProber.DataPath] = root
        for path, dirs, files in os.walk(DataProber.DataPath):
            element = directory_map[path]

            for name in dirs:
                item = { "name": name, "dirs" : [], "files" : []}
                item["name"] = name
                directory_map[os.path.join(path, name)] = item
                element["dirs"].append(item)
            element["files"] = []
            for name in files:
                relpath = os.path.relpath(os.path.join(path, name),
                    DataProber.DataPath)
                item = { "name" : name, "itemValue" : relpath}
                element["files"].append(item)
        cls.Database = root
        cls.View = simple.CreateRenderView()
        simple.Render()

    @classmethod
    def endInteractionCallback(cls, factory):
        def callback(caller, event):
            caller.GetProperty("Point1WorldPosition").Copy(
                caller.GetProperty("Point1WorldPositionInfo"))
            caller.GetProperty("Point2WorldPosition").Copy(
                caller.GetProperty("Point2WorldPositionInfo"))
            factory.dispatch("http://paraview.org/event#probeDataChanged", True)
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
                cls.endInteractionCallback(self.factory))

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

    @exportRpc("loadData")
    def loadData(self, datafile):
        """Load a data file. The argument is a path relative to the DataPath
        pointing to the dataset to load.

        Returns True if the dataset was loaded successfully, otherwise returns
        False.

        If the dataset is loaded, this methods setups the visualization
        pipelines for interactive probing all loaded datasets.
        """

        datafile = os.path.join(DataProber.DataPath, datafile)
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
        DataProber.PipelineObjects.append(item)

    @exportRpc("loadDatasets")
    def loadDatasets(self, datafiles):
        # initially, we'll only support loading 1 dataset.
        for item in DataProber.PipelineObjects:
            simple.Delete(item["Probe"])
            simple.Delete(item["ReaderRepresentation"])
            simple.Delete(item["Reader"])
        DataProber.PipelineObjects = []

        for path in datafiles:
            self.loadData(path)
        bounds = self.update3DWidget()
        self.resetCameraWithBounds(bounds)
        return True

    @exportRpc("getProbeData")
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
        for item in DataProber.PipelineObjects:
            name = item["name"]
            probe = item["Probe"]
            probe.Source.Point1 = DataProber.Widget.Point1WorldPosition
            probe.Source.Point2 = DataProber.Widget.Point2WorldPosition
            print "Probing ", probe.Source.Point1, probe.Source.Point2
            simple.UpdatePipeline(proxy=probe)
            # fetch probe result from root node.
            do = simple.servermanager.Fetch(probe, 0)
            data = web.vtkPVWebUtilities.WriteAttributesToJavaScript(
                vtk.vtkDataObject.POINT, do)
            headers = web.vtkPVWebUtilities.WriteAttributeHeadersToJavaScript(
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
            DataProber.View.SMProxy.ResetCamera(bounds)
            DataProber.View.CenterOfRotation = [
                (bounds[0] + bounds[1]) * 0.5,
                (bounds[2] + bounds[3]) * 0.5,
                (bounds[4] + bounds[5]) * 0.5]

    @exportRpc("getDatabase")
    def getDatabase(self):
        return DataProber.Database

    @exportRpc("getDatabaseAsHTML")
    def getDatabaseAsHTML(self):
        return DataProber.toHTML(DataProber.Database)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="ParaView Web Data-Prober")
    web.add_arguments(parser)
    parser.add_argument("--data-dir",
        help="path to data directory", dest="path")
    args = parser.parse_args()

    DataProber.DataPath = args.path
    DataProber.setupApplication()

    web.start_webserver(options=args, protocol=DataProber)
