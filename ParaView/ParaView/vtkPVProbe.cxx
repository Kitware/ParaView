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

#include "vtkArrayMap.txx"
#include "vtkIdTypeArray.h"
#include "vtkKWCheckButton.h"
#include "vtkKWCompositeCollection.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWToolbar.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVData.h"
#include "vtkPVInputMenu.h"
#include "vtkPVLineWidget.h"
#include "vtkPVPointWidget.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPolyData.h"
#include "vtkSource.h"
#include "vtkStringList.h"
#include "vtkTclUtil.h"

vtkCxxSetObjectMacro(vtkPVProbe, InputMenu, vtkPVInputMenu);

int vtkPVProbeCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVProbe::vtkPVProbe()
{
  static int instanceCount = 0;
  
  this->CommandFunction = vtkPVProbeCommand;

  this->ScalarArrayMenu = vtkPVArrayMenu::New();

  this->DimensionalityMenu = vtkKWOptionMenu::New();
  this->DimensionalityLabel = vtkKWLabel::New();
  
  this->SelectedPointFrame = vtkKWWidget::New();
  this->SelectedPointLabel = vtkKWLabel::New();
  this->SelectedXEntry = vtkKWLabeledEntry::New();
  this->SelectedYEntry = vtkKWLabeledEntry::New();
  this->SelectedZEntry = vtkKWLabeledEntry::New();
  this->PointDataLabel = vtkKWLabel::New();
  
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
  this->ShowXYPlotToggle = vtkKWCheckButton::New();

  this->LineWidget = vtkPVLineWidget::New();
  this->PointWidget = vtkPVPointWidget::New();
  
  this->ProbeFrame = vtkKWWidget::New();

  this->Dimensionality = -1;
  
  this->XYPlotTclName = NULL;
  
  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
  
  this->ReplaceInputOff();
  this->InputMenu = 0;
}

//----------------------------------------------------------------------------
vtkPVProbe::~vtkPVProbe()
{
  this->DimensionalityLabel->Delete();
  this->DimensionalityLabel = NULL;
  this->DimensionalityMenu->Delete();
  this->DimensionalityMenu = NULL;
  
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
  
  this->LineWidget->Delete();
  this->PointWidget->Delete();
  
  this->DivisionsEntry->Delete();
  this->DivisionsEntry = NULL;

  this->ShowXYPlotToggle->Delete();
  this->ShowXYPlotToggle = NULL;
  
  this->ProbeFrame->Delete();
  this->ProbeFrame = NULL;
  
  if (this->XYPlotTclName)
    {
    this->Script("%s Delete", this->XYPlotTclName);
    this->SetXYPlotTclName(NULL);
    }

  this->ScalarArrayMenu->Delete();
  this->ScalarArrayMenu = NULL;
  this->SetInputMenu(0);
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
void vtkPVProbe::SetPVInput(vtkPVData *pvd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp == NULL)
    {
    vtkErrorMacro("No Application. Create the source before setting the input.");
    return;
    }

  this->SetNthPVInput(0, pvd);

  // Notice this is set to the source and not the input.
  // This fits the interface better and was necessary for the
  // rather inflexible vtkPVArrayMenu.
  pvApp->BroadcastScript("%s SetSource %s", this->GetVTKSourceTclName(),
                         pvd->GetVTKDataTclName());
}




//----------------------------------------------------------------------------
void vtkPVProbe::UpdateScalars()
{
}

