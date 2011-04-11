# Test the vtkSMPropertyAdaptor.

import SMPythonTesting
import os
import os.path
import sys
from paraview import servermanager

SMPythonTesting.ProcessCommandLineArguments()

servermanager.Connect()

pxm = servermanager.ProxyManager()
propAdap = servermanager.vtkSMPropertyAdaptor()

# GlyphSource2D proxy
glyphSourceProxy = pxm.NewProxy("sources", "GlyphSource2D")
pxm.RegisterProxy("sources", "glyphs", glyphSourceProxy)
# GlyphType property
propAdap.SetProperty(glyphSourceProxy.GetProperty("GlyphType"))
if propAdap.GetPropertyType() != 1:
  print "ERROR: Wrong property type reported for GlyphType property of GlyphSource2D proxy."
  sys.exit(1)
if propAdap.GetNumberOfEnumerationElements() != 12:
  print "ERROR: Wrong number of enumeration elements reported for GlyphType property of GlyphSource2D proxy."
  sys.exit(1)
if propAdap.GetEnumerationName(3) != "ThickCross":
  print "ERROR: Wrong enumeration name reported for GlyphType property of GlyphSource2D proxy."
  sys.exit(1)
propAdap.SetEnumerationValue("7")
if propAdap.GetEnumerationValue() != "7":
  print "ERROR: Wrong enumeration value reported for GlyphType property of GlyphSource2D proxy."
  sys.exit(1)
if propAdap.GetElementType() != 6:
  print "ERROR: Wrong element type reported."
  sys.exit(1)

# Filled property
propAdap.SetProperty(glyphSourceProxy.GetProperty("Filled"))
if propAdap.GetPropertyType() != 1:
  print "ERROR: Wrong property type reported for Filled property of GlyphSource2D proxy."
  sys.exit(1)
if propAdap.GetElementType() != 9:
  print "ERROR: Wrong element type reported for Filled property of GlyphSource2D proxy."
  sys.exit(1)
if propAdap.GetNumberOfEnumerationElements() != 2:
  print "ERROR: Wrong number of enumeration elements reported for Filled property of GlyphSource2D proxy."
  sys.exit(1)
if propAdap.GetEnumerationName(1) != "1":
  print "ERROR: Wrong enumeration name reported for Filled property of GlyphSource2D proxy."
  sys.exit(1)
propAdap.SetEnumerationValue("0")
if propAdap.GetEnumerationValue() != "0":
  print "ERROR: Wrong enumeration value reported for Filled property of GlyphSorce2D proxy."
  sys.exit(1)

# ArrowSource proxy
arrowSourceProxy = pxm.NewProxy("sources", "ArrowSource")
pxm.RegisterProxy("sources", "arrow", arrowSourceProxy)
# TipResolution property
propAdap.SetProperty(arrowSourceProxy.GetProperty("TipResolution"))
if propAdap.GetPropertyType() != 3:
  print "ERROR: Wrong property type reported for TipResolution property of ArrowSource proxy."
  sys.exit(1)
if propAdap.GetRangeMinimum(0) != "1":
  print "ERROR: Wrong range minimum reported for TipResolution property of ArrowSource proxy."
  sys.exit(1)
if propAdap.GetRangeMaximum(0) != "128":
  print "ERROR: Wrong range maximum reported for TipResolution property of ArrowSource proxy."
  sys.exit(1)
if propAdap.GetNumberOfRangeElements() != 1:
  print "ERROR: Wrong number of range elements reported for TipResolution property of ArrowSource proxy."
  sys.exit(1)
propAdap.SetGenericValue(0, "50")
if propAdap.GetRangeValue(0) != "50":
  print "ERROR: Wrong range value reported for TipResolution property of ArrowSource proxy."
  sys.exit(1)
# TipRadius property
propAdap.SetProperty(arrowSourceProxy.GetProperty("TipRadius"))
if propAdap.GetRangeMinimum(0) != "0":
  print "ERROR: Wrong range minimum reported for TipRadius property of ArrowSource proxy."
  sys.exit(1)
