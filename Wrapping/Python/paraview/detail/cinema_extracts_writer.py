r"""
Internal module used by `vtkSMCinemaExtractWriterProxy` to write Cinema
extracts.

This needs some major revision. Due to lack of time, this is currently simply a
migration of the old code / logic in a slightly more manageable package.
"""
from .. import servermanager as sm
from .. import logger

def _has_volumes(view):
    """returns True if the view has visible or hidden volume representations"""
    for rep in view.Representations:
        if hasattr(rep, "Representation") and getattr(rep, "Representation") == "Volume":
            return True
    return False

def get_angular_division(divisions, start, stop):
    if divisions < 2:
        return [0]
    delta = (stop - start) / divisions
    return [ (start + i * delta) for i in range(divisions) ]

def write(writer, controller):
    writer = sm._getPyProxy(writer)
    view = writer.View
    if not view.IsA("vtkSMRenderViewProxy"):
        raise RuntimeError("Cinema extracts are only support for render view")

    properties_to_restore = {}
    properties_to_restore["ViewTime"] = view.ViewTime
    properties_to_restore["ViewSize"] = view.ViewSize
    properties_to_restore["LockBounds"] = view.LockBounds
    properties_to_restore["MaxClipBounds"] = view.MaxClipBounds
    view.ViewTime = controller.GetTime()
    view.ViewSize = writer.ImageResolution

    cinema_params = {}

    deferredRendering = writer.DeferredRendering
    if deferredRendering == "None":
        cinema_params["composite"] = False
        cinema_params["noValues"] = True
    elif deferredRendering == "Composable":
        cinema_params["composite"] = True
        cinema_params["noValues"] = True
    elif deferredRendering == "Composable+Recolorable":
        cinema_params["composite"] = True
        cinema_params["noValues"] = False
    else:
        raise RuntimeError("Unexpected DeferredRendering mode '%s'" % deferredRendering)

    if cinema_params["composite"]:
        if _has_volumes(view):
            if cinema_params["noValues"] == False:
                raise RuntimeError(\
                    "View has volume representations which does with " \
                    "'DeferredRendering' mode 'Composable+Recolorable'. ")
            else:
                logger.warning(\
                    "View has volume representations which may cause "
                    "artifacts in 'Composable' mode.")


    cameraModel = writer.CameraModel
    if deferredRendering == "None" and \
            cameraModel != "Static" and cameraModel != "Phi-Theta":
        logger.warning(\
            "Camera model incompatible with 'DeferredRendering' mode. "\
            "Falling back to Phi-Theta")
        cameraModel = "Phi-Theta"

    if cameraModel == "Static":
        cinema_params["camera"] = 'static'
    elif cameraModel == "Phi-Theta":
        cinema_params["camera"] = 'phi-theta'
        cinema_params["phi"] = get_angular_division(writer.CameraPhiDivisions, -180.0, 180.0)
        cinema_params["theta"] = get_angular_division(writer.CameraThetaDivisions, -90.0, 90.0)
    elif cameraModel == "Inward Pose":
        cinema_params["camera"] = 'azimuth-elevation-roll'
        cinema_params["phi"] = [int(writer.CameraPhiDivisions)]
        cinema_params["theta"] = [int(writer.CameraThetaDivisions)]
        cinema_params["roll"] = [int(writer.CameraRollDivisions)]
    elif cameraModel == "unknown": # yes, this case was in old code by not really
                                  # useable; I am just adding it for reference.
        cinema_params["camera"] = 'yaw-pitch-roll'
        cinema_params["phi"] = [int(writer.CameraPhiDivisions)]
        cinema_params["theta"] = [int(writer.CameraThetaDivisions)]
        cinema_params["roll"] = [int(writer.CameraRollDivisions)]

    if cameraModel != "Inward Pose" and writer.TrackObject != "None":
        logger.warning("Object tracking requires pose-based camera. "\
            "Falling back to no-tracking.")
        cinema_params["tracking"] = { 'object' : 'None' }
    else:
        cinema_params["tracking"] = { 'object' : writer.TrackObject }

    camera = view.GetActiveCamera()
    camera_params = {}
    camera_params["at"] = view.CenterOfRotation
    camera_params["eye"] = camera.GetPosition()
    camera_params["up"] = camera.GetViewUp()
    cinema_params["initial"] = camera_params
    del camera_params

    # now, I am calling what used to be coprocessing.CoProcessor.UpdateCinema
    # constructing similar params
    if cinema_params["composite"]:
        specLevel = 'B'
    else:
        specLevel = 'A'
    update_cinema(cinema_params, view, specLevel, writer, controller)

    for key, value in properties_to_restore.items():
        setattr(view, key, value)

def update_cinema(cinema_params, view, specLevel, writer, controller):
    import paraview.tpl.cinema_python.adaptors.explorers as explorers
    import paraview.tpl.cinema_python.adaptors.paraview.pv_explorers as pv_explorers
    import paraview.tpl.cinema_python.adaptors.paraview.pv_introspect as pv_introspect

    import os.path

    cdb_filename = os.path.join(controller.GetImageExtractsOutputDirectory(), writer.FileName)
    info_json = os.path.join(cdb_filename, "info.json")

    # figure out what we show now
    pxystate= pv_introspect.record_visibility()

    # a conservative global bounds for consistent z scaling
    minbds, maxbds  = pv_introspect.max_bounds()

    # make sure depth rasters are consistent
    view.MaxClipBounds = [minbds, maxbds, minbds, maxbds, minbds, maxbds]
    view.LockBounds = 1

    tracks = {}
    if "phi" in cinema_params:
        tracks["phi"] = cinema_params["phi"]
    if "theta" in cinema_params:
        tracks["theta"] = cinema_params["theta"]
    if "roll" in cinema_params:
        tracks["roll"] = cinema_params["roll"]


    if specLevel=="B":
        p = pv_introspect.inspect(skip_invisible=True)
    else:
        p = pv_introspect.inspect(skip_invisible=False)
    fs = pv_introspect.make_cinema_store(p, info_json, view,
                                         forcetime = "%.6e" % controller.GetTime(),
                                         userDefined = tracks,
                                         specLevel = specLevel,
                                         camType = cinema_params["camera"],
                                         extension = ".png",
                                         disableValues = cinema_params["noValues"])

    # all nodes participate, but only root can writes out the files
    pm = sm.vtkProcessModule.GetProcessModule()
    pid = pm.GetPartitionId()

    new_files = {}
    ret = pv_introspect.explore(fs, p, iSave = (pid == 0),
                          currentTime = { 'time': "%.6e" % controller.GetTime() },
                          userDefined = tracks,
                          specLevel = specLevel,
                          camType = cinema_params["camera"],
                          tracking = cinema_params["tracking"],
                          # now we can rely on float textures everywhere
                          floatValues = True,
                          disableValues = cinema_params["noValues"])
    if pid == 0:
        fs.save()
    new_files = {}
    new_files[cdb_filename] = ret;

    #restore what we showed
    pv_introspect.restore_visibility(pxystate)
