/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDeskTopRenderModuleUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDeskTopRenderModuleUI.h"
#include "vtkPVDeskTopRenderModule.h"
#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"
#include "vtkKWCheckButton.h"
#include "vtkKWScale.h"
#include "vtkPVApplication.h"
#include "vtkTimerLog.h"
#include "vtkKWFrameLabeled.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDeskTopRenderModuleUI);
vtkCxxRevisionMacro(vtkPVDeskTopRenderModuleUI, "1.7");

int vtkPVDeskTopRenderModuleUICommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVDeskTopRenderModuleUI::vtkPVDeskTopRenderModuleUI()
{
  this->CommandFunction = vtkPVDeskTopRenderModuleUICommand;
}


//----------------------------------------------------------------------------
vtkPVDeskTopRenderModuleUI::~vtkPVDeskTopRenderModuleUI()
{
}

//----------------------------------------------------------------------------
void vtkPVDeskTopRenderModuleUI::Create(vtkKWApplication *app, const char *)
{
  // Skip over LOD res and threshold.
  
  if (this->IsCreated())
    {
    vtkErrorMacro("RenderModuleUI already created");
    return;
    }

  this->Superclass::Create(app, NULL);

  this->Script("pack forget %s",
               this->ParallelRenderParametersFrame->GetWidgetName());
  //this->CompositeCompressionCheck->EnabledOff();

  //this->SquirtCheck->SetState(0);
  //this->SquirtLabel->EnabledOff();
  //this->SquirtCheck->EnabledOff();
  //this->SquirtLevelScale->EnabledOff();
  //this->SquirtLevelLabel->EnabledOff();

  //this->SetReductionFactor(1);
  //this->ReductionCheck->EnabledOff();
  //this->ReductionLabel->EnabledOff();
}

//----------------------------------------------------------------------------
void vtkPVDeskTopRenderModuleUI::EnableRenductionFactor()
{
  this->SetReductionFactor(2);
  this->ReductionCheck->EnabledOn();
  this->ReductionLabel->EnabledOn();
}



//----------------------------------------------------------------------------
void vtkPVDeskTopRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

