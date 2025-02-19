# This demonstrates how to access Python defined Proxies

from paraview.simple import *

from pathlib import Path

current_file = Path(__file__).resolve()
plugin_path = current_file.parents[1] / 'complex_python_plugin.py'

# ns=globals() puts the plugin proxies as class of current namespace
LoadPlugin(str(plugin_path), ns=globals())

my_source = MyPythonSource(ThetaResolution = 24)

print("source has Value: ", my_source.Value)
range = my_source.GetPropertyValue("ValueRangeInfo")
print("expected from XML default: ", (range[1] - range[0]) / 2)

my_filter = MyPythonFilter()

# Enumeration properties can use either displayed text or value
if my_filter.DerivatedMethod == 'Gradient':
    print ("DerivatedMethod has its default value. Change it to Cumulative Sum.")
    my_filter.DerivatedMethod = 1

my_filter.UpdatePipeline()

# display
GetActiveViewOrCreate("RenderView")
Show()
Render()

ColorBy(value=["POINTS", "cumsum"])

Render()
