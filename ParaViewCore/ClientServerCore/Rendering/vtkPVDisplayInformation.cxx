/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDisplayInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDisplayInformation.h"

#include "vtkClientServerStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkToolkits.h"

#include "vtkRenderingOpenGLConfigure.h" // needed for VTK_USE_X
#if defined(VTK_USE_X)
#include <X11/Xlib.h>
#endif

#include <vtksys/SystemTools.hxx>
vtkStandardNewMacro(vtkPVDisplayInformation);

int vtkPVDisplayInformation::GlobalCanOpenDisplayLocally = -1;
int vtkPVDisplayInformation::GlobalSupportsOpenGL = -1;

//----------------------------------------------------------------------------
vtkPVDisplayInformation::vtkPVDisplayInformation()
{
  this->CanOpenDisplay = 1;
  this->SupportsOpenGL = 1;
}

//----------------------------------------------------------------------------
vtkPVDisplayInformation::~vtkPVDisplayInformation()
{
}

//----------------------------------------------------------------------------
void vtkPVDisplayInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CanOpenDisplay: " << this->CanOpenDisplay << endl;
  os << indent << "SupportsOpenGL: " << this->SupportsOpenGL << endl;
}

//----------------------------------------------------------------------------
bool vtkPVDisplayInformation::CanOpenDisplayLocally()
{
#if defined(VTK_USE_X)
  if (vtkPVDisplayInformation::GlobalCanOpenDisplayLocally != -1)
  {
    return vtkPVDisplayInformation::GlobalCanOpenDisplayLocally == 1;
  }
  vtkPVOptions* options = vtkProcessModule::GetProcessModule()
    ? vtkProcessModule::GetProcessModule()->GetOptions()
    : NULL;
  if (options && options->GetDisableXDisplayTests() == 0)
  {
    Display* dId = XOpenDisplay((char*)NULL);
    if (dId)
    {
      XCloseDisplay(dId);
      vtkPVDisplayInformation::GlobalCanOpenDisplayLocally = 1;
      return true;
    }
    vtkPVDisplayInformation::GlobalCanOpenDisplayLocally = 0;
    return false;
  }
#endif
  return true;
}

//----------------------------------------------------------------------------
bool vtkPVDisplayInformation::SupportsOpenGLLocally()
{
#ifdef VTKGL2
  if (vtksys::SystemTools::GetEnv("PV_DEBUG_SKIP_OPENGL_VERSION_CHECK") != NULL)
  {
    return true;
  }
  if (!vtkPVDisplayInformation::CanOpenDisplayLocally())
  {
    return false;
  }
  if (vtkPVDisplayInformation::GlobalSupportsOpenGL != -1)
  {
    return vtkPVDisplayInformation::GlobalSupportsOpenGL == 1;
  }

  // We're going to skip OpenGL version checks too  if DisableXDisplayTests
  // command line option is set.
  vtkPVOptions* options = vtkProcessModule::GetProcessModule()
    ? vtkProcessModule::GetProcessModule()->GetOptions()
    : NULL;
  if (options && options->GetDisableXDisplayTests() == 0)
  {
    vtkNew<vtkRenderWindow> window;
    vtkPVDisplayInformation::GlobalSupportsOpenGL = window->SupportsOpenGL();
    return vtkPVDisplayInformation::GlobalSupportsOpenGL == 1;
  }
  return true;
#else
  // We don't do any OpenGL check for old rendering backend since the old
  // backend requires that the window is created for SupportsOpenGL() check to
  // pass.
  return true;
#endif
}

//----------------------------------------------------------------------------
void vtkPVDisplayInformation::CopyFromObject(vtkObject*)
{
  this->CanOpenDisplay = vtkPVDisplayInformation::CanOpenDisplayLocally() ? 1 : 0;
  this->SupportsOpenGL = vtkPVDisplayInformation::SupportsOpenGLLocally() ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkPVDisplayInformation::AddInformation(vtkPVInformation* pvi)
{
  vtkPVDisplayInformation* di = vtkPVDisplayInformation::SafeDownCast(pvi);
  if (!di)
  {
    return;
  }
  if (!this->CanOpenDisplay || !di->CanOpenDisplay)
  {
    this->CanOpenDisplay = 0;
  }
  if (!this->SupportsOpenGL || !di->SupportsOpenGL)
  {
    this->SupportsOpenGL = 0;
  }
}

//----------------------------------------------------------------------------
void vtkPVDisplayInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply << this->CanOpenDisplay << this->SupportsOpenGL
       << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVDisplayInformation::CopyFromStream(const vtkClientServerStream* css)
{
  css->GetArgument(0, 0, &this->CanOpenDisplay);
  css->GetArgument(0, 1, &this->SupportsOpenGL);
}
