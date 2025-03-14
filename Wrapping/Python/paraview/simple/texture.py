from paraview import servermanager

# ==============================================================================
# Materials.
# ==============================================================================


def GetMaterialLibrary():
    """Returns the material library for the active session.

    :return: The material library
    :rtype: :class:`paraview.servermanager.MaterialLibrary`"""
    if not servermanager.ActiveConnection:
        raise RuntimeError("Missing active session")
    session = servermanager.ActiveConnection.Session
    controller = servermanager.ParaViewPipelineController()
    return controller.FindMaterialLibrary(session)


# ==============================================================================
# Textures.
# ==============================================================================
def CreateTexture(filename=None, trivial_producer_key=None, proxyname=None):
    """Creates and returns a new ImageTexture proxy.
    The texture is not attached to anything by default but it can be applied
    to things, for example the view, like so::

        GetActiveView().UseTexturedBackground = 1
        GetActiveView().BackgroundTexture = CreateTexture("foo.png")

    :param filename: The name of the image file to load as a texture. If can be
        `None`, in which case the texture is read from a trivial producer indentified
        by the `trivial_producer_key`
    :type filename: str
    :param trivial_producer_key: Identifier of the texture image source on the
        server. This is mainly used by scene importers when `filename` is `None`.
    :type trivial_producer_key: str
    :param proxyname: The name for the texture when it is registered in ParaView.
    :type proxyname: str
    :return: The newly created texture.
    :rtype: Image texture proxy.
    """
    pxm = servermanager.ProxyManager()
    textureproxy = pxm.NewProxy("textures", "ImageTexture")
    controller = servermanager.ParaViewPipelineController()
    if filename is not None:
        controller.SMController.RegisterTextureProxy(textureproxy, filename)
    elif trivial_producer_key is not None:
        controller.SMController.RegisterTextureProxy(
            textureproxy, trivial_producer_key, proxyname
        )
    return servermanager._getPyProxy(textureproxy)


def FindTextureOrCreate(registrationName, filename=None, trivial_producer_key=None):
    """Finds or creates a new ImageTexture proxy.

    :param registrationName: The name for the texture when it is registered in ParaView.
    :type registrationName: str
    :param filename: Path to the texture image source file. Optional, if not provided,
        the `trivial_producer_key` should be provided to tell which image source to use
        for the texture.
    :type filename: str
    :param trivial_producer_key: Identifier of the texture image source on the
        server. This is mainly used by scene importers when `filename` is `None`.
    :type trivial_producer_key: str"""
    pxm = servermanager.ProxyManager()
    textureproxy = pxm.GetProxy("textures", registrationName)
    if textureproxy is None:
        return CreateTexture(filename, trivial_producer_key, registrationName)
    else:
        return textureproxy
