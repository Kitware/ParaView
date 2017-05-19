from paraview import simple
from vtk.web  import camera

def update_camera(viewProxy, cameraData):
    viewProxy.CameraFocalPoint = cameraData['focalPoint']
    viewProxy.CameraPosition = cameraData['position']
    viewProxy.CameraViewUp = cameraData['viewUp']
    simple.Render(viewProxy)

def create_spherical_camera(viewProxy, dataHandler, phiValues, thetaValues):
    return camera.SphericalCamera(dataHandler, viewProxy.CenterOfRotation, viewProxy.CameraPosition, viewProxy.CameraViewUp, phiValues, thetaValues)

def create_cylindrical_camera(viewProxy, dataHandler, phiValues, translationValues):
    return camera.CylindricalCamera(dataHandler, viewProxy.CenterOfRotation, viewProxy.CameraPosition, viewProxy.CameraViewUp, phiValues, translationValues)

def create_cube_camera(viewProxy, dataHandler, viewForward, viewUp, positions):
    return camera.CubeCamera(dataHandler, viewForward, viewUp, positions)

def create_stereo_cube_camera(viewProxy, dataHandler, viewForward, viewUp, positions, eyeSeparation = 6.5):
    return camera.StereoCubeCamera(dataHandler, viewForward, viewUp, positions, eyeSeparation)
