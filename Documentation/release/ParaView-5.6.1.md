ParaView 5.6.1 Release Notes
============================

ParaView 5.6.1 is a patch release that is intended to address bugs that were found in 5.6.0 after its release. While there are no critical security issues in the patch set, we recommend that everyone upgrade to 5.6.1.

# ParaView level Changes

Fixed a bug in the new *Catalyst Export Inspector* panel in which all sources in the pipeline would be exported into the script as if they were simulation outputs==Catalyst inputs. This would cause runtime errors. In 5.6.1 sources are handled correctly.

The **Merge Blocks** filter was broken in 5.6.0 such that point attributes would be aggregated incorrectly. We fixed this in 5.6.1 by backporting a new filter implementation that will appear in 5.7.

Problems were found with Catalyst system deployments of 5.6.0 in which ParaView’s dependencies, referenced in the `ParaViewConfig.cmake` module, were not consistently findable. The issue first arose in 5.6.0 when vtk-m updated to the modern imported target approach. The fix demonstrated in 5.6.1’s changes to ParaView’s included Catalyst adaptors is to propagate ParaView’s `CMAKE_PREFIX_PATH` into the dependent codes so that they find the same versions of shared dependencies that ParaView does.

Similarly, we improved the placement of vtk-m’s libraries and executables so that they are more consistently findable and in particular no longer require Windows users to extend the system `PATH` to run tests.

The NetCDF library built for use in ParaView was fixed to support loading of NetCDF and Exodus files larger than 2 GB on 64-bit Windows systems.

A redundant display of opacity values in color maps used for indexed color mode was removed from the user interface.

A problem on macOS where the displayed properties in the Properties tab of the pipeline browser could get out of sync with the actual property values in certain situations was fixed.

Superbuild level Changes

Two changes were made in support of OSPRay. The first was to make AVX the minimum supported instruction set. This avoids a bug building Embree on Macs for SSE instruction sets. The second was to copy OSPRay’s headers into the paraview include directory for OSPRay enabled ParaView SDKs.

The Eye Dome Lighting plugin did not work correctly in parallel runs, but that defect has been corrected in 5.6.1.

Turned on BoxLib’s particles feature to fix a build problem on some ParaView configurations that required it.

The ParaView Lite application was bumped to 1.2.1 to fix an issue with the loading of the distributed plugin.

Added Docker and Singularity recipes for building ParaView to the ParaViewSuperbuild.  The Dockerfile allows users to build a few different flavors of ParaView in an Ubuntu 18.04 container.  Via build-time arguments (`--build-arg OPTION=VALUE`), this recipe allows you to choose either OSMesa or EGL for rendering, to customize the base image, and to pick versions of ParaView and the ParaViewSuperbuild to use when building the container. Early support for building ParaView within a Singularity container was also added.  This recipe works in a similar fashion, but due to the lack of build-time argument support in Singularity, it requires you to edit the recipe to produce OSMesa vs. EGL builds or pick arbitrary versions of the software to build.  See the `README.md` files co-located with these new Docker and Singularity additions for more information.
