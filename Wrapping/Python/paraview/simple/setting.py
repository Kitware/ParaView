from paraview import servermanager


def GetSettingsProxy(type):
    """Given a string giving the type of settings to access, returns the
    proxy for those settings.

    :param type: The type of settings to access. Valid types may be found
        by calling :func:`GetAllSettings()`.
    :type type: str"""
    pxm = servermanager.ProxyManager()
    proxy = pxm.GetProxy("settings", type)
    return proxy


def GetAllSettings():
    """Get a list of strings that return valid settings proxy types for the
    :func:GetSettingsProxy() function.

    :return: List of valid settings proxy names
    :rtype: list of str"""
    settingsList = []
    pxm = servermanager.ProxyManager()
    pdm = pxm.GetProxyDefinitionManager()
    iter = pdm.NewSingleGroupIterator("settings")
    iter.GoToFirstItem()

    while not iter.IsDoneWithTraversal():
        settingsList.append(iter.GetProxyName())
        iter.GoToNextItem()
    return settingsList
