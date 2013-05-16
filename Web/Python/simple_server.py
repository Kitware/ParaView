r"""
    This module is a ParaViewWeb server application.
    The following command line illustrate how to use it::

        $ pvpython .../simple_server.py

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

import sys

from paraview import simple, web

if __name__ == '__main__':
    # Setup the visualization pipeline.
    simple.Cone()
    simple.Show()
    simple.Render()

    # Start the web-server.
    web.start(
        description="""ParaView/Web sample visaulization web-application.""")
