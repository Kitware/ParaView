/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProbe.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVProbe.h"
#include "vtkPVApplication.h"
#include "vtkStringList.h"
#include "vtkPVSourceInterface.h"
#include "vtkObjectFactory.h"
#include "vtkTclUtil.h"
#include "vtkKWToolbar.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPVArraySelection.h"

int vtkPVProbeCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVProbe::vtkPVProbe()
{
  static int instanceCount = 0;
  
  this->CommandFunction = vtkPVProbeCommand;

  this->DimensionalityMenu = vtkKWOptionMenu::New();
  this->DimensionalityLabel = vtkKWLabel::New();
  this->SelectPointButton = vtkKWRadioButton::New();
  
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
  this->End1XEntry = vtkKWLabeledEntry::New();
  this->End1YEntry = vtkKWLabeledEntry::New();
  this->End1ZEntry = vtkKWLabeledEntry::New();
  
  this->EndPoint2Frame = vtkKWWidget::New();
  this->EndPoint2Label = vtkKWLabel::New();
  this->End2XEntry = vtkKWLabeledEntry::New();
  this->End2YEntry = vtkKWLabeledEntry::New();
  this->End2ZEntry = vtkKWLabeledEntry::New();
  
  this->DivisionsEntry = vtkKWLabeledEntry::New();
  
  this->ProbeFrame = vtkKWWidget::New();

  this->Interactor = NULL;
  
  this->ProbeSourceTclName = NULL;
  this->Dimensionality = -1;
  this->CurrentEndPoint = -1;
  
  this->PVProbeSource = NULL;
  
  this->XYPlotTclName = NULL;
  
  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
  
  this->PreviousInteractor = NULL;
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
  this->End1XEntry->Delete();
  this->End1XEntry = NULL;
  this->End1YEntry->Delete();
  this->End1YEntry = NULL;
  this->End1ZEntry->Delete();
  this->End1ZEntry = NULL;
  this->EndPoint1Frame->Delete();
  this->EndPoint1Frame = NULL;
  
  this->EndPoint2Label->Delete();
  this->EndPoint2Label = NULL;
  this->End2XEntry->Delete();
  this->End2XEntry = NULL;
  this->End2YEntry->Delete();
  this->End2YEntry = NULL;
  this->End2ZEntry->Delete();
  this->End2ZEntry = NULL;
  this->EndPoint2Frame->Delete();
  this->EndPoint2Frame = NULL;
  
  this->DivisionsEntry->Delete();
  this->DivisionsEntry = NULL;

  this->ProbeFrame->Delete();
  this->ProbeFrame = NULL;

  this->SetPVProbeSource(NULL);
  
  if (this->XYPlotTclName)
    {
    this->Script("%s Delete", this->XYPlotTclName);
    this->SetXYPlotTclName(NULL);
    }
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
  float bounds[6], point[3];
  vtkKWWidget *frame;
  vtkPVApplication* pvApp = this->GetPVApplication();
  char tclName[100];
  
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
  this->DimensionalityMenu->SetValue("Point");
  
  this->Script("pack %s %s -side left",
               this->DimensionalityLabel->GetWidgetName(),
               this->DimensionalityMenu->GetWidgetName());
  
  this->SelectPointButton->SetParent(this->GetPVWindow()->GetInteractorToolbar());
  this->SelectPointButton->Create(pvApp, "-indicatoron 0 -image PV3DCursorButton -selectimage PVActive3DCursorButton -bd 0");
  this->SelectPointButton->SetBalloonHelpString("3D Cursor");
  this->SelectPointButton->SetCommand(this, "SetInteractor");
  this->SetInteractor();

  this->PVProbeSource->GetBounds(bounds);
  this->Interactor->SetBounds(bounds);
  this->Interactor->GetSelectedPoint(point);
  
  this->ProbeFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->ProbeFrame->Create(pvApp, "frame", "");
  
  this->Script("pack %s %s",
               frame->GetWidgetName(),
               this->ProbeFrame->GetWidgetName());
  
  frame->Delete();

  // widgets for points
  this->SelectedPointFrame->SetParent(this->ProbeFrame);
  this->SelectedPointFrame->Create(pvApp, "frame", "");
  
  this->SelectedPointLabel->SetParent(this->SelectedPointFrame);
  this->SelectedPointLabel->Create(pvApp, "");
  this->SelectedPointLabel->SetLabel("Point");
  this->SelectedXEntry->SetParent(this->SelectedPointFrame);
  this->SelectedXEntry->Create(pvApp);
  this->SelectedXEntry->SetLabel("X:");
  this->Script("bind %s <KeyPress> {%s ChangeAcceptButtonColor}",
               this->SelectedXEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->SelectedXEntry->GetEntry()->GetWidgetName());
  this->SelectedXEntry->SetValue(point[0], 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeXPosition}",
               this->SelectedXEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());
  
  this->SelectedYEntry->SetParent(this->SelectedPointFrame);
  this->SelectedYEntry->Create(pvApp);
  this->SelectedYEntry->SetLabel("Y:");
  this->Script("bind %s <KeyPress> {%s ChangeAcceptButtonColor}",
               this->SelectedYEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->SelectedYEntry->GetEntry()->GetWidgetName());
  this->SelectedYEntry->SetValue(point[1], 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeYPosition}",
               this->SelectedYEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->SelectedZEntry->SetParent(this->SelectedPointFrame);
  this->SelectedZEntry->Create(pvApp);
  this->SelectedZEntry->SetLabel("Z:");
  this->Script("bind %s <KeyPress> {%s ChangeAcceptButtonColor}",
               this->SelectedZEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->SelectedZEntry->GetEntry()->GetWidgetName());
  this->SelectedZEntry->SetValue(point[2], 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeZPosition}",
               this->SelectedZEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->Script("pack %s %s %s %s -side left",
               this->SelectedPointLabel->GetWidgetName(),
               this->SelectedXEntry->GetWidgetName(),
               this->SelectedYEntry->GetWidgetName(),
               this->SelectedZEntry->GetWidgetName());
  
  this->PointDataLabel->SetParent(this->ProbeFrame);
  this->PointDataLabel->Create(pvApp, "");

  // widgets for lines
  this->EndPointMenuFrame->SetParent(this->ProbeFrame);
  this->EndPointMenuFrame->Create(pvApp, "frame", "");
  this->EndPointLabel->SetParent(this->EndPointMenuFrame);
  this->EndPointLabel->Create(pvApp, "");
  this->EndPointLabel->SetLabel("Select End Point:");
  this->EndPointMenu->SetParent(this->EndPointMenuFrame);
  this->EndPointMenu->Create(pvApp, "");
  this->EndPointMenu->AddEntryWithCommand("End Point 1", this,
                                          "SetCurrentEndPoint 1");
  this->EndPointMenu->AddEntryWithCommand("End Point 2", this,
                                          "SetCurrentEndPoint 2");
  this->EndPointMenu->SetValue("End Point 1");
  
  this->Script("pack %s %s -side left", this->EndPointLabel->GetWidgetName(),
               this->EndPointMenu->GetWidgetName());

  this->EndPoint1Frame->SetParent(this->ProbeFrame);
  this->EndPoint1Frame->Create(pvApp, "frame", "");
  
  this->EndPoint1Label->SetParent(this->EndPoint1Frame);
  this->EndPoint1Label->Create(pvApp, "");
  this->EndPoint1Label->SetLabel("End Point 1");
  this->End1XEntry->SetParent(this->EndPoint1Frame);
  this->End1XEntry->Create(pvApp);
  this->End1XEntry->SetLabel("X:");
  this->Script("bind %s <KeyPress> {%s ChangeAcceptButtonColor}",
               this->End1XEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->End1XEntry->GetEntry()->GetWidgetName());
  this->End1XEntry->SetValue(point[0], 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeXPosition}",
               this->End1XEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->End1YEntry->SetParent(this->EndPoint1Frame);
  this->End1YEntry->Create(pvApp);
  this->End1YEntry->SetLabel("Y:");
  this->Script("bind %s <KeyPress> {%s ChangeAcceptButtonColor}",
               this->End1YEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->End1YEntry->GetEntry()->GetWidgetName());
  this->End1YEntry->SetValue(point[1], 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeYPosition}",
               this->End1YEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->End1ZEntry->SetParent(this->EndPoint1Frame);
  this->End1ZEntry->Create(pvApp);
  this->End1ZEntry->SetLabel("Z:");
  this->Script("bind %s <KeyPress> {%s ChangeAcceptButtonColor}",
               this->End1ZEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->End1ZEntry->GetEntry()->GetWidgetName());
  this->End1ZEntry->SetValue(point[2], 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeZPosition}",
               this->End1ZEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->Script("pack %s %s %s %s -side left",
               this->EndPoint1Label->GetWidgetName(),
               this->End1XEntry->GetWidgetName(),
               this->End1YEntry->GetWidgetName(),
               this->End1ZEntry->GetWidgetName());
  
  this->EndPoint2Frame->SetParent(this->ProbeFrame);
  this->EndPoint2Frame->Create(pvApp, "frame", "");
  
  this->EndPoint2Label->SetParent(this->EndPoint2Frame);
  this->EndPoint2Label->Create(pvApp, "");
  this->EndPoint2Label->SetLabel("End Point 2");
  this->End2XEntry->SetParent(this->EndPoint2Frame);
  this->End2XEntry->Create(pvApp);
  this->End2XEntry->SetLabel("X:");
  this->Script("bind %s <KeyPress> {%s ChangeAcceptButtonColor}",
               this->End2XEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->End2XEntry->GetEntry()->GetWidgetName());
  this->End2XEntry->SetValue(point[0], 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeXPosition}",
               this->End2XEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->End2YEntry->SetParent(this->EndPoint2Frame);
  this->End2YEntry->Create(pvApp);
  this->End2YEntry->SetLabel("Y:");
  this->Script("bind %s <KeyPress> {%s ChangeAcceptButtonColor}",
               this->End2YEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->End2YEntry->GetEntry()->GetWidgetName());
  this->End2YEntry->SetValue(point[1], 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeYPosition}",
               this->End2YEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->End2ZEntry->SetParent(this->EndPoint2Frame);
  this->End2ZEntry->Create(pvApp);
  this->End2ZEntry->SetLabel("Z:");
  this->Script("bind %s <KeyPress> {%s ChangeAcceptButtonColor}",
               this->End2ZEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->End2ZEntry->GetEntry()->GetWidgetName());
  this->End2ZEntry->SetValue(point[2], 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeZPosition}",
               this->End2ZEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->Script("pack %s %s %s %s -side left",
               this->EndPoint2Label->GetWidgetName(),
               this->End2XEntry->GetWidgetName(),
               this->End2YEntry->GetWidgetName(),
               this->End2ZEntry->GetWidgetName());
  
  this->DivisionsEntry->SetParent(this->ProbeFrame);
  this->DivisionsEntry->Create(pvApp);
  this->DivisionsEntry->SetLabel("Number of Line Divisions:");
  this->DivisionsEntry->SetValue(10);
  this->Script("bind %s <KeyPress> {%s ChangeAcceptButtonColor}",
               this->DivisionsEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  
  this->Script("grab release %s", this->ParameterFrame->GetWidgetName());

  this->UsePoint();
  
  sprintf(tclName, "XYPlot%d", this->InstanceCount);
  this->SetXYPlotTclName(tclName);
  pvApp->BroadcastScript("vtkXYPlotActor %s", this->XYPlotTclName);
  this->Script("[%s GetPositionCoordinate] SetValue 0.05 0.05 0",
	       this->XYPlotTclName);
  this->Script("[%s GetPosition2Coordinate] SetValue 0.8 0.3 0",
	       this->XYPlotTclName);
  this->Script("%s SetNumberOfXLabels 5", this->XYPlotTclName);
  this->Script("%s SetXTitle {Line Divisions}", this->XYPlotTclName);
}

