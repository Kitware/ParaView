## SPDX Generation in module

ParaView now supports the Software Package Data Exchange (SPDX) standard for communicating
software bill of materials (SBOM) information. This standard allows for the accurate
identification of software components, explicit mapping of relationships between these
components, and the association of security and licensing information with each component.

See [](https://docs.vtk.org/en/latest/advanced/spdx_and_sbom.html).

To support the standard, each VTK module may be described by a `.spdx` file. Configuring
ParaView with the option `PARAVIEW_GENERATE_SPDX`, set to `ON` enables spdx generation
for each VTK module.

Generated SPDX files are based on the SPDX 2.2 specification and are named after `<ModuleName>.spdx`.
The generation of SPDX files is considered experimental and both the VTK Module system API and
the `SPDXID` used in the generated files may change.

SPDX information have been added and replace all previous copyright declaration in all ParaView
files. Please follow VTK recommandations on that regard.