//----------------------------------------------------------------------------
void vtkPVProbe::CreateProperties()
{
  float bounds[6];
  vtkKWWidget *frame;
  vtkPVApplication* pvApp = this->GetPVApplication();
  char tclName[100];
  
  this->vtkPVSource::CreateProperties();

  this->AddInputMenu("Input", "PVInput", "vtkDataSet",
                     "Set the input to this filter.",
                     this->GetPVWindow()->GetSourceList("Sources")); 
  this->Script("pack %s", this->InputMenu->GetWidgetName());

  pvApp->BroadcastScript("%s SetSpatialMatch 2", this->VTKSourceTclName);

  // We should really use the vtkPVSource helper methods.
  this->ScalarArrayMenu->SetNumberOfComponents(1);
  this->ScalarArrayMenu->ShowComponentMenuOn();
  this->ScalarArrayMenu->SetInputName("Input");
  this->ScalarArrayMenu->SetAttributeType(vtkDataSetAttributes::SCALARS);
  this->ScalarArrayMenu->SetParent(this->ParameterFrame->GetFrame());
  this->ScalarArrayMenu->SetLabel("Scalars");
  this->ScalarArrayMenu->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  this->ScalarArrayMenu->Create(this->Application);
  this->ScalarArrayMenu->SetBalloonHelpString("Choose the scalar array to graph.");
  this->AddPVWidget(this->ScalarArrayMenu);
  if (this->InputMenu == NULL)
    {
    vtkErrorMacro("Could not find the input menu.");
    }
  else
    {
    this->ScalarArrayMenu->SetInputMenu(this->InputMenu);
    }


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
  this->DimensionalityMenu->AddEntryWithCommand("NewPoint", this, "UseNewPoint");
  this->DimensionalityMenu->SetValue("Point");
  
  this->Script("pack %s %s -side left",
               this->DimensionalityLabel->GetWidgetName(),
               this->DimensionalityMenu->GetWidgetName());
  
  this->GetPVInput()->GetBounds(bounds);
  
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
  this->Script("bind %s <KeyPress> {%s SetAcceptButtonColorToRed}",
               this->SelectedXEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->SelectedXEntry->GetEntry()->GetWidgetName());
  this->SelectedXEntry->SetValue((bounds[0]+bounds[1])*0.5, 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeXPosition}",
               this->SelectedXEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());
  
  this->SelectedYEntry->SetParent(this->SelectedPointFrame);
  this->SelectedYEntry->Create(pvApp);
  this->SelectedYEntry->SetLabel("Y:");
  this->Script("bind %s <KeyPress> {%s SetAcceptButtonColorToRed}",
               this->SelectedYEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->SelectedYEntry->GetEntry()->GetWidgetName());
  this->SelectedYEntry->SetValue((bounds[2]+bounds[3])*0.5, 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeYPosition}",
               this->SelectedYEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->SelectedZEntry->SetParent(this->SelectedPointFrame);
  this->SelectedZEntry->Create(pvApp);
  this->SelectedZEntry->SetLabel("Z:");
  this->Script("bind %s <KeyPress> {%s SetAcceptButtonColorToRed}",
               this->SelectedZEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->SelectedZEntry->GetEntry()->GetWidgetName());
  this->SelectedZEntry->SetValue((bounds[4]+bounds[5])*0.5, 4);
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
  this->EndPoint1Frame->SetParent(this->ProbeFrame);
  this->EndPoint1Frame->Create(pvApp, "frame", "");
  
  this->EndPoint1Label->SetParent(this->EndPoint1Frame);
  this->EndPoint1Label->Create(pvApp, "");
  this->EndPoint1Label->SetLabel("End Point 1");
  this->End1XEntry->SetParent(this->EndPoint1Frame);
  this->End1XEntry->Create(pvApp);
  this->End1XEntry->SetLabel("X:");
  this->Script("bind %s <KeyPress> {%s SetAcceptButtonColorToRed}",
               this->End1XEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->End1XEntry->GetEntry()->GetWidgetName());
  this->End1XEntry->SetValue((bounds[0]+bounds[1])*0.5, 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeXPosition}",
               this->End1XEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->End1YEntry->SetParent(this->EndPoint1Frame);
  this->End1YEntry->Create(pvApp);
  this->End1YEntry->SetLabel("Y:");
  this->Script("bind %s <KeyPress> {%s SetAcceptButtonColorToRed}",
               this->End1YEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->End1YEntry->GetEntry()->GetWidgetName());
  this->End1YEntry->SetValue((bounds[2]+bounds[3])*0.5, 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeYPosition}",
               this->End1YEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->End1ZEntry->SetParent(this->EndPoint1Frame);
  this->End1ZEntry->Create(pvApp);
  this->End1ZEntry->SetLabel("Z:");
  this->Script("bind %s <KeyPress> {%s SetAcceptButtonColorToRed}",
               this->End1ZEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->End1ZEntry->GetEntry()->GetWidgetName());
  this->End1ZEntry->SetValue((bounds[4]+bounds[5])*0.5, 4);
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
  this->Script("bind %s <KeyPress> {%s SetAcceptButtonColorToRed}",
               this->End2XEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->End2XEntry->GetEntry()->GetWidgetName());
  this->End2XEntry->SetValue((bounds[0]+bounds[1])*0.5, 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeXPosition}",
               this->End2XEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->End2YEntry->SetParent(this->EndPoint2Frame);
  this->End2YEntry->Create(pvApp);
  this->End2YEntry->SetLabel("Y:");
  this->Script("bind %s <KeyPress> {%s SetAcceptButtonColorToRed}",
               this->End2YEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->End2YEntry->GetEntry()->GetWidgetName());
  this->End2YEntry->SetValue((bounds[2]+bounds[3])*0.5, 4);
  this->Script("bind %s <KeyPress-Return> {%s ChangeYPosition}",
               this->End2YEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->End2ZEntry->SetParent(this->EndPoint2Frame);
  this->End2ZEntry->Create(pvApp);
  this->End2ZEntry->SetLabel("Z:");
  this->Script("bind %s <KeyPress> {%s SetAcceptButtonColorToRed}",
               this->End2ZEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Script("%s configure -width 10",
               this->End2ZEntry->GetEntry()->GetWidgetName());
  this->End2ZEntry->SetValue((bounds[4]+bounds[5])*0.5, 4);
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
  this->Script("bind %s <KeyPress> {%s SetAcceptButtonColorToRed}",
               this->DivisionsEntry->GetEntry()->GetWidgetName(), this->GetTclName());
  
  this->ShowXYPlotToggle->SetParent(this->ProbeFrame);
  this->ShowXYPlotToggle->Create(pvApp, "-text \"Show XY-Plot\"");
  this->ShowXYPlotToggle->SetState(1);
  this->Script("%s configure -command {%s SetAcceptButtonColorToRed}",
               this->ShowXYPlotToggle->GetWidgetName(), this->GetTclName());

  this->LineWidget->SetPVSource(this);
  this->LineWidget->SetParent(this->ProbeFrame);
  this->LineWidget->Create(pvApp);
  this->AddPVWidget(this->LineWidget);
  this->LineWidget->SetModifiedCommand(this->GetTclName(),
				       "SetAcceptButtonColorToRed");
  this->LineWidget->SetPoint1Method(this->GetTclName(), "EndPoint1");
  this->LineWidget->SetPoint2Method(this->GetTclName(), "EndPoint2");
  this->LineWidget->SetResolutionMethod(this->GetTclName(), "NumberOfLineDivisions");

  this->PointWidget->SetPVSource(this);
  this->PointWidget->SetParent(this->ProbeFrame);
  this->PointWidget->Create(pvApp);
  this->AddPVWidget(this->PointWidget);
  this->PointWidget->SetModifiedCommand(this->GetTclName(),
				       "SetAcceptButtonColorToRed");
  this->PointWidget->SetPositionMethod(this->GetTclName(), "PointPosition");
  
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

  this->SetEndPoint1(bounds[0], bounds[2], bounds[4]);
  this->SetEndPoint2(bounds[1], bounds[3], bounds[5]);
  this->SetPointPosition((bounds[0]+bounds[1])/2, 
			 (bounds[2]+bounds[3])/2, 
			 (bounds[4]+bounds[5])/2);
  this->SetNumberOfLineDivisions(10);
  this->LineWidget->PlaceWidget();
  this->PointWidget->PlaceWidget();
}

//----------------------------------------------------------------------------
void vtkPVProbe::AcceptCallback()
{
  int i;
  const char *arrayName = this->ScalarArrayMenu->GetValue();
  int component;
  
  this->LineWidget->Accept();
  this->PointWidget->Accept();

  // This can't be an accept command because this calls SetInput for the
  // vtkProbeFilter, and vtkPVSource::AcceptCallback expects that the
  // input has already been set.
  this->UpdateProbe();
  
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->Dimensionality == 0)
    {
    pvApp->AddTraceEntry("$kw(%s) UsePoint", this->GetTclName());
    }
  else
    {
    pvApp->AddTraceEntry("$kw(%s) UseLine", this->GetTclName());
    pvApp->AddTraceEntry("$kw(%s) UpdateScalars", this->GetTclName());
    pvApp->AddTraceEntry("[$kw(%s) GetShowXYPlotToggle] SetState %d",
                         this->GetTclName(),
                         this->ShowXYPlotToggle->GetState());
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
    component = this->ScalarArrayMenu->GetSelectedComponent();
    if (component > 1)
      {
      this->Script("%s SetYTitle {%s %d}", this->XYPlotTclName, 
                   arrayName, component);
      }
    else
      {
      this->Script("%s SetYTitle {%s}", this->XYPlotTclName, arrayName);
      }
    this->Script("%s RemoveAllInputs", this->XYPlotTclName);
    this->Script("%s AddInput %s %s %d", this->XYPlotTclName,
                 this->GetPVOutput()->GetVTKDataTclName(), 
                 arrayName, component);
        
    // Color by the choosen array in the output.
    this->GetPVOutput()->ColorByPointFieldComponent(arrayName, component);
    // The menu's not changing, but It is going to replaced soon.
    // For now I will put this here.
    char tmp[256];
    sprintf(tmp, "Point %s %d", arrayName, component);
    this->GetPVOutput()->GetColorMenu()->SetValue(tmp);

    if (this->ShowXYPlotToggle->GetState())
      {
      this->Script("%s AddActor %s",
                   window->GetMainView()->GetRendererTclName(),
                   this->XYPlotTclName);
      }
    window->GetMainView()->Render();
    }
}

