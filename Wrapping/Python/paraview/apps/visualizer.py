from . import _internals as internals

appname = "visualizer"

def main():
    app = internals.load_webapp(appname)
    if not app:
        raise RuntimeError("This application is not available in your build package.")

    internals.start_server(appname = appname,
            description = "ParaView Web Visualizer",
            module = app,
            protocol = app._VisualizerServer)

def is_supported():
    if internals.find_webapp(appname):
        return True
    return False

if __name__ == "__main__":
    main()
