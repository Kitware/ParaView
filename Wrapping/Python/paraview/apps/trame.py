import paraview.web.venv  # noqa
import importlib
import os
import sys

TRAME_APP = None

if "--trame-app" in sys.argv:
    base_index = sys.argv.index("--trame-app")
    TRAME_APP = sys.argv[base_index + 1]
    # remove --trame-app TRAME_APP from args
    sys.argv.pop(base_index)
    sys.argv.pop(base_index)
else:
    TRAME_APP = os.environ.get("TRAME_APP", None)


def main():
    if TRAME_APP is None:
        print("Could not find environment variable TRAME_APP or --trame-app arg")
        return

    module = importlib.import_module(TRAME_APP)
    module.main()


if __name__ == "__main__":
    main()
