# import to process args
import os

try:
    import argparse
except ImportError:
    # since  Python 2.6 and earlier don't have argparse, we simply provide
    # the source for the same as _argparse and we use it instead.
    import _argparse as argparse

# import paraview modules.
from paraview import web, paraviewweb_wamp, paraviewweb_protocols

# Setup global variables
directoryToList = None

# Define ParaView Protocol
class PipelineManager(paraviewweb_wamp.ServerProtocol):

    def initialize(self):
        global directoryToList
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebMouseHandler())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebViewPort())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebViewPortImageDelivery())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebViewPortGeometryDelivery())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebTimeHandler())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebPipelineManager())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebFileManager(directoryToList))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="ParaView/Web Pipeline Manager web-application")
    web.add_arguments(parser)
    parser.add_argument("--data-dir", default=os.getcwd(),
        help="path to data directory to list", dest="path")
    args = parser.parse_args()
    directoryToList = args.path

    web.start_webserver(options=args, protocol=PipelineManager)