//----------------------------------------------------------------------------
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
  vtkIdType numPoints;
  vtkIdType numRemotePoints = 0;
  vtkIdType j, pointId;
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
                           this->PointPosition[0],
			   this->PointPosition[1],
			   this->PointPosition[2]);
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
			   this->EndPoint1[0], this->EndPoint1[1],
			   this->EndPoint1[2]);
    pvApp->BroadcastScript("line SetPoint2 %f %f %f",
			   this->EndPoint2[0], this->EndPoint2[1],
			   this->EndPoint2[2]);
    pvApp->BroadcastScript("line SetResolution %d",
			   this->NumberOfLineDivisions);
    pvApp->BroadcastScript("%s SetInput [line GetOutput]",
                           this->GetVTKSourceTclName());
    pvApp->BroadcastScript("line Delete");  
    }
  else
    {
    pvApp->BroadcastScript("vtkPolyData probeInput");
    pvApp->BroadcastScript("vtkIdList pointList");
    pvApp->BroadcastScript("pointList Allocate 1 1");
    pvApp->BroadcastScript("pointList InsertNextId 0");
    pvApp->BroadcastScript("vtkPoints points");
    pvApp->BroadcastScript("points Allocate 1 1");
    pvApp->BroadcastScript("points InsertNextPoint %f %f %f",
                           this->PointPosition[0],
			   this->PointPosition[1],
			   this->PointPosition[2]);
    pvApp->BroadcastScript("probeInput Allocate 1 1");
    pvApp->BroadcastScript("probeInput SetPoints points");
    pvApp->BroadcastScript("probeInput InsertNextCell 1 pointList");
    pvApp->BroadcastScript("%s SetInput probeInput",
                           this->GetVTKSourceTclName());
    pvApp->BroadcastScript("pointList Delete");
    pvApp->BroadcastScript("points Delete");
    pvApp->BroadcastScript("probeInput Delete");
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

void vtkPVProbe::Deselect(int doPackForget)
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
  
  this->vtkPVSource::Deselect(doPackForget);
}

