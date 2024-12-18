from paraview import servermanager

from paraview.simple.session import GetActiveSource, GetActiveView
from paraview.simple.proxy import GetSources

# ==============================================================================
# Animation methods
# ==============================================================================


def GetTimeKeeper():
    """Returns the timekeeper proxy for the active session. Timekeeper is often used
    to manage time step information known to the ParaView application.

    :return: timekeeper
    :rtype: :class:`paraview.servermanager.TimeKeeper`"""
    if not servermanager.ActiveConnection:
        raise RuntimeError("Missing active session")
    session = servermanager.ActiveConnection.Session
    controller = servermanager.ParaViewPipelineController()
    return controller.FindTimeKeeper(session)


def GetAnimationScene():
    """Returns the application-wide animation scene. ParaView has only one global
    animation scene. This method provides access to that. You are free to create
    additional animation scenes directly, but those scenes won't be shown in the
    ParaView GUI.

    :return: The animation scene
    :rtype: :class:`paraview.servermanager.AnimationScene`"""
    if not servermanager.ActiveConnection:
        raise RuntimeError("Missing active session")
    session = servermanager.ActiveConnection.Session
    controller = servermanager.ParaViewPipelineController()
    return controller.GetAnimationScene(session)


# -----------------------------------------------------------------------------


def AnimateReader(reader=None, view=None):
    """This is a utility function that, given a reader and a view
    animates over all time steps of the reader.

    :param reader: The reader source to iterate over. Optional, defaults
        to the active source.
    :type reader: reader proxy
    :param view: The view to animate. Optional, defaults to the active view.
    :type view: view proxy
    :return: A new animation scene used to play the animation.
    :rtype: :class:`paraview.simple.AnimationSceneProxy`"""
    if not reader:
        reader = GetActiveSource()
    if not view:
        view = GetActiveView()

    return servermanager.AnimateReader(reader, view)


# -----------------------------------------------------------------------------


def GetRepresentationAnimationHelper(sourceproxy):
    """Method that returns the representation animation helper for a
    source proxy. It creates a new one if none exists."""
    # ascertain that proxy is a source proxy
    if not sourceproxy in GetSources().values():
        return None
    for proxy in servermanager.ProxyManager():
        if proxy.GetXMLName() == "RepresentationAnimationHelper" and proxy.GetProperty(
            "Source"
        ).IsProxyAdded(sourceproxy.SMProxy):
            return proxy
    # helper must have been created during RegisterPipelineProxy().
    return None


# -----------------------------------------------------------------------------


def GetAnimationTrack(propertyname_or_property, index=None, proxy=None):
    """Returns an animation cue for the property. If one doesn't exist then a
    new one will be created.
    Typical usage::

      track = GetAnimationTrack("Center", 0, sphere) or
      track = GetAnimationTrack(sphere.GetProperty("Radius")) or

      # this returns the track to animate visibility of the active source in
      # all views.
      track = GetAnimationTrack("Visibility")

    For animating properties on implicit planes etc., use the following
    signatures::

      track = GetAnimationTrack(slice.SliceType.GetProperty("Origin"), 0) or
      track = GetAnimationTrack("Origin", 0, slice.SliceType)
    """
    if not proxy:
        proxy = GetActiveSource()
    if not isinstance(proxy, servermanager.Proxy):
        raise TypeError("proxy must be a servermanager.Proxy instance")
    if isinstance(propertyname_or_property, str):
        propertyname = propertyname_or_property
    elif isinstance(propertyname_or_property, servermanager.Property):
        prop = propertyname_or_property
        propertyname = prop.Name
        proxy = prop.Proxy
    else:
        raise TypeError(
            "propertyname_or_property must be a string or servermanager.Property"
        )

    # To handle the case where the property is actually a "display" property, in
    # which case we are actually animating the "RepresentationAnimationHelper"
    # associated with the source.
    if (
        propertyname in ["Visibility", "Opacity"]
        and proxy.GetXMLName() != "RepresentationAnimationHelper"
    ):
        proxy = GetRepresentationAnimationHelper(proxy)
    if not proxy or not proxy.GetProperty(propertyname):
        raise AttributeError("Failed to locate property %s" % propertyname)

    scene = GetAnimationScene()
    for cue in scene.Cues:
        try:
            if cue.AnimatedProxy == proxy and cue.AnimatedPropertyName == propertyname:
                if index == None or index == cue.AnimatedElement:
                    return cue
        except AttributeError:
            pass

    # matching animation track wasn't found, create a new one.
    cue = servermanager.animation.KeyFrameAnimationCue()
    cue.AnimatedProxy = proxy
    cue.AnimatedPropertyName = propertyname
    if index != None:
        cue.AnimatedElement = index
    scene.Cues.append(cue)
    return cue


# -----------------------------------------------------------------------------


def GetCameraTrack(view=None):
    """Returns the camera animation track for the given view.

    :param view: The view whose camera animation track should be retrieved.
                 If no view is specified, the active view will be used. If no existing camera
                 animation track is found, a new one will be created.
    :type view: View proxy
    :return: Camera animation cue
    :rtype: :class:`paraview.servermanager.CameraAnimationCue`

    """
    if not view:
        view = GetActiveView()
    if not view:
        raise ValueError("No view specified")
    scene = GetAnimationScene()
    for cue in scene.Cues:
        if cue.AnimatedProxy == view and cue.GetXMLName() == "CameraAnimationCue":
            return cue
    # no cue was found, create a new one.
    cue = servermanager.animation.CameraAnimationCue()
    cue.AnimatedProxy = view
    scene.Cues.append(cue)
    return cue


# -----------------------------------------------------------------------------


def GetTimeTrack():
    """Returns the animation track used to control the time requested from all
    sources during playback

    :return: The time animation track.
    :rtype: :class:`paraview.servermanager.TimeAnimationCue`"""
    scene = GetAnimationScene()
    if not scene:
        raise RuntimeError("Missing animation scene")
    controller = servermanager.ParaViewPipelineController()
    return controller.GetTimeAnimationTrack(scene)
