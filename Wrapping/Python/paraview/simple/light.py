from paraview import servermanager

from paraview.simple.session import GetActiveView

# ==============================================================================
# Dynamic lights.
# ==============================================================================


def CreateLight():
    """Makes a new light and returns it, unattached to a view.

    :return: The new light
    :rtype: :class:`paraview.servermanager.Light`"""
    pxm = servermanager.ProxyManager()
    lightproxy = pxm.NewProxy("additional_lights", "Light")

    controller = servermanager.ParaViewPipelineController()
    controller.SMController.RegisterLightProxy(lightproxy, None)

    return servermanager._getPyProxy(lightproxy)


def AddLight(view=None):
    """Makes a new light and adds it to the designated or active view.

    :param view: The view to add the light to. Optional, defaults to the active
        view.
    :return: The new light
    :rtype: :class:`paraview.servermanager.Light`"""
    view = view if view else GetActiveView()
    if not view:
        raise ValueError("No 'view' was provided and no active view was found.")
    if view.IsA("vtkSMRenderViewProxy") is False:
        return

    lightproxy = CreateLight()

    nowlights = [l for l in view.AdditionalLights]
    nowlights.append(lightproxy)
    view.AdditionalLights = nowlights
    # This is not the same as returning lightProxy
    return GetLight(len(view.AdditionalLights) - 1, view)


def RemoveLight(light):
    """Removes an existing light from its view.

    :param light: The light to be removed.
    :type light: :class:`paraview.servermanager.Light`"""
    if not light:
        raise ValueError("No 'light' was provided.")
    view = GetViewForLight(light)
    if view:
        if (not view.IsA("vtkSMRenderViewProxy")) or (len(view.AdditionalLights) < 1):
            raise RuntimeError("View retrieved inconsistent with owning a 'light'.")

        nowlights = [l for l in view.AdditionalLights if l != light]
        view.AdditionalLights = nowlights

    controller = servermanager.ParaViewPipelineController()
    controller.SMController.UnRegisterProxy(light.SMProxy)


def GetLight(number, view=None):
    """Get a previously added light.

    :param number: The index of the light to obtain.
    :type number: int
    :param view: The view holding the light.
    :type view: View proxy. Option, if not provided the active view is used.
    :return: The requested light.
    :rtype: :class:`paraview.servermanager.Light` or `None`
    """
    if not view:
        view = active_objects.view
    numlights = len(view.AdditionalLights)
    if numlights < 1 or number < 0 or number >= numlights:
        return
    return view.AdditionalLights[number]


def GetViewForLight(proxy):
    """Given a light proxy, find which view it belongs to

    :param proxy: The light proxy whose view you want to retrieve.
    :type proxy: Light proxy
    :return: The view associated with the light if found, otherwise `None`
    :rtype: View proxy or `None`"""
    # search current views for this light.
    for cc in range(proxy.GetNumberOfConsumers()):
        consumer = proxy.GetConsumerProxy(cc)
        consumer = consumer.GetTrueParentProxy()
        if consumer.IsA("vtkSMRenderViewProxy") and proxy in consumer.AdditionalLights:
            return consumer
    return None
