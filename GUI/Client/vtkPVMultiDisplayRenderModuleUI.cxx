/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMultiDisplayRenderModuleUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMultiDisplayRenderModuleUI.h"
#include "vtkObjectFactory.h"
#include "vtkKWCheckButton.h"



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMultiDisplayRenderModuleUI);
vtkCxxRevisionMacro(vtkPVMultiDisplayRenderModuleUI, "1.6.10.1");

int vtkPVMultiDisplayRenderModuleUICommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVMultiDisplayRenderModuleUI::vtkPVMultiDisplayRenderModuleUI()
{
  this->CommandFunction = vtkPVMultiDisplayRenderModuleUICommand;
  this->CompositeOptionEnabled = 0;
}

//----------------------------------------------------------------------------
vtkPVMultiDisplayRenderModuleUI::~vtkPVMultiDisplayRenderModuleUI()
{
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayRenderModuleUI::Create(vtkKWApplication *app, const char *)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkPVMultiDisplayRenderModuleUI already created");
    return;
    }

  this->Superclass::Create(app, NULL);

  // We do not have these options.
  this->CompositeWithFloatCheck->SetState(0);
  this->CompositeWithFloatCheck->SetEnabled(0);
  this->CompositeWithRGBACheck->SetState(0);
  this->CompositeWithRGBACheck->SetEnabled(0);
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

