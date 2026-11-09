ParaView 6.2.1 Release Notes
=============================

Bug fixes made since ParaView 6.2.0 are listed in this document. The full list of issues addressed by this patch release is available [here](https://gitlab.kitware.com/paraview/paraview/-/milestones/37).

ParaView 6.2.1 is built with VTK 9.7.2, which contains additional bug fixes since VTK 9.7.1, which was used for ParaView 6.2.0. The bug fixes in VTK most relevant to ParaView are listed in this document.

## Mouse wheel scrolling over unfocused widgets in the _Properties_ panel fixed

Scrolling the mouse wheel or using a trackpad over an unfocused combo box, slider, spin box, tree view, or table view in the _Properties_ panel now works again as it did in ParaView 5.13.3 and before [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23443)

## Fixed _Properties_ panel search box losing keyboard focus with _Auto Apply_

With _Auto Apply_ enabled, typing in the _Properties_ panel search box no longer loses keyboard focus after each character. More generally, the widget that has keyboard focus when an apply starts keeps it once the apply finishes. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23413)

## Fixed custom color legend component titles being reset

A custom **Component Title** set in the _Color Legend Properties_ dialog is again preserved when creating a new layout or switching between layouts. It is also preserved when the representation type changes. The component title is now updated automatically only when the names of the color array's components change. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23446)

## Fixed DPvtk reader reading only the first time step

The **DPvtk MultiBlock Data Reader** in the DPvtkReader plugin now reads all the time steps stored in a DPvtk file. Previously, only the first time step was read. For a requested time, the reader loads the first time step with a time greater than or equal to it, or the last time step, the same as the PVD reader.

The reader is also more robust:

* It no longer leaks memory each time it loads a time step.
* Errors while reading a DPvtk file are now reported instead of being ignored. When reading in parallel, an error on one process is reported on all processes instead of making the other processes hang.
* A time step that stores two partition files with the same name is now reported as an error instead of silently reading the wrong data.

A DPvtk file stores all the time steps, so you can no longer open several DPvtk files as a file series.

The DPvtkReader plugin now requires the `VTK::ParallelCore` module, and it uses the MPI version of DPvz only when the `VTK::ParallelMPI` module is enabled. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23414)

## Fixed crash in **Plot On Sorted Lines** with unordered line segments

**Plot On Sorted Lines** no longer crashes when the line segments of its input are not listed in the order they connect, which is common for the output of **Slice**. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7977)

## Fixed **Global Point And Cell Ids** numbering of composite datasets

The **Global Point And Cell Ids** filter now numbers the points and cells of all the partitions of a Multi-block Dataset, a Partitioned Dataset or a Partitioned Dataset Collection together. Previously, each partition was numbered on its own, so the same ID was given to unrelated points of different partitions, and points that should not be merged ended up sharing an ID. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23155)

## Fixed point picking on sources with multiple output ports

`vtkSMRenderViewProxy::ConvertDisplayToPointOnSurface` now correctly intersects the picked output port, instead of always using the first one, when the source has multiple output ports. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7982)

## Reduced data transferred when gathering composite data information in parallel

When gathering data information from all ranks for composite datasets, only the data assembly and hierarchy of rank 0 are used, so the other ranks no longer send them. This reduces unnecessary data transferr between processes for large composite datasets, such as large CTH AMR datasets. Requests for the information of a specific rank work as before. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23407)

## Added option to disable static mesh optimization in **Clip**

The static mesh optimization of the **Clip** filter builds an internal structure the second time the filter executes. In early benchmarks, the execution that builds this structure took about 2.5 times as long as without the optimization, while later executions took about 0.7 times as long, so it takes about 5 time steps before the optimization is a net gain. The new advanced **Use Cache** property of the **Clip** filter can be turned off to disable the optimization. It is on by default. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7975)

## OptiX pathtracer ray tracing backend restored

The _OptiX pathtracer_ ray tracing backend, which was not available in ParaView 6.2.0, is available again. To make this possible, ParaView binaries once again include the older VisRTX library that provides this backend, and ANARI now uses the `helide` device by default on all platforms instead of `visrtx`. To use ANARI with VisRTX, install a VisRTX version compatible with ANARI (0.2.0 or later) yourself. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23426)

## Fixed incorrect OSPRay pathtracer images in binary packages

In the ParaView 6.2.0 binary packages, setting **Samples Per Pixel** to a value higher than 1 with the OSPRay pathtracer produced an image with a blue tint. This has been fixed. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23404)

In the same packages, enabling **Denoise** had no effect on the OSPRay pathtracer image. This has also been fixed. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23405)