void vtkPVProbe::SetInteractor()
{
  vtkPVRenderView *view = this->GetPVWindow()->GetMainView();
  
  if (!this->Interactor)
    {
    this->Interactor = 
      vtkKWSelectPointInteractor::SafeDownCast(this->GetPVWindow()->
                                               GetSelectPointInteractor());
    this->Interactor->SetPVProbe(this);
    this->Interactor->SetToolbarButton(this->SelectPointButton);
    }

  if (this->PreviousInteractor != view->GetInteractor() &&
      view->GetInteractor() != this->Interactor)
    {
    this->PreviousInteractor = view->GetInteractor();
    }
  
  view->SetInteractor(this->Interactor);
}

void vtkPVProbe::AcceptCallback()
{
  int i;

  // This can't be an accept command because this calls SetInput for the
  // vtkProbeFilter, and vtkPVSource::AcceptCallback expects that the the
  // input has already been set.
  this->UpdateProbe();
  
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->Dimensionality == 0)
    {
    pvApp->AddTraceEntry("$pv(%s) UsePoint", this->GetTclName());
    pvApp->AddTraceEntry("[$pv(%s) GetInteractor] SetSelectedPoint %f %f %f",
                         this->GetTclName(),
                         this->SelectedXEntry->GetValueAsFloat(),
                         this->SelectedYEntry->GetValueAsFloat(),
                         this->SelectedZEntry->GetValueAsFloat());    
    }
  else
    {
    pvApp->AddTraceEntry("$pv(%s) UseLine", this->GetTclName());
    pvApp->AddTraceEntry("[$pv(%s) GetScalarOperationMenu] SetValue %s",
                         this->GetTclName(),
                         this->GetScalarOperationMenu()->GetValue());
    pvApp->AddTraceEntry("$pv(%s) UpdateScalars", this->GetTclName());
    pvApp->AddTraceEntry("[$pv(%s) GetEndPointMenu] SetValue \"End Point 1\"",
                         this->GetTclName());
    pvApp->AddTraceEntry("$pv(%s) SetCurrentEndPoint 1",
                         this->GetTclName());
    pvApp->AddTraceEntry("[$pv(%s) GetInteractor] SetSelectedPoint %f %f %f",
                         this->GetTclName(),
                         this->End1XEntry->GetValueAsFloat(),
                         this->End1YEntry->GetValueAsFloat(),
                         this->End1ZEntry->GetValueAsFloat());
    pvApp->AddTraceEntry("[$pv(%s) GetEndPointMenu] SetValue \"End Point 2\"",
                         this->GetTclName());
    pvApp->AddTraceEntry("$pv(%s) SetCurrentEndPoint 2",
                         this->GetTclName());
    pvApp->AddTraceEntry("[$pv(%s) GetInteractor] SetSelectedPoint %f %f %f",
                         this->GetTclName(),
                         this->End2XEntry->GetValueAsFloat(),
                         this->End2YEntry->GetValueAsFloat(),
                         this->End2ZEntry->GetValueAsFloat());
    }
  
  // call the superclass's method
  this->vtkPVSource::AcceptCallback();
  
  vtkPVWindow *window = this->GetPVWindow();
  
  if (this->Dimensionality == 0)
    {
    // update the ui to see the point data for the probed point
    int error;
    vtkPolyData *probeOutput = (vtkPolyData *)
      (vtkTclGetPointerFromObject(this->GetNthPVOutput(0)->GetVTKDataTclName(),
				  "vtkPolyData", pvApp->GetMainInterp(),
				  error));

    vtkPointData *pd = probeOutput->GetPointData();
    
    int numArrays = pd->GetNumberOfArrays();
    
    vtkIdType j, numComponents;
    vtkDataArray *array;
    char label[256]; // this may need to be longer
    char arrayData[256];
    char tempArray[32];

    // label needs to be initialized so strcat doesn't fail
    label[0] = '\0';
    
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
  else if (this->Dimensionality == 1)
    {    
    if (!this->ChangeScalarsFilterTclName)
      {
      char tclName[256];
      sprintf(tclName, "ChangeScalars%d", this->InstanceCount);
      this->SetChangeScalarsFilterTclName(tclName);
      pvApp->BroadcastScript("vtkFieldDataToAttributeDataFilter %s",
                             this->ChangeScalarsFilterTclName);
      pvApp->BroadcastScript("%s SetInput [%s GetPolyDataOutput]",
                             this->ChangeScalarsFilterTclName,
                             this->VTKSourceTclName);
      }
    pvApp->BroadcastScript("%s SetInputFieldToPointDataField",
                           this->ChangeScalarsFilterTclName);
    pvApp->BroadcastScript("%s SetOutputAttributeDataToPointData",
                           this->ChangeScalarsFilterTclName);
    pvApp->BroadcastScript("%s SetScalarComponent 0 %s 0",
                           this->ChangeScalarsFilterTclName,
                           this->DefaultScalarsName);

    this->Script("%s SetYTitle %s", this->XYPlotTclName, this->DefaultScalarsName);

    this->Script("set numItems [[%s GetInputList] GetNumberOfItems]",
		 this->XYPlotTclName);

    if (atoi(pvApp->GetMainInterp()->result) > 0)
      {
      this->Script("%s RemoveInput [[%s GetInputList] GetItem 0]",
                   this->XYPlotTclName, this->XYPlotTclName);
      }
    
    this->Script("%s AddInput [%s GetOutput]", this->XYPlotTclName,
                 this->ChangeScalarsFilterTclName);
    
    this->Script("%s AddActor %s",
		 window->GetMainView()->GetRendererTclName(),
		 this->XYPlotTclName);
    window->GetMainView()->Render();
    }
}

