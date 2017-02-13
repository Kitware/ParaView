/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOpenGLInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifdef VTKGL2
#include "vtk_glew.h"
#else
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLExtensionManagerConfigure.h"
#include "vtkgl.h"
#endif

#include "vtkPVConfig.h"
#include "vtkPVOpenGLInformation.h"

#include "vtkClientServerStream.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <vtksys/SystemTools.hxx>

#define safes(arg) (arg ? ((const char*)arg) : "")

vtkStandardNewMacro(vtkPVOpenGLInformation);

//----------------------------------------------------------------------------
vtkPVOpenGLInformation::vtkPVOpenGLInformation()
{
  this->RootOnly = 1;
  this->LocalDisplay = false;
  this->Vendor = "Information Unavailable";
  this->Version = "Information Unavailable";
  this->Renderer = "Information Unavailable";
}

//----------------------------------------------------------------------------
vtkPVOpenGLInformation::~vtkPVOpenGLInformation()
{
}

//-----------------------------------------------------------------------------
void vtkPVOpenGLInformation::CopyFromObject(vtkObject* obj)
{
  vtkSmartPointer<vtkRenderWindow> renWin = vtkRenderWindow::SafeDownCast(obj);
  if (!renWin)
  {
    renWin = vtkSmartPointer<vtkRenderWindow>::New();
    renWin->SetOffScreenRendering(1);
    vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();
    renWin->SetDeviceIndex(options->GetEGLDeviceIndex());
    renWin->Render();
  }
  this->SetLocalDisplay(vtkPVDisplayInformation::CanOpenDisplayLocally());
  if (this->LocalDisplay)
  {
    this->SetVendor();
    this->SetVersion();
    this->SetRenderer();
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
  return;
}

//-----------------------------------------------------------------------------
void vtkPVOpenGLInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply << this->Vendor << this->Version << this->Renderer
       << vtkClientServerStream::End;
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

  this->Modified();
#undef PARSE_NEXT_VALUE
}

//----------------------------------------------------------------------------
void vtkPVOpenGLInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const std::string& vtkPVOpenGLInformation::GetVendor()
{
  return this->Vendor;
}

//----------------------------------------------------------------------------
const std::string& vtkPVOpenGLInformation::GetVersion()
{
  return this->Version;
}

//----------------------------------------------------------------------------
const std::string& vtkPVOpenGLInformation::GetRenderer()
{
  return this->Renderer;
}

//----------------------------------------------------------------------------
void vtkPVOpenGLInformation::SetVendor()
{
  this->Vendor = std::string(safes(glGetString(GL_VENDOR)));
}

//----------------------------------------------------------------------------
void vtkPVOpenGLInformation::SetVersion()
{
  this->Version = std::string(safes(glGetString(GL_VERSION)));
}

//----------------------------------------------------------------------------
void vtkPVOpenGLInformation::SetRenderer()
{
  this->Renderer = std::string(safes(glGetString(GL_RENDERER)));
}

//----------------------------------------------------------------------------
bool vtkPVOpenGLInformation::GetLocalDisplay()
{
  return this->LocalDisplay;
}

//----------------------------------------------------------------------------
void vtkPVOpenGLInformation::SetLocalDisplay(bool val)
{
  this->LocalDisplay = val;
}

#undef safes
