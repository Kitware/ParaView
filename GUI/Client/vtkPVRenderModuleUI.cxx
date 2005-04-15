/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderModuleUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRenderModuleUI.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkSMRenderModuleProxy.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRenderModuleUI);
vtkCxxRevisionMacro(vtkPVRenderModuleUI, "1.10");
vtkCxxSetObjectMacro(vtkPVRenderModuleUI, RenderModuleProxy, vtkSMRenderModuleProxy);
//----------------------------------------------------------------------------

int vtkPVRenderModuleUICommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVRenderModuleUI::vtkPVRenderModuleUI()
{
  this->CommandFunction = vtkPVRenderModuleUICommand;
  this->RenderModuleProxy = 0;
  this->OutlineThreshold = 5000000.0;
}


//----------------------------------------------------------------------------
vtkPVRenderModuleUI::~vtkPVRenderModuleUI()
{
  this->SetRenderModuleProxy(0);
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVRenderModuleUI::GetPVApplication()
{
  if (this->GetApplication() == NULL)
    {
    return NULL;
    }
  
  if (this->GetApplication()->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->GetApplication());
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}

//----------------------------------------------------------------------------
void vtkPVRenderModuleUI::Create(vtkKWApplication* app, const char *)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", "-bd 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OutlineThreshold: " << this->OutlineThreshold << endl;
  os << indent << "RenderModuleProxy: " << this->RenderModuleProxy << endl;
}

