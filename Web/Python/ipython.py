r"""
The ParaViewWeb iPython module is used as a helper to create custom
iPython notebook profile.

The following sample show how the helper class can be used inside
an iPython profile.

# iPython import
from IPython.display import HTML
from IPython.parallel import Client
from paraview.web import ipython as pv_ipython
from vtk import *

iPythonClient = None
paraviewHelper = pv_ipython.ParaViewIPython()
webArguments = pv_ipython.WebArguments('/.../path-to-web-directory')

def _start_paraview():
    paraviewHelper.Initialize()

    paraviewHelper.SetWebProtocol(IPythonProtocol, webArguments)
    return paraviewHelper.Start()


def _stop_paraview():
    paraviewHelper.Finalize()


def _pv_activate_dataset():
    IPythonProtocol.ActivateDataSet('iPython-demo')


def _push_new_timestep():
    # processing code generating new vtkDataSet
    # newDataset = ...
    IPythonProtocol.RegisterDataSet('iPython-demo', newDataset)


def StartParaView(height=600, path='/apps/WebVisualizer/'):
    global iPythonClient, paraviewHelper
    if not iPythonClient:
        iPythonClient = Client()
    urls = iPythonClient[:].apply_sync(lambda:_start_paraview())
    url = ""
    for i in urls:
        if len(i) > 0:
            url = i
    return  HTML("<iframe src='%s/%s' width='100%%' height='%i'></iframe>"%(url, path, height))


def StopParaView():
    global iPythonClient, paraviewHelper
    iPythonClient[:].apply_sync(lambda:_stop_paraview())


def ActivateDataSet():
    iPythonClient[:].apply_sync(lambda:_pv_activate_dataset())


def ComputeNextTimeStep(ds):
    iPythonClient[:].apply_sync(lambda:_push_new_timestep())

"""

import exceptions, logging, random, sys, threading, time, os, paraview

from mpi4py import MPI
from vtk.web import server
from paraview import simple
from paraview.vtk import *
from vtkCommonCorePython import *
from vtkCommonDataModelPython import *
from vtkCommonExecutionModelPython import *
from vtkFiltersSourcesPython import *
from vtkParallelCorePython import *
from vtkParaViewWebCorePython import *
from vtkPVClientServerCoreCorePython import *
from vtkPVServerManagerApplicationPython import *
from vtkPVServerManagerCorePython import *
from vtkPVVTKExtensionsCorePython import *

from paraview.web import wamp as pv_wamp

#------------------------------------------------------------------------------
# Global variables
#------------------------------------------------------------------------------
logger = logging.getLogger()
logger.setLevel(logging.ERROR)

#------------------------------------------------------------------------------
# Global internal methods
#------------------------------------------------------------------------------
def _get_hostname():
    import socket
    if socket.gethostname().find('.')>=0:
        return socket.gethostname()
    else:
        return socket.gethostbyaddr(socket.gethostname())[0]

