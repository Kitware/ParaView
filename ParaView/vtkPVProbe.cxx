/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVProbe.cxx
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
#include "vtkPVProbe.h"
#include "vtkPVApplication.h"
#include "vtkStringList.h"
#include "vtkPVSourceInterface.h"
#include "vtkObjectFactory.h"
#include "vtkTclUtil.h"

int vtkPVProbeCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVProbe::vtkPVProbe()
{
  this->CommandFunction = vtkPVProbeCommand;
  
  this->SelectedPointLabel = vtkKWLabel::New();
  
  this->PointDataLabel = vtkKWLabel::New();
  this->SelectPointsButton = vtkKWPushButton::New();
  
  this->Interactor = NULL;
  
  this->SelectedPoint[0] = 0;
  this->SelectedPoint[1] = 0;
  this->SelectedPoint[2] = 0;

  this->ProbeSourceTclName = NULL;
}

//----------------------------------------------------------------------------
vtkPVProbe::~vtkPVProbe()
{
  this->SelectedPointLabel->Delete();
  this->SelectedPointLabel = NULL;
  
  this->PointDataLabel->Delete();
  this->PointDataLabel = NULL;
  this->SelectPointsButton->Delete();
  this->SelectPointsButton = NULL;
}

//----------------------------------------------------------------------------
vtkPVProbe* vtkPVProbe::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVProbe");
  if (ret)
    {
    return (vtkPVProbe*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVProbe();
}

//----------------------------------------------------------------------------
void vtkPVProbe::CreateProperties()
{
  int error;
  float bounds[6];
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVData *pvd =
    (vtkPVData*)(vtkTclGetPointerFromObject(this->ProbeSourceTclName,
                                            "vtkPVData",
                                            pvApp->GetMainInterp(), error));
  
  this->vtkPVSource::CreateProperties();

  this->SelectPointsButton->SetParent(this->GetParameterFrame()->GetFrame());
  this->SelectPointsButton->Create(pvApp, "-text \"Select Points\"");
  this->SelectPointsButton->SetCommand(this, "SetInteractor");
  
  this->SelectedPointLabel->SetParent(this->GetParameterFrame()->GetFrame());
  this->SelectedPointLabel->Create(pvApp, "");
  this->SelectedPointLabel->SetLabel("Selected Point:");
  
  this->PointDataLabel->SetParent(this->GetParameterFrame()->GetFrame());
  this->PointDataLabel->Create(pvApp, "");
  
  this->Script("pack %s %s %s",
               this->SelectPointsButton->GetWidgetName(),
               this->SelectedPointLabel->GetWidgetName(),
               this->PointDataLabel->GetWidgetName());
  
  this->AcceptCommands->AddString("%s UpdateProbe",
                                  this->GetTclName());
  
  this->SetInteractor();
  
  pvd->GetBounds(bounds);
  this->Interactor->SetBounds(bounds);
  
  this->Script("grab release %s", this->ParameterFrame->GetWidgetName());
}

void vtkPVProbe::SetInteractor()
{
  if (!this->Interactor)
    {
    this->Interactor = 
      vtkKWSelectPointInteractor::SafeDownCast(this->GetWindow()->
                                               GetSelectPointInteractor());
    }
  
  this->GetWindow()->GetMainView()->SetInteractor(this->Interactor);
  this->Interactor->SetCursorVisibility(1);
}

void vtkPVProbe::AcceptCallback()
{
  // call the superclass's method
  this->vtkPVSource::AcceptCallback();
  
  // update the ui to see the point data for the probed point
  vtkPointData *pd = this->GetNthPVOutput(0)->GetVTKData()->GetPointData();
  int numArrays = pd->GetNumberOfArrays();
  int i, j, numComponents;
  vtkDataArray *array;
  char label[256]; // this may need to be longer
  char arrayData[256];
  char tempArray[32];
  vtkPVWindow *window = this->GetWindow();
  vtkPVApplication *pvApp = this->GetPVApplication();
  int numMenus;

  sprintf(label, "Selected Point: (%f, %f, %f)", this->SelectedPoint[0],
          this->SelectedPoint[1], this->SelectedPoint[2]);
  this->SelectedPointLabel->SetLabel(label);
  
  sprintf(label, "");
  
  for (i = 0; i < numArrays; i++)
    {
    array = pd->GetArray(i);
    numComponents = array->GetNumberOfComponents();
    if (numComponents > 1)
      {
      sprintf(arrayData, "%s: (", array->GetName());
      for (j = 0; j < numComponents; j++)
        {
        sprintf(tempArray, "%f", array->GetComponent(0, j));
        if (j < numComponents - 1)
          {
          strcat(tempArray, ",");
          if (j % 3 == 2)
            {
            strcat(tempArray, "\n\t");
            }
          else
            {
            strcat(tempArray, " ");
            }
          }
        else
          {
          strcat(tempArray, ")\n");
          }
        strcat(arrayData, tempArray);
        }
      strcat(label, arrayData);
      }
    else
      {
      sprintf(arrayData, "%s: %f\n", array->GetName(),
              array->GetComponent(0, 0));
      strcat(label, arrayData);
      }
    }
  
  this->PointDataLabel->SetLabel(label);

  this->Interactor->SetCursorVisibility(0);
  
  this->Script("%s index end", window->GetMenu()->GetWidgetName());
  numMenus = atoi(pvApp->GetMainInterp()->result);
  
  for (i = 0; i <= numMenus; i++)
    {
    this->Script("%s entryconfigure %d -state normal",
                 window->GetMenu()->GetWidgetName(), i);
    }
  this->Script("%s configure -state normal",
               window->GetCalculatorButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetThresholdButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetContourButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetGlyphButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetProbeButton()->GetWidgetName());
}

void vtkPVProbe::UpdateProbe()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->Interactor->GetSelectedPoint(this->SelectedPoint);
  
  pvApp->BroadcastScript("vtkPolyData probeSource");
  pvApp->BroadcastScript("vtkIdList pointList");
  pvApp->BroadcastScript("pointList Allocate 1 1");
  pvApp->BroadcastScript("pointList InsertNextId 0");
  pvApp->BroadcastScript("vtkPoints points");
  pvApp->BroadcastScript("points Allocate 1 1");
  pvApp->BroadcastScript("points InsertNextPoint %f %f %f",
                         this->SelectedPoint[0], this->SelectedPoint[1],
                         this->SelectedPoint[2]);
  pvApp->BroadcastScript("probeSource Allocate 1 1");
  pvApp->BroadcastScript("probeSource SetPoints points");
  pvApp->BroadcastScript("probeSource InsertNextCell 1 pointList");
  pvApp->BroadcastScript("%s SetInput probeSource",
                         this->GetVTKSourceTclName());
  pvApp->BroadcastScript("%s SetSource [%s GetVTKData]",
                         this->GetVTKSourceTclName(),
                         this->ProbeSourceTclName);
  pvApp->BroadcastScript("pointList Delete");
  pvApp->BroadcastScript("points Delete");
  pvApp->BroadcastScript("probeSource Delete");
}

void vtkPVProbe::Deselect(vtkKWView *view)
{
  this->Interactor->SetCursorVisibility(0);
  this->vtkPVSource::Deselect(view);
}

void vtkPVProbe::DeleteCallback()
{
  vtkPVWindow *window = this->GetWindow();
  vtkPVApplication *pvApp = this->GetPVApplication();
  int i, numMenus;
  
  this->Interactor->SetCursorVisibility(0);
  
  this->Script("%s index end", window->GetMenu()->GetWidgetName());
  numMenus = atoi(pvApp->GetMainInterp()->result);
  
  for (i = 0; i <= numMenus; i++)
    {
    this->Script("%s entryconfigure %d -state normal",
                 window->GetMenu()->GetWidgetName(), i);
    }
  this->Script("%s configure -state normal",
               window->GetCalculatorButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetThresholdButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetContourButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetGlyphButton()->GetWidgetName());
  this->Script("%s configure -state normal",
               window->GetProbeButton()->GetWidgetName()); 

  // call the superclass
  this->vtkPVSource::DeleteCallback();  
}
