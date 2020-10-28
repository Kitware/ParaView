from . import _internals as internals

appname = "flow"

def main():
    app = internals.load_webapp(appname)
    if not app:
        raise RuntimeError("This application is not available in your build package.")

    internals.start_server(appname = appname,
            description = "ParaView Flow - ParaViewWeb viewer for ParFlow data",
            module = app,
            protocol = app._Server)

def is_supported():
    if internals.find_webapp(appname):
        return True
    return False

if __name__ == "__main__":
    main()