#------------------------------------------------------------------------------
# ParaView iPython helper class
#------------------------------------------------------------------------------
class ParaViewIPython(object):
    processModule     = None
    globalController  = None
    localController   = None
    webProtocol       = None
    webArguments      = None
    processId         = -1
    number_of_process = -1

    def Initialize(self, log_file_path = None, logging_level = logging.DEBUG):
        if not processModule:
            vtkInitializationHelper.Initialize("ipython-notebook", 4)
            ParaViewIPython.processModule = vtkProcessModule.GetProcessModule()
            ParaViewIPython.globalController = ParaViewIPython.processModule.GetGlobalController()

            if MPI.COMM_WORLD.Get_size() > 1 and (ParaViewIPython.globalController is None or ParaViewIPython.globalController.IsA("vtkDummyController") == True):
                import vtkParallelMPIPython
                ParaViewIPython.globalController = vtkParallelMPIPython.vtkMPIController()
                ParaViewIPython.globalController.Initialize()
                ParaViewIPython.globalController.SetGlobalController(ParaViewIPython.globalController)

            ParaViewIPython.processId = ParaViewIPython.globalController.GetLocalProcessId()
            ParaViewIPython.number_of_process = ParaViewIPython.globalController.GetNumberOfProcesses()
            ParaViewIPython.localController = ParaViewIPython.globalController.PartitionController(ParaViewIPython.number_of_process, ParaViewIPython.processId)

            # must unregister if the reference count is greater than 1
            if ParaViewIPython.localController.GetReferenceCount() > 1:
                ParaViewIPython.localController.UnRegister(None)

            ParaViewIPython.globalController.SetGlobalController(ParaViewIPython.localController)

            if log_file_path:
                formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
                fh = logging.FileHandler('%s-%s.txt' % (log_file_path, str(self.GetProcessId())))
                fh.setLevel(logging_level)
                fh.setFormatter(formatter)
                logger.addHandler(fh)
                logger.info("Process %i initialized for ParaView" % os.getpid())
                logger.info("Sub-Controller: " + str(ParaViewIPython.localController.GetLocalProcessId()) + "/" + str(ParaViewIPython.localController.GetNumberOfProcesses()))
                logger.info("GlobalController: " + str(ParaViewIPython.processId) + "/" + str(ParaViewIPython.number_of_process))
        else:
            logger.info("ParaView has already been initialized. No operation was performed.")

    def Finalize(self):
        if ParaViewIPython.processModule:
            vtkInitializationHelper.Finalize()
            ParaViewIPython.processModule = None

    def GetProcessId(self):
        return ParaViewIPython.processId

    def GetNumberOfProcesses(self):
        return ParaViewIPython.number_of_process

    def __repr__(self):
        return self.__str__()

    def __str__(self):
        return "Host: %s - Controller: %s - Rank: %d/%d" % (_get_hostname(), ParaViewIPython.localController.GetClassName(), self.GetProcessId(), self.GetNumberOfProcesses())

    def SetWebProtocol(self, protocol, arguments):
        ParaViewIPython.webProtocol = protocol
        ParaViewIPython.webArguments = arguments
        if not hasattr(ParaViewIPython.webArguments, 'port'):
            ParaViewIPython.webArguments.port = 8080
        ParaViewIPython.webProtocol.rootNode = (self.GetProcessId() == 0)

    def _start_satelite(self):
        logger.info('ParaView Satelite %d - Started' % self.GetProcessId())
        sid = vtkSMSession.ConnectToSelf();
        vtkPVWebUtilities.ProcessRMIs()
        ParaViewIPython.processModule.UnRegisterSession(sid);
        logger.info('ParaView Satelite  %d - Ended' % self.GetProcessId())

    def _start_web_server(self):
        server.start_webserver(options=ParaViewIPython.webArguments, protocol=ParaViewIPython.webProtocol)
        simple.Disconnect()
        ParaViewIPython.localController.TriggerBreakRMIs()

    def Start(self):
        thread = None
        if self.GetProcessId() == 0:
            thread = threading.Thread(target=ParaViewIPython._start_web_server)
            thread.start()
            logger.info("WebServer thread started")
            return "http://%s:%d" % (_get_hostname(), ParaViewIPython.webArguments.port)
        else:
            thread = threading.Thread(target=ParaViewIPython._start_satelite)
            thread.start()
            logger.info("Satelite thread started")
            return ""

#------------------------------------------------------------------------------
# ParaView iPython protocol
#------------------------------------------------------------------------------

class IPythonProtocol(pv_wamp.PVServerProtocol):
    rootNode   = False
    dataDir    = None
    authKey    = "vtkweb-secret"
    fileToLoad = None
    producer   = simple.DistributedTrivialProducer()

    def ActivateDataSet(cls, key):
        if cls.rootNode:
            cls.producer.UpdateDataSet = key

    def RegisterDataSet(cls, key, dataset):
        vtkDistributedTrivialProducer.SetGlobalOutput(key, dataset)

    def initialize(self):
        simple.Show(IPythonProtocol.producer)

        # Bring used components
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebMouseHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPort())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortImageDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortGeometryDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebTimeHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebPipelineManager(IPythonProtocol.fileToLoad))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebFileManager(IPythonProtocol.dataDir))

        # Update authentication key to use
        self.updateSecret(IPythonProtocol.authKey)

#------------------------------------------------------------------------------
# ParaView iPython default arguments
#------------------------------------------------------------------------------

class WebArguments(object):

    def __init__(self, webDir = None):
        self.content = webDir
        self.port             = 8080
        self.host             = 'localhost'
        self.debug            = 0
        self.timeout          = 30
        self.content          = None
        self.nosignalhandlers = True