void vtkPVProbe::UpdateProbe()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  if (!pvApp)
    {
    vtkErrorMacro("No vtkPVApplication set");
    return;
    }

  vtkMultiProcessController *controller = pvApp->GetController();
  
  if (!controller)
    {
    vtkErrorMacro("No vtkMultiProcessController");
    return;
    }
  
  vtkPolyData *probeOutput, *remoteProbeOutput = vtkPolyData::New();
  int error, i, k, numProcs, numComponents;
  vtkIdType numPoints, numRemotePoints, j, pointId;
  vtkIdTypeArray *validPoints = vtkIdTypeArray::New();
  vtkPointData *pointData, *remotePointData;
  float *tuple;
  char *outputTclName;
  
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
                           this->End1XEntry->GetValueAsFloat(),
                           this->End1YEntry->GetValueAsFloat(),
                           this->End1ZEntry->GetValueAsFloat());
    pvApp->BroadcastScript("line SetPoint2 %f %f %f",
                           this->End2XEntry->GetValueAsFloat(),
                           this->End2YEntry->GetValueAsFloat(),
                           this->End2ZEntry->GetValueAsFloat());
    pvApp->BroadcastScript("line SetResolution %d",
			   this->DivisionsEntry->GetValueAsInt());
    pvApp->BroadcastScript("%s SetInput [line GetOutput]",
                           this->GetVTKSourceTclName());
    pvApp->BroadcastScript("line Delete");  
    }
  
  this->Script("set isPresent [[%s GetProps] IsItemPresent %s]",
               this->GetPVRenderView()->GetRendererTclName(),
               this->XYPlotTclName);
  
  if (atoi(this->GetPVApplication()->GetMainInterp()->result) > 0)
    {
    this->Script("%s RemoveActor %s",
                 this->GetPVRenderView()->GetRendererTclName(),
                 this->XYPlotTclName);
    this->GetPVRenderView()->Render();
    }    
  
  pvApp->BroadcastScript("%s SetSource %s",
                         this->GetVTKSourceTclName(),
                         this->ProbeSourceTclName);
  this->GetNthPVOutput(0)->Update();
  
  pvApp->BroadcastScript("Application SendProbeData %s",
			 this->GetVTKSourceTclName());

  outputTclName = this->GetNthPVOutput(0)->GetVTKDataTclName();
  
  probeOutput =
    (vtkPolyData*)(vtkTclGetPointerFromObject(outputTclName, "vtkPolyData",
					      pvApp->GetMainInterp(), error));
  pointData = probeOutput->GetPointData();  
  numPoints = probeOutput->GetNumberOfPoints();
  numProcs = controller->GetNumberOfProcesses();
  numComponents = pointData->GetNumberOfComponents();
  tuple = new float[numComponents];
  
  for (i = 1; i < numProcs; i++)
    {
    controller->Receive(&numRemotePoints, 1, i, 1970);
    if (numRemotePoints > 0)
      {
      controller->Receive(validPoints, i, 1971);
      controller->Receive(remoteProbeOutput, i, 1972);
      
      remotePointData = remoteProbeOutput->GetPointData();
      for (j = 0; j < numRemotePoints; j++)
	{
	pointId = validPoints->GetValue(j);
	
	remotePointData->GetTuple(pointId, tuple);
	
	for (k = 0; k < numComponents; k++)
	  {
	  this->Script("[%s GetPointData] SetComponent %d %d %f",
		       outputTclName, pointId, k, tuple[k]);
	  }
	}
      }
    }  

  delete [] tuple;
  validPoints->Delete();
  remoteProbeOutput->Delete();
}

