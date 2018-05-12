r"""
    This module is a ParaViewWeb server application.
    The following command line illustrate how to use it::

        $ pvpython .../pv_web_catalyst.py --data-dir /.../path-to-your-data-directory

        --data-dir is used to list that directory on the server and let the client choose a file to load.

    Any ParaViewWeb executable script come with a set of standard arguments that
    can be overridden if need be::

        --port 8080
             Port number on which the HTTP server will listen to.

        --content /path-to-web-content/
             Directory that you want to server as static web content.
             By default, this variable is empty which mean that we rely on another server
             to deliver the static content and the current process only focus on the
             WebSocket connectivity of clients.

        --authKey vtkweb-secret
             Secret key that should be provided by the client to allow it to make any
             WebSocket communication. The client will assume if none is given that the
             server expect "vtkweb-secret" as secret key.

"""

# import to process args
import os
import math

# import paraview modules.
import paraview
# for 4.1 compatibility till we fix ColorArrayName and ColorAttributeType usage.
paraview.compatibility.major = 4
paraview.compatibility.minor = 1
from paraview import simple
from paraview.web import wamp      as pv_wamp
from paraview.web import protocols as pv_protocols

from vtk.web import server

try:
    import argparse
except ImportError:
    # since  Python 2.6 and earlier don't have argparse, we simply provide
    # the source for the same as _argparse and we use it instead.
    from vtk.util import _argparse as argparse

# import annotations
from autobahn.wamp import register as exportRpc

# =============================================================================
# Handle function helpers
# =============================================================================

def convert_to_float(v):
    return float(v)

# -----------------------------------------------------------------------------

def convert_to_float_array(v):
    return [ float(v) ]

# -----------------------------------------------------------------------------

def update_property(handle, value):
    property = handle['proxy'].GetProperty(handle['property'])
    if handle.has_key('convert'):
        property.SetData( handle['convert'](value) )
    else:
        property.SetData( value )

# -----------------------------------------------------------------------------

def create_property_handle(proxy, property_name, convert = None):
    handle = { 'proxy': proxy, 'property': property_name, 'update': update_property }
    if convert:
        handle['convert'] = convert
    return handle

# -----------------------------------------------------------------------------

def update_representation(handle, value):
    proxy = handle['proxy']
    if handle['color_list'].has_key(value):
        for propName in handle['color_list'][value]:
            prop = proxy.GetProperty(propName)
            if prop != None:
                prop.SetData(handle['color_list'][value][propName])

        if handle.has_key('override_location'):
            proxy.ColorAttributeType = handle['override_location']


# -----------------------------------------------------------------------------

def create_representation_handle(representation_proxy, colorByList, array_location = None):
    handle = { 'proxy': representation_proxy, 'color_list': colorByList, 'update': update_representation }
    if array_location:
        handle['override_location'] = array_location
    return handle

# =============================================================================
# Define a pipeline object
# =============================================================================

class CatalystBasePipeline(object):

    def __init__(self):
        self.field_data = {}
        self.handles = {}
        self.metadata = {}

    def apply_pipeline(self, input_data, time_steps):
        '''
        Method called when data to process is ready
        '''
        self.metadata['time'] = { "default": time_steps[0], "type": "range", "values": time_steps, "label": "time", "priority": 0 }

    def add_key(self, key, default_value, data_type, values, label, priority, handles):
        self.handles[key] = handles
        self.metadata[key] = {
            "default": default_value,
            "type": data_type,
            "values": values,
            "label": label,
            "priority": priority
        }

    def register_data(self, fieldName, location, scalarRange, lutType):
        self.field_data[fieldName] = {
            "ColorArrayName": (location, fieldName),
            "Range": scalarRange,
            "LookupTable": self._create_lookup_table(fieldName, scalarRange, lutType),
            "ScalarOpacityFunction": self._create_piecewise_function(scalarRange)
        }

    def update_argument(self, key, value):
        for handle in self.handles[key]:
            handle['update'](handle, value)

    def get_metadata(self):
        return self.metadata

    def _create_data_values(self, scalarRange, number_of_values):
        inc = float(scalarRange[1]-scalarRange[0]) / float(number_of_values)
        values = []
        for i in range(number_of_values+1):
            values.append(float(scalarRange[0] + (float(i)*inc) ))
        return values

    def _create_lookup_table(self, name, scalarRange, lutType):
        if lutType == 'blueToRed':
            return simple.GetLookupTableForArray( name, 1, RGBPoints=[scalarRange[0], 0.231373, 0.298039, 0.752941, (scalarRange[0]+scalarRange[1])/2, 0.865003, 0.865003, 0.865003, scalarRange[1], 0.705882, 0.0156863, 0.14902], VectorMode='Magnitude', NanColor=[0.0, 0.0, 0.0], ColorSpace='Diverging', ScalarRangeInitialized=1.0, LockScalarRange=1)
        else:
            return simple.GetLookupTableForArray( name, 1, RGBPoints=[scalarRange[0], 0.0, 0.0, 1.0, scalarRange[1], 1.0, 0.0, 0.0], VectorMode='Magnitude', NanColor=[0.0, 0.0, 0.0], ColorSpace='HSV', ScalarRangeInitialized=1.0, LockScalarRange=1)

    def _create_piecewise_function(self, scalarRange):
        return simple.CreatePiecewiseFunction( Points=[scalarRange[0], 0.0, 0.5, 0.0, scalarRange[1], 1.0, 0.5, 0.0] )

