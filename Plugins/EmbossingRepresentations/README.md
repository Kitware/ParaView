EmbossingRepresentations plugin for ParaView
===========================================

by M.Migliore & J.Pouderoux, Kitware SAS 2017

This work was supported by the German Climate Computing Center (DKRZ).

Description
-----------

This plugin provides two new representations called "Bump Mapped Surface" and
"Extrusion Surface" in ParaView.
The "Bump Mapped Surface" computes normals in fragment shader based on variation
of selected scalar point data.
The "Extrusion Surface" behaves differently depending on the type of selected
data. If the selected data is a point data, the representation is similar to a
height field representation, a displacement is applied on vertex according the
point value.
If the selected data is a cell data, the cell is extruded along its normal, the
length depending on the value in the cell data.
The UI panel (ie. proxy) allows to specify:

* Data for each representation.
* A factor to apply to values of data for each representation.
  Supports negative values to reverse direction.
* A checkbox "Draw basis" for "Extrusion Surface" to optionally draw initial cells.
* A checkbox "Normalize data" which convert values to [0;1] range.
* If the last option is enabled, a new checkbox "Auto scaling" is available, which normalize values
  based on the minimum and maximum values of the current timestep.
* Is "Auto scaling" is disabled, one can specify a range of values.

How to test
-----------

Load the plugin "EmbossingRepresentations".
Create/load a dataset. Set the "Bump Mapped Surface" or "Extrusion Surface"
representation modes.
