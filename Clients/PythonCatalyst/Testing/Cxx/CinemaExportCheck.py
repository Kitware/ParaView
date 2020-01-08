#!/usr/bin/env python
import sys
import argparse
import os

# ----------------------------------------------------------------------------
#
# parse command line options
#
# ----------------------------------------------------------------------------

helptext = "\n\
\n\
examples: \n\
\n\
    CinemaExportCheck \n\
       check cinema export datasets \n\
  \n\
"

# normal option parsing
parser = argparse.ArgumentParser(
            description="a cinema database comparison tool",
            epilog=helptext,
            formatter_class=argparse.RawDescriptionHelpFormatter )

# parser.add_argument( "year", nargs="?", help="run reports for a single year" )

parser.add_argument(  "-b", "--batch",
                    dest="batch",
                    default=None,
                    help="batch dataset")

parser.add_argument(  "-i", "--interactive",
                    dest="interactive",
                    default=None,
                    help="interactive dataset")

args = parser.parse_args()



print("----------------------------------------------------------")
print("Running CinemaExportCheck")

if ((args.interactive != None) and (args.batch != None)):
    print("  comparing ...")
    print("    batch       data: {}".format(args.batch))
    print("    interactive data: {}".format(args.interactive))
    # TODO: Actually check if they are the same
    #   - In the past, we diff'ed the two and checked to see if they opened in cinema_explorer
    #   - Create a baseline data.csv that we know works and check against that?
else:
    print("  ERROR: incorrect number of arguments")
    exit(1)

print("Completed CinemaExportCheck")
print("----------------------------------------------------------")