# =============================================================================
# Create custom Catalyst Pipeline Manager class to handle clients requests
# =============================================================================

class _PVCatalystManager(pv_wamp.PVServerProtocol):

    dataDir = None
    authKey = "vtkweb-secret"
    plugins = None
    pipeline_handler = None

    @staticmethod
    def add_arguments(parser):
        parser.add_argument("--data-dir", default=os.getcwd(), help="path to data directory to list", dest="path")
        parser.add_argument("--plugins", default="", help="List of fully qualified path names to plugin objects to load", dest="plugins")

    @staticmethod
    def configure(args):
        _PVCatalystManager.authKey = args.authKey
        _PVCatalystManager.dataDir = args.path
        _PVCatalystManager.plugins = args.plugins

    def initialize(self):
        # Bring used components
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebStartupPluginLoader(_PVCatalystManager.plugins))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebMouseHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPort())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortImageDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortGeometryDelivery())

        # Update authentication key to use
        self.updateSecret(_PVCatalystManager.authKey)

        view = simple.GetRenderView()
        view.Background = [1,1,1]

    @exportRpc("catalyst.file.open")
    def openFileFromPath(self, files):
        fileToLoad = []
        number_of_time_steps = 1
        if type(files) == list:
            number_of_time_steps = len(files)
            for file in files:
               fileToLoad.append(os.path.join(_PVCatalystManager.dataDir, file))
        else:
            fileToLoad.append(os.path.join(_PVCatalystManager.dataDir, files))
        self.time_steps = [ i for i in range(number_of_time_steps)]
        reader = simple.OpenDataFile(fileToLoad)
        if _PVCatalystManager.pipeline_handler:
            _PVCatalystManager.pipeline_handler.apply_pipeline(reader, self.time_steps)

    @exportRpc("catalyst.pipeline.initialize")
    def initializePipeline(self, conf):
        if _PVCatalystManager.pipeline_handler and "initialize_pipeline" in dir(_PVCatalystManager.pipeline_handler):
            _PVCatalystManager.pipeline_handler.initialize_pipeline(conf)


    @exportRpc("catalyst.active.argument.update")
    def updateActiveArgument(self, key, value):
        if key == "time":
            simple.GetAnimationScene().TimeKeeper.Time = float(value)
        elif _PVCatalystManager.pipeline_handler:
            _PVCatalystManager.pipeline_handler.update_argument(key, value)


    @exportRpc("catalyst.arguments.get")
    def getArguments(self):
        if _PVCatalystManager.pipeline_handler:
            return _PVCatalystManager.pipeline_handler.get_metadata()
        else:
            return { "time": {
                        "default": "0",
                        "type": "range",
                        "values": self.time_steps,
                        "label": "time",
                        "priority": 0 } }

# =============================================================================
# Main: Parse args and start server
# =============================================================================

if __name__ == "__main__":
    # Create argument parser
    parser = argparse.ArgumentParser(description="ParaView/Web Pipeline Manager web-application")

    # Add arguments
    server.add_arguments(parser)
    _PVCatalystManager.add_arguments(parser)

    # Extract arguments
    args = parser.parse_args()

    # Configure our current application
    _PVCatalystManager.configure(args)

    # Start server
    server.start_webserver(options=args, protocol=_PVCatalystManager)
