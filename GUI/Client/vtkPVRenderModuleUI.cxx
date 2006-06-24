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
#include "vtkKWCheckButton.h"
#include "vtkPVWindow.h"
#include "vtkPVRenderView.h"
#include "vtkPVCornerAnnotationEditor.h"
#include "vtkKWFrameWithLabel.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRenderModuleUI);
vtkCxxRevisionMacro(vtkPVRenderModuleUI, "1.18");
vtkCxxSetObjectMacro(vtkPVRenderModuleUI, RenderModuleProxy, vtkSMRenderModuleProxy);
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkPVRenderModuleUI::vtkPVRenderModuleUI()
{
  this->RenderModuleProxy = 0;
  this->OutlineThreshold = 5000000.0;
  this->RenderModuleFrame = vtkKWFrameWithLabel::New();
  this->MeasurePolygonsPerSecondFlag = vtkKWCheckButton::New();
}


//----------------------------------------------------------------------------
vtkPVRenderModuleUI::~vtkPVRenderModuleUI()
{
  this->MeasurePolygonsPerSecondFlag->Delete();
  this->MeasurePolygonsPerSecondFlag = NULL;
  this->RenderModuleFrame->Delete();
  this->RenderModuleFrame = NULL;
  this->SetRenderModuleProxy(0);
}

//----------------------------------------------------------------------------
void vtkPVRenderModuleUI::PrepareForDelete()
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
void vtkPVRenderModuleUI::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->RenderModuleFrame->SetParent(this);
  this->RenderModuleFrame->Create();
  this->RenderModuleFrame->SetLabelText("Measurements");
  this->Script("pack %s -padx 2 -pady 2 -fill x -expand yes -anchor w",
               this->RenderModuleFrame->GetWidgetName());

  this->MeasurePolygonsPerSecondFlag->SetParent(this->RenderModuleFrame->GetFrame());
  this->MeasurePolygonsPerSecondFlag->Create();
  this->MeasurePolygonsPerSecondFlag->SetText("Measure Polygons Per Second");
  this->MeasurePolygonsPerSecondFlag->SetCommand(this, "MeasurePolygonsPerSecondCallback");
  this->Script("pack %s -side top -anchor w",
               this->MeasurePolygonsPerSecondFlag->GetWidgetName());

}
//----------------------------------------------------------------------------
void vtkPVRenderModuleUI::ResetSettingsToDefault()
{
  
}

//----------------------------------------------------------------------------
void vtkPVRenderModuleUI::MeasurePolygonsPerSecondCallback(int state)
{
  if (state)
    {
    this->RenderModuleProxy->ResetPolygonsPerSecondResults();
    this->RenderModuleProxy->MeasurePolygonsPerSecondOn();
    this->GetPVApplication()->GetMainWindow()->GetMainView()->GetCornerAnnotation()->SetCornerText(
      "Last: [[$Application GetRenderModuleProxy] GetLastPolygonsPerSecond]\n"
      "Maximum: [[$Application GetRenderModuleProxy] GetMaximumPolygonsPerSecond]\n"
      "Average: [[$Application GetRenderModuleProxy] GetAveragePolygonsPerSecond]", 1);
    this->GetPVApplication()->GetMainWindow()->GetMainView()->GetCornerAnnotation()->VisibilityOn();
    }
  else
    {
    this->RenderModuleProxy->MeasurePolygonsPerSecondOff();
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderModuleUI::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->RenderModuleFrame);
  this->PropagateEnableState(this->MeasurePolygonsPerSecondFlag);
}

//----------------------------------------------------------------------------
void vtkPVRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OutlineThreshold: " << this->OutlineThreshold << endl;
  os << indent << "RenderModuleProxy: " << this->RenderModuleProxy << endl;
}

