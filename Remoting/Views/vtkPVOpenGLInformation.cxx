// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVOpenGLInformation.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderingCapabilitiesInformation.h"
#include "vtkPVView.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <unordered_map>

namespace
{
// Map from OpenGL render window class names to human-readable window backend names.
std::unordered_map<std::string, std::string> WindowBackendMap{
  { "vtkCocoaRenderWindow", "Cocoa" },
  { "vtkEGLRenderWindow", "EGL" },
  { "vtkOSOpenGLRenderWindow", "OSMesa" },
  { "vtkXOpenGLRenderWindow", "GLX" },
  { "vtkWin32OpenGLRenderWindow", "Win32" },
};
}

vtkStandardNewMacro(vtkPVOpenGLInformation);
//----------------------------------------------------------------------------
vtkPVOpenGLInformation::vtkPVOpenGLInformation()
{
  this->RootOnly = 1;
  this->Vendor = this->Version = this->Renderer = this->Capabilities = this->WindowBackend =
    "Information Unavailable";
}

//----------------------------------------------------------------------------
vtkPVOpenGLInformation::~vtkPVOpenGLInformation() = default;

//-----------------------------------------------------------------------------
void vtkPVOpenGLInformation::CopyFromObject(vtkObject* obj)
{
  this->Vendor = this->Version = this->Renderer = this->Capabilities = this->WindowBackend =
    "Information Unavailable";

  vtkPVView* view = vtkPVView::SafeDownCast(obj);
  vtkSmartPointer<vtkRenderWindow> renWin =
    view ? view->GetRenderWindow() : vtkRenderWindow::SafeDownCast(obj);
  if (renWin == nullptr)
  {
    // create new one based on current process's capabilities.
    renWin = vtkPVRenderingCapabilitiesInformation::NewOffscreenRenderWindow();

    // ensure context is created and OpenGL initialized.
    renWin->Render();
  }

  if (const char* opengl_capabilities = renWin->ReportCapabilities())
  {
    vtksys::RegularExpression reVendor("OpenGL vendor string:[ ]*([^\n\r]*)");
    if (reVendor.find(opengl_capabilities))
    {
      this->Vendor = reVendor.match(1);
    }

    vtksys::RegularExpression reVersion("OpenGL version string:[ ]*([^\n\r]*)");
    if (reVersion.find(opengl_capabilities))
    {
      this->Version = reVersion.match(1);
    }

    vtksys::RegularExpression reRenderer("OpenGL renderer string:[ ]*([^\n\r]*)");
    if (reRenderer.find(opengl_capabilities))
    {
      this->Renderer = reRenderer.match(1);
    }

    this->Capabilities = opengl_capabilities;
  }
  if (const auto it = WindowBackendMap.find(renWin->GetClassName()); it != WindowBackendMap.end())
  {
    this->WindowBackend = it->second;
  }
}

//-----------------------------------------------------------------------------
void vtkPVOpenGLInformation::AddInformation(vtkPVInformation* pvinfo)
{
  if (!pvinfo)
  {
    return;
  }

  vtkPVOpenGLInformation* info = vtkPVOpenGLInformation::SafeDownCast(pvinfo);
  if (!info)
  {
    vtkErrorMacro("Could not downcast to vtkPVOpenGLInformation.");
    return;
  }
  this->Vendor = info->Vendor;
  this->Version = info->Version;
  this->Renderer = info->Renderer;
  this->Capabilities = info->Capabilities;
  this->WindowBackend = info->WindowBackend;
}

//-----------------------------------------------------------------------------
void vtkPVOpenGLInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply << this->Vendor << this->Version << this->Renderer
       << this->Capabilities << this->WindowBackend << vtkClientServerStream::End;
}

//-----------------------------------------------------------------------------
void vtkPVOpenGLInformation::CopyFromStream(const vtkClientServerStream* css)
{
#define PARSE_NEXT_VALUE(_ivarName)                                                                \
  if (!css->GetArgument(0, i++, &this->_ivarName))                                                 \
  {                                                                                                \
    vtkErrorMacro("Error parsing " #_ivarName " from message.");                                   \
    return;                                                                                        \
  }

  int i = 0;
  PARSE_NEXT_VALUE(Vendor);
  PARSE_NEXT_VALUE(Version);
  PARSE_NEXT_VALUE(Renderer);
  PARSE_NEXT_VALUE(Capabilities);
  PARSE_NEXT_VALUE(WindowBackend);
  this->Modified();
#undef PARSE_NEXT_VALUE
}

//----------------------------------------------------------------------------
void vtkPVOpenGLInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
