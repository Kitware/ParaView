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

  this->DimensionalityMenu = vtkKWOptionMenu::New();
  this->DimensionalityLabel = vtkKWLabel::New();
  this->SelectPointButton = vtkKWPushButton::New();
  
  this->SelectedPointFrame = vtkKWWidget::New();
  this->SelectedPointLabel = vtkKWLabel::New();
  this->SelectedXEntry = vtkKWLabeledEntry::New();
  this->SelectedYEntry = vtkKWLabeledEntry::New();
  this->SelectedZEntry = vtkKWLabeledEntry::New();
  this->PointDataLabel = vtkKWLabel::New();
  
  this->EndPointMenuFrame = vtkKWWidget::New();
  this->EndPointLabel = vtkKWLabel::New();
  this->EndPointMenu = vtkKWOptionMenu::New();

  this->EndPoint1Frame = vtkKWWidget::New();
  this->EndPoint1Label = vtkKWLabel::New();
  this->X1Entry = vtkKWLabeledEntry::New();
  this->Y1Entry = vtkKWLabeledEntry::New();
  this->Z1Entry = vtkKWLabeledEntry::New();
  
  this->EndPoint2Frame = vtkKWWidget::New();
  this->EndPoint2Label = vtkKWLabel::New();
  this->X2Entry = vtkKWLabeledEntry::New();
  this->Y2Entry = vtkKWLabeledEntry::New();
  this->Z2Entry = vtkKWLabeledEntry::New();
  this->SetPointButton = vtkKWPushButton::New();
  
  this->ProbeFrame = vtkKWWidget::New();

  this->Interactor = NULL;
  
  this->SelectedPoint[0] = this->SelectedPoint[1] = this->SelectedPoint[2] = 0;
  this->EndPoint1[0] = this->EndPoint1[1] = this->EndPoint1[2] = 0;
  this->EndPoint2[0] = this->EndPoint2[1] = this->EndPoint2[2] = 0;
  this->ProbeSourceTclName = NULL;
  
  this->Dimensionality = -1;
}

