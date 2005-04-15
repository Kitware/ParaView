/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIceTRenderModuleUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVIceTRenderModuleUI.h"
#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"
#include "vtkKWCheckButton.h"
#include "vtkKWScale.h"
#include "vtkPVApplication.h"
#include "vtkTimerLog.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVIceTRenderModuleUI);
vtkCxxRevisionMacro(vtkPVIceTRenderModuleUI, "1.5");

int vtkPVIceTRenderModuleUICommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVIceTRenderModuleUI::vtkPVIceTRenderModuleUI()
{
  this->CommandFunction = vtkPVIceTRenderModuleUICommand;
  this->CompositeOptionEnabled = 1;
}


//----------------------------------------------------------------------------
vtkPVIceTRenderModuleUI::~vtkPVIceTRenderModuleUI()
{
}





//----------------------------------------------------------------------------
void vtkPVIceTRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

