# called when this package is directly invoked.

import argparse

def _get_options():
    parser = argparse.ArgumentParser("paraview.apps",
                                     description="ParaView Applications",
                                     epilog="""This package provides access to ParaView applications.
Launch a specific application using '-m <appname'. For example:
'pvpython -m paraview.apps.trame'. Available applications can be listed
using the '-l' or '--list'.""")
    parser.add_argument("-l", "--list",
                        help="list available applications",
                        action="store_true")

    return parser

if __name__ == '__main__':
    parser = _get_options()
    args = parser.parse_args()
    if args.list:
        from . import glance

        print("Available applications:")
        if glance.is_supported():
            print("  glance")

        print("  packages [--format requirements]")