void vtkPVProbe::Select(vtkKWView *view)
{
  vtkPVRenderView *renderView = this->GetPVWindow()->GetMainView();
  
  this->Script("pack %s -padx 2 -pady 2",
               this->SelectPointButton->GetWidgetName());
  if (this->PreviousInteractor != renderView->GetInteractor() &&
      renderView->GetInteractor() != this->Interactor)
    {
    this->PreviousInteractor = renderView->GetInteractor();
    }

  if (this->Interactor)
    {
    this->Interactor->SetCursorVisibility(1);
    }
  
  this->vtkPVSource::Select(view);
}

void vtkPVProbe::Deselect(vtkKWView *view)
{
  this->Script("set isPresent [[%s GetProps] IsItemPresent %s]",
	       this->GetPVRenderView()->GetRendererTclName(),
	       this->XYPlotTclName);
  
  if (atoi(this->GetPVApplication()->GetMainInterp()->result) > 0)
    {
    this->Script("%s RemoveActor %s",
		 this->GetPVRenderView()->GetRendererTclName(),
		 this->XYPlotTclName);
    this->GetPVRenderView()->Render();
    }
  
  this->Script("pack forget %s", this->SelectPointButton->GetWidgetName());
  if (this->GetPVWindow()->GetMainView()->GetInteractor() == this->Interactor)
    {
    this->GetPVWindow()->GetMainView()->SetInteractor(this->PreviousInteractor);
    }
  
  if (this->Interactor)
    {
    this->Interactor->SetCursorVisibility(0);
    }
  
  this->vtkPVSource::Deselect(view);
}

