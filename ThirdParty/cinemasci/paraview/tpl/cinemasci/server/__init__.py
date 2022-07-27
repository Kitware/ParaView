import http.server
import socketserver
from urllib.parse import urlparse
from urllib.parse import parse_qs
from os import chdir
from os import path
from os.path import relpath
from os import getcwd
from os import access
from os import R_OK
import pathlib
import json
from .. import cdb

#
# global variables - can't seem to add an instance variable to the
# subclass of SimpleHTTPRequestHandler
#
CinemaInstallPath = "CINEMAJUNK"
ServerInstallPath = "CINEMAJUNK"

def set_install_path():
    global CinemaInstallPath
    global ServerInstallPath

    ServerInstallPath = str(pathlib.Path(__file__).parent.absolute())
    CinemaInstallPath = str(pathlib.Path(__file__).parent.absolute())
    # edit the path to get the correct installation path
    CinemaInstallPath = CinemaInstallPath.strip("/server")
    CinemaInstallPath = "/" + CinemaInstallPath + "/viewers"

def verify_cinema_databases( runpath, databases ):
    result = False

    curdir = getcwd()
    chdir(runpath)
    for db in databases:
        if not path.isdir(db):
            print("")
            print("ERROR: Cinema server is looking for the database:") 
            print("           '{}\',".format(db)) 
            print("       but it does not exist relative to the server runpath:")
            print("           '{}\',".format(runpath)) 
            print("")
            result = False
            break;
        else:
            result = True

    chdir(curdir)
    return result 

#
# CinemaSimpleReqestHandler
#
# Processes GET requests to find viewers and databases
#
class CinemaSimpleRequestHandler(http.server.SimpleHTTPRequestHandler):

    @property
    def silent(self):
        return self._silent

    @silent.setter
    def silent(self, value):
        self._silent = value

    @property
    def rundir(self):
        return self._rundir

    @rundir.setter
    def rundir(self, value):
        self._rundir = value

    @property
    def databases(self):
        return self._databases

    @databases.setter
    def databases(self, value):
        self._databases = value

    @property
    def verbose(self):
        return self._verbose

    @verbose.setter
    def verbose(self, value):
        self._verbose = value

    @property
    def assetname(self):
        return self._assetname

    @assetname.setter
    def assetname(self, value):
        self._assetname = value

    @property
    def viewer(self):
        return self._viewer

    @viewer.setter
    def viewer(self, value):
        self._viewer = value

    def translate_path(self, path ):
        # print("translate_path: {}".format(path))
        return path

    def log(self, message):
        if self.verbose:
            print(message)

    def log_message( self, format, *args ):
        pass

    def do_GET(self):
        global CinemaInstallPath
        global ServerInstallPath

        self.log(" ")
        self.log("PATH     : {}".format(self.path))

        # the request is for a viewer
        if self.path == "/":
            for db in self.databases:
                if not path.isdir(db):
                    self.log("ERROR")
                    self.path = ServerInstallPath + "/error_no-database.html"
                    return http.server.SimpleHTTPRequestHandler.do_GET(self)

            if self.viewer == "explorer": 
                # handle a request for the Cinema:Explorer viewer
                self.log("EXPLORER")
                self.path = CinemaInstallPath + "/cinema_explorer.html"
                self.log(self.path)
                return http.server.SimpleHTTPRequestHandler.do_GET(self)

            elif self.viewer == "view": 
                # handle a request for the Cinema:View viewer
                self.log("VIEW")
                self.path = CinemaInstallPath + "/cinema_view.html"
                return http.server.SimpleHTTPRequestHandler.do_GET(self)

            if self.viewer == "test": 
                # handle a request for the Cinema:Explorer viewer
                self.log("TEST")
                self.path = ServerInstallPath + "/cinema_test.html"
                return http.server.SimpleHTTPRequestHandler.do_GET(self)

            else:
                self.log("VIEWER: {}".format(self.viewer))


        if self.path.endswith("cinema_attributes.json"):
            # this is a request to the server for attributes
            self.log("ATTRIBUTE REQUEST") 

            if (not self.assetname == None):
                json_string = "{{\"assetname\" : \"{}\"}}".format(self.assetname)
                self.send_response(200)
                self.send_header("Content-type", "text/html")
                self.send_header("Content-length", len(json_string))
                self.end_headers()
                self.wfile.write(str.encode(json_string))
            return

        if self.path.endswith("databases.json"):
            self.log("DATABASES   : {}".format(self.path))
            json_string = self.get_database_json()

            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.send_header("Content-length", len(json_string))
            self.end_headers()
            self.wfile.write(json_string.encode())
            return


        if self.path.startswith("/cinema/"):
            # handle a requests for sub components of the viewers 
            # NOTE: fragile - requires 'cinema' path be unique

            self.log("CINEMA   : {}".format(self.path))
            self.path = CinemaInstallPath + self.path 
            self.log("         : {}".format(self.path))
            return http.server.SimpleHTTPRequestHandler.do_GET(self)

        else:
            # everything else
            self.log("NORMAL   : {}".format(self.path))
            self.path = relpath(self.path.strip("/"), getcwd())
            # self.log("UPDATED  : {}".format(self.path))
            return http.server.SimpleHTTPRequestHandler.do_GET(self)

    def get_database_json( self ):
        dbj = [] 
        if   self.viewer == "explorer": 
            for db in self.databases:
                dbj.append({    "name": db,
                                "directory": db 
                           })
        elif self.viewer == "view":
            dbs = []
            for db in self.databases:
                dbs.append( {
                                "name": db,
                                "location": db
                            }
                          )
            dbj.append({    
                        "database_name": db,
                        "datasets": dbs,
                       })
        elif self.viewer == "simple": 
            for db in self.databases:
                dbj.append(db)

        return json.dumps(dbj, indent=4)

def run_cinema_server( viewer, rundir, databases, port, assetname="FILE", verbose=False, silent=False):
    localhost = "http://127.0.0.1"

    if (verbose):
        print("Running cinema server:")
        print("    rundir   : {}".format(rundir))
        print("    viewer   : {}".format(viewer))
        print("    databases: {}".format(databases))
        print("    port     : {}".format(port))
        print("    assetname: {}".format(assetname))
        print("")

    expanded_rundir = path.expanduser(rundir)
    fullpath = path.abspath(expanded_rundir)
    if verify_cinema_databases(fullpath, databases) :
        chdir(fullpath)
        set_install_path()
        cin_handler = CinemaSimpleRequestHandler
        cin_handler.verbose   = verbose
        cin_handler.viewer    = viewer
        cin_handler.assetname = assetname
        cin_handler.databases = databases
        socketserver.TCPServer.allow_reuse_address = True
        with socketserver.TCPServer(("", port), cin_handler) as httpd:
            urlstring = "{}:{}".format(localhost, port)
            if not silent:
                print(urlstring)
            try:
                httpd.serve_forever()
            except( KeyboardInterrupt, Exception ) as e:
                if not silent:
                    print(str(e))

