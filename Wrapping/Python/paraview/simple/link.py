from paraview import servermanager

from paraview.simple.session import GetActiveView, GetActiveSource

# ==============================================================================
# General puprpose links methods
# ==============================================================================


def RemoveLink(linkName):
    """Remove a named link.

    :param linkName: Name of the link to remove.
    :type linkName: str"""
    servermanager.ProxyManager().UnRegisterLink(linkName)


def AddProxyLink(proxy1, proxy2, linkName="", link=None):
    """Create a link between two proxies and return its name.

    An instance of a `vtkSMProxyLink` subclass can be given, otherwise a `vtkSMProxyLink`
    is created. This does not link proxy properties. See
    `vtkSMProxyLink.LinkProxyPropertyProxies`.

    :param linkName: Name of link to create. If empty, a default one is created for
        registration. If a link with the given name already exists it will be removed
        first.
    :type linkName: str
    :return: The link registration name.
    :rtype: str
    """
    if link == None:
        link = servermanager.vtkSMProxyLink()

    link.LinkProxies(proxy1.SMProxy, proxy2.SMProxy)
    pm = servermanager.ProxyManager()
    if linkName == "":
        name1 = pm.GetProxyName(proxy1.SMProxy.GetXMLGroup(), proxy1.SMProxy)
        name2 = pm.GetProxyName(proxy2.SMProxy.GetXMLGroup(), proxy2.SMProxy)
        linkName = name1 + "-" + name2 + "-link"

    RemoveLink(linkName)
    pm.RegisterLink(linkName, link)
    return linkName


# ==============================================================================
# ViewLink methods
# ==============================================================================


def AddViewLink(viewProxy, viewProxyOther, linkName=""):
    """Create a view link between two view proxies.

    A view link is an extension of a proxy link that also performs rendering when a
    property changes.

    Cameras in the views are not linked.

    If a link with the given name already exists it will be removed first and replaced
    with the newly created link.

    :param viewProxy: The first view to link.
    :type viewProxy: View proxy.
    :param viewProxyOther: The second view to link.
    :type viewProxyOther: View proxy.
    :param linkName: The name of the link to create. Optional, defaults to
        automatically generating a link name.
    :type linkName: str
    :return: The link registration name.
    :rtype: str
    """
    link = servermanager.vtkSMViewLink()
    return AddProxyLink(viewProxy, viewProxyOther, linkName, link)


def AddRenderViewLink(viewProxy, viewProxyOther, linkName="", linkCameras=False):
    """Create a named view link between two render view proxies.

    It also creates links for the AxesGrid proxy property in each view. By default,
    cameras are not linked.

    If a link with the given name already exists it will be removed first and replaced
    with the newly created link.

    :param viewProxy: The first view to link.
    :type viewProxy: View proxy.
    :param viewProxyOther: The second view to link.
    :type viewProxyOther: View proxy.
    :param linkName: The name of the link to create.
    :type linkName: str
    :param linkCameras: If `True`, also link the view cameras.
    :type linkCamera: bool
    :return: The link registration name.
    :rtype: str
    """
    linkName = AddViewLink(viewProxy, viewProxyOther, linkName)
    pm = servermanager.ProxyManager()
    link = pm.GetRegisteredLink(linkName)
    link.EnableCameraLink(linkCameras)
    link.LinkProxyPropertyProxies(viewProxy.SMProxy, viewProxyOther.SMProxy, "AxesGrid")
    return linkName


# ==============================================================================
# CameraLink methods
# ==============================================================================


def AddCameraLink(viewProxy, viewProxyOther, linkName=""):
    """Create a named camera link between two view proxies. If a link with the given
    name already exists it will be removed first and replaced with the newly created
    link.

    :param viewProxy: The first view to link.
    :type viewProxy: View proxy.
    :param viewProxyOther: The second view to link.
    :type viewProxyOther: View proxy.
    :param linkName: The name of the link to create. Optional, defaults to
        automatically generating a link name.
    :type linkName: str
    :return: The link registration name.
    :rtype: str
    """
    if not viewProxyOther:
        viewProxyOther = GetActiveView()
    link = servermanager.vtkSMCameraLink()
    if linkName == "":
        pm = servermanager.ProxyManager()
        name1 = pm.GetProxyName(viewProxy.SMProxy.GetXMLGroup(), viewProxy.SMProxy)
        name2 = pm.GetProxyName(
            viewProxyOther.SMProxy.GetXMLGroup(), viewProxyOther.SMProxy
        )
        linkName = name1 + "-" + name2 + "-cameraLink"

    return AddProxyLink(viewProxy, viewProxyOther, linkName, link)


# -----------------------------------------------------------------------------


def RemoveCameraLink(linkName):
    """Remove a camera link with the given name.

    :param linkName: Name of the link to remove.
    :type linkName: str"""
    RemoveLink(linkName)


# ==============================================================================
# SelectionLink methods
# ==============================================================================


def AddSelectionLink(objProxy, objProxyOther, linkName, convertToIndices=True):
    """Create a named selection link between two source proxies.
    If a link with the given name already exists it will be removed first and
    repaced with the newly created link.

    :param objProxy: First proxy to link.
    :type objProxy: Source proxy
    :param objProxyOther: Second proxy to link. If `None`, uses the active
        source.
    :type objProxyOther: Source proxy
    :param linkName: Name of the created link.
    :type linkName: str
    :param convertToIndices: When `True` (default value), the input selection
        will always be converted into an indices-based selection before being
        applied to outputs.
    :type convertToIndices: bool
    """
    if not objProxyOther:
        objProxyOther = GetActiveSource()
    link = servermanager.vtkSMSelectionLink()
    link.SetConvertToIndices(convertToIndices)
    link.AddLinkedSelection(objProxy.SMProxy, 2)
    link.AddLinkedSelection(objProxyOther.SMProxy, 2)
    link.AddLinkedSelection(objProxyOther.SMProxy, 1)
    link.AddLinkedSelection(objProxy.SMProxy, 1)
    RemoveSelectionLink(linkName)
    servermanager.ProxyManager().RegisterLink(linkName, link)
    return link


# -----------------------------------------------------------------------------


def RemoveSelectionLink(linkName):
    """Remove a selection link with the given name.

    :param linkName: Name of the link to remove.
    :type linkName: str"""
    RemoveLink(linkName)
