/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVContour.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkPVContour.h"
#include "vtkPVApplication.h"
#include "vtkStringList.h"

int vtkPVContourCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVContour::vtkPVContour()
{
  this->CommandFunction = vtkPVContourCommand;
  
  this->Contour = NULL;
  
  this->ContourValuesLabel = vtkKWLabel::New();
  this->ContourValuesList = vtkKWListBox::New();
  this->NewValueFrame = vtkKWWidget::New();
  this->NewValueLabel = vtkKWLabel::New();
  this->NewValueEntry = vtkKWEntry::New();
  this->AddValueButton = vtkKWPushButton::New();
  this->DeleteValueButton = vtkKWPushButton::New();
  this->ComputeNormalsCheck = vtkKWCheckButton::New();
  this->ComputeGradientsCheck = vtkKWCheckButton::New();
  this->ComputeScalarsCheck = vtkKWCheckButton::New();
}

//----------------------------------------------------------------------------
vtkPVContour::~vtkPVContour()
{
  this->ContourValuesLabel->Delete();
  this->ContourValuesLabel = NULL;
  this->ContourValuesList->Delete();
  this->ContourValuesList = NULL;
  this->NewValueLabel->Delete();
  this->NewValueLabel = NULL;
  this->NewValueEntry->Delete();
  this->NewValueEntry = NULL;
  this->AddValueButton->Delete();
  this->AddValueButton = NULL;
  this->NewValueFrame->Delete();
  this->NewValueFrame = NULL;
  this->DeleteValueButton->Delete();
  this->DeleteValueButton = NULL;
  this->ComputeNormalsCheck->Delete();
  this->ComputeNormalsCheck = NULL;
  this->ComputeGradientsCheck->Delete();
  this->ComputeGradientsCheck = NULL;
  this->ComputeScalarsCheck->Delete();
  this->ComputeScalarsCheck = NULL;
}

//----------------------------------------------------------------------------
vtkPVContour* vtkPVContour::New()
{
  return new vtkPVContour();
}

//----------------------------------------------------------------------------
void vtkPVContour::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  
  this->vtkPVSource::CreateProperties();
  
  this->Contour = (vtkKitwareContourFilter*)this->GetVTKSource();
  if (!this->Contour)
    {
    return;
    }
  
  this->ContourValuesLabel->SetParent(this->GetParameterFrame()->GetFrame());
  this->ContourValuesLabel->Create(pvApp, "");
  this->ContourValuesLabel->SetLabel("Contour Values");
  
  this->ContourValuesList->SetParent(this->GetParameterFrame()->GetFrame());
  this->ContourValuesList->Create(pvApp, "");
  this->ContourValuesList->SetHeight(5);
  
  this->AcceptCommands->AddString("%s ContourValuesCallback",
                                  this->GetTclName());
  
  this->NewValueFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->NewValueFrame->Create(pvApp, "frame", "");
  
  this->Script("pack %s %s %s", this->ContourValuesLabel->GetWidgetName(),
               this->ContourValuesList->GetWidgetName(),
               this->NewValueFrame->GetWidgetName());
  
  this->NewValueLabel->SetParent(this->NewValueFrame);
  this->NewValueLabel->Create(pvApp, "");
  this->NewValueLabel->SetLabel("New Value:");
  
  this->NewValueEntry->SetParent(this->NewValueFrame);
  this->NewValueEntry->Create(pvApp, "");
  this->NewValueEntry->SetValue("");
  
  this->AddValueButton->SetParent(this->NewValueFrame);
  this->AddValueButton->Create(pvApp, "-text \"Add Value\"");
  this->AddValueButton->SetCommand(this, "AddValueCallback");
  
  this->Script("pack %s %s %s -side left",
               this->NewValueLabel->GetWidgetName(),
               this->NewValueEntry->GetWidgetName(),
               this->AddValueButton->GetWidgetName());
  
  this->DeleteValueButton->SetParent(this->GetParameterFrame()->GetFrame());
  this->DeleteValueButton->Create(pvApp, "-text \"Delete Value\"");
  this->DeleteValueButton->SetCommand(this, "DeleteValueCallback");
  
  this->ComputeNormalsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  this->ComputeNormalsCheck->Create(pvApp, "-text \"Compute Normals\"");
  this->ComputeNormalsCheck->SetState(this->Contour->GetComputeNormals());
  
  // Command to update the UI.
  this->CancelCommands->AddString("%s SetState [%s %s]",
                                  this->ComputeNormalsCheck->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "GetComputeNormals");

  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [%s GetState]",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "SetComputeNormals",
                                  this->ComputeNormalsCheck->GetTclName());

  this->ComputeGradientsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  this->ComputeGradientsCheck->Create(pvApp, "-text \"Compute Gradients\"");
  this->ComputeGradientsCheck->SetState(this->Contour->GetComputeGradients());
  
  // Command to update the UI.
  this->CancelCommands->AddString("%s SetState [%s %s]",
                                  this->ComputeGradientsCheck->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "GetComputeGradients");

  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [%s GetState]",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "SetComputeGradients",
                                  this->ComputeGradientsCheck->GetTclName());

  this->ComputeScalarsCheck->SetParent(this->GetParameterFrame()->GetFrame());
  this->ComputeScalarsCheck->Create(pvApp, "-text \"Compute Scalars\"");
  this->ComputeScalarsCheck->SetState(this->Contour->GetComputeScalars());
  
  // Command to update the UI.
  this->CancelCommands->AddString("%s SetState [%s %s]",
                                  this->ComputeScalarsCheck->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "GetComputeScalars");

  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [%s GetState]",
                                  this->GetTclName(),
                                  this->GetVTKSourceTclName(),
                                  "SetComputeScalars",
                                  this->ComputeScalarsCheck->GetTclName());

  this->Script("pack %s %s %s %s -anchor w -padx 10",
               this->DeleteValueButton->GetWidgetName(),
               this->ComputeNormalsCheck->GetWidgetName(),
               this->ComputeGradientsCheck->GetWidgetName(),
               this->ComputeScalarsCheck->GetWidgetName());
}

void vtkPVContour::AddValueCallback()
{
  if (strcmp(this->NewValueEntry->GetValue(), "") == 0)
    {
    return;
    }

  this->ContourValuesList->AppendUnique(this->NewValueEntry->GetValue());
  this->NewValueEntry->SetValue("");
}

void vtkPVContour::DeleteValueCallback()
{
  int index;
  
  index = this->ContourValuesList->GetSelectionIndex();
  this->ContourValuesList->DeleteRange(index, index);
}

void vtkPVContour::ContourValuesCallback()
{
  int i;
  float value;
  int numContours = this->ContourValuesList->GetNumberOfItems();
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->Contour->SetNumberOfContours(numContours);
  
  for (i = 0; i < numContours; i++)
    {
    value = atof(this->ContourValuesList->GetItem(i));
    this->Contour->SetValue(i, value);
    pvApp->BroadcastScript("%s SetValue %d %f",
                           this->GetVTKSourceTclName(),
                           i, value);
    }
}
