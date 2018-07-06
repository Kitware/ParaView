GmshReader Plugin
ParaView/VTK reader for visualization of high-order polynomial solutions under the Gmsh format.
Version: 1.0

See Copyright.txt, License.txt and Credits.txt for respective copyright,
license and authors information. You should have received a copy of
these files along with ParaViewGMSHReaderPlugin.

-For more information on Gmsh see http://geuz.org/gmsh/
-For more information on ParaView see http://paraview.org/

Contact Info:
------------

This plugin can certainly be improved and any suggestions or contributions are welcome.
Please report all bugs, problems and suggestions to <michel.rasquin@cenaero.be>,
<joachim.pouderoux@kitware.com> and <mathieu.westphal@kitware.com>.

Pre-requirements for building GmshReader Plugin:
----------------------------------------------------

To build this plugin you will need a version of the Gmsh library compiled with specific options.
The Gmsh version required by this plugin must be at least 3.0.7.
The Gmsh source code can be downloaded from http://gitlab.onelab.info/gmsh/gmsh.git.
For more information on Gmsh, see also http://gitlab.onelab.info/gmsh/gmsh/wikis/home.

The recommended cmake command for the configuration of Gmsh as an external library with the minimum components required for this plugin is the following:

cmake \
-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_INSTALL_PREFIX=/path_to_gmsh_install_dir/ \
-DENABLE_BUILD_SHARED=ON \
-DENABLE_BUILD_LIB=ON \
-DDEFAULT=0 \
-DENABLE_POST=ON \
-DENABLE_PARSER=ON \
-DENABLE_PRIVATE_API=ON \
../gmsh-source

Then, build and install the library with for instance:

make -j 8 install

This configuration should keep the memory requirement for the Gmsh library as low as possible.
Beware: the default Gmsh library provided by some Linux distributions under their software package manager, or simply built with default cmake variables may include additional components that are not compatible with this plugin.
It is strongly advised to build your own version of the Gmsh library using the cmake variables listed above.
Please contact us if you can't build your own version with the cmake command above.

Building GmshReader Plugin at compilation of ParaView:
------------------------------------------------------

Configure ParaView's CMake with the following additional recommended options:

 -DPARAVIEW_BUILD_PLUGIN_GMSHReader=ON
 -DGmsh_INCLUDE_DIR=/path_to_gmsh_install_dir/include/gmsh
 -DGmsh_LIBRARY=/path_to_install_dir/lib64/libgmsh.so

where Gmsh_INCLUDE_DIR points to the Gmsh include directory where all related header files are located, and Gmsh_LIBRARY points to the Gmsh library.

Note that if you do not specify the cmake variables Gmsh_INCLUDE_DIR and Gmsh_LIBRARY, a cmake find module will try to find Gmsh for you.
In this case, it is advised though to help cmake find the paths to the right version of the Gmsh library and include directory by setting CMAKE_PREFIX_PATH for instance.


Loading the ParaView Gmsh reader plugin:
---------------------------------------

First make sure that libgmsh.so will be found at runtime, this will be automatic if you have set a standard CMAKE_INSTALL_PREFIX for gmsh
Alternativaly, you can set LD_LIBRARY_PATH to the directory containing libgmsh.so.

Open up ParaView, from the "Tools" menu, then choose "Manage Plugins".
You should see "GmshReader" in the client/GUI and, if available, server panel.
Selected it and click on "Load Selected".
You may want to expand it and check the "Auto Load" box.
This plugin is a client and server plugin, so make sure to load it in both in client and server mode.

Note on the MSH file format:
---------------------------

This plugin has been tested with Gmsh format 2.0.
Under Gmsh format version 2.0, a partitioned mesh with N parts can be saved in

 1. either one single .msh file (in this case, the lines storing the connectivity information for each element will also include the partition ID of this element);
 2. N distinct .msh files corresponding to one file per mesh part.
See http://geuz.org/gmsh/doc/texinfo/gmsh.pdf for more details.

To save a partitioned mesh in N distinct files from the Gmsh GUI, follow this procedure:

 * save as
 * select "Mesh - Gmsh msh" format
 * check "Save all" and "Save one file per partition".

Reading your msh files:
----------------------

Once the Gmsh plugin is loaded, you should now be able to select and load an XML interface file with the extension ".mshi" file (MSH Input).
This XML file specifies all the information required by the plugin.
For a demo xml file, configure ParaView with BUILD_TESTING set to ON, then build and check in your build directory ExternalData/Testing/Data/Gmsh/viz_naca0012.mshi and viz_naca0012_meshonly.mshi, along with the corresponding msh data.

The format of this xml file is the following:

<GmshMetaFile number_of_partitioned_msh_files="4">
<GeometryInfo has_piece_entry="1"
              has_time_entry="0"
              pattern="./naca0012/naca_4part.msh_%06d"/>
<FieldInfo has_piece_entry="1"
           has_time_entry="1">
           <PatternList pattern="./naca0012/domainPressure_t%d.msh_%06d"/>
           <PatternList pattern="./naca0012/domainVelocity_t%d.msh_%06d"/>