if propAdap.GetRangeMaximum(0) != "10":
  print "ERROR: Wrong range maximum reported for TipRadius property of ArrowSource proxy."
  sys.exit(1)
if propAdap.GetNumberOfRangeElements() != 1:
  print "ERROR: Wrong number of range elements reported for TipRadius property of ArrowSource proxy."
  sys.exit(1)
propAdap.SetRangeValue(0, "5")
if propAdap.GetRangeValue(0) != "5":
  print "ERROR: Wrong range value reported for TipRadius property of ArrowSource proxy."
  sys.exit(1)

# Append filter
appendFilterProxy = pxm.NewProxy("filters", "Append")
# Input property
propAdap.SetProperty(appendFilterProxy.GetProperty("Input"))
if propAdap.GetPropertyType() != 1:
  print "ERROR: Wrong property type reported for Input property of Append proxy."
  sys.exit(1)
if propAdap.GetElementType() != 10:
  print "ERROR: Wrong element type reported for Input property of Append proxy."
  sys.exit(1)
propAdap.SetEnumerationValue("1")
if propAdap.GetEnumerationValue() != "1":
  print "ERROR: Wrong enumeration value reported for Input property of Append proxy."
  sys.exit(1)
propAdap.SetEnumerationValue("1")
if propAdap.GetNumberOfEnumerationElements() != 2:
  print "ERROR: Wrong number of enumeration elements reported for Input property of Append proxy."
  sys.exit(1)
if propAdap.GetEnumerationName(1) != "glyphs":
  print "ERROR: Wrong enumeration name reported for Input property of Append proxy."
  sys.exit(1)

# PVD reader proxy
pvdReaderProxy = pxm.NewProxy("sources", "PVDReader")
# FileName property
propAdap.SetProperty(pvdReaderProxy.GetProperty("FileName"))
if propAdap.GetPropertyType() != 4:
  print "ERROR: Wrong property type reported for FileName property of PVDReader proxy."
  sys.exit(1)
if propAdap.GetElementType() != 8:
  print "ERROR: Wrong element type reported for FileName property of PVDReader proxy."
  sys.exit(1)
fileListDomain = pvdReaderProxy.GetProperty("FileName").GetDomain("files")
fileListDomain.AddString("foo.pvd")
fileListDomain.AddString("bar.pvd")
if propAdap.GetNumberOfEnumerationElements() != 2:
  print "ERROR: Wrong number of enumeration elements reported for FileName property of PVDReader proxy."
  sys.exit(1)
propAdap.SetEnumerationValue("1")
if propAdap.GetEnumerationValue() != "1":
  print "ERROR: Wrong enumeration value reported for FileName property of PVDReader proxy."
  sys.exit(1)
if propAdap.GetNumberOfRangeElements() != 1:
  print "ERROR: Wrong number of range elements reported for FileName property of PVDReader proxy."
  sys.exit(1)
propAdap.SetRangeValue(0, "foo.pvd")
if propAdap.GetRangeValue(0) != "foo.pvd":
  print "ERROR: Wrong range value reported for FileName property of PVDReader proxy."
  sys.exit(1)
if propAdap.GetEnumerationName(1) != "bar.pvd":
  print "ERROR: Wrong enumeration name reported for FileName property of PVDReader proxy."
  sys.exit(1)

# MinMax filter proxy
minMaxProxy = pxm.NewProxy("filters", "MinMax")
# Operation property
propAdap.SetProperty(minMaxProxy.GetProperty("Operation"))
if propAdap.GetPropertyType() != 1:
  print "ERROR: Wrong property type reported for Operation property of MinMax proxy."
  sys.exit(1)
if propAdap.GetRangeMinimum(0) != None:
  print "ERROR: Wrong range minimum reported for Operation property of MinMax proxy."
  sys.exit(1)
if propAdap.GetRangeMaximum(0) != None:
  print "ERROR: Wrong range maximum reported for Operation property of MinMax proxy."
  sys.exit(1)
if propAdap.GetNumberOfEnumerationElements() != 3:
  print "ERROR: Wrong number of enumeration elements reported for Operation property of MinMax proxy."
  sys.exit(1)
