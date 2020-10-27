# called when this package is directly invoked.

import argparse

parser = argparse.ArgumentParser("paraview.apps",
        description="ParaView Applications",
        epilog="""This package provides access to ParaView applications.
Launch a specific application using '-m <appname'. For example:
'pvpython -m paraview.apps.visualizer'. Available applications can be listed
using the '-l' or '--list'.""")
parser.add_argument("-l", "--list",
        help="list available applications",
        action="store_true")

args = parser.parse_args()
if args.list:
    from . import divvy
    from . import flow
    from . import glance
    from . import lite
    from . import visualizer
    print("Available applications:")
    if divvy.is_supported():
        print("  divvy")
    if flow.is_supported():
        print("  flow")
    if glance.is_supported():
        print("  glance")
    if lite.is_supported():
        print("  lite")
    if visualizer.is_supported():
        print("  visualizer")