</FieldInfo>
<TimeSteps number_of_steps="3"
           auto_generate_indices="1"
           start_index="4"
           increment_index_by="2"
           start_value="0."
           increment_value_by="0.01"/>
<AdaptView adaptation_level="2"
           adaptation_tol="-0.001"/>
</GmshMetaFile>

The meaning of these XML tags is the following:

 * The GmshMetaFile has only one attribute so far:
    1. number_of_partitioned_msh files: In how many .msh files has your mesh and/or solution been partitioned into?
For Gmsh format 2.0, this entry can only be either 1 or N for a mesh partitioned in N parts (see README.md).
When a mesh is partitioned in N parts but saved in a single file, the part ID of each element is then stored along with its connectivity information (see Gmsh documentation).

For faster IO performance with paraview, it is recommended to partition both the mesh and the solution files into N files, provided that the mesh itself has been partitioned into N parts as explained above.
That said, it is possible to still load a single mesh file along with N partitioned solution files in a pvserver running in parallel.
In this case, this xml variable should be set to N, although there is only a single mesh file.
However, each pvserver process will then have to read the single mesh file completely, adding some unnecessary load on the file system.

If a mesh has been partitioned in N parts but both the mesh and solution files have been saved in a single file, rank 0 of the pvserver will load all the data (mesh plus solution), which can be extremely slow and put a high pressure on the memory consumption if the data are heavy.
If this step succeeds, it is still possible to enjoy the parallelism of the pvserver by redistributing the data amongs the pvserver processes with the D3 filter.

Side note: The upcoming Gmsh format version 3.0 under development should rely on a parallel IO format which should allow more flexibility in terms of number of mesh part per files.

 * The GeometryInfo has three attributes:
    1. has_piece_entry (0 or 1): Specifies whether the path pattern to the mesh msh file has a file (piece) placeholder.
                             The piece placeholder ("%06d" in the example above) is automatically  by the update piece number.
                             Note that the current convention in Gmsh for file IDs includes always 6 digits
                             for better sorting and must be prepadded with 0 accordingly (use %06d instead of %d).
    2. has_time_entry (0 or 1): Specifies whether the pattern has a time placeholder.
                            The time placeholder ("%d" in the example above) is replaced by an index specified in the TimeSteps element below.
                            Use 0 unless the mesh changes with time step through adaptation for instance.
    3. pattern: This is the pattern used to access the Gmsh mesh, which includes the path
            (absolute or relative to the mshi file location) and the mesh filename(s).
            The %d and %06d placeholders in the example above will be replaced by appropriate indices following C++ convention.
            The first index is time (if specified), the second one is piece.

 * The FieldInfo element is optional. If not present, the gmsh mesh with no additional data will be displayed.
If present, FieldInfo contains at least three elements related to the solution data:
    1. has_piece_entry (0 or 1): same as above.
    2. has_time_entry (0 or 1): same as above.
    3. PatternList pattern: This is the pattern used to get the Gmsh solution, which includes the path
                        (absolute or relative to the mshi file location) and the solution filename(s).
                        There can be any arbitrary number of Gmsh solution filenames (velocity, pressure, etc).
                        In the examples above. two distinct sets of files are loaded (domainPressure* and domainVelocity*).
                        All the pfields included in these Gmsh files will be passed to ParaView.
                        As usual, the %d and %06d placeholders will be replaced by appropriate indices.
                        The first index is time (if specified), the second one is piece.

 * The TimeSteps element contains TimeStep sub-elements. Each TimeStep element specifies an index (index_attribute), an index used in the geometry filename pattern (geometry_index), an index used in the field filename pattern (field_index) and a time value (float).
    1. number_of_steps specifies how many steps of your solution you want to visualize. These steps can then be visualized sequentially with the ParaView play button (green arrow).
    2. Normally there is one TimeStep element per timestep. However, it is possible to ask the reader to automatically generate timestep entries. This is done by setting the (optional) auto_generate_indices to 1. This is the usual and recommended mode for constant intervals between successive time steps. In this case, the reader will generate number_of_steps entries.
    3. The geometry_index and field_index of these entries will start at start_index and will be incremented by increment_index_by.
    4. The time value of these entries will start at start_value and will be incremented by increment_value_by.

Note that it is possible to use a combination of both approaches.
Any values specified with the TimeStep elements will overwrite anything automatically computed.
A common use of this is to let the reader compute all indices for field files and overwrite the geometry indices with TimeStep elements.
In the above example, 3 time steps (#4, #6 and #8) are loaded.

 * The AdaptView element is a key feature for visualization of high-order polynomial solutions and it contains two elements:
    1. adaptation_level: this corresponds to the adaptatation/refinement level of the mesh for visualization purpose.
                     This enables the same adaptation features available in the Gmsh GUI.
                     Beware of the memory consumption which can increases quickly with the number of adaptation level.
                     It is recommended to start with 0 first.
                     Typically, nice visualizations of HO solutions are obtained with a value of 2 or 3 if memory allows it
    2. adaptation_tol: adaptation tolerance or error threshold for visualization, like in the Gmsh GUI.
                   If this value is <0, all elements are uniformly refined.
                   If there are more than one Gmsh solution files, only a negative value is accepted
                   since a positive value may lead to different adapted meshes for each field.
