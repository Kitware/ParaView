# NetCDF Time Annotation Filter Plugin

The `NetCDFTimeAnnotationPlugin` exposes a filter called "NetCDF Time
Annotation Filter" which can be used to easily display an annotation with
the current NetCDF time of the data. This filter takes benefit from a new
feature of the vtkNetCDFReader which now creates Field Data arrays with the
time units and calendar of the produced data. The filter is a compound filter
that encapsulates a Programmable Filter and a Python Annotation filter.

## Usage
The filter exposes a property called `Expression` that is used to format the time
string.
The default date and time annotation, created by the following command in the
`Expression` property, which can be modified by the user:

"On %02i.%02i.%02i at %02i:%02i" % (Date[0], Date[1], Date[2], Date[3], Date[4])

The 5 elements of the Date array are:

Date[0]: year
Date[1]: month
Date[2]: day
Date[3]: hours
Date[4]: minutes

In case you need the numerals only, you can delete “On” and “at” from the string
on the left side, or you may want to replace the dot “.” separating the date by,
e.g., a dash “-“. To change the order of the date elements in the annotation,
you need to change the order of the Date array elements as in the following example:

"%02i.%02i.%02i %02i:%02i" % (Date[2], Date[1], Date[0], Date[3], Date[4]) yields:

17.01.2001 16:00

## Important note

This plugin takes benefit from the third-party `netcdftime`
Python module, it is mandatory to have it installed in order to build
and use this filter.

## Example
User can refer to this page
https://www.dkrz.de/up/services/analysis/visualization/sw/paraview/tutorial/adding-date-and-time

## Thanks
The filter class was written by Joachim Pouderoux, Kitware 2018.
This work was supported by the German Climate Computing Center (DKRZ).
