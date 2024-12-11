# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-License-Identifier: BSD-3-Clause
import paraview
from paraview import servermanager


def GetParaViewVersion():
    """:return: The version of the ParaView build in (major, minor) form.
    :rtype: 2-element tuple"""
    return paraview._version(
        servermanager.vtkSMProxyManager.GetVersionMajor(),
        servermanager.vtkSMProxyManager.GetVersionMinor(),
    )


def GetParaViewSourceVersion():
    """:return: the ParaView source version string, e.g.,
        'paraview version x.x.x, Date: YYYY-MM-DD'.
    :rtype: str"""
    return servermanager.vtkSMProxyManager.GetParaViewSourceVersion()


def GetOpenGLInformation(location=servermanager.vtkSMSession.CLIENT):
    """Recover OpenGL information on either the client or server.

    :param location: Where the OpenGL information should be retrieved, e.g.,
        pass `vtkPVSession.CLIENT` if you want OpenGL info from the client system
        (default value), pass in `vtkPVSession.SERVERS` if you want the server.
    :type location: `vtkPVServer.ServerFlags` enum value"""
    openGLInfo = paraview.modules.vtkRemotingViews.vtkPVOpenGLInformation()
    session = servermanager.vtkSMProxyManager.GetProxyManager().GetActiveSession()
    session.GatherInformation(location, openGLInfo, 0)
    return openGLInfo
