r"""
The ParaViewWeb iPython module is used as a helper to create custom
iPython notebook profile.

The following sample show how the helper class can be used inside
an iPython profile.

# Global python import
import exceptions, logging, random, sys, threading, time, os

# Update python path to have ParaView libs
pv_path = '/.../ParaView/build'
sys.path.append('%s/lib' % pv_path)
sys.path.append('%s/lib/site-packages' % pv_path)

# iPython import
from IPython.display import HTML
from IPython.parallel import Client
import paraview
from paraview.web import ipython as pv_ipython
from vtk import *

iPythonClient = None
paraviewHelper = pv_ipython.ParaViewIPython()
webArguments = pv_ipython.WebArguments('/.../path-to-web-directory')

def _start_paraview():
    paraviewHelper.Initialize()
    paraviewHelper.SetWebProtocol(pv_ipython.IPythonProtocol, webArguments)
    return paraviewHelper.Start()


def _stop_paraview():
    paraviewHelper.Finalize()


def _pv_activate_dataset():
    pv_ipython.IPythonProtocol.ActivateDataSet('iPython-demo')


def _push_new_timestep():
    # processing code generating new vtkDataSet
    # newDataset = ...
    pv_ipython.IPythonProtocol.RegisterDataSet('iPython-demo', newDataset)


def StartParaView(height=600, path='/apps/Visualizer/'):
    global iPythonClient, paraviewHelper
    if not iPythonClient:
        iPythonClient = Client(profile='pvw')
    urls = iPythonClient[:].apply_sync(lambda:_start_paraview())
    url = ""
    for i in urls:
        if len(i) > 0:
            url = i
    return  HTML("<iframe src='%s%s' width='100%%' height='%i'></iframe>"%(url, path, height))


def StopParaView():
    global iPythonClient, paraviewHelper
    iPythonClient[:].apply_sync(lambda:_stop_paraview())


def ActivateDataSet():
    iPythonClient[:].apply_sync(lambda:_pv_activate_dataset())


def ComputeNextTimeStep():
    global iPythonClient
    if not iPythonClient:
        iPythonClient = Client(profile='pvw')
    iPythonClient[:].apply_sync(lambda:_push_new_timestep())

"""

import exceptions, traceback, logging, random, sys, threading, time, os, paraview

