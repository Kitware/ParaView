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
#include "vtkPVRenderModule.h"



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRenderModuleUI);
vtkCxxRevisionMacro(vtkPVRenderModuleUI, "1.5");

int vtkPVRenderModuleUICommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVRenderModuleUI::vtkPVRenderModuleUI()
{
  this->CommandFunction = vtkPVRenderModuleUICommand;
}


//----------------------------------------------------------------------------
vtkPVRenderModuleUI::~vtkPVRenderModuleUI()
{
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVRenderModuleUI::GetPVApplication()
{
  if (this->Application == NULL)
    {
    return NULL;
    }
  
  if (this->Application->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->Application);
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}

//----------------------------------------------------------------------------
// Not needed in superclass.
void vtkPVRenderModuleUI::SetRenderModule(vtkPVRenderModule *)
{
}


//----------------------------------------------------------------------------
void vtkPVRenderModuleUI::Create(vtkKWApplication* app, const char *)
{
  if (this->Application)
    {
    vtkErrorMacro("Widget has already been created.");
    return;
    }

  this->SetApplication(app);

  // Create this widgets frame.
  this->Script("frame %s -bd 0",this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

