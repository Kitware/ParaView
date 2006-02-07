/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIceTDesktopRenderModuleUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVIceTDesktopRenderModuleUI.h"
#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWScale.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkTimerLog.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVIceTDesktopRenderModuleUI);
vtkCxxRevisionMacro(vtkPVIceTDesktopRenderModuleUI, "1.11");

//----------------------------------------------------------------------------
vtkPVIceTDesktopRenderModuleUI::vtkPVIceTDesktopRenderModuleUI()
{
  this->OrderedCompositingCheck = vtkKWCheckButton::New();
  this->OrderedCompositingFlag = 0;
}


//----------------------------------------------------------------------------
vtkPVIceTDesktopRenderModuleUI::~vtkPVIceTDesktopRenderModuleUI()
{
  // Save UI values in regisitry
  vtkPVApplication *pvapp = this->GetPVApplication();
  if (pvapp)
    {
    pvapp->SetRegistryValue(2, "RunTime", "OrderedCompositing", "%d",
                            this->OrderedCompositingFlag);
    }

  this->OrderedCompositingCheck->Delete();
}

//----------------------------------------------------------------------------
void vtkPVIceTDesktopRenderModuleUI::Create()
{
  // Skip over LOD res and threshold.
  
  if (this->IsCreated())
    {
    vtkErrorMacro("RenderModuleUI already created");
    return;
    }

  this->Superclass::Create();

  vtkPVApplication *pvapp = 
    vtkPVApplication::SafeDownCast(this->GetApplication());

  this->Script("pack forget %s",
               this->ParallelRenderParametersFrame->GetWidgetName());

  this->OrderedCompositingCheck->SetParent(this->LODFrame->GetFrame());
  this->OrderedCompositingCheck->Create();
  this->OrderedCompositingCheck->SetText("Enabled Ordered Compositing");
  this->OrderedCompositingCheck->SetCommand(this,
                                            "SetOrderedCompositingFlag");

  if (pvapp && pvapp->GetRegistryValue(2, "RunTime", "OrderedCompositing", 0))
    {
    this->OrderedCompositingFlag
      = pvapp->GetIntRegistryValue(2, "RunTime", "OrderedCompositing");
    }
  this->OrderedCompositingCheck->SetSelectedState(this->OrderedCompositingFlag);
  // This call just forwards the value to the render module.
  this->SetOrderedCompositingFlag(
    this->OrderedCompositingCheck->GetSelectedState());

  this->OrderedCompositingCheck->SetBalloonHelpString(
    "Toggle the use of ordered compositing.  Ordered compositing makes updates "
    "and animations slower, but make volume rendering correct and may speed "
    "up compositing in general.");

  this->Script("pack %s -side top -anchor w",
               this->OrderedCompositingCheck->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVIceTDesktopRenderModuleUI::ResetSettingsToDefault()
{
  this->Superclass::ResetSettingsToDefault();
  this->SetOrderedCompositingFlag(1);
}

//-----------------------------------------------------------------------------
void vtkPVIceTDesktopRenderModuleUI::SetOrderedCompositingFlag(int state)
{
  if (this->OrderedCompositingCheck->GetSelectedState() != state)
    {
    this->OrderedCompositingCheck->SetSelectedState(state);
    }

  this->OrderedCompositingFlag = state;

  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
                    this->RenderModuleProxy->GetProperty("OrderedCompositing"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property OrderedCompositing on "
                  "RenderModuleProxy.");
    return;
    }
  ivp->SetElements1(this->OrderedCompositingFlag);
  this->RenderModuleProxy->UpdateVTKObjects();

  // We use a catch in this trace because the paraview executing
  // the trace might not have this module
  this->GetTraceHelper()->AddEntry(
                                 "catch {$kw(%s) SetOrderedCompositingFlag %d}",
                                 this->GetTclName(),
                                 this->OrderedCompositingFlag);

  this->GetPVApplication()->GetMainView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVIceTDesktopRenderModuleUI::EnableRenductionFactor()
{
  this->SetReductionFactor(2);
  this->ReductionCheck->EnabledOn();
  this->ReductionLabel->EnabledOn();
}



//----------------------------------------------------------------------------
void vtkPVIceTDesktopRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