void vtkPVProbe::UsePoint()
{
  this->Dimensionality = 0;
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->ProbeFrame->GetWidgetName());
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->ScalarOperationFrame->GetWidgetName());
  this->Script("pack %s %s",
               this->SelectedPointFrame->GetWidgetName(),
               this->PointDataLabel->GetWidgetName());
  this->ChangeAcceptButtonColor();
}

void vtkPVProbe::UseLine()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int error;
  vtkDataSet *dataSet = 
    (vtkDataSet*)(vtkTclGetPointerFromObject(this->ProbeSourceTclName,
                                             "vtkDataSet",
                                             pvApp->GetMainInterp(),
                                             error));
  this->ScalarOperationMenu->SetVTKData(dataSet);
  
  this->Dimensionality = 1;
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->ProbeFrame->GetWidgetName());
  this->Script("pack %s %s %s %s",
               this->EndPointMenuFrame->GetWidgetName(),
               this->EndPoint1Frame->GetWidgetName(),
               this->EndPoint2Frame->GetWidgetName(),
	       this->DivisionsEntry->GetWidgetName());

  
  this->PackScalarsMenu();
  
  char* endPt = this->EndPointMenu->GetValue();
  if (strcmp(endPt, "End Point 1") == 0)
    {
    this->SetCurrentEndPoint(1);
    }
  else if (strcmp(endPt, "End Point 2") == 0)
    {
    this->SetCurrentEndPoint(2);
    }
  this->ChangeAcceptButtonColor();
}

