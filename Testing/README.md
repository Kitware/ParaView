Baseline images
---------------

ParaView has a large number of tests that involve executing some ParaView
functionality and then checking that the resulting image in the RenderView
is the same as some reference baseline image to within some tolerance. These
baseline images are stored in this directory under `Data/Baseline`. In many
cases, there is one baseline image per test. However, some tests generate more
than one output image. Each output has a distinct baseline image against which
it is compared.

Alternate baselines
-------------------

Some tests produce slightly different results on different platforms.
To accommodate these differences, ParaView's testing infrastructure
supports alternate baseline images. Naming alternate baseline images
follows a simple pattern involving adding an underscore followed by a
sequentially increasing integer to the end of the test name. For instance,
for a test named MyTest, the primary and alternate baselines could be:

MyTest.png
MyTest_1.png
MyTest_2.png

Unfortunately, this naming scheme does not provide any information
about which dashboard configuration requires the alternate
baseline. This file is intended to serve as a map between alternate
baseline name and the dashboard configuration that requires it.  This
information can be used to easily generate new alternate baselines
with the appropriate number by mapping the dashboard configuration
back to the alternate baseline number.

Note that one alternate baseline may be appropriate for more than one
dashboard configuration. In such cases, only one dashboard configuration
is listed as it should be sufficient to generate the alternate baseline
for that one machine.

Alternate baseline to generating dashboard configuration map
------------------------------------------------------------

AxesGridTestLines_1.png  - amber8-linux-static-release+mpi+offscreen+osmesa+python
ColorOpacityTableEditing_1.png - bigmac-osx-shared-debug+clang+gui+python+qt4]
LoadStateMultiView_1.png - ista-osx-shared-release+gui+kits+python
SaveColorMap_1.png - tylo-windows-shared-release+gui+python+python3+tbb]
SaveLargeScreenshot_1.png - nemesis-windows-shared-release+gui+kits+mpi+python
ScalarOpacityFunctionPythonState_1.png - tylo-windows-shared-release+gui+python+python3+tbb
TestSubhaloFinder_1.png  - vall-linux-shared-debug+doc+extdeps+gui+mpi+python+python3
