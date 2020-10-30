from vtkmodules.vtkCommonMisc import vtkResourceFileLocator
import os.path, sys, platform

def find_webapp(appname):
    """Returns the path to the  is web app with given name is found in the package."""

    if platform.system() == "Darwin":
        root = "Resources/"
    else:
        from paraview.servermanager import vtkSMProxyManager
        pv_version= "%d.%d" % (vtkSMProxyManager.GetVersionMajor(), vtkSMProxyManager.GetVersionMinor())
        root = "share/paraview-%s/" % pv_version

    filename = root + "web/%s/server/pvw-%s.py" % (appname, appname)
    locator = vtkResourceFileLocator()
    result = locator.Locate(os.path.dirname(os.path.abspath(__file__)), filename)
    if result:
        return os.path.join(result, filename)
    return None

def find_webappcontents(appname):
    path = find_webapp(appname)
    if not path:
        return None

    root = os.path.dirname(os.path.dirname(path))
    return os.path.join(root, "www")

def load_webapp(appname):
    path = find_webapp(appname)
    if not path:
        return None

    # the web-apps are not proper packages right now, which makes it a little
    # ugly to import them.
    sys.path.insert(0, os.path.dirname(path))
    packagename = os.path.splitext(os.path.basename(path))[0]
    return __import__(packagename)

def get_commandline_args(appname):
    clargs = sys.argv[1:]
    if not "-c" in clargs and not "--content" in clargs:
        clargs.append("-c")
        clargs.append(find_webappcontents(appname))
    return clargs

def start_server(appname, description, module, protocol):
    import argparse

    # Create argument parser
    parser = argparse.ArgumentParser(description=description)

    # Add arguments
    module.server.add_arguments(parser)
    protocol.add_arguments(parser)
    args = parser.parse_args(get_commandline_args(appname))
    protocol.configure(args)

    # Start server
    module.server.start_webserver(options=args, protocol=protocol)