void vtkPVProbe::SetSelectedPoint(float point[3])
{
  this->SelectedXEntry->SetValue(point[0], 4);
  this->SelectedYEntry->SetValue(point[1], 4);
  this->SelectedZEntry->SetValue(point[2], 4);
}

void vtkPVProbe::SetEndPoint1(float point[3])
{
  this->End1XEntry->SetValue(point[0], 4);
  this->End1YEntry->SetValue(point[1], 4);
  this->End1ZEntry->SetValue(point[2], 4);
}

void vtkPVProbe::SetEndPoint2(float point[3])
{
  this->End2XEntry->SetValue(point[0], 4);
  this->End2YEntry->SetValue(point[1], 4);
  this->End2ZEntry->SetValue(point[2], 4);
}

void vtkPVProbe::SetCurrentEndPoint(int id)
{
  this->CurrentEndPoint = id;
  
  if (id == 1)
    {
    this->Script("%s configure -state disabled",
                 this->End2XEntry->GetEntry()->GetWidgetName());
    this->Script("%s configure -state disabled",
                 this->End2YEntry->GetEntry()->GetWidgetName());
    this->Script("%s configure -state disabled",
                 this->End2ZEntry->GetEntry()->GetWidgetName());
    this->Script("%s configure -state normal",
                 this->End1XEntry->GetEntry()->GetWidgetName()); 
    this->Script("%s configure -state normal",
                 this->End1YEntry->GetEntry()->GetWidgetName()); 
    this->Script("%s configure -state normal",
                 this->End1ZEntry->GetEntry()->GetWidgetName()); 
    }
  else if (id == 2)
    {
    this->Script("%s configure -state disabled",
                 this->End1XEntry->GetEntry()->GetWidgetName());
    this->Script("%s configure -state disabled",
                 this->End1YEntry->GetEntry()->GetWidgetName());
    this->Script("%s configure -state disabled",
                 this->End1ZEntry->GetEntry()->GetWidgetName());
    this->Script("%s configure -state normal",
                 this->End2XEntry->GetEntry()->GetWidgetName()); 
    this->Script("%s configure -state normal",
                 this->End2YEntry->GetEntry()->GetWidgetName()); 
    this->Script("%s configure -state normal",
                 this->End2ZEntry->GetEntry()->GetWidgetName()); 
    }
}

