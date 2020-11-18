import argparse, json, os, os.path, sys

def splitall(path):
    allparts = []
    while 1:
        parts = os.path.split(path)
        if parts[0] == path:  # sentinel for absolute paths
            allparts.insert(0, parts[0])
            break
        elif parts[1] == path: # sentinel for relative paths
            allparts.insert(0, parts[1])
            break
        else:
            path = parts[0]
            allparts.insert(0, parts[1])
    return allparts


def find_subdirectory(subdir, node):
    contents = node["contents"]
    for item in contents:
        if item["type"] == "directory" and item["name"] == subdir:
            return item
    raise RuntimeError("Missing subdir '%s'" % subdir)

def find_directory(subdir, node):
    if subdir == '.' or subdir == None:
        return node
    for part in splitall(subdir):
        node = find_subdirectory(part, node)
    return node

def get_baseline(tname, path, image, root):
    import re
    filename = os.path.join(path, image)
    relpath = os.path.relpath(filename, root)
    return tname + "_" + re.sub('[^0-9a-zA-Z.]+', '_', relpath)

parser = argparse.ArgumentParser(\
        description="Validate Extrats generated in tests")

parser.add_argument("--name", type=str, required=True,
    help="name of the test being validated")
parser.add_argument("--subdir", type=str, default=None,
    help="sub directory to validate")
parser.add_argument("--root", type=str, required=True,
    help ="root directory with results")
parser.add_argument("--json", type=str, required=True,
    help="validation json file")
parser.add_argument("--baseline-dir", type=str, default=None,
    help="directory for baselines for regression tests, if any")
parser.add_argument("--temp-dir", type=str, default="/tmp",
    help="directory for results for regression test failures, if any")

args = parser.parse_args()

with open(args.json, 'r') as f:
    data = json.load(f)[0]

node = data
test_dir = args.root
if args.subdir is not None:
    node = find_directory(args.subdir, node)
    test_dir = os.path.join(args.root, args.subdir)

# Check that directory exists since `os.walk()` does not fail
# for non extant directories.
if not os.path.isdir(test_dir):
    raise RuntimeError("Missing directory: '%s'" % test_dir)

# Validate directory structure.
regressions_failure_count = 0
for dirpath, dirs, files in os.walk(test_dir):
    # skip hidden files and directories
    # ref: https://stackoverflow.com/questions/13454164/os-walk-without-hidden-folders
    files = [f for f in files if not f[0] == '.']
    dirs[:] = [d for d in dirs if not d[0] == '.']

    relpath = os.path.relpath(dirpath, test_dir)
    currentnode = find_directory(relpath, node)
    if not currentnode:
        raise RuntimeError("Unexpected directory was found: '%s'" % dirpath)

    dirs = set(dirs)
    files = set(files)
    expected_dirs = set()
    expected_files = set()

    regression_test = []
    for item in currentnode["contents"]:
        if item["type"] == "directory":
            expected_dirs.add(item["name"])
        elif item["type"] == "file":
            expected_files.add(item["name"])
            if item.get("compare", False):
                regression_test.append(item["name"])

    if expected_dirs != dirs:
        missing = expected_dirs - dirs
        unexpected = dirs - expected_dirs
        raise RuntimeError(\
            "Mismatched directories under '%s' found.\n"\
            "Missing : %s\n" \
            "Unexpected: %s" % (dirpath, missing, unexpected))

    if expected_files != files:
        missing = expected_files - files
        unexpected = files - expected_files
        raise RuntimeError(\
            "Mismatched files under '%s' found.\n"\
            "Missing : %s\n" \
            "Unexpected: %s" % (dirpath, missing, unexpected))

    if regression_test:
        from vtkmodules.vtkTestingRendering import vtkTesting
        t = vtkTesting()
        for image in regression_test:
            t.CleanArguments()
            t.AddArgument("-V")
            baseline_name = get_baseline(args.name, dirpath, image, args.root)
            t.AddArgument(os.path.join(args.baseline_dir, baseline_name))
            t.AddArgument("-T")
            t.AddArgument(args.temp_dir)
            if t.RegressionTest(os.path.join(dirpath, image), 15) != t.PASSED:
                regressions_failure_count += 1
                print("\n")
if regressions_failure_count:
    raise RuntimeError("ERROR: %d regression tests failed!" % regressions_failure_count)
