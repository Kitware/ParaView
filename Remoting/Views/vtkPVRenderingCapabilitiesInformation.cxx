/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderingCapabilitiesInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRenderingCapabilitiesInformation.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtksys/SystemTools.hxx"

#include <cassert>

// needed for VTK_OPENGL_HAS_OSMESA, VTK_OPENGL_HAS_EGL, VTK_USE_COCOA, VTK_USE_X.
#include "vtkRenderingOpenGLConfigure.h"

#if defined(VTK_USE_X)
#include <X11/Xlib.h>
#endif

#if defined(VTK_OPENGL_HAS_EGL)
#include "vtkEGLRenderWindow.h"
#endif

#if defined(VTK_OPENGL_HAS_OSMESA)
#include "vtkOSOpenGLRenderWindow.h"
#endif

namespace
{
bool SkipDisplayTest()
{
  vtkPVOptions* options = vtkProcessModule::GetProcessModule()
    ? vtkProcessModule::GetProcessModule()->GetOptions()
    : nullptr;
  return options ? (options->GetDisableXDisplayTests() != 0) : false;
}

bool SkipOpenGLTest()
{
  return (vtksys::SystemTools::GetEnv("PV_DEBUG_SKIP_OPENGL_VERSION_CHECK") != nullptr) ||
    SkipDisplayTest();
}

#if defined(VTK_OPENGL_HAS_EGL)
int GetEGLDeviceIndex()
{
  vtkPVOptions* options = vtkProcessModule::GetProcessModule()
    ? vtkProcessModule::GetProcessModule()->GetOptions()
    : nullptr;
  return options ? options->GetEGLDeviceIndex() : -1;
}
#endif
}

vtkStandardNewMacro(vtkPVRenderingCapabilitiesInformation);
//----------------------------------------------------------------------------
vtkPVRenderingCapabilitiesInformation::vtkPVRenderingCapabilitiesInformation()
  : Capabilities(vtkPVRenderingCapabilitiesInformation::NONE)
{
}

//----------------------------------------------------------------------------
vtkPVRenderingCapabilitiesInformation::~vtkPVRenderingCapabilitiesInformation() = default;

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkPVRenderingCapabilitiesInformation::GetLocalCapabilities()
{
  static bool capabilities_initialized = false;
  static vtkTypeUInt32 capabilities = 0;
  if (capabilities_initialized)
  {
    return capabilities;
  }
  capabilities_initialized = true;

#if defined(VTK_USE_COCOA) || defined(_WIN32)
  capabilities |= ONSCREEN_RENDERING;
#elif defined(VTK_USE_X)
  // if using X, need to check if display is accessible.
  if (!SkipDisplayTest())
  {
    Display* dId = XOpenDisplay((char*)nullptr);
    if (dId)
    {
      XCloseDisplay(dId);
      capabilities |= ONSCREEN_RENDERING;
    }
  }
  else
  {
    capabilities |= ONSCREEN_RENDERING;
  }
#endif

#if defined(VTK_OPENGL_HAS_OSMESA)
  capabilities |= HEADLESS_RENDERING_USES_OSMESA;
#endif

#if defined(VTK_OPENGL_HAS_EGL)
  capabilities |= HEADLESS_RENDERING_USES_EGL;
#endif

  if ((capabilities & RENDERING) != 0)
  {
    // now test OpenGL capabilities.
    if (!SkipOpenGLTest())
    {
      vtkSmartPointer<vtkRenderWindow> window =
        vtkPVRenderingCapabilitiesInformation::NewOffscreenRenderWindow();
      if (window && window->SupportsOpenGL())
      {
        capabilities |= OPENGL;
      }
    }
    else
    {
      capabilities |= OPENGL;
    }
  }

  return capabilities;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkRenderWindow> vtkPVRenderingCapabilitiesInformation::NewOffscreenRenderWindow()
{
  vtkSmartPointer<vtkRenderWindow> window;

// if headless rendering is supported, let's create the headless render
// window.
#if defined(VTK_OPENGL_HAS_EGL)
  window.TakeReference(vtkEGLRenderWindow::New());
  // vtkEGLRenderWindow gets initialized with `VTK_DEFAULT_EGL_DEVICE_INDEX`
  // CMake variable. If the command line options overrode it, change it.
  if (window)
  {
    int deviceIndex = GetEGLDeviceIndex();
    if (deviceIndex >= 0)
    {
      window->SetDeviceIndex(deviceIndex);
    }
  }
#elif defined(VTK_OPENGL_HAS_OSMESA)
  window.TakeReference(vtkOSOpenGLRenderWindow::New());
#endif
  if (!window)
  {
    // if not, let VTK create a default based on CMake flags specified.
    window = vtkSmartPointer<vtkRenderWindow>::New();
  }

  window->SetOffScreenRendering(1); // we want to keep the window unmapped.
  // this should be largely unnecessary, but vtkRenderWindow subclasses
  // are fairly inconsistent about this so let's just set it always.

  return window;
}

//----------------------------------------------------------------------------
void vtkPVRenderingCapabilitiesInformation::CopyFromObject(vtkObject*)
{
  this->Capabilities = vtkPVRenderingCapabilitiesInformation::GetLocalCapabilities();
}

//----------------------------------------------------------------------------
void vtkPVRenderingCapabilitiesInformation::AddInformation(vtkPVInformation* other)
{
  if (vtkPVRenderingCapabilitiesInformation* pvci =
        vtkPVRenderingCapabilitiesInformation::SafeDownCast(other))
  {
    this->Capabilities &= pvci->Capabilities;
  }
}

//----------------------------------------------------------------------------
void vtkPVRenderingCapabilitiesInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply << this->Capabilities << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVRenderingCapabilitiesInformation::CopyFromStream(const vtkClientServerStream* css)
{
  css->GetArgument(0, 0, &this->Capabilities);
}

//----------------------------------------------------------------------------
void vtkPVRenderingCapabilitiesInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Capabilities: " << this->Capabilities << endl;
}