//----------------------------------------------------------------------------
void vtkPVProbe::UsePoint()
{
  this->Dimensionality = 0;
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->ProbeFrame->GetWidgetName());
  this->Script("pack %s %s %s",
               this->SelectedPointFrame->GetWidgetName(),
               this->PointDataLabel->GetWidgetName(),
	       this->PointWidget->GetWidgetName());
  this->Script("pack forget %s", this->ScalarArrayMenu->GetWidgetName());
  this->SetAcceptButtonColorToRed();
}

//----------------------------------------------------------------------------
void vtkPVProbe::UseLine()
{
  this->Dimensionality = 1;
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->ProbeFrame->GetWidgetName());

  this->Script("pack %s -side top -fill x -expand t -after %s",
               this->ScalarArrayMenu->GetWidgetName(),
               this->InputMenu->GetWidgetName());
  this->ScalarArrayMenu->Update();

  this->Script("pack %s %s",
               this->ShowXYPlotToggle->GetWidgetName(),
	       this->LineWidget->GetWidgetName());    
  this->SetAcceptButtonColorToRed();
}

//----------------------------------------------------------------------------
void vtkPVProbe::UseNewPoint()
{
  this->Dimensionality = 3;
  this->Script("catch {eval pack forget [pack slaves %s]}",
               this->ProbeFrame->GetWidgetName());

  this->Script("pack %s %s",
               this->SelectedPointFrame->GetWidgetName(),
               this->PointDataLabel->GetWidgetName());
  this->ScalarArrayMenu->Update();

  this->Script("pack %s %s",
               this->ShowXYPlotToggle->GetWidgetName(),
	       this->PointWidget->GetWidgetName());
    
  this->SetAcceptButtonColorToRed();
}

//----------------------------------------------------------------------------
void vtkPVProbe::SetSelectedPoint(float point[3])
{
  this->SelectedXEntry->SetValue(point[0], 4);
  this->SelectedYEntry->SetValue(point[1], 4);
  this->SelectedZEntry->SetValue(point[2], 4);
}

//----------------------------------------------------------------------------
void vtkPVProbe::SetEndPoint1(float point[3])
{
  this->End1XEntry->SetValue(point[0], 4);
  this->End1YEntry->SetValue(point[1], 4);
  this->End1ZEntry->SetValue(point[2], 4);
  this->EndPoint1[0] = point[0];
  this->EndPoint1[1] = point[1];
  this->EndPoint1[2] = point[2];
}

