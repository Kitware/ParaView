from paraview.simple.session import active_objects
from paraview.simple.rendering import Render

# -----------------------------------------------------------------------------


def ResetCamera(view=None):
    """Resets camera settings to make the whole scene visible while preserving
    orientation.

    :param view: The camera for this view is reset. Optional, defaults to the
        active view.
    :type view: View proxy."""
    if not view:
        view = active_objects.view
    if hasattr(view, "ResetCamera"):
        view.ResetCamera()
    if hasattr(view, "ResetDisplay"):
        view.ResetDisplay()
    Render(view)


# -----------------------------------------------------------------------------


def ResetCameraToDirection(
    position=None, direction=None, up=None, view=None, bounds=None, sphere_bounds=None
):
    """Resets the settings of the camera to the given position and direction.

    :param position: Position of the camera. (default: [0,0,0])
    :type position: 3-element tuple or list of floats.
    :param direction: Direction of the camera. (default: [0, 0, -1])
    :type direction: 3-element tuple or list of floats.
    :param up: If provided, will be used as the camera's up direction.
    :type up: 3-element tuple or list of floats.
    :param view: If provided, modifies the camera in this view.
    :type view: View proxy.
    :param bounds: If provided, reset the camera using that bounds (min_x, max_x, min_y, max_y, min_z, max_z).
    :type bounds: 6-element tuple or list of floats.
    :param sbounds: If provided, reset the camera using computed bounds from a sphere_bounds (center_x, center_y, center_z, radius).
    :type sbounds: 4-element tuple or list of floats.

    Examples:

    >> ResetCameraToDirection(
    ...   direction=[-1, -1, -1],
    ...   sphere_bounds=(-5, -4, -3, 25),
    ... )

    >> ResetCameraToDirection(
    ...   direction=[-1, -1, -1],
    ...   bounds=(
    ...      -30, 20,
    ...      -29, 21,
    ...      -28, 22,
    ...   ),
    ... )

    """
    if position is None:
        position = [0, 0, 0]

    if direction is None:
        direction = [0, 0, -1]

    if not view:
        view = active_objects.view

    if hasattr(view, "CameraFocalPoint"):
        view.CameraFocalPoint = position
    if hasattr(view, "CameraPosition"):
        for i in range(3):
            view.CameraPosition[i] = position[i] - direction[i]
    if hasattr(view, "CameraViewUp") and up:
        view.CameraViewUp = up

    # More compact bounds definition
    if sphere_bounds is not None:
        x, y, z, r = sphere_bounds
        bounds = [
            x - r,
            x + r,
            y - r,
            y + r,
            z - r,
            z + r,
        ]

    if hasattr(view, "ResetCamera") and bounds is not None:
        view.ResetCamera(bounds)
        Render(view)
    else:
        ResetCamera(view)
