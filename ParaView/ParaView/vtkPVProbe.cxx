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

#include "vtkIdTypeArray.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVData.h"
#include "vtkPVInputMenu.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkString.h"
#include "vtkTclUtil.h"
#include "vtkSource.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVProbe);
vtkCxxRevisionMacro(vtkPVProbe, "1.76.2.2");

vtkCxxSetObjectMacro(vtkPVProbe, InputMenu, vtkPVInputMenu);

int vtkPVProbeCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVProbe::vtkPVProbe()
{
  static int instanceCount = 0;
  
  this->CommandFunction = vtkPVProbeCommand;

  this->ScalarArrayMenu = vtkPVArrayMenu::New();

  this->SelectedPointFrame = vtkKWWidget::New();
  this->SelectedPointLabel = vtkKWLabel::New();
  this->PointDataLabel = vtkKWLabel::New();
  
  this->ShowXYPlotToggle = vtkKWCheckButton::New();

  
  this->ProbeFrame = vtkKWWidget::New();
  
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
  this->SelectedPointLabel->Delete();
  this->SelectedPointLabel = NULL;
  this->SelectedPointFrame->Delete();
  this->SelectedPointFrame = NULL;
  this->PointDataLabel->Delete();
  this->PointDataLabel = NULL;  
  
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

  this->Script("pack %s", this->ScalarArrayMenu->GetWidgetName());
  this->ScalarArrayMenu->Update();

  this->ProbeFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->ProbeFrame->Create(pvApp, "frame", "");
  
  this->Script("pack %s",
               this->ProbeFrame->GetWidgetName());

  // widgets for points
  this->SelectedPointFrame->SetParent(this->ProbeFrame);
  this->SelectedPointFrame->Create(pvApp, "frame", "");
  
  
  
  this->SelectedPointLabel->SetParent(this->SelectedPointFrame);
  this->SelectedPointLabel->Create(pvApp, "");
  this->SelectedPointLabel->SetLabel("Point");

  this->Script("pack %s -side left",
               this->SelectedPointLabel->GetWidgetName());
  
  this->PointDataLabel->SetParent(this->ProbeFrame);
  this->PointDataLabel->Create(pvApp, "");

  this->ShowXYPlotToggle->SetParent(this->ProbeFrame);
  this->ShowXYPlotToggle->Create(pvApp, "-text \"Show XY-Plot\"");
  this->ShowXYPlotToggle->SetState(1);
  this->Script("%s configure -command {%s SetAcceptButtonColorToRed}",
               this->ShowXYPlotToggle->GetWidgetName(), this->GetTclName());

  this->Script("pack %s",
               this->ShowXYPlotToggle->GetWidgetName());

  sprintf(tclName, "XYPlot%d", this->InstanceCount);
  this->SetXYPlotTclName(tclName);
  pvApp->BroadcastScript("vtkXYPlotActor %s", this->XYPlotTclName);
  this->Script("[%s GetPositionCoordinate] SetValue 0.05 0.05 0",
               this->XYPlotTclName);
  this->Script("[%s GetPosition2Coordinate] SetValue 0.8 0.3 0",
               this->XYPlotTclName);
  this->Script("%s SetNumberOfXLabels 5", this->XYPlotTclName);
  this->Script("%s SetXTitle {Line Divisions}", this->XYPlotTclName);

  pvApp->BroadcastScript("%s SetController [ $Application GetController ] ", 
                        this->GetVTKSourceTclName());
                        
}