//----------------------------------------------------------------------------
void vtkPVProbe::SetEndPoint2(float point[3])
{
  this->End2XEntry->SetValue(point[0], 4);
  this->End2YEntry->SetValue(point[1], 4);
  this->End2ZEntry->SetValue(point[2], 4);
  this->EndPoint2[0] = point[0];
  this->EndPoint2[1] = point[1];
  this->EndPoint2[2] = point[2];
}

//----------------------------------------------------------------------------
void vtkPVProbe::SetNumberOfLineDivisions(int i)
{
  this->NumberOfLineDivisions = i;
  this->DivisionsEntry->SetValue(i);
}

//----------------------------------------------------------------------------
vtkPVInputMenu *vtkPVProbe::AddInputMenu(char *label, char *inputName, 
					 char *inputType, char *help, 
					 vtkPVSourceCollection *sources)
{
  vtkPVInputMenu* inputMenu = this->Superclass::AddInputMenu(label, inputName,
							     inputType, help,
							     sources);
  this->SetInputMenu(inputMenu);
  return inputMenu;
}

//----------------------------------------------------------------------------
void vtkPVProbe::SaveInTclScript(ofstream *file)
{
  Tcl_Interp *interp = this->GetPVApplication()->GetMainInterp();
  char *tempName;
  vtkPVSource *pvs = this->GetPVInput()->GetPVSource();
  
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

  if (pvs && strcmp(pvs->GetSourceClassName(), "vtkGenericEnSightReader") == 0)
    {
    const char *probeSourceTclName = this->GetPVInput()->GetVTKDataTclName();
    char *dataName = new char[strlen(probeSourceTclName) + 1];
    char *charFound;
    int pos;
    
    strcpy(dataName, probeSourceTclName);
    charFound = strrchr(dataName, 't');
    tempName = strtok(dataName, "O");
    *file << tempName << " GetOutput ";
    pos = charFound - dataName + 1;
    *file << dataName+pos << "]\n\n";
    delete [] dataName;
    }
  else if (pvs && strcmp(pvs->GetSourceClassName(), "vtkPDataSetReader") == 0)
    {
    const char *probeSourceTclName = this->GetPVInput()->GetVTKDataTclName();
    char *dataName = new char[strlen(probeSourceTclName) + 1];
    strcpy(dataName, probeSourceTclName);
    
    tempName = strtok(dataName, "O");
    *file << tempName << " GetOutput]\n\n";
    delete [] dataName;
    }
  else
    {
    *file << this->GetPVInput()->GetPVSource()->GetVTKSourceTclName()
          << " GetOutput]\n\n";
    }
  
  if (this->Dimensionality == 1)
    {    
    if (this->ShowXYPlotToggle->GetState())
      {
      *file << "vtkXYPlotActor " << this->XYPlotTclName << "\n";
      *file << "\t[" << this->XYPlotTclName
            << " GetPositionCoordinate] SetValue 0.05 0.05 0\n";
      *file << "\t[" << this->XYPlotTclName
            << " GetPosition2Coordinate] SetValue 0.8 0.3 0\n";
      *file << "\t" << this->XYPlotTclName << " SetNumberOfXLabels 5\n";
      *file << "\t" << this->XYPlotTclName << " SetXTitle \"Line Divisions\"\n";
      }
    }
  
  this->GetPVOutput(0)->SaveInTclScript(file);
}

//----------------------------------------------------------------------------
void vtkPVProbe::SetPointPosition(float point[3])
{
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->PointPosition[cc] = point[cc];
    }
  this->SelectedXEntry->SetValue(point[0], 3);
  this->SelectedYEntry->SetValue(point[1], 3);
  this->SelectedZEntry->SetValue(point[2], 3);
}

//----------------------------------------------------------------------------
void vtkPVProbe::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Dimensionality: " << this->GetDimensionality() << endl;
  os << indent << "ShowXYPlotToggle: " << this->GetShowXYPlotToggle() << endl;
  os << indent << "NumberOfLineDivisions: " 
     << this->NumberOfLineDivisions << endl;
  os << indent << "EndPoint1: " << this->EndPoint1[0] << ", " 
     << this->EndPoint1[1] << ", " << this->EndPoint1[2] << endl;
  os << indent << "EndPoint2: " << this->EndPoint2[0] << ", " 
     << this->EndPoint2[1] << ", " << this->EndPoint2[2] << endl;
  os << indent << "PointPosition: " << this->PointPosition[0] << ", " 
     << this->PointPosition[1] << ", " << this->PointPosition[2] << endl;
}
