# NetCDF Time Annotation Filter Plugin

The new `NetCDFTimeAnnotationPlugin` exposes a filter called "NetCDF Time
Annotation Filter" which can be used to easily display an annotation with
the current NetCDF time of the data. This filter takes benefit from a new
feature of the vtkNetCDFReader which now creates Field Data arrays with the
time units and calendar of the produced data. The filter is a compound filter
that encapsulates a Programmable Filter and a Python Annotation filter.

Important note: This plugin takes benefit from the third-party `netcdftime`
Python module, it is mandatory to have it installed in order to build
and use this filter.
