/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIceTDisplayRenderModuleUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVIceTDisplayRenderModuleUI.h"
#include "vtkPVIceTDisplayRenderModule.h"
#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"
#include "vtkKWCheckButton.h"
#include "vtkKWScale.h"
#include "vtkPVApplication.h"
#include "vtkTimerLog.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVIceTDisplayRenderModuleUI);
vtkCxxRevisionMacro(vtkPVIceTDisplayRenderModuleUI, "1.3");

int vtkPVIceTDisplayRenderModuleUICommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVIceTDisplayRenderModuleUI::vtkPVIceTDisplayRenderModuleUI()
{
  this->CommandFunction = vtkPVIceTDisplayRenderModuleUICommand;

  this->ReductionLabel = vtkKWLabel::New();
  this->ReductionCheck = vtkKWCheckButton::New();
  this->ReductionFactorScale = vtkKWScale::New();
  this->ReductionFactorLabel = vtkKWLabel::New();
  this->ReductionFactor = 2;

  this->IceTRenderModule = NULL;
}


//----------------------------------------------------------------------------
vtkPVIceTDisplayRenderModuleUI::~vtkPVIceTDisplayRenderModuleUI()
{
  // Save UI values in registery.
  vtkPVApplication* pvapp = this->GetPVApplication();
  if (pvapp)
    {
    pvapp->SetRegisteryValue(2, "RunTime", "ReductionFactor", "%d",
                             this->ReductionFactor);
    }


  this->ReductionLabel->Delete();
  this->ReductionLabel = NULL;
  this->ReductionCheck->Delete();
  this->ReductionCheck = NULL;
  this->ReductionFactorScale->Delete();
  this->ReductionFactorScale = NULL;
  this->ReductionFactorLabel->Delete();
  this->ReductionFactorLabel = NULL;

  if (this->IceTRenderModule)
    {
    this->IceTRenderModule->UnRegister(this);
    this->IceTRenderModule = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPVIceTDisplayRenderModuleUI::SetRenderModule(vtkPVRenderModule* rm)
{
  // Super class has a duplicate pointer.
  this->Superclass::SetRenderModule(rm);

  if (this->IceTRenderModule)
    {
    this->IceTRenderModule->UnRegister(this);
    this->IceTRenderModule = NULL;
    }
  this->IceTRenderModule = vtkPVIceTDisplayRenderModule::SafeDownCast(rm);
  if (this->IceTRenderModule)
    {
    this->IceTRenderModule->Register(this);
    }

  if (rm != NULL && this->IceTRenderModule == NULL)
    {
    vtkErrorMacro("Expecting a IceTRenderModule.");
    }
}


//----------------------------------------------------------------------------
void vtkPVIceTDisplayRenderModuleUI::Create(vtkKWApplication *app, const char *)
{
  vtkPVApplication *pvapp = vtkPVApplication::SafeDownCast(app);
  // Skip over LOD res and threshold.
  int row = 4;
  
  if (this->Application)
    {
    vtkErrorMacro("RenderModuleUI already created");
    return;
    }
  this->Superclass::Create(app, NULL);


  // Determines which reduction/subsampling factor to use.
  this->ReductionLabel->SetParent(this->LODScalesFrame);
  this->ReductionLabel->Create(this->Application, "-anchor w");
  this->ReductionLabel->SetLabel("Subsample Rate:");

  this->ReductionCheck->SetParent(this->LODScalesFrame);
  this->ReductionCheck->Create(this->Application, "");
  this->ReductionCheck->SetState(1);
  this->ReductionCheck->SetCommand(this, "ReductionCheckCallback");

  this->ReductionFactorScale->SetParent(this->LODScalesFrame);
  this->ReductionFactorScale->Create(this->Application,
                                      "-orient horizontal");
  this->ReductionFactorScale->SetRange(2, 5);
  this->ReductionFactorScale->SetResolution(1);
  this->ReductionFactorScale->SetValue(this->ReductionFactor);
  this->ReductionFactorScale->SetCommand(this, 
                                         "ReductionFactorScaleCallback");
  this->ReductionFactorScale->SetBalloonHelpString(
           "Subsampling is a compositing LOD technique. "
           "Subsampling will use larger pixels during interaction.");

  this->ReductionFactorLabel->SetParent(this->LODScalesFrame);
  this->ReductionFactorLabel->SetLabel("2 Pixels");
  this->ReductionFactorLabel->Create(this->Application, "-anchor w");
  if (pvapp &&
      pvapp->GetRegisteryValue(2, "RunTime", "ReductionFactor", 0))
    {
    this->SetReductionFactor(
      pvapp->GetIntRegisteryValue(2, "RunTime", "ReductionFactor"));
    }
  else
    {
    this->SetReductionFactor(this->ReductionFactor);
    }

  pvapp->Script("grid %s -row %d -column 2 -sticky nws", 
                this->ReductionFactorLabel->GetWidgetName(), row++);
  pvapp->Script("grid %s -row %d -column 0 -sticky nws", 
                this->ReductionLabel->GetWidgetName(), row);
  pvapp->Script("grid %s -row %d -column 1 -sticky nes", 
                this->ReductionCheck->GetWidgetName(), row);
  pvapp->Script("grid %s -row %d -column 2 -sticky news", 
                this->ReductionFactorScale->GetWidgetName(), row++);
}


//----------------------------------------------------------------------------
void vtkPVIceTDisplayRenderModuleUI::ReductionFactorScaleCallback()
{
  int val = (int)(this->ReductionFactorScale->GetValue());
  this->SetReductionFactor(val);
}

//----------------------------------------------------------------------------
void vtkPVIceTDisplayRenderModuleUI::ReductionCheckCallback()
{
  int val = this->ReductionCheck->GetState();
  if (val)
    {
    val = (int)(this->ReductionFactorScale->GetValue());
    }
  else
    { // value of 1 is disabled.
    val = 1;
    }
  this->SetReductionFactor(val);
}

//----------------------------------------------------------------------------
void vtkPVIceTDisplayRenderModuleUI::SetReductionFactor(int factor)
{
  if (this->ReductionFactor == factor)
    {
    return;
    }

  this->AddTraceEntry("$kw(%s) SetReductionFactor %d", 
                      this->GetTclName(), factor);
  this->ReductionFactor = factor;

  if (factor == 1)
    {
    this->ReductionFactorScale->EnabledOff();
    this->ReductionFactorLabel->EnabledOff();
    this->ReductionCheck->SetState(0);
    this->ReductionFactorLabel->SetLabel("Subsampling Disabled"); 
    vtkTimerLog::MarkEvent("--- Reduction disabled.");
    }
  else
    {
    this->ReductionFactorScale->EnabledOn();
    this->ReductionFactorLabel->EnabledOn();
    this->ReductionFactorScale->SetValue(factor);
    this->ReductionCheck->SetState(1);
    char str[128];
    sprintf(str, "%d Pixels", factor);
    this->ReductionFactorLabel->SetLabel(str); 
     vtkTimerLog::FormatAndMarkEvent("--- Reduction factor %d.", factor);
   }


  if (this->IceTRenderModule)
    {
    this->IceTRenderModule->SetReductionFactor(factor);
    }
}





//----------------------------------------------------------------------------
void vtkPVIceTDisplayRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ReductionFactor: " << this->ReductionFactor << endl;
}

