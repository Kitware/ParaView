import sys

from paraview import smtesting

if not "pvpython" in sys.executable:
  raise smtesting.TestError(f"Wrong value for sys.executable : {sys.executable} does not contain 'pvpython'")
