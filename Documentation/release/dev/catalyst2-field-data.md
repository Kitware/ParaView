# Support Field Data in Catalyst 2

Catalyst now supports field data on mesh.
In the conduit node, the fields should be defined under the `state/fields` node of the mesh.
The name of the sub-node will be the name of the field array. String value and numeric array, including [MCArray Blueprint](https://llnl-conduit.readthedocs.io/en/latest/blueprint_mcarray.html), are supported.
