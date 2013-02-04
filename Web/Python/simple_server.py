r"""
Sample application demonstrating how to present any visualization pipeline for
consumption over the Web.
"""

import argparse
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
