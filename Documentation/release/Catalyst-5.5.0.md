ParaView Catalyst 5.5.0 Changes
===============================

The big difference in ParaView Catalyst for ParaView 5.5 will be how Catalyst Python scripts are generated in the graphical user interface (GUI). With all of the functionality that’s gone into Catalyst, the script generator plugin in the GUI organically grew to the point that we needed to do a full rework to simplify the interface. While the changes in ParaView 5.5 are not radical, they are the first steps toward a much better setup. This setup should be completed in the next release of ParaView.

The most significant change is the move of the functionality of the Catalyst script generator plugin to the Catalyst GUI menu. With this change, the plugin won’t have to be loaded in order to generate Catalyst scripts. If you build ParaView with Python support, the GUI will automatically include the menu item to add in data extract writers to the Catalyst pipeline and the menu item to generate the Catalyst Python script. Both of these menu items will be available under the Catalyst menu instead of under the CoProcessing and Writers menus, which are added to the GUI when the Catalyst Script Generator plugin is loaded. Note that the menu items will still only be available when ParaView is built with Python wrapping enabled.

Another change will enable all the filters that you can use. Previously, if ParaView uses the built-in server or a separate pvserver that runs with a single process, only the “serial” filters are made available in the Filters menu. These filters exclude `Process Id Scalars` and `Ghost Cells Generator`. While `Process Id Scalars` and `Ghost Cells Generator` aren’t normally useful in serial, they will be available in the Filters menu, in case you want to save a state file, generate a Python script or create a Catalyst Python script that will be used later in a parallel operation.

The next change will affect the naming of Catalyst simulation inputs. Simulation inputs allow the adaptor to build only what Catalyst needs to perform proper in situ processing. An example use case is a Particle-In-Cell simulation, where there are two logical data types: a grid-based data type that is used for continuous, distribution type fields (e.g., an electric field) and a particle-based data type that is used for discrete type fields (e.g., the momentum for each particle). By implementing Catalyst channels, the adaptor will be able to fully provide both the cell-based information and the particle-based information. As a result, when outputting iso-surfaces of the cell-based information, the particle-based information will never need to be constructed. This will save both computation and memory.

An additional change relates to the adaptor. Currently, when the adaptor provides more than a single channel, the Catalyst pipeline needs to identify the appropriate channel, according to the channel input name. As a result, when you construct a Catalyst pipeline through the GUI, you have to specify what inputs were needed by this same channel name. In ParaView 5.5, Catalyst will automatically add the channel name as field information when it gets a dataset from the adaptor. When Catalyst produces a data extract, the GUI will know from which channel that data extract originated. Keep in mind that a pipeline can request datasets for all channels in a combined analysis and visualization. With the change, multiple datasets will be read into the GUI and processed together.

# Catalyst Examples

We are continuing to add to and improve the Catalyst examples, which are available in the Examples/Catalyst subdirectory of the source code. These examples have been invaluable for getting started and instrumenting code with Catalyst. They also help to demonstrate Catalyst functionality under different conditions. The following Catalyst examples will be added in ParaView 5.5:

## CxxGhostCellsExample

This example shows how ghost cells need to be marked in the adaptor for simulations that use ghost cell information. The number of ghost cell levels generated in the adaptor can be controlled with the -l command-line option. By default, the adaptor will produce an unstructured grid (i.e., `vtkUnstructuredGrid`), but the -i command line option will make the adaptor generate a Cartesian grid (i.e., `vtkImageData`). This example is also useful for simplified parallel testing of Catalyst output, when the adaptor produces ghost cells.

## CxxMultiChannelInputExample

This example shows how multiple channels, or inputs, can be provided by the adaptor. The example is meant to mimic a Particle-In-Cell simulation where there is an underlying Cartesian grid—identified with the “volumetric grid” channel name—and a set of particles stored in a polydata—identified with the “particles” channel name. As discussed above, Catalyst pipelines can output results based on particle information only, volumetric grid information only and/or combined grid inputs. The adaptor only produces the grids that the Catalyst pipelines need at each time step.