void vtkPVProbe::ChangeXPosition()
{
  vtkKWEntry *entry;
  
  if (!this->Interactor)
    {
    return;
    }
  
  if (this->Dimensionality == 0)
    {
    entry = this->SelectedXEntry->GetEntry();
    }
  else
    {
    if (this->CurrentEndPoint == 1)
      {
      entry = this->End1XEntry->GetEntry();
      }
    else
      {
      entry = this->End2XEntry->GetEntry();
      }
    }
  
  this->Interactor->SetSelectedPointX(entry->GetValueAsFloat());
}

void vtkPVProbe::ChangeYPosition()
{
  vtkKWEntry *entry;
  
  if (!this->Interactor)
    {
    return;
    }
  
  if (this->Dimensionality == 0)
    {
    entry = this->SelectedYEntry->GetEntry();
    }
  else
    {
    if (this->CurrentEndPoint == 1)
      {
      entry = this->End1YEntry->GetEntry();
      }
    else
      {
      entry = this->End2YEntry->GetEntry();
      }
    }
  
  this->Interactor->SetSelectedPointY(entry->GetValueAsFloat());
}

void vtkPVProbe::ChangeZPosition()
{
  vtkKWEntry *entry;
  
  if (!this->Interactor)
    {
    return;
    }
  
  if (this->Dimensionality == 0)
    {
    entry = this->SelectedZEntry->GetEntry();
    }
  else
    {
    if (this->CurrentEndPoint == 1)
      {
      entry = this->End1ZEntry->GetEntry();
      }
    else
      {
      entry = this->End2ZEntry->GetEntry();
      }
    }
  
  this->Interactor->SetSelectedPointZ(entry->GetValueAsFloat());
}

void vtkPVProbe::UpdateScalars()
{
  char *newScalars = this->ScalarOperationMenu->GetValue();

  if (strcmp(newScalars, "") == 0)
    {
    return;
    }
  
  if (this->DefaultScalarsName)
    {
    if (strcmp(newScalars, this->DefaultScalarsName) == 0)
      {
      return;
      }
    }

  if (this->DefaultScalarsName)
    {
    delete [] this->DefaultScalarsName;
    this->DefaultScalarsName = NULL;
    }
  this->SetDefaultScalarsName(newScalars);
}

