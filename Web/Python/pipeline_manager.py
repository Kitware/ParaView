r"""
    This module is a ParaViewWeb server application.
    The following command line illustrate how to use it::

        $ pvpython .../pipeline_manager.py --data-dir /.../path-to-your-data-directory

    --data-dir is used to list that directory on the server and let the client choose a file to load.

    Any ParaViewWeb executable script come with a set of standard arguments that
    can be overriden if need be::

        --port 8080
             Port number on which the HTTP server will listen to.

        --content /path-to-web-content/
             Directory that you want to server as static web content.
             By default, this variable is empty which mean that we rely on another server
             to deliver the static content and the current process only focus on the
             WebSocket connectivity of clients.

        --authKey paraviewweb-secret
             Secret key that should be provided by the client to allow it to make any
             WebSocket communication. The client will assume if none is given that the
             server expect "paraviewweb-secret" as secret key.

"""

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

class _PipelineManager(paraviewweb_wamp.ServerProtocol):

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
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebRemoteConnection())
        self.registerParaViewWebProtocol(paraviewweb_protocols.ParaViewWebFileManager(_PipelineManager.dataDir))

        # Update authentication key to use
        self.updateSecret(_PipelineManager.authKey)

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
    _PipelineManager.authKey = args.authKey
    _PipelineManager.dataDir = args.path

    # Start server
    web.start_webserver(options=args, protocol=_PipelineManager)
