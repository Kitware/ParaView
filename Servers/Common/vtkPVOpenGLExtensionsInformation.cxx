/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOpenGLExtensionsInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVOpenGLExtensionsInformation.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkProcessModule.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVOptions.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkToolkits.h"

#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtksys/SystemTools.hxx>
#include <vtkstd/set>
#include <vtkstd/algorithm>

//-----------------------------------------------------------------------------
class vtkPVOpenGLExtensionsInformationInternal
{
public:
  typedef vtkstd::vector<vtkstd::string> VectorOfStrings;
  VectorOfStrings Extensions;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkPVOpenGLExtensionsInformation);
vtkCxxRevisionMacro(vtkPVOpenGLExtensionsInformation, "1.1.8.3");
//-----------------------------------------------------------------------------
vtkPVOpenGLExtensionsInformation::vtkPVOpenGLExtensionsInformation()
{
  this->Internal = new vtkPVOpenGLExtensionsInformationInternal;
}

//-----------------------------------------------------------------------------
vtkPVOpenGLExtensionsInformation::~vtkPVOpenGLExtensionsInformation()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkPVOpenGLExtensionsInformation::CopyFromObject(vtkObject* obj)
{
  this->Internal->Extensions.clear();

  vtkProcessModule* pm = vtkProcessModule::SafeDownCast(obj);
  if (!pm)
    {
    vtkErrorMacro("Cannot downcast to vtkProcessModule.");
    return;
    }
  vtkSmartPointer<vtkPVDisplayInformation> di = 
    vtkSmartPointer<vtkPVDisplayInformation>::New();
  di->CopyFromObject(pm);

  // If we are using Mesa and offscreen rendering, pretend no
  // extensions are supported. Although this is not necessarily true,
  // it is acceptable to disable extensions when using software rendering.
#ifdef VTK_OPENGL_HAS_OSMESA
  vtkPVOptions* options = pm->GetOptions();
  if (options->GetUseOffscreenRendering())
    {
    return;
    }
#endif

  if (!di->GetCanOpenDisplay())
    {
    return;
    }

  vtkRenderWindow* renWin = vtkRenderWindow::New();
  if (!renWin)
    {
    vtkErrorMacro("Cannot create render window.");
    return;
    }
  renWin->SetSize(1,1);
  vtkOpenGLExtensionManager* mgr = vtkOpenGLExtensionManager::New();
  mgr->SetRenderWindow(renWin);
  mgr->Update();
  vtksys::SystemTools::Split(mgr->GetExtensionsString(), 
    this->Internal->Extensions, ' ');
  mgr->Delete();
  renWin->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVOpenGLExtensionsInformation::AddInformation(vtkPVInformation* pvinfo)
{
  if (!pvinfo)
    {
    return;
    }

  vtkPVOpenGLExtensionsInformation* info = 
    vtkPVOpenGLExtensionsInformation::SafeDownCast(pvinfo);
  if (!info)
    {
    vtkErrorMacro("Could not downcast to vtkPVOpenGLExtensionsInformation.");
    return;
    }
  vtkstd::set<vtkstd::string> setSelf;
  vtkstd::set<vtkstd::string> setOther;

  vtkPVOpenGLExtensionsInformationInternal::VectorOfStrings::iterator iter;
  for (iter = this->Internal->Extensions.begin(); 
    iter != this->Internal->Extensions.end();
    ++iter)
    {
    setSelf.insert(*iter);
    }

  for (iter = info->Internal->Extensions.begin(); 
    iter != info->Internal->Extensions.end();
    ++iter)
    {
    setOther.insert(*iter);
    }

  this->Internal->Extensions.clear();

  vtkstd::set_intersection(setSelf.begin(), setSelf.end(),
    setOther.begin(), setOther.end(),
    vtkstd::inserter(this->Internal->Extensions, this->Internal->Extensions.begin()));
}

//-----------------------------------------------------------------------------
void vtkPVOpenGLExtensionsInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;

  vtkstd::string data;
  vtkPVOpenGLExtensionsInformationInternal::VectorOfStrings::iterator iter;
  for (iter = this->Internal->Extensions.begin(); 
    iter != this->Internal->Extensions.end();
    ++iter)
    {
    data += (*iter) + " ";
    }

  *css << data.c_str();
  *css << vtkClientServerStream::End;
}

//-----------------------------------------------------------------------------
void vtkPVOpenGLExtensionsInformation::CopyFromStream(
  const vtkClientServerStream* css)
{
  this->Internal->Extensions.clear();
  const char* ext = 0;
  if (!css->GetArgument(0, 0, &ext))
    {
    vtkErrorMacro("Error parsing extensions string from message.");
    return;
    }
  vtksys::SystemTools::Split(ext, this->Internal->Extensions, ' ');
}

//-----------------------------------------------------------------------------
bool vtkPVOpenGLExtensionsInformation::ExtensionSupported(const char* ext)
{
  vtkPVOpenGLExtensionsInformationInternal::VectorOfStrings::iterator iter;
  for (iter = this->Internal->Extensions.begin(); 
    iter != this->Internal->Extensions.end();
    ++iter)
    {
    if (*iter == ext)
      {
      return true;
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
void vtkPVOpenGLExtensionsInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Supported Extensions: " << endl;
  vtkPVOpenGLExtensionsInformationInternal::VectorOfStrings::iterator iter;
  for (iter = this->Internal->Extensions.begin(); 
    iter != this->Internal->Extensions.end();
    ++iter)
    {
    os << indent.GetNextIndent() << *iter << endl;
    }
}