//----------------------------------------------------------------------------
void vtkPVProbe::AcceptCallbackInternal()
{
  int i;
  const char *arrayName = this->ScalarArrayMenu->GetValue();
  int component;
  
  // This can't be an accept command because this calls SetInput for
  // the vtkProbeFilter, and vtkPVSource::AcceptCallbackInternal
  // expects that the input has already been set.
  this->UpdateProbe();
  
  vtkPVApplication *pvApp = this->GetPVApplication();

  pvApp->AddTraceEntry("$kw(%s) UpdateScalars", this->GetTclName());
  pvApp->AddTraceEntry("[$kw(%s) GetShowXYPlotToggle] SetState %d",
                       this->GetTclName(),
                       this->ShowXYPlotToggle->GetState());
  
  // call the superclass's method
  this->vtkPVSource::AcceptCallbackInternal();
  
  vtkPVWindow *window = this->GetPVWindow();
  
  int error;
  vtkPolyData *probeOutput = (vtkPolyData *)
    (vtkTclGetPointerFromObject(this->GetNthPVOutput(0)->GetVTKDataTclName(),
                                "vtkPolyData", pvApp->GetMainInterp(),
                                error));

  vtkPointData *pd = probeOutput->GetPointData();
    
  int numArrays = pd->GetNumberOfArrays();
  vtkDataArray *array;

  if (this->GetDimensionality() == 0)
    {
    // update the ui to see the point data for the probed point
    
    vtkIdType j, numComponents;
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
    this->Script("pack %s", this->PointDataLabel->GetWidgetName());
    }
  else if (this->GetDimensionality() == 1)
    {
    this->Script("pack forget %s", this->PointDataLabel->GetWidgetName());
    this->Script("%s RemoveAllInputs", this->XYPlotTclName);
    component = this->ScalarArrayMenu->GetSelectedComponent();
    this->Script("%s SetYTitle {}", this->XYPlotTclName);
      
    this->Script("%s PlotPointsOn", this->XYPlotTclName);
    this->Script("%s PlotLinesOn", this->XYPlotTclName);
    this->Script("[%s GetProperty] SetColor 1 .8 .8", this->XYPlotTclName);
    this->Script("[%s GetProperty] SetPointSize 2", this->XYPlotTclName);
    this->Script("%s SetLegendPosition 0.4 0.6", this->XYPlotTclName);
    this->Script("%s SetLegendPosition2 0.5 0.25", this->XYPlotTclName);
    
    float cstep = 1.0 / numArrays;
    float ccolor = 0;
    for ( i = 0; i < numArrays; i++)
      {
      array = pd->GetArray(i);
      arrayName = array->GetName();
      this->Script("%s AddInput %s %s %d", this->XYPlotTclName,
                   this->GetPVOutput()->GetVTKDataTclName(), 
                   arrayName, component);
      this->Script("%s SetPlotLabel %d \"%s\"", this->XYPlotTclName, i, arrayName);
      float r, g, b;
      this->HSVtoRGB(ccolor, 1, 1, &r, &g, &b);
      this->Script("%s SetPlotColor %d %f %f %f", 
                   this->XYPlotTclName, i,
                   r, g, b);
      ccolor += cstep;
      
      // Color by the choosen array in the output.
      // The menu's not changing, but It is going to replaced soon.
      // For now I will put this here.
      char tmp[256];
      sprintf(tmp, "Point %s %d", arrayName, component);
      this->GetPVOutput()->GetColorMenu()->SetValue(tmp);
      }
    this->GetPVOutput()->ColorByPointFieldComponent(arrayName, component);
    
    if ( numArrays > 1 )
      {
      this->Script("%s LegendOn", this->XYPlotTclName);
      }
    else 
      {
      this->Script("%s LegendOff", this->XYPlotTclName);
      this->Script("%s SetYTitle {%s}", this->XYPlotTclName, arrayName);
      this->Script("%s SetPlotColor 0 1 1 1", this->XYPlotTclName);
     }
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
}

void vtkPVProbe::Deselect(int doPackForget)
{
  this->ScalarArrayMenu->Update();
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
void vtkPVProbe::SaveInTclScript(ofstream *file, int interactiveFlag, 
                                  int vtkFlag)
{
  Tcl_Interp *interp = this->GetPVApplication()->GetMainInterp();  

  if (this->VisitedFlag)
    {
    return;
    } 
  this->VisitedFlag = 1;

  if (this->GetDimensionality() == 0)
    {
    *file << "vtkIdList pointList" << this->InstanceCount << "\n";
    *file << "\tpointList" << this->InstanceCount << " Allocate 1 1\n";
    *file << "\tpointList" << this->InstanceCount << " InsertNextId 0\n\n";

    this->Script("set coords [[%s GetInput] GetPoint 0]",
                 this->GetVTKSourceTclName());
    *file << "vtkPoints points" << this->InstanceCount << "\n";
    *file << "\tpoints" << this->InstanceCount << " Allocate 1 1\n";
    *file << "\tpoints" << this->InstanceCount << " InsertNextPoint " << interp->result << "\n\n";
    
    *file << "vtkPolyData probeInput" << this->InstanceCount << "\n";
    *file << "\tprobeInput" << this->InstanceCount << " Allocate 1 1\n";
    *file << "\tprobeInput" << this->InstanceCount << " SetPoints points" << this->InstanceCount << "\n";
    *file << "\tprobeInput" << this->InstanceCount << " InsertNextCell 1 pointList" << this->InstanceCount << "\n\n";
    }
  else
    {
    *file << "vtkLineSource line" << this->InstanceCount << "\n";

    this->Script("set point1 [[[%s GetInput] GetSource] GetPoint1]",
                 this->GetVTKSourceTclName());
    *file << "\tline" << this->InstanceCount << " SetPoint1 " << interp->result << "\n";
    this->Script("set point2 [[[%s GetInput] GetSource] GetPoint2]",
                 this->GetVTKSourceTclName());
    *file << "\tline" << this->InstanceCount << " SetPoint2 " << interp->result << "\n";
    this->Script("set res [[[%s GetInput] GetSource] GetResolution]",
                 this->GetVTKSourceTclName());
    *file << "\tline" << this->InstanceCount << " SetResolution " << interp->result << "\n\n";
    }
  
  *file << this->VTKSource->GetClassName() << " "
        << this->GetVTKSourceTclName() << "\n";
  if (this->GetDimensionality() == 0)
    {
    *file << "\t" << this->GetVTKSourceTclName() << " SetInput probeInput" 
          << this->InstanceCount << "\n";
    }
  else
    {
    *file << "\t" << this->GetVTKSourceTclName()
          << " SetInput [line" << this->InstanceCount << " GetOutput]\n";
    }
  
  *file << "\t" << this->GetVTKSourceTclName() << " SetSource [";
  *file << this->GetPVInput()->GetPVSource()->GetVTKSourceTclName()
        << " GetOutput]\n\n";
  
  if (this->GetDimensionality() == 1)
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
  
  this->GetPVOutput(0)->SaveInTclScript(file, interactiveFlag, vtkFlag);
}

//----------------------------------------------------------------------------
int vtkPVProbe::ClonePrototypeInternal(int makeCurrent, vtkPVSource*& clone)
{
  int retVal = this->Superclass::ClonePrototypeInternal(makeCurrent, clone);
  return retVal;
}

//----------------------------------------------------------------------------
int vtkPVProbe::GetDimensionality()
{
  const char* sourceName = this->GetVTKSourceTclName();
  if (!sourceName)
    {
    return 0;
    }
  
  const char* input = 
    this->Script("%s GetInput", this->GetVTKSourceTclName());

  const char* name = 0;
  if ( vtkString::Length(input) > 0 )
    {
    name = this->Script("[ %s GetSource ] GetClassName", input);
    }
  if ( vtkString::Equals(name, "vtkLineSource") )
    {
    return 1;
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkPVProbe::HSVtoRGB(float h, float s, float v, float *r, float *g, float *b)
{
  float R, G, B;
  float max = 1.0;
  float third = max / 3.0;
  float temp;

  // compute rgb assuming S = 1.0;
  if (h >= 0.0 && h <= third) // red -> green
    {
    G = h/third;
    R = 1.0 - G;
    B = 0.0;
    }
  else if (h >= third && h <= 2.0*third) // green -> blue
    {
    B = (h - third)/third;
    G = 1.0 - B;
    R = 0.0;
    }
  else // blue -> red
    {
    R = (h - 2.0 * third)/third;
    B = 1.0 - R;
    G = 0.0;
    }
        
  // add Saturation to the equation.
  s = s / max;
  //R = S + (1.0 - S)*R;
  //G = S + (1.0 - S)*G;
  //B = S + (1.0 - S)*B;
  // what happend to this?
  R = s*R + (1.0 - s);
  G = s*G + (1.0 - s);
  B = s*B + (1.0 - s);
      
  // Use value to get actual RGB 
  // normalize RGB first then apply value
  temp = R + G + B; 
  //V = 3 * V / (temp * max);
  // and what happend to this?
  v = 3 * v / (temp);
  R = R * v;
  G = G * v;
  B = B * v;
      
  // clip below 255
  //if (R > 255.0) R = max;
  //if (G > 255.0) G = max;
  //if (B > 255.0) B = max;
  // mixed constant 255 and max ?????
  if (R > max)
    {
    R = max;
    }
  if (G > max)
    {
    G = max;
    }
  if (B > max)
    {
    B = max;
    }
  *r = R;
  *g = G;
  *b = B;
}
//----------------------------------------------------------------------------
void vtkPVProbe::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Dimensionality: " << this->GetDimensionality() << endl;
  os << indent << "ShowXYPlotToggle: " << this->GetShowXYPlotToggle() << endl;
}
