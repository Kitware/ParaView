import sys
from pathlib import Path

# Add current dir in the Python Path so import can work.
# This is not necessary if this directory is already under PYTHONPATH
current_dir = Path(__file__).resolve().parent
sys.path.append(str(current_dir))

# import the proxies
from my_module import MyPythonSource, MyPythonFilter
