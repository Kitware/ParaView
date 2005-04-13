/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCaveRenderModuleUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCaveRenderModuleUI.h"
#include "vtkObjectFactory.h"
#include "vtkKWCheckButton.h"



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCaveRenderModuleUI);
vtkCxxRevisionMacro(vtkPVCaveRenderModuleUI, "1.1.10.1");

int vtkPVCaveRenderModuleUICommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVCaveRenderModuleUI::vtkPVCaveRenderModuleUI()
{
  this->CommandFunction = vtkPVCaveRenderModuleUICommand;
}

//----------------------------------------------------------------------------
vtkPVCaveRenderModuleUI::~vtkPVCaveRenderModuleUI()
{
}


//----------------------------------------------------------------------------
void vtkPVCaveRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

