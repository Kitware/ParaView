
# A hack to get the window and view.
set pvWindow [lindex [vtkPVWindow ListInstances] 0]
set pvView [$pvWindow GetMainView]


# Load the model of the continents.
set pvCont [$pvWindow Open "E:/Law/ParaView/ParaView/data/POP/contDeciFull.vtk"]
# set the color of the continent model.
set pvContOutput [$pvCont GetNthPVOutput 0]
[$pvContOutput GetColorMenu] SetValue {Property}
$pvContOutput ColorByProperty
$pvContOutput ChangeActorColor 0 [expr 197.0/255.0] 1.0


# Load the model of the ocean floor.
set pvFloor [$pvWindow Open "E:/Law/ParaView/ParaView/data/POP/oceanFloorSmall.vtk"]
# set the color of the ocean floor model.
set pvFloorOutput [$pvFloor GetNthPVOutput 0]
[$pvFloorOutput GetColorMenu] SetValue {Property}
$pvFloorOutput ColorByProperty
$pvFloorOutput ChangeActorColor 0.5 0.4 0.4


# load the pop volume
set pvPOP [$pvWindow Open "E:/Law/ParaView/ParaView/data/POP/small.pop"]
$pvPOP AcceptCallback

set sInt [$pvWindow GetSourceInterface "vtkExtractGrid"]
set pvExtractGrid [$sInt CreateCallback]
[$pvExtractGrid GetVTKSource] SetVOI 0 360 0 239 0 0
$pvExtractGrid UpdateParameterWidgets
$pvExtractGrid AcceptCallback

# change to the data page.
[$pvExtractGrid GetNotebook] Raise {Display}

# Setup color map to show temperature.
set pvExtractGridOutput [$pvExtractGrid GetNthPVOutput 0]
[$pvExtractGridOutput GetColorMenu] SetValue {Point Temperature}
$pvExtractGridOutput ColorByPointFieldComponent Temperature 0
[$pvExtractGridOutput GetColorMapMenu] SetValue {Blue to Red}
$pvExtractGridOutput ChangeColorMap

$pvExtractGrid SetVisibility 0


# Create an iso surface of Salinity and color it by temerature.
$pvWindow SetCurrentPVSource $pvPOP

set pvContour [$pvWindow ContourCallback]
[$pvContour GetVTKSource] SetValue 0 0.035
$pvContour UpdateParameterWidgets

[$pvContour GetScalarOperationMenu] SetValue "Salinity"
$pvContour ChangeScalars
$pvContour AcceptCallback

# change to the data page.
[$pvContour GetNotebook] Raise {Display}

# Setup color map to show temperature.
set pvContourOutput [$pvContour GetNthPVOutput 0]
[$pvContourOutput GetColorMenu] SetValue {Point Temperature}
$pvContourOutput ColorByPointFieldComponent Temperature 0
[$pvContourOutput GetColorMapMenu] SetValue {Blue to Red}
$pvContourOutput ChangeColorMap

$pvContourOutput SetVisibility 0