# Overall Improvements

With the growth of the Catalyst user base, it’s become even more important for us to improve the production quality of Catalyst through bug fixes and general improvements.

For ParaView 5.5, bug fixes will do the following:

* Allow Catalyst Live table sources to be displayed as text in the render view for the **Annotate Time** source or the **Annotate Time** filter.

* Fix the time issue with Fit to Screen for image output.

When using the Fit to Screen option, if you first set the view time, it will fix the issue.

* Fix the **Annotate Attribute** filter to work with `vtkStringArray`.

* Fix the issue with multiple input channels.

Currently, 1) if Catalyst Live is enabled; 2) there are multiple input channels provided by the simulation adaptor; and 3) an output screenshot is used for both input channels, then not all of the input channels are properly requested from the adaptor.

* Fix an issue that occurs when you force pipeline output.

When you force output, the `CatalystRequestDataDescription()` method will mark that all channels/inputs are needed.

* Move client-server XML for OpenGL2 pieces to `views_and_representations.xml`.

 Since we don't have the OpenGL1 option anymore in ParaView, we can move the definitions of specialized proxies in `proxies_opengl2.xml` to `views_and_representations.xml`. Consequently, the "Extension" information for representations in `proxies_opengl2.xml` will be directly placed in the proper spot in `view_and_representations.xml`.

* Remove unnecessary files from the generated Catalyst edition source trees.

* Enable catalyzing from a release version of ParaView.

 Currently, catalyze.py does a `git describe` to successfully obtain version information from ParaView. As a result of the bug fix, the version information will come from version.txt, if you are not using a ParaView Git repo, which handles the case for release versions from a tarball.

* Fix the auto rescale data range.

* Fix an issue in which ParaView is not properly imported for Cinema error reporting.

 General improvements will do the following:

* Change the output filename for `gridwriter.py` to `input_%t.<extension>`.

* Enable the installation of development files for Catalyst editions.

 Presently, if you install a Catalyst edition, you may want development files to be included in the install. This change will enable you to install development files as the default during the Catalyst configuration process.

* Improve the `allinputsgridwriter.py` Catalyst example script.

 Right now, you have to specify the names of all of the inputs in order for the `allinputsgridwriter.py` script to properly work.

* Add the capability to load Cinema spec A databases in ParaView.

# Catalyst Edition Improvements

 While you may use a full ParaView build, especially for development work, the Catalyst editions provide you with a slimmed-down library that can be significantly easier to build. Catalyst editions improvements in ParaView 5.5 do the following:

* Enhance support for generating Catalyst editions through the ParaView superbuild.

 If the Ninja generator is used for the ParaView superbuild, it will now be passed to the Catalyst edition build from the ParaView superbuild.

* Add a Catalyst volume rendering edition.

 Add several filters to each of the Base, Essentials and Extras Catalyst editions for ParaView’s `server-manager` wrapping.

 These filters were already available in the corresponding Catalyst edition’s source code, but they aren’t available directly in Catalyst. Menno Deij-van Rijswijk of the [Maritime Research Institute Netherlands](http://www.marin.nl) worked on this improvement.

* Add in a Catalyst C++ pipeline to write out all data.

 The `vtkCPXMLPWriterPipeline` class will make it easy to use Catalyst without Python to write out a full dataset. ParaView 5.5 will also update Catalyst examples to C++11 coding conventions.

* Improve Catalyst for testing.

 The Python tests for the Catalyst editions will have a `regex` for failing to find a proxy. There will also be a ParaView test that you can use to see if you can properly create Catalyst editions.

* Improve Python3 compliance.

 The `catalyze.py` file will work with Python2 and Python3.

* Make unicode fixes for `filters.xml` and `catalyze.py`.