## Fixed OSPRay denoiser

Two problems with the OSPRay denoiser were fixed. When OpenImageDenoise selected a GPU device, the denoiser silently returned the image exactly as rendered; the image is now denoised, and a warning is reported if the device reports an error. When OpenImageDenoise was used on the CPU, the denoised frame could be fully transparent and be dropped by the compositor; the color image is now denoised in place so that its alpha channel is preserved. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13914)

## Fixed Plot Over Line warnings in parallel

When the **Sampling Pattern** property of the **Plot Over Line** filter was set to _Sample At Cell Boundaries_, a warning `No points read!` was reported for each rank of a parallel server that had no intersection between the line and its part of the data. Ranks without intersection no longer send their result, so the warnings are no longer reported. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23441)

## IOSS-related fixes

The IOSS reader now reports the errors raised by the underlying IOSS library instead of failing silently. For example, enabling **Group Numeric Vector Field Components** on a dataset that already has an array with the name of the grouped vector now reports why the dataset cannot be read. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23416)

The IOSS writer now reports an error rather than crashing when a block is selected to be written as more than one entity type, such as both an element block and a side set. It also reports an error when the hierarchy cannot be generated, and stops iterating over time steps after a failed write. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23439)

## **Stream Tracer** performance regression improved

The performance of `vtkJumpAndWalkCellLocator` cell lookups has been improved by looking in the neighborhood of the seed cell first. This addresses part of the performance regression seen in the **Stream Tracer** filter in ParaView 6.2.0. [(details)](https://gitlab.kitware.com/paraview/paraview/-/work_items/23430)

## Reverted point matching tolerance for generated global ids in Ghost Cells

ParaView 6.2.0 set a tolerance equal to machine epsilon when `vtkGhostCellsGenerator` generates global point ids. The intent was to make the output of the **Ghost Cells** filter the same whether or not **Generate Global Ids** is enabled. That change has been reverted, so global point ids are generated without a tolerance again. As a result, points on different processes whose coordinates differ slightly are not matched when **Generate Global Ids** is enabled, and the ghost cells and points generated can differ from those generated when it is disabled.

## Fixed color mapping of out-of-range and negative values with logarithmic scale

When mapping colors on a logarithmic scale, scalar values outside the valid range are now mapped to the below range and above range colors. Previously, `vtkLookupTable` clamped them to the ends of the valid range, and `vtkColorTransferFunction` could produce undefined (`NaN`) values for negative inputs. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13762) [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13778)

## VTKHDF reader and writer fixes

The VTKHDF reader and writer now support 2D and 1D image data and have improved support for reading sub-extents. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13886)

Cache invalidation in the VTKHDF reader for temporal image data when switching between time steps has been fixed. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13775)

Fixed VTKHDF reader caching when re-enabling point or cell arrays, changing structured extents, and reading structured composite leaves with matching array names and extents. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13884)

## Fixed XML MultiBlock Data Reader with unindexed and nested blocks

A `.vtm` file whose `DataSet` elements have no `index` attribute was split into a named empty block and an unnamed block that holds the data. Unindexed and nested blocks are now read correctly. [(details)](https://gitlab.kitware.com/vtk/vtk/-/work_items/20162)

## Fixed integer overflow in vtkGridSynchronizedTemplates3D on large grids

`vtkGridSynchronizedTemplates3D` now uses `vtkIdType` for strides, offsets, counts, and the buffers that hold output point ids, which fixes an integer overflow when processing large grids. [(details)](https://gitlab.kitware.com/vtk/vtk/-/work_items/20185)

## Fixed **Axes Grid** creation being slow and using excessive memory

Creation time for the _Axes Grid_ in a view has been significantly improved and memory usage has been greatly reduced [(details)](https://gitlab.kitware.com/vtk/vtk/-/work_items/20201)

## Python scripting fixes

Retrieving many data arrays in Python, for example calling `GetAbstractArray()` on each of thousands of arrays in a `vtkCellData`, is no longer quadratric. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13763)

Arithmetic on VTK arrays in Python now follows numpy's scalar promotion rules. For example, adding a Python `int` or `float` to a `float32` array keeps the `float32` type, and constant arrays keep their own data type. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13752)

## ONNX models on Windows fixed

* Fixed loading ONNX models on Windows. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13942)

## Quieted some information messages

Fixed undesired warnings from the EGL render window that could be repeated on every render. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13791)

`vtkTableBasedClipDataSet` no longer reports a warning when output cells have duplicate points, which is valid behavior, to avoid excessive warnings from the **Clip** filter. The message is now only reported at the `TRACE` verbosity level. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/13874)