//----------------------------------------------------------------------------
vtkPVProbe::~vtkPVProbe()
{
  this->DimensionalityLabel->Delete();
  this->DimensionalityLabel = NULL;
  this->DimensionalityMenu->Delete();
  this->DimensionalityMenu = NULL;
  
  this->SelectPointButton->Delete();
  this->SelectPointButton = NULL;
  
  this->SelectedPointLabel->Delete();
  this->SelectedPointLabel = NULL;
  this->SelectedXEntry->Delete();
  this->SelectedXEntry = NULL;
  this->SelectedYEntry->Delete();
  this->SelectedYEntry = NULL;
  this->SelectedZEntry->Delete();
  this->SelectedZEntry = NULL;
  this->SelectedPointFrame->Delete();
  this->SelectedPointFrame = NULL;
  this->PointDataLabel->Delete();
  this->PointDataLabel = NULL;

  this->EndPointLabel->Delete();
  this->EndPointLabel = NULL;
  this->EndPointMenu->Delete();
  this->EndPointMenu = NULL;
  this->EndPointMenuFrame->Delete();
  this->EndPointMenuFrame = NULL;

  this->EndPoint1Label->Delete();
  this->EndPoint1Label = NULL;
  this->X1Entry->Delete();
  this->X1Entry = NULL;
  this->Y1Entry->Delete();
  this->Y1Entry = NULL;
  this->Z1Entry->Delete();
  this->Z1Entry = NULL;
  this->EndPoint1Frame->Delete();
  this->EndPoint1Frame = NULL;
  
  this->EndPoint2Label->Delete();
  this->EndPoint2Label = NULL;
  this->X2Entry->Delete();
  this->X2Entry = NULL;
  this->Y2Entry->Delete();
  this->Y2Entry = NULL;
  this->Z2Entry->Delete();
  this->Z2Entry = NULL;
  this->EndPoint2Frame->Delete();
  this->EndPoint2Frame = NULL;

  this->SetPointButton->Delete();
  this->SetPointButton = NULL;
  
  this->ProbeFrame->Delete();
  this->ProbeFrame = NULL;
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
  vtkKWWidget *frame;
  vtkPVApplication* pvApp = this->GetPVApplication();
  
  this->vtkPVSource::CreateProperties();

  frame = vtkKWWidget::New();
  frame->SetParent(this->GetParameterFrame()->GetFrame());
  frame->Create(pvApp, "frame", "");
  
  this->DimensionalityLabel->SetParent(frame);
  this->DimensionalityLabel->Create(pvApp, "");
  this->DimensionalityLabel->SetLabel("Probe Dimensionality:");
  
  this->DimensionalityMenu->SetParent(frame);
  this->DimensionalityMenu->Create(pvApp, "");
  this->DimensionalityMenu->AddEntryWithCommand("Point", this, "UsePoint");
  this->DimensionalityMenu->AddEntryWithCommand("Line", this, "UseLine");
  this->DimensionalityMenu->AddEntryWithCommand("Plane", this, "UsePlane");
  this->DimensionalityMenu->SetValue("Point");
  
  this->Script("pack %s %s -side left",
               this->DimensionalityLabel->GetWidgetName(),
               this->DimensionalityMenu->GetWidgetName());
  
  this->SelectPointButton->SetParent(this->GetParameterFrame()->GetFrame());
  this->SelectPointButton->Create(pvApp, "-text \"Select Point\"");
  this->SelectPointButton->SetCommand(this, "SetInteractor");
  
  this->SetPointButton->SetParent(this->ParameterFrame->GetFrame());
  this->SetPointButton->Create(pvApp, "");
  this->SetPointButton->SetLabel("Set Point");
  this->SetPointButton->SetCommand(this, "SetPoint");
  
  this->ProbeFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->ProbeFrame->Create(pvApp, "frame", "");
  
  this->Script("pack %s %s %s %s",
               frame->GetWidgetName(),
               this->SelectPointButton->GetWidgetName(),
               this->SetPointButton->GetWidgetName(),
               this->ProbeFrame->GetWidgetName());
  
  frame->Delete();

  this->SelectedPointFrame->SetParent(this->ProbeFrame);
  this->SelectedPointFrame->Create(pvApp, "frame", "");
  
  this->SelectedPointLabel->SetParent(this->SelectedPointFrame);
  this->SelectedPointLabel->Create(pvApp, "");
  this->SelectedPointLabel->SetLabel("Point");
  this->SelectedXEntry->SetParent(this->SelectedPointFrame);
  this->SelectedXEntry->Create(pvApp);
  this->SelectedXEntry->SetLabel("X:");
  this->Script("%s configure -width 10",
               this->SelectedXEntry->GetEntry()->GetWidgetName());
  this->SelectedXEntry->SetValue(this->SelectedPoint[0], 4);
  this->SelectedYEntry->SetParent(this->SelectedPointFrame);
  this->SelectedYEntry->Create(pvApp);
  this->SelectedYEntry->SetLabel("Y:");
  this->Script("%s configure -width 10",
               this->SelectedYEntry->GetEntry()->GetWidgetName());
  this->SelectedYEntry->SetValue(this->SelectedPoint[1], 4);
  this->SelectedZEntry->SetParent(this->SelectedPointFrame);
  this->SelectedZEntry->Create(pvApp);
  this->SelectedZEntry->SetLabel("Z:");
  this->Script("%s configure -width 10",
               this->SelectedZEntry->GetEntry()->GetWidgetName());
  this->SelectedZEntry->SetValue(this->SelectedPoint[2], 4);
  this->Script("pack %s %s %s %s -side left",
               this->SelectedPointLabel->GetWidgetName(),
               this->SelectedXEntry->GetWidgetName(),
               this->SelectedYEntry->GetWidgetName(),
               this->SelectedZEntry->GetWidgetName());
  
  this->PointDataLabel->SetParent(this->ProbeFrame);
  this->PointDataLabel->Create(pvApp, "");

  this->EndPointMenuFrame->SetParent(this->ProbeFrame);
  this->EndPointMenuFrame->Create(pvApp, "frame", "");
  this->EndPointLabel->SetParent(this->EndPointMenuFrame);
  this->EndPointLabel->Create(pvApp, "");
  this->EndPointLabel->SetLabel("Select End Point:");
  this->EndPointMenu->SetParent(this->EndPointMenuFrame);
  this->EndPointMenu->Create(pvApp, "");
  this->EndPointMenu->AddEntry("End Point 1");
  this->EndPointMenu->AddEntry("End Point 2");
  this->EndPointMenu->SetValue("End Point 1");
  
  this->Script("pack %s %s -side left", this->EndPointLabel->GetWidgetName(),
               this->EndPointMenu->GetWidgetName());

  this->EndPoint1Frame->SetParent(this->ProbeFrame);
  this->EndPoint1Frame->Create(pvApp, "frame", "");
  
  this->EndPoint1Label->SetParent(this->EndPoint1Frame);
  this->EndPoint1Label->Create(pvApp, "");
  this->EndPoint1Label->SetLabel("End Point 1");
  this->X1Entry->SetParent(this->EndPoint1Frame);
  this->X1Entry->Create(pvApp);
  this->X1Entry->SetLabel("X:");
  this->Script("%s configure -width 10",
               this->X1Entry->GetEntry()->GetWidgetName());
  this->X1Entry->SetValue(this->EndPoint1[0], 4);
  this->Y1Entry->SetParent(this->EndPoint1Frame);
  this->Y1Entry->Create(pvApp);
  this->Y1Entry->SetLabel("Y:");
  this->Script("%s configure -width 10",
               this->Y1Entry->GetEntry()->GetWidgetName());
  this->Y1Entry->SetValue(this->EndPoint1[1], 4);
  this->Z1Entry->SetParent(this->EndPoint1Frame);
  this->Z1Entry->Create(pvApp);
  this->Z1Entry->SetLabel("Z:");
  this->Script("%s configure -width 10",
               this->Z1Entry->GetEntry()->GetWidgetName());
  this->Z1Entry->SetValue(this->EndPoint1[2], 4);
  this->Script("pack %s %s %s %s -side left",
               this->EndPoint1Label->GetWidgetName(),
               this->X1Entry->GetWidgetName(), this->Y1Entry->GetWidgetName(),
               this->Z1Entry->GetWidgetName());
  
  this->EndPoint2Frame->SetParent(this->ProbeFrame);
  this->EndPoint2Frame->Create(pvApp, "frame", "");
  
  this->EndPoint2Label->SetParent(this->EndPoint2Frame);
  this->EndPoint2Label->Create(pvApp, "");
  this->EndPoint2Label->SetLabel("End Point 2");
  this->X2Entry->SetParent(this->EndPoint2Frame);
  this->X2Entry->Create(pvApp);
  this->X2Entry->SetLabel("X:");
  this->Script("%s configure -width 10",
               this->X2Entry->GetEntry()->GetWidgetName());
  this->X2Entry->SetValue(this->EndPoint2[0], 4);
  this->Y2Entry->SetParent(this->EndPoint2Frame);
  this->Y2Entry->Create(pvApp);
  this->Y2Entry->SetLabel("Y:");
  this->Script("%s configure -width 10",
               this->Y2Entry->GetEntry()->GetWidgetName());
  this->Y2Entry->SetValue(this->EndPoint2[1], 4);
  this->Z2Entry->SetParent(this->EndPoint2Frame);
  this->Z2Entry->Create(pvApp);
  this->Z2Entry->SetLabel("Z:");
  this->Script("%s configure -width 10",
               this->Z2Entry->GetEntry()->GetWidgetName());
  this->Z2Entry->SetValue(this->EndPoint2[2], 4);
  this->Script("pack %s %s %s %s -side left",
               this->EndPoint2Label->GetWidgetName(),
               this->X2Entry->GetWidgetName(), this->Y2Entry->GetWidgetName(),
               this->Z2Entry->GetWidgetName());
  
  this->AcceptCommands->AddString("%s UpdateProbe",
                                  this->GetTclName());
  
  this->SetInteractor();
  
  this->PVProbeSource->GetBounds(bounds);
  this->Interactor->SetBounds(bounds);
  
  this->Script("grab release %s", this->ParameterFrame->GetWidgetName());

  this->UsePoint();
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
  
  vtkPVWindow *window = this->GetWindow();
  vtkPVApplication *pvApp = this->GetPVApplication();
  int numMenus, i;
  
  if (this->Dimensionality == 0)
    {
    // update the ui to see the point data for the probed point
    vtkPointData *pd = this->GetNthPVOutput(0)->GetVTKData()->GetPointData();
    int numArrays = pd->GetNumberOfArrays();
    vtkIdType j, numComponents;
    vtkDataArray *array;
    char label[256]; // this may need to be longer
    char arrayData[256];
    char tempArray[32];
    
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
    }

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

  if (this->Dimensionality == 0)
    {
    pvApp->BroadcastScript("vtkPolyData probeInput");
    pvApp->BroadcastScript("vtkIdList pointList");
    pvApp->BroadcastScript("pointList Allocate 1 1");
    pvApp->BroadcastScript("pointList InsertNextId 0");
    pvApp->BroadcastScript("vtkPoints points");
    pvApp->BroadcastScript("points Allocate 1 1");
    pvApp->BroadcastScript("points InsertNextPoint %f %f %f",
                           this->SelectedXEntry->GetValueAsFloat(),
                           this->SelectedYEntry->GetValueAsFloat(),
                           this->SelectedZEntry->GetValueAsFloat());
    pvApp->BroadcastScript("probeInput Allocate 1 1");
    pvApp->BroadcastScript("probeInput SetPoints points");
    pvApp->BroadcastScript("probeInput InsertNextCell 1 pointList");
    pvApp->BroadcastScript("%s SetInput probeInput",
                           this->GetVTKSourceTclName());
    pvApp->BroadcastScript("pointList Delete");
    pvApp->BroadcastScript("points Delete");
    pvApp->BroadcastScript("probeInput Delete");
    }
  else if (this->Dimensionality == 1)
    {
    pvApp->BroadcastScript("vtkLineSource line");
    pvApp->BroadcastScript("line SetPoint1 %f %f %f",
                           this->X1Entry->GetValueAsFloat(),
                           this->Y1Entry->GetValueAsFloat(),
                           this->Z1Entry->GetValueAsFloat());
    pvApp->BroadcastScript("line SetPoint2 %f %f %f",
                           this->X2Entry->GetValueAsFloat(),
                           this->Y2Entry->GetValueAsFloat(),
                           this->Z2Entry->GetValueAsFloat());
    pvApp->BroadcastScript("line SetResolution 10");
    pvApp->BroadcastScript("%s SetInput [line GetOutput]",
                           this->GetVTKSourceTclName());
    pvApp->BroadcastScript("line Delete");
    
    this->Script("vtkXYPlotActor xyPlot");
    this->Script("xyPlot AddInput [%s GetOutput]", this->GetVTKSourceTclName());
    this->Script("[xyPlot GetPositionCoordinate] SetValue 0.05 0.05 0");
    this->Script("[xyPlot GetPosition2Coordinate] SetValue 0.8 0.3 0");
    this->Script("xyPlot SetNumberOfXLabels 5");
    this->Script("%s AddActor xyPlot",
                 this->GetWindow()->GetMainView()->GetRendererTclName());
    this->Script("xyPlot Delete");
    }
  else if (this->Dimensionality == 2)
    {
    }
  
  pvApp->BroadcastScript("%s SetSource %s",
                         this->GetVTKSourceTclName(),
                         this->ProbeSourceTclName);
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

void vtkPVProbe::UsePoint()
{
  this->Dimensionality = 0;
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->ProbeFrame->GetWidgetName());
  this->Script("pack %s %s",
               this->SelectedPointFrame->GetWidgetName(),
               this->PointDataLabel->GetWidgetName());
}

