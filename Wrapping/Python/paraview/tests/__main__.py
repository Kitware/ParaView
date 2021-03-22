greeting = """
ParaView mini-test suite
=========================

This is a collection of verification and validate tests that
confirm that few core capabilities are working in this build.
The suite consists of several individual tests that can be launched
on their own as follows:

   pvpython -m paraview.tests.verify_eyedomelighting --output /tmp/result.png

Each tests supports a '--help' option that can be used to obtain list of
available options supported by that particular test.

This root package can be used to launch all the tests one after another.

   pvpython -m paraview.tests
            --output_directory /tmp/outputs
            --baseline_directory /tmp/baselines
"""


import argparse, textwrap, os, os.path
from .. import print_info as log

parser = argparse.ArgumentParser(
        prog="paraview.tests",
        description=textwrap.dedent(greeting),
        formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument("-i", "--interactive", help="enable interaction", action="store_true")
parser.add_argument("-o", "--output_directory", help="output directory", type=str)
parser.add_argument("-v", "--baseline_directory", help="baseline directory (for comparison)", type=str)

def single_yes_or_no_question(question, default_no=True):
    choices = ' [y/N]: ' if default_no else ' [Y/n]: '
    default_answer = 'n' if default_no else 'y'
    reply = str(input(question + choices)).lower().strip() or default_answer
    if reply[0] == 'y':
        return True
    if reply[0] == 'n':
        return False
    else:
        return False if default_no else True

def main(opts):
    import importlib
    tests = [ "verify_eyedomelighting", "basic_rendering" ]

    if opts.output_directory:
        os.makedirs(opts.output_directory, exist_ok=True)

    for tname in tests:
        if opts.interactive and not single_yes_or_no_question("Run test '%s'" % tname):
            break

        targs = []
        if opts.interactive:
            targs.append("-i")
        if opts.output_directory:
            targs.append("-o")
            targs.append(os.path.join(opts.output_directory, tname + ".png"))
        if opts.baseline_directory:
            targs.append("-v")
            targs.append(os.path.join(opts.baseline_directory, tname + ".png"))

        log("start '%s'" % tname)
        tmodule = importlib.import_module(".%s" % tname, __package__)
        log(textwrap.wrap(tmodule.__doc__, width=30))
        tmodule.main(targs)
        log("done '%s'" % tname)

args = parser.parse_args()
main(args)
