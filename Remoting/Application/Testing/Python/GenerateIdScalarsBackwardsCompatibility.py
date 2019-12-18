from paraview.simple import *
from paraview import NotSupportedException

w = Wavelet()
ids = GenerateIds()
ids.PointIdsArrayName = "pid"
ids.CellIdsArrayName = "cid"

try:
    foo = ids.ArrayName
except NotSupportedException:
    pass
else:
    raise RuntimeError("`NotSupportedException` expected!")

try:
    ids.ArrayName = "foo"
except NotSupportedException:
    pass
else:
    raise RuntimeError("`NotSupportedException` expected!")

# Now switch backwards compatibility to 5.6
paraview.compatibility.major = 5
paraview.compatibility.minor = 6

assert (ids.ArrayName.GetData() == "pid"), "`ArrayName` must return point ids array name"

ids.ArrayName = "ids"
assert (ids.PointIdsArrayName == "ids"), "`ArrayName` must set PointIdsArrayName"
assert (ids.CellIdsArrayName == "ids"), "`ArrayName` must set CellIdsArrayName"

xml = """
<ParaView>
  <ServerManagerState version="5.6.0">
    <Proxy group="sources" type="RTAnalyticSource" id="7996" servers="1">
    </Proxy>
    <Proxy group="filters" type="GenerateIdScalars" id="8397" servers="1">
      <Property name="Input" id="8397.Input" number_of_elements="1">
        <Proxy value="7996" output_port="0"/>
      </Property>
      <Property name="ArrayName" id="8397.ArrayName" number_of_elements="1">
        <Element index="0" value="ids"/>
      </Property>
    </Proxy>
    <ProxyCollection name="sources">
      <Item id="8397" name="sGenerateIds1" />
      <Item id="7996" name="sWavelet1" />
    </ProxyCollection>
  </ServerManagerState>
</ParaView>
"""

from paraview import smtesting
smtesting.ProcessCommandLineArguments()

import os.path
fname = os.path.join(smtesting.TempDir, "generateidscalarsbackwardscompatibility.xml")
with open(fname, "w") as f:
    f.write(xml)

LoadState(fname)
sids = FindSource("sGenerateIds1")
assert (sids.PointIdsArrayName == "ids"), "PointIdsArrayName must be set from `ArrayName`"
assert (sids.CellIdsArrayName == "ids"), "CellIdsArrayName must be set from `ArrayName`"

from os import remove
remove(fname)
