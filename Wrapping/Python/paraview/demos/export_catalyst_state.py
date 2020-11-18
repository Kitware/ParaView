r"""
This script takes in any PVSM file and generates a Catalyst Python state file.
"""

import argparse

parser = argparse.ArgumentParser(\
    description="Generate Catalyst Python State")
parser.add_argument("--output", type=str,
    help="name for the output file (*.py)", required=True)
parser.add_argument("--extracts-dir", type=str,
    help="path to directory where to make the Catalyst script save generated extracts",
    required=True)
parser.add_argument("--cinema",
    help="generate Cinema specification summary for the extracts generated.",
    action="store_true")
parser.add_argument("--pvsm", type=str,
    help="pvsm state file to load", required=True)

args = parser.parse_args()

from paraview import simple, catalyst
simple.LoadState(args.pvsm)

options = catalyst.Options()
options.ExtractsOutputDirectory = args.extracts_dir
if args.cinema:
    options.GenerateCinemaSpecification = 1

# TODO: add a 'simple' version for this, we don't want users to
# import anything from 'detail' and hence we don't want that in a demo
# script.
import os, os.path
# remove output if it exists
if os.path.exists(args.output):
    print("removing existing output file:", args.output)
    os.remove(args.output)

from paraview.detail.catalyst_export import save_catalyst_state
save_catalyst_state(args.output, options)

assert os.path.exists(args.output)