from mpi4py import MPI
from vtk.web import server
from paraview.vtk import *
from vtk.vtkCommonCore import *
from vtk.vtkCommonDataModel import *
from vtk.vtkCommonExecutionModel import *
from vtk.vtkFiltersSources import *
from vtk.vtkParallelCore import *
from vtk.vtkParaViewWebCore import *
from vtk.vtkPVClientServerCoreCore import *
from vtk.vtkPVServerManagerApplication import *
from vtk.vtkPVServerManagerCore import *
from vtk.vtkPVVTKExtensionsCore import *
from vtk.vtkWebCore import *

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
        if not ParaViewIPython.processModule:
            vtkInitializationHelper.Initialize("ipython-notebook", 4) # 4 is type of process
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
                fh = logging.FileHandler('%s-%s.txt' % (log_file_path, str(ParaViewIPython.processId)))
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
        return "Host: %s - Controller: %s - Rank: %d/%d" % (_get_hostname(), ParaViewIPython.localController.GetClassName(), ParaViewIPython.processId, ParaViewIPython.number_of_process)

    def SetWebProtocol(self, protocol, arguments):
        ParaViewIPython.webProtocol = protocol
        ParaViewIPython.webArguments = arguments
        if not hasattr(ParaViewIPython.webArguments, 'port'):
            ParaViewIPython.webArguments.port = 8080
        ParaViewIPython.webProtocol.rootNode = (self.GetProcessId() == 0)
        ParaViewIPython.webProtocol.updateArguments(ParaViewIPython.webArguments)

    @staticmethod
    def _start_satelite():
        logger.info('ParaView Satelite %d - Started' % ParaViewIPython.processId)
        sid = vtkSMSession.ConnectToSelf();
        vtkWebUtilities.ProcessRMIs()
        ParaViewIPython.processModule.UnRegisterSession(sid);
        logger.info('ParaView Satelite  %d - Ended' % ParaViewIPython.processId)

    @staticmethod
    def _start_web_server():
        server.start_webserver(options=ParaViewIPython.webArguments, protocol=ParaViewIPython.webProtocol)
        from paraview import simple
        simple.Disconnect()
        ParaViewIPython.localController.TriggerBreakRMIs()

    @staticmethod
    def debug():
        for i in range(10):
            logger.info('In debug loop ' + str(i))

    def Start(self):
        thread = None
        if self.GetProcessId() == 0:
            thread = threading.Thread(target=ParaViewIPython._start_web_server)
            thread.start()
            time.sleep(10)
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
    rootNode       = False
    dataDir        = None
    authKey        = "vtkweb-secret"
    fileToLoad     = None
    producer       = None
    groupRegex     = "[0-9]+\\."
    excludeRegex   = "^\\.|~$|^\\$"

    @staticmethod
    def ActivateDataSet(key):
        if IPythonProtocol.rootNode and IPythonProtocol.producer:
            IPythonProtocol.producer.UpdateDataset = ''
            IPythonProtocol.producer.UpdateDataset = key

    @staticmethod
    def RegisterDataSet(key, dataset):
        vtkDistributedTrivialProducer.SetGlobalOutput(key, dataset)

    @staticmethod
    def updateArguments(options):
        IPythonProtocol.dataDir      = options.dataDir
        IPythonProtocol.authKey      = options.authKey
        IPythonProtocol.fileToLoad   = options.fileToLoad
        IPythonProtocol.authKey      = options.authKey
        IPythonProtocol.groupRegex   = options.groupRegex
        IPythonProtocol.excludeRegex = options.excludeRegex

    def initialize(self):
        from paraview import simple
        from paraview.web import protocols as pv_protocols

        # Make sure ParaView is initialized
        if not simple.servermanager.ActiveConnection:
            simple.Connect()

        if not IPythonProtocol.producer:
            IPythonProtocol.producer = simple.DistributedTrivialProducer()
            IPythonProtocol.ActivateDataSet('iPython-demo')
            simple.Show(IPythonProtocol.producer)
            simple.Render()

        # Bring used components
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebFileListing(IPythonProtocol.dataDir, "Home", IPythonProtocol.excludeRegex, IPythonProtocol.groupRegex))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebPipelineManager(IPythonProtocol.dataDir, IPythonProtocol.fileToLoad))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebMouseHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPort())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortImageDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortGeometryDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebTimeHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebRemoteConnection())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebFileManager(IPythonProtocol.dataDir))

        # Update authentication key to use
        self.updateSecret(IPythonProtocol.authKey)

    def __str__(self):
        return "Root node: " + str(IPythonProtocol.rootNode)

#------------------------------------------------------------------------------
# ParaView iPython default arguments
#------------------------------------------------------------------------------

class WebArguments(object):

    def __init__(self, webDir = None):
        self.content          = webDir
        self.port             = 8080
        self.host             = 'localhost'
        self.debug            = 0
        self.timeout          = 120
        self.nosignalhandlers = True
        self.authKey          = 'vtkweb-secret'
        self.uploadDir        = ""
        self.testScriptPath   = ""
        self.baselineImgDir   = ""
        self.useBrowser       = ""
        self.tmpDirectory     = ""
        self.testImgFile      = ""
        self.forceFlush       = False
        self.dataDir          = '.'
        self.groupRegex       = "[0-9]+\\."
        self.excludeRegex     = "^\\.|~$|^\\$"
        self.fileToLoad       = None


    def __str__(self):
        return "http://%s:%d/%s" % (self.host, self.port, self.content)
