from . import _internals as internals

appname = "divvy"

def main():
    app = internals.load_webapp(appname)
    if not app:
        raise RuntimeError("This application is not available in your build package.")

    internals.start_server(appname = appname,
            description = "Divvy, your data analytic with ParaView",
            module = app,
            protocol = app._DivvyServer)

def is_supported():
    if internals.find_webapp(appname):
        return True
    return False

if __name__ == "__main__":
    main()
