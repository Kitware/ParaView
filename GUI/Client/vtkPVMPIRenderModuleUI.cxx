/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMPIRenderModuleUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMPIRenderModuleUI.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMPIRenderModuleUI);
vtkCxxRevisionMacro(vtkPVMPIRenderModuleUI, "1.3.14.1");

int vtkPVMPIRenderModuleUICommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVMPIRenderModuleUI::vtkPVMPIRenderModuleUI()
{
  this->CommandFunction = vtkPVMPIRenderModuleUICommand;
}


//----------------------------------------------------------------------------
vtkPVMPIRenderModuleUI::~vtkPVMPIRenderModuleUI()
{
}



//----------------------------------------------------------------------------
void vtkPVMPIRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

