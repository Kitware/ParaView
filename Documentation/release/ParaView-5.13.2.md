ParaView 5.13.2 Release Notes
============================

Bug fixes made since ParaView 5.13.1 are listed below:

* Solved integer overflow when opening large datasets with the EnSight Reader when reading EnSight Gold Binary files. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11518)
* Resolved performance problem in **Slice** filter applied to unstructured grids with polyhedra. [(details)](https://gitlab.kitware.com/vtk/vtk/-/issues/19499)
* Fixed problem with **Ghost Cells** filter operating on datasets with polyhedra. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11583)
* Fixed problem that prevented "Select Cells On" from working with partitioned datasets and partitioned dataset collections and showing partitioned datasets in a SpreadSheet view. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22637)
* Fixed a bug where the _MultiBlock Inspector_ hid all MultiBlock data blocks in the view when unselecting only one block. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7062)
* Patched expat 2.4.8 to fix [CVE-2024-50602](https://nvd.nist.gov/vuln/detail/CVE-2024-50602). [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11609)
* Fixed issue with reverse connection through SSL port not working. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22795)
* Fixed crash that could occur when no representation proxy was set. [(details)](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/7140)
* When building ParaView with VTK's vendored NetCDF, the CDF-5 format is now enabled. [(details)](https://gitlab.kitware.com/paraview/paraview/-/issues/22791)
* Avoid crash in **Mesh Quality** filter when empty meshes are present. [(details)](https://gitlab.kitware.com/vtk/vtk/-/issues/19543)
* VTKHDF file format - fixed array names that missed the `s` in case of the temporal OverlappingAMR to be consistent with other dataset types. It concerns these previous names for arrays: NumberOfBox -> NumberOfAMRBoxes, AMRBoxOffset -> AMRBoxOffsets, Point/Cell/FieldDataOffset -> Point/Cell/FieldDataOffsets. Increased supported VTKHDF version number to 2.3. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11701)
* Expanded types of data arrays valid for Catalyst "time" or "TimeValue" arrays. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11638)
* Fixed volume rendering of rectilinear grids where the full volume was not rendered. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11764)
* Positional light attenuation is now correctly supported in ray tracing. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11757)
* Ensure that the OSPRay renderer uses ambient illumination even if existing ambient light is switched off. [(details)](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/11780)
