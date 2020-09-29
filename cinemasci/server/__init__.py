import http.server
import socketserver
from urllib.parse import urlparse
from urllib.parse import parse_qs
from os import path
from os.path import relpath
from os import getcwd
from os import access
from os import R_OK
import pathlib

#
# global variables - can't seem to add an instance variable to the
# subclass of SimpleHTTPRequestHandler
#
TheDatabase = "CINEMAJUNK" 
CinemaInstallPath = "CINEMAJUNK"

def set_install_path():
    global CinemaInstallPath

    CinemaInstallPath = str(pathlib.Path(__file__).parent.absolute())
    # edit the path to get the correct installation path
    CinemaInstallPath = CinemaInstallPath.strip("/server")
    CinemaInstallPath = "/" + CinemaInstallPath + "/viewers"
    print("CinemaInstallPath: {}".format(CinemaInstallPath))

def get_relative_install_path( initpath ):
    global CinemaInstallPath 

    result = path.join(CinemaInstallPath, initpath.strip("/"))
    result = relpath(result, getcwd())
    print("REL IN PATH: {}".format(result))
    return result

#
# CinemaReqestHandler
#
# Processes GET requests to find viewers and databases
#
class CinemaRequestHandler(http.server.SimpleHTTPRequestHandler):

    def log(self, message):
        if True:
            print(message)

    def do_GET(self):
        global TheDatabase

        self.log("PATH ORIG: {}".format(self.path))
        query_components = parse_qs(urlparse(self.path).query)
        self.log("QUERY    : {}".format(query_components))
        self.path = self.path.split("?")[0]
        self.log("PATH     : {}".format(self.path))

        # set attributes from a query in the GET URL
        if "databases" in query_components:
            TheDatabase = query_components["databases"][0]
            # if not TheDatabase.startswith("/"):
                # TheDatabase = "/" + TheDatabase
            self.log("SET DB   : {}".format(TheDatabase))

        if "viewer" in query_components: 
            # handle a request for a viewer 
            viewer = query_components["viewer"][0]
            if viewer == "explorer": 
                # handle a request for the Cinema:Explorer viewer
                self.log("EXPLORER")
                self.path = get_relative_install_path("../viewers/cinema_explorer.html")
                return http.server.SimpleHTTPRequestHandler.do_GET(self)

            elif viewer == "view": 
                # handle a request for the Cinema:View viewer
                self.log("VIEW")
                self.path = get_relative_install_path("../viewers/cinema_view.html")
                return http.server.SimpleHTTPRequestHandler.do_GET(self)

            else:
                self.log("VIEWER: -{}-".format(viewer))

        if self.path.startswith(TheDatabase):
            # handle requests to the database

            # remap absolute paths
            if TheDatabase.startswith("/"):
                self.log("DB QUERY : {}".format(self.path))
                self.path = relpath(self.path, getcwd())
                self.log("CWD      : {}".format(getcwd()))
                self.log("REL DB   : {}".format(self.path))

            if access(self.path, R_OK):
                self.log("ACCESSING: {}".format(self.path))
                return http.server.SimpleHTTPRequestHandler.do_GET(self)
            else:
                print("ERROR: cannot access file: {}".format(self.path))

        elif self.path.startswith("/cinema"):
            # handle a requests for sub components of the viewers 
            # NOTE: fragile - requires 'cinema' path be unique

            self.log("CINEMA   : {}".format(self.path))
            self.path = get_relative_install_path(self.path)
            self.log("        {}".format(self.path))
            return http.server.SimpleHTTPRequestHandler.do_GET(self)

        else:
            # everything else
            self.log("NORMAL   : {}".format(self.path))
            return http.server.SimpleHTTPRequestHandler.do_GET(self)

def run_cinema_server( viewer, data, port, assetname=None):
    localhost = "http://127.0.0.1"

    set_install_path()
    my_handler = CinemaRequestHandler 
    with socketserver.TCPServer(("", port), my_handler) as httpd:
        urlstring = "{}:{}/?viewer={}&databases={}".format(localhost, port, viewer, data)
        if not assetname is None:
            urlstring = urlstring + "&assetname{}".format(assetname)
        print(urlstring)
        httpd.serve_forever()

