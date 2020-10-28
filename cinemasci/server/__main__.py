import http.server
import socketserver
from urllib.parse import urlparse
from urllib.parse import parse_qs
# from os import path
# from os.path import relpath
# from os import getcwd
# from os import access
# from os import R_OK

from . import run_cinema_server

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="run a Cinema Viewer")
    parser.add_argument("--data", required=True, default=None, help="database to view (required)") 
    parser.add_argument("--viewer", required=True, default='explorer', help="viewer type to use. One of [explorer, view] (required)") 
    parser.add_argument("--assetname", default=None, help="asset name to use (optional)") 
    parser.add_argument("--port", type=int, default=8000, help="port to use (optional)") 
    args = parser.parse_args()

    run_cinema_server(args.viewer, args.data, args.port, args.assetname)
