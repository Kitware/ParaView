# hack to get the window.
set pvWindow [lindex [vtkPVWindow ListInstances] 0]

# get the source menu
set pvSourceMenu [$pvWindow GetSourceMenu]

# create a sphere
$pvSourceMenu Invoke [$pvSourceMenu GetIndex "SphereSource"]
set pvSphere [$pvWindow GetCurrentPVSource]
$pvSphere AcceptCallback

#vtkWindowToImageFilter winToImage
#  winToImage SetInput RenWin1

#vtkPNGWriter writer
#  writer SetInput [winToImage GetOutput]
#  writer SetFileName "sphere.png"
#  writer Write

exit