if propAdap.GetEnumerationName(1) != "MAX":
  print "ERROR: Wrong enumeration name reported for Operation property of MinMax proxy."
  sys.exit(1)

# XMLPolyDataReader proxy
xmlPolyDataReaderProxy = pxm.NewProxy("sources", "XMLPolyDataReader")
# CellArrayStatus property
propAdap.SetProperty(xmlPolyDataReaderProxy.GetProperty("CellArrayStatus"))
if propAdap.GetPropertyType() != 2:
  print "ERROR: Wrong property type reported for CellArrayStatus property of XMLPolyDataReader proxy."
  sys.exit(1)
if propAdap.GetElementType() != 3:
  print "ERROR: Wrong element type reported for CellArrayStatus property of XMLPolyDataReader proxy."
  sys.exit(1)
arraySelDomain = xmlPolyDataReaderProxy.GetProperty("CellArrayStatus").GetDomain("array_list")
arraySelDomain.AddString("arrayFoo")
arraySelDomain.AddMinimum(0, 0)
arraySelDomain.AddMaximum(0, 1)
arraySelDomain.AddString("arrayBar")
arraySelDomain.AddMinimum(1, 0)
arraySelDomain.AddMaximum(1, 1)
if propAdap.GetNumberOfSelectionElements() != 2:
  print "ERROR: Wrong number of selection elements reported for CellArrayStatus property of XMLPolyDataReader proxy."
  sys.exit(1)
if propAdap.GetSelectionName(1) != "arrayBar":
  print "ERROR: Wrong selection name reported for CellArrayStatus property of XMLPolyDataReader proxy."
  sys.exit(1)
propAdap.SetSelectionValue(0, "arrayFoo")
if propAdap.GetSelectionValue(0) != "arrayFoo":
  print "ERROR: Wrong selection value reported for CellArrayStatus property of XMLPolyDataReader proxy."
  sys.exit(1)
if propAdap.GetSelectionMinimum(0) != "0":
  print "ERROR: Wrong selecton minimum reported for CellArrayStatus property of XMLPolyDataReader proxy."
  sys.exit(1)
if propAdap.GetSelectionMaximum(1) != "1":
  print "ERROR: Wrong selecton maximum reported for CellArrayStatus property of XMLPolyDataReader proxy."
  sys.exit(1)

stringInfoProp = xmlPolyDataReaderProxy.GetProperty("CellArrayInfo")
stringInfoProp.SetElement(0, "arrayFooBar")
stringInfoProp.SetElement(1, "arrayBarFoo")
propAdap.InitializePropertyFromInformation()
if propAdap.GetRangeValue(0) != "arrayFooBar":
  print "ERROR: Wrong range value reported for CellArrayStatus property of XMLPolyDataReader proxy."
  sys.exit(1)

# ExodusReader proxy
exodusReaderProxy = pxm.NewProxy("sources", "ExodusIIReader")
# FileRange property
propAdap.SetProperty(exodusReaderProxy.GetProperty("FileRange"))
intInfoProp = exodusReaderProxy.GetProperty("FileRangeInfo")
intInfoProp.SetElement(0, 5)
intInfoProp.SetElement(1, 10)
propAdap.InitializePropertyFromInformation()
if propAdap.GetRangeValue(1) != "10":
  print "ERROR: Wrong range value reported for FileRange property of ExodusReader proxy."
  sys.exit(1)

# Camera proxy
cameraProxy = pxm.NewProxy("camera", "Camera")
# CameraPosition property
propAdap.SetProperty(cameraProxy.GetProperty("CameraPosition"))
doubleInfoProp = cameraProxy.GetProperty("CameraPositionInfo")
doubleInfoProp.SetElement(0, 5.0)
doubleInfoProp.SetElement(1, 10.0)
doubleInfoProp.SetElement(2, 15.0)
propAdap.InitializePropertyFromInformation()
if propAdap.GetRangeValue(0) != "5":
  print "ERROR: Wrong range value reported for CameraPosition property of Camera proxy."
  sys.exit(1)
