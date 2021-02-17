import http.server
import socketserver
from urllib.parse import urlparse
from urllib.parse import parse_qs
from os import chdir
from os import listdir
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

def verify_cinema_database( viewer, cdatabase, assetname ):
    result = False

    if viewer == "view":
        # this is the default case
        if assetname is None:
            assetname = "FILE"

        db = cdb.cdb(cdatabase)
        db.read_data_from_file()

        if db.parameter_exists(assetname):
            result = True
        else:
            print("")
            print("ERROR: Cinema viewer \'view\' is looking for a column named \'{}\', but the".format(assetname)) 
            print("       the cinema database \'{}\' doesn't have one.".format(cdatabase)) 
            print("")
            print("       use \'--assetname <name>\' where <name> is one of these possible values") 
            print("       that were found in \'{}\':".format(cdatabase))
            print("")
            print("           \"" + ' '.join(db.get_parameter_names()) + "\"")
            print("")
    else:
        result = True

    return result 

#
# CinemaSimpleReqestHandler
#
# Processes GET requests to find viewers and databases
#
class CinemaSimpleRequestHandler(http.server.SimpleHTTPRequestHandler):

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
            if not path.isdir(self.base_path):
                self.log("ERROR")
                self.path = ServerInstallPath + "/error_no-database.html"
                return http.server.SimpleHTTPRequestHandler.do_GET(self)

            elif self.viewer == "explorer": 
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
                json = "{{\"assetname\" : \"{}\"}}".format(self.assetname)
                self.send_response(200)
                self.send_header("Content-type", "text/html")
                self.send_header("Content-length", len(json))
                self.end_headers()
                self.wfile.write(str.encode(json))
            return

        if self.path.endswith("databases.json"):
            self.log("DATABASES   : {}".format(self.path))
            json = self.create_database_list()

            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.send_header("Content-length", len(json))
            self.end_headers()
            self.wfile.write(str.encode(json))
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

    def create_database_list( self ):
        json_string = ""
        if self.viewer == "view":
            dbs = [self.base_path]
            cdbname = path.basename(dbs[0])
            json_string = "[{{ \"database_name\": \"{}\", \"datasets\": [ ".format(cdbname)

            for db in dbs:
                cdbname = path.basename(db)
                json_string += "{{ \"name\" : \"{}\", \"location\" : \"{}\" }},".format(cdbname, db)

            # remove the last comma
            json_string = json_string[:-1]
            # close the string
            json_string += "]}]"

        elif self.viewer == "explorer":
            dbs = [self.base_path]
            cdbname = path.basename(dbs[0])
            json_string = "["

            for db in dbs:
                cdbname = path.basename(db)
                json_string += "{{ \"name\" : \"{}\", \"directory\" : \"{}\" }},".format(cdbname, db)

            # remove the last comma
            json_string = json_string[:-1]
            # close the string
            json_string += "]"

        else:
            self.log("ERROR: invalid view type {}".format(this.viewer))

        return json_string 

def run_cinema_server( viewer, data, port, assetname="FILE"):
    localhost = "http://127.0.0.1"

    fullpath  = path.abspath(data)
    datadir   = path.dirname(fullpath)
    cinemadir = path.basename(fullpath)

    chdir(datadir)

    if verify_cinema_database(viewer, cinemadir, assetname) :
        set_install_path()
        cin_handler = CinemaSimpleRequestHandler
        cin_handler.base_path = cinemadir
        cin_handler.verbose   = False
        cin_handler.viewer    = viewer
        cin_handler.assetname = assetname
        with socketserver.TCPServer(("", port), cin_handler) as httpd:
            urlstring = "{}:{}".format(localhost, port)
            print(urlstring)
            httpd.serve_forever()