void vtkPVProbe::UseLine()
{
  this->Dimensionality = 1;
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->ProbeFrame->GetWidgetName());
  this->Script("pack %s %s %s",
               this->EndPointMenuFrame->GetWidgetName(),
               this->EndPoint1Frame->GetWidgetName(),
               this->EndPoint2Frame->GetWidgetName());
}

void vtkPVProbe::UsePlane()
{
  this->Dimensionality = 2;
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->ProbeFrame->GetWidgetName());
}

void vtkPVProbe::SetPoint()
{
  if (this->Dimensionality == 0)
    {
    this->Interactor->GetSelectedPoint(this->SelectedPoint);
    this->SelectedXEntry->SetValue(this->SelectedPoint[0], 4);
    this->SelectedYEntry->SetValue(this->SelectedPoint[1], 4);
    this->SelectedZEntry->SetValue(this->SelectedPoint[2], 4);
    }
  else if (this->Dimensionality == 1)
    {
    char *endPoint = this->EndPointMenu->GetValue();
    if (strcmp(endPoint, "End Point 1") == 0)
      {
      this->Interactor->GetSelectedPoint(this->EndPoint1);
      this->X1Entry->SetValue(this->EndPoint1[0], 4);
      this->Y1Entry->SetValue(this->EndPoint1[1], 4);
      this->Z1Entry->SetValue(this->EndPoint1[2], 4);
      }
    else if (strcmp(endPoint, "End Point 2") == 0)
      {
      this->Interactor->GetSelectedPoint(this->EndPoint2);
      this->X2Entry->SetValue(this->EndPoint2[0], 4);
      this->Y2Entry->SetValue(this->EndPoint2[1], 4);
      this->Z2Entry->SetValue(this->EndPoint2[2], 4);
      }
    }
  else if (this->Dimensionality == 2)
    {
    }
}
