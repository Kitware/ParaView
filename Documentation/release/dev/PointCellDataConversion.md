 * Exposing new PointCellData conversion option

 In VTK, the PointData to CellData and CellData to PointData filters
 just got new properties to be able to only process certain arrays
 instead of systematically processing all arrays. We expose
 the properties in the xml of these filter so they can be used
 in ParaView.

 * Using these properties with AutoConvert properties
 AutoConvert properties settings allows user to use
 some filter with Point/Cell data even when
 they do not accept it. The conversion is done
 in vtkPVPostFilter but it used to convert all
 arrays instead of only the needed one.
 This uses the new PointData to CellData and CellData to
 PointData filter properties to be able to convert
 only the needed arrays.
