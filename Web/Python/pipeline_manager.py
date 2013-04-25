# import to process args
import os

# import paraview modules.
from paraview import web, paraviewweb_wamp, paraviewweb_protocols

try:
    import argparse
except ImportError:
    # since  Python 2.6 and earlier don't have argparse, we simply provide
    # the source for the same as _argparse and we use it instead.
    import _argparse as argparse

# =============================================================================
# Create custom Pipeline Manager class to handle clients requests
# =============================================================================

class PipelineManager(paraviewweb_wamp.ServerProtocol):

    dataDir = None
    authKey = "paraviewweb-secret"

    def initialize(self):
        # Bring used components
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebMouseHandler())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebViewPort())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebViewPortImageDelivery())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebViewPortGeometryDelivery())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebTimeHandler())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebPipelineManager())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebFileManager(PipelineManager.dataDir))

        # Update authentication key to use
        self.updateSecret(PipelineManager.authKey)

# =============================================================================
# Main: Parse args and start server
# =============================================================================

if __name__ == "__main__":
    # Create argument parser
    parser = argparse.ArgumentParser(description="ParaView/Web Pipeline Manager web-application")

    # Add default arguments
    web.add_arguments(parser)

    # Add local arguments
    parser.add_argument("--data-dir", default=os.getcwd(), help="path to data directory to list", dest="path")

    # Exctract arguments
    args = parser.parse_args()

    # Configure our current application
    PipelineManager.authKey = args.authKey
    PipelineManager.dataDir = args.path

    # Start server
    web.start_webserver(options=args, protocol=PipelineManager)
