# load the shared libraries
load vtktcl
load vtkKWWidgetsTcl
load vtkKWParaViewTcl

# remove the top level shell
wm withdraw .

# create the app
vtkPVApplication app
app Start
