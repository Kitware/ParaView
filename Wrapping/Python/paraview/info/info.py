try:
    import paraview
except ImportError as error:
    print("ERROR: Could not import paraview module")
    exit(1)
try:
    from paraview import vtk
except ImportError:
    print("ERROR: Could not import vtk module")
    exit(1)

import json


def _collect_opengl_info(info_dict):
    try:
        from paraview.modules.vtkRemotingViews import vtkPVOpenGLInformation

        ginfo = vtkPVOpenGLInformation()
        ginfo.CopyFromObject(None)
        info_dict["OpenGL Vendor"] = ginfo.GetVendor()
        info_dict["OpenGL Version"] = ginfo.GetVersion()
        info_dict["OpenGL Renderer"] = ginfo.GetRenderer()
        info_dict["OpenGL Window Backend"] = ginfo.GetWindowBackend()
    except:
        info_dict["OpenGL"] = "Information Unavailable"
        pass
    return info_dict


def _collect_python_info(info_dict):
    pvinfo = paraview.vtkRemotingCore.vtkPVPythonInformation()
    pvinfo.CopyFromObject(None)
    info_dict["Python Library Path"] = pvinfo.GetPythonPath()
    info_dict["Python Library Version"] = pvinfo.GetPythonVersion()
    info_dict["Python Numpy Support"] = pvinfo.GetNumpySupport()
    info_dict["Python Numpy Version"] = pvinfo.GetNumpyVersion()
    info_dict["Python Matplotlib Support"] = pvinfo.GetMatplotlibSupport()
    info_dict["Python Matplotlib Version"] = pvinfo.GetMatplotlibVersion()
    return info_dict


def _collect_mpi_info(info_dict):
    has_mpi = False
    rank = 0
    size = 0
    try:
        from vtk import vtkMultiProcessController

        controller = vtkMultiProcessController.GetGlobalController()

        has_mpi = True
        rank = controller.GetLocalProcessId()
        size = controller.GetNumberOfProcesses()
    except:
        # compiled without MPI
        pass
    info_dict["MPI Enabled"] = has_mpi
    if has_mpi:
        info_dict["MPI Rank"] = rank
        info_dict["MPI Size"] = size
    return info_dict


def _collect_multithreading_info(info_dict):
    from vtk import vtkSMPTools

    info_dict["SMP Backend"] = vtkSMPTools.GetBackend()
    info_dict["SMP Max Number of Threads"] = vtkSMPTools.GetEstimatedNumberOfThreads()
    return info_dict


def _collect_remoting_info(info_dict):
    from paraview.modules.vtkRemotingCore import vtkRemotingCoreConfiguration

    info_dict["Disable Registry"] = (
        vtkRemotingCoreConfiguration.GetInstance().GetDisableRegistry()
    )
    info_dict["HostName"] = vtkRemotingCoreConfiguration.GetInstance().GetHostName()
    info_dict["Client HostName"] = (
        vtkRemotingCoreConfiguration.GetInstance().GetClientHostName()
    )
    info_dict["Server Port"] = (
        vtkRemotingCoreConfiguration.GetInstance().GetServerPort()
    )
    info_dict["Bind Address"] = (
        vtkRemotingCoreConfiguration.GetInstance().GetBindAddress()
    )
    info_dict["Display"] = vtkRemotingCoreConfiguration.GetInstance().GetDisplay()
    info_dict["Timeout"] = vtkRemotingCoreConfiguration.GetInstance().GetTimeout()
    info_dict["ServerURL"] = vtkRemotingCoreConfiguration.GetInstance().GetServerURL()
    info_dict["Plugin Search Paths"] = (
        vtkRemotingCoreConfiguration.GetInstance().GetPluginSearchPaths()
    )
    info_dict["Plugins"] = vtkRemotingCoreConfiguration.GetInstance().GetPlugins()
    return info_dict


def _collect_config_path_info(info_dict):
    # see https://gitlab.kitware.com/paraview/paraview/-/issues/22967
    return info_dict


def _format_info(info, indent_chars=0):
    INDENT = " " * indent_chars
    for key, value in info.items():
        line = f"{INDENT}{key: <30}"
        if type(value) == dict:
            print(line)
            _format_info(value, indent_chars + 5)
        elif type(value) == list:
            print(line)
            for item in value:
                print(f":  {item}")
        else:
            line += f":  {value}"
            print(line)


def runtime_env(info_dict=None):
    if info_dict is None:
        info_dict = {}
    info_dict = _collect_multithreading_info(info_dict)
    info_dict = _collect_opengl_info(info_dict)
    info_dict = _collect_remoting_info(info_dict)
    return info_dict


def build_env(info_dict=None):
    if info_dict is None:
        info_dict = {}
    info_dict["ParaView Version"] = paraview.__version__
    info_dict["VTK Version"] = vtk.vtkVersion.GetVTKVersionFull()
    info_dict = _collect_mpi_info(info_dict)
    info_dict = _collect_python_info(info_dict)
    return info_dict


def env():
    info_dict = {}
    info_dict = build_env(info_dict)
    info_dict = runtime_env(info_dict)
    return info_dict


def print_env(as_json=False):
    """Get Diagnostics for a given paraview build.

    This function can be called from pvpython/pvbatch or inside a ParaViewCatalyst script.
    """
    info = env()
    if as_json:
        print(json.dumps(info))
    else:
        _format_info(info)
