# GetOpenGLInformation method in simple.py

A new function, GetOpenGLInformation, has been added to simple.py
It enables to recover OpenGL related information, like GPU vendor,
OpenGL version and capabilities.

It can be used as follows

  openGLInfo = GetOpenGLInformation()
  openGLInfo.GetVendor()
  openGLInfo.GetVersion()
  openGLInfo.GetRenderer()
  openGLInfo.GetCapabilities()

  openGLInfo = GetOpenGLInformation(servermanager.vtkSMSession.RENDER_SERVER)
  openGLInfo.GetVendor()
  openGLInfo.GetVersion()
  openGLInfo.GetRenderer()
  openGLInfo.GetCapabilities()