void vtkPVProbe::SaveInTclScript(ofstream *file)
{
  Tcl_Interp *interp = this->GetPVApplication()->GetMainInterp();
  char *tempName;
  vtkPVSourceInterface *pvsInterface =
    this->PVProbeSource->GetPVSource()->GetInterface();
  
  if (this->Dimensionality == 0)
    {
    *file << "vtkIdList pointList\n";
    *file << "\tpointList Allocate 1 1\n";
    *file << "\tpointList InsertNextId 0\n\n";

    this->Script("set coords [[%s GetInput] GetPoint 0]",
                 this->GetVTKSourceTclName());
    *file << "vtkPoints points\n";
    *file << "\tpoints Allocate 1 1\n";
    *file << "\tpoints InsertNextPoint " << interp->result << "\n\n";
    
    *file << "vtkPolyData probeInput\n";
    *file << "\tprobeInput Allocate 1 1\n";
    *file << "\tprobeInput SetPoints points\n";
    *file << "\tprobeInput InsertNextCell 1 pointList\n\n";
    }
  else
    {
    *file << "vtkLineSource line\n";

    this->Script("set point1 [[[%s GetInput] GetSource] GetPoint1]",
                 this->GetVTKSourceTclName());
    *file << "\tline SetPoint1 " << interp->result << "\n";
    this->Script("set point2 [[[%s GetInput] GetSource] GetPoint2]",
                 this->GetVTKSourceTclName());
    *file << "\tline SetPoint2 " << interp->result << "\n";
    this->Script("set res [[[%s GetInput] GetSource] GetResolution]",
                 this->GetVTKSourceTclName());
    *file << "\tline SetResolution " << interp->result << "\n\n";
    }
  
  *file << this->VTKSource->GetClassName() << " "
        << this->GetVTKSourceTclName() << "\n";
  if (this->Dimensionality == 0)
    {
    *file << "\t" << this->GetVTKSourceTclName() << " SetInput probeInput\n";
    }
  else
    {
    *file << "\t" << this->GetVTKSourceTclName()
          << " SetInput [line GetOutput]\n";
    }
  
  *file << "\t" << this->GetVTKSourceTclName() << " SetSource [";

  if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                             "vtkGenericEnSightReader") == 0)
    {
    char *dataName = new char[strlen(this->ProbeSourceTclName) + 1];
    char *charFound;
    int pos;
    
    strcpy(dataName, this->ProbeSourceTclName);
    charFound = strrchr(dataName, 't');
    tempName = strtok(dataName, "O");
    *file << tempName << " GetOutput ";
    pos = charFound - dataName + 1;
    *file << dataName+pos << "]\n\n";
    delete [] dataName;
    }
  else if (pvsInterface && strcmp(pvsInterface->GetSourceClassName(),
                                  "vtkPDataSetReader") == 0)
    {
    char *dataName = new char[strlen(this->ProbeSourceTclName) + 1];
    strcpy(dataName, this->ProbeSourceTclName);
    
    tempName = strtok(this->ProbeSourceTclName, "O");
    *file << tempName << " GetOutput]\n\n";
    delete [] dataName;
    }
  else
    {
    *file << this->PVProbeSource->GetPVSource()->GetVTKSourceTclName()
          << " GetOutput]\n\n";
    }
  
  if (this->Dimensionality == 1)
    {
    *file << "vtkFieldDataToAttributeDataFilter "
          << this->ChangeScalarsFilterTclName << "\n";
    *file << "\t" << this->ChangeScalarsFilterTclName << " SetInput ["
          << this->VTKSourceTclName << " GetOutput]\n";
    *file << "\t" << this->ChangeScalarsFilterTclName
          << " SetInputFieldToPointDataField\n";
    *file << "\t" << this->ChangeScalarsFilterTclName
          << " SetOutputAttributeDataToPointData\n\n";
    
    *file << "vtkXYPlotActor " << this->XYPlotTclName << "\n";
    *file << "\t[" << this->XYPlotTclName
          << " GetPositionCoordinate] SetValue 0.05 0.05 0\n";
    *file << "\t[" << this->XYPlotTclName
          << " GetPosition2Coordinate] SetValue 0.8 0.3 0\n";
    *file << "\t" << this->XYPlotTclName << " SetNumberOfXLabels 5\n";
    *file << "\t" << this->XYPlotTclName << " SetXTitle \"Line Divisions\"\n";
    *file << "\t" << this->XYPlotTclName << " SetYTitle "
          << this->DefaultScalarsName << "\n";
    *file << "\t" << this->XYPlotTclName << " AddInput ["
          << this->ChangeScalarsFilterTclName << " GetOutput]\n\n";
    }
  
  this->GetPVOutput(0)->SaveInTclScript(file, this->VTKSourceTclName);
}
