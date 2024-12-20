import paraview
from paraview import servermanager
from paraview.util import proxy as proxy_util

from paraview.simple.session import GetActiveSource
from paraview.simple import GetAnimationScene


# ==============================================================================
# Extracts and Extractors
# ==============================================================================
def CreateExtractor(name, proxy=None, registrationName=None):
    """Creates a new extractor and returns it.

    :param name: The type of the extractor to create.
    :type name: `str`
    :param proxy: The proxy to generate the extract from. If not specified
        the active source is used.
    :type proxy: :class:`paraview.servermanager.Proxy`, optional
    :param registrationName: The registration name to use to register
        the extractor with the ProxyManager. If not specified a unique name
        will be generated.
    :type name: `str`, optional

    :return: The extractor created, on success, else None
    :rtype: :class:`paraview.servermanager.Proxy` or `None`

    """
    if proxy is None:
        proxy = GetActiveSource()
    if not proxy:
        raise RuntimeError("Could determine input for extractor")

    if proxy.Port > 0:
        rawProxy = proxy.SMProxy.GetOutputPort(proxy.Port)
    else:
        rawProxy = proxy.SMProxy

    controller = servermanager.vtkSMExtractsController()

    # Special case of 'steering' extractors : add it to the steerable extractors list
    # managed by the in situ helper class, not to the extracts controller, because
    # they are not associated with a writer.
    from paraview.catalyst.detail import IsInsitu, RegisterExtractor

    if name == "steering" and IsInsitu():
        rawExtractor = controller.CreateSteeringExtractor(rawProxy, registrationName)
        extractor = servermanager._getPyProxy(rawExtractor)
        UpdateSteerableParameters(extractor, registrationName)
    else:
        rawExtractor = controller.CreateExtractor(rawProxy, name, registrationName)
        extractor = servermanager._getPyProxy(rawExtractor)
        if IsInsitu():
            # tag the extractor to know which pipeline this came from.
            RegisterExtractor(extractor)
    return extractor


def GetExtractors(proxy=None):
    """Returns a list of extractors associated with the proxy.

    :param proxy: The proxy to return the extractors associated with.
                  If not specified (or is None) then all extractors are returned.
    :type proxy: :class:`paraview.servermanager.Proxy`, optional

    :return: List of associated extractors if any. May return an empty list.
    :rtype: list of :class:`paraview.servermanager.Proxy`

    """
    controller = servermanager.vtkSMExtractsController()
    pxm = servermanager.ProxyManager()
    extractors = pxm.GetProxiesInGroup("extractors").values()
    if proxy is None:
        return list(extractors)
    else:
        return [
            g for g in extractors if controller.IsExtractor(g.SMProxy, proxy.SMProxy)
        ]


def FindExtractor(registrationName):
    """
    Returns an extractor with a specific registrationName.

    :param registrationName: Registration name for the extractor.
    :type registrationName: `str`

    :return: The extractor or None
    :rtype: :class:`paraview.servermanager.Proxy` or `None`
    """
    pxm = servermanager.ProxyManager()
    return pxm.GetProxy("extractors", registrationName)


def SaveExtractsUsingCatalystOptions(options):
    """Generate extracts using a :class:`CatalystOptions` object. This is intended for
    use when a Catalyst analysis script is being run in batch mode.

    :param options: Catalyst options proxy
    :type options: :class:`CatalystOptions`
    """
    # handle the case where options is CatalystOptions
    return SaveExtracts(
        ExtractsOutputDirectory=options.ExtractsOutputDirectory,
        GenerateCinemaSpecification=options.GenerateCinemaSpecification,
    )


def SaveExtracts(**kwargs):
    """Generate extracts. Parameters are forwarded for 'SaveAnimationExtracts'
    Currently, supported keyword parameters are:

    :param ExtractsOutputDirectory: root directory for extracts. If the directory
        does not exist, it will be created.
    :type ExtractsOutputDirectory: `str`
    :param GenerateCinemaSpecification: If set to `True`, enables generation of a
        cinema specification.
    :type GenerateCinemaSpecification: bool
    :param FrameRate: The frame rate in frames-per-second if the extractor
        supports it.
    :type FrameRate: int
    :param FrameStride: The number of timesteps to skip after a timestep is saved.
    :type FrameStride: int
    :param FrameWindow: The range of timesteps to extract.
    :type FrameWindow: 2-element tuple or list of ints
    """
    # currently, Python state files don't have anything about animation,
    # however, we rely on animation to generate the extracts, so we ensure
    # that the animation is updated based on timesteps in data by explicitly
    # calling UpdateAnimationUsingDataTimeSteps.
    scene = GetAnimationScene()
    if not scene:
        from paraview import print_warning

        print_warning("This Edition does not support animations. Cannot save extracts.")
        return False

    scene.UpdateAnimationUsingDataTimeSteps()

    controller = servermanager.ParaViewPipelineController()
    proxy = servermanager.misc.SaveAnimationExtracts()
    controller.PreInitializeProxy(proxy)
    proxy.AnimationScene = scene
    controller.PostInitializeProxy(proxy)
    proxy_util.set(proxy, **kwargs)
    return proxy.SaveExtracts()


def CreateSteerableParameters(
    steerable_proxy_type_name,
    steerable_proxy_registration_name="SteeringParameters",
    result_mesh_name="steerable",
):
    """Creates a steerable proxy for Catalyst use cases.

    :param steerable_proxy_type_name: Name of the proxy type to create.
    :type steerable_proxy_type_name: str
    :param steerable_proxy_registration_name: Registration name of the proxy
        created by this function. If not provided, defaults to "SteeringParameters".
    :type steerable_proxy_registration_name: str
    :param result_mesh_name: The name of the resulting mesh. If not provided,
        defaults to "steerable".
    :type result_mesh_name: str
    :return: Proxy of the specified type.
    :rtype: Steerable proxy."""
    pxm = servermanager.ProxyManager()
    steerable_proxy = pxm.NewProxy("sources", steerable_proxy_type_name)
    pxm.RegisterProxy("sources", steerable_proxy_registration_name, steerable_proxy)
    steerable_proxy_wrapper = servermanager._getPyProxy(steerable_proxy)
    UpdateSteerableParameters(steerable_proxy_wrapper, result_mesh_name)
    return steerable_proxy_wrapper


def UpdateSteerableParameters(steerable_proxy, steerable_source_name):
    """Updates a steerable proxy of a provided source name.

    :param steerable_proxy: The steerable proxy to update.
    :type steerable_proxy: Source proxy.
    :param steerable_source_name: The name of the steerable source proxy.
    :type steerable_source_name: str"""
    helper = paraview.modules.vtkPVInSitu.vtkInSituInitializationHelper
    helper.UpdateSteerableParameters(steerable_proxy.SMProxy, steerable_source_name)
