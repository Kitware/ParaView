/*=========================================================================

  Program:   ParaView
  Module:    vtkPVColorMap.cxx
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
#include "vtkPVColorMap.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWEntry.h"
#include "vtkKWWidget.h"
#include "vtkKWCheckButton.h"
#include "vtkKWPushButton.h"
#include "vtkPVRenderView.h"
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVSourceCollection.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVColorMap);
vtkCxxRevisionMacro(vtkPVColorMap, "1.8");

int vtkPVColorMapCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

vtkCxxSetObjectMacro(vtkPVColorMap,PVRenderView,vtkPVRenderView);


//----------------------------------------------------------------------------
vtkPVColorMap::vtkPVColorMap()
{
  this->CommandFunction = vtkPVColorMapCommand;

  // Used to be in vtkPVActorComposite
  static int instanceCount = 0;

  this->ParameterName = NULL;
  this->ScalarBarVisibility = 0;
  this->ScalarBarOrientation = 1;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
  this->Initialized = 0;

  this->PVRenderView = NULL;
  this->LookupTableTclName = NULL;
  this->ScalarBarTclName = NULL;

  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;    
  
  this->NumberOfVectorComponents = 1;
  this->VectorComponent = 0;


  // User interaface.
  this->ScalarBarFrame = vtkKWLabeledFrame::New();

  this->LabelEntry = vtkKWLabeledEntry::New();
  this->ScalarBarCheckFrame = vtkKWWidget::New();
  this->ScalarBarCheck = vtkKWCheckButton::New();
  this->ScalarBarOrientationCheck = vtkKWCheckButton::New();
  
  // Stuff for setting the range of the color map.
  this->ColorRangeFrame = vtkKWWidget::New();
  this->ColorRangeResetButton = vtkKWPushButton::New();
  this->ColorRangeMinEntry = vtkKWLabeledEntry::New();
  this->ColorRangeMaxEntry = vtkKWLabeledEntry::New();

  this->ColorMapMenuLabel = vtkKWLabel::New();
  this->ColorMapMenu = vtkKWOptionMenu::New();
}

//----------------------------------------------------------------------------
vtkPVColorMap::~vtkPVColorMap()
{
  // Used to be in vtkPVActorComposite........
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->SetScalarBarVisibility(0);

  if (this->ParameterName)
    {
    delete [] this->ParameterName;
    this->ParameterName = NULL;
    }

  this->SetPVRenderView(NULL);

  if (this->LookupTableTclName)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->LookupTableTclName);
      }
    this->SetLookupTableTclName(NULL);
    }

  if (this->ScalarBarTclName)
    {
    if ( pvApp )
      {
      pvApp->Script("%s Delete", this->ScalarBarTclName);
      }
    this->SetScalarBarTclName(NULL);
    }
    
  // User interaface.
  this->ScalarBarFrame->Delete();
  this->ScalarBarFrame = NULL;

  this->LabelEntry->Delete();
  this->LabelEntry = NULL;
  this->ScalarBarCheckFrame->Delete();
  this->ScalarBarCheckFrame = NULL;
  this->ScalarBarCheck->Delete();
  this->ScalarBarCheck = NULL;
  this->ScalarBarOrientationCheck->Delete();
  this->ScalarBarOrientationCheck = NULL;
  
  // Stuff for setting the range of the color map.
  this->ColorRangeFrame->Delete();
  this->ColorRangeFrame = NULL;
  this->ColorRangeResetButton->Delete();
  this->ColorRangeResetButton = NULL;
  this->ColorRangeMinEntry->Delete();
  this->ColorRangeMinEntry = NULL;
  this->ColorRangeMaxEntry->Delete();      
  this->ColorRangeMaxEntry = NULL;     

  this->ColorMapMenuLabel->Delete();
  this->ColorMapMenuLabel = NULL;
  this->ColorMapMenu->Delete();
  this->ColorMapMenu = NULL;
}


//----------------------------------------------------------------------------
void vtkPVColorMap::Create(vtkKWApplication *app)
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);
  const char* wname;
  
  if (this->Application)
    {
    vtkErrorMacro("LabeledToggle already created");
    return;
    }
  // Superclass create takes a KWApplication, but we need a PVApplication.
  if (pvApp == NULL)
    {
    vtkErrorMacro("Need a PV application");
    return;
    }
  this->SetApplication(app);
  this->CreateParallelTclObjects(pvApp);
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);
  
  // Now for the UI.
  this->ScalarBarFrame->SetParent(this);
  this->ScalarBarFrame->ShowHideFrameOff();
  this->ScalarBarFrame->Create(this->Application);
  this->ScalarBarFrame->SetLabel("Scalar Bar");

  this->ScalarBarCheckFrame->SetParent(this->ScalarBarFrame->GetFrame());
  this->ScalarBarCheckFrame->Create(this->Application, "frame", "");

  this->ColorMapMenuLabel->SetParent(this->ScalarBarCheckFrame);
  this->ColorMapMenuLabel->Create(this->Application, "");
  this->ColorMapMenuLabel->SetLabel("Color map:");
  
  this->ColorMapMenu->SetParent(this->ScalarBarCheckFrame);
  this->ColorMapMenu->Create(this->Application, "");
  this->ColorMapMenu->AddEntryWithCommand("Red to Blue", this,
                                          "SetColorSchemeToRedBlue");
  this->ColorMapMenu->AddEntryWithCommand("Blue to Red", this,
                                          "SetColorSchemeToBlueRed");
  this->ColorMapMenu->AddEntryWithCommand("Grayscale", this,
                                          "SetColorSchemeToGrayscale");
  this->ColorMapMenu->SetValue("Red to Blue");
  
  this->ColorRangeFrame->SetParent(this->ScalarBarFrame->GetFrame());
  this->ColorRangeFrame->Create(this->Application, "frame", "");
  this->ColorRangeResetButton->SetParent(this->ColorRangeFrame);
  this->ColorRangeResetButton->Create(this->Application, 
                                      "-text {Reset Range}");
  this->ColorRangeResetButton->SetCommand(this, "ResetScalarRange");
  this->ColorRangeMinEntry->SetParent(this->ColorRangeFrame);
  this->ColorRangeMinEntry->Create(this->Application);
  this->ColorRangeMinEntry->SetLabel("Min:");
  this->ColorRangeMinEntry->GetEntry()->SetWidth(7);
  this->Script("bind %s <KeyPress-Return> {%s ColorRangeEntryCallback}",
               this->ColorRangeMinEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());
  this->Script("bind %s <FocusOut> {%s ColorRangeEntryCallback}",
               this->ColorRangeMinEntry->GetEntry()->GetWidgetName(),
               this->GetTclName()); 
  this->ColorRangeMaxEntry->SetParent(this->ColorRangeFrame);
  this->ColorRangeMaxEntry->Create(this->Application);
  this->ColorRangeMaxEntry->SetLabel("Max:");
  this->ColorRangeMaxEntry->GetEntry()->SetWidth(7);
  this->Script("bind %s <KeyPress-Return> {%s ColorRangeEntryCallback}",
               this->ColorRangeMaxEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());
  this->Script("bind %s <FocusOut> {%s ColorRangeEntryCallback}",
               this->ColorRangeMaxEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());

  this->ScalarBarCheck->SetParent(this->ScalarBarCheckFrame);
  this->ScalarBarCheck->Create(this->Application, "-text Visibility");
  this->Application->Script(
    "%s configure -command {%s ScalarBarCheckCallback}",
    this->ScalarBarCheck->GetWidgetName(),
    this->GetTclName());

  this->ScalarBarOrientationCheck->SetParent(this->ScalarBarCheckFrame);
  this->ScalarBarOrientationCheck->Create(this->Application, "-text Vertical");
  this->ScalarBarOrientationCheck->SetState(1);
  this->ScalarBarOrientationCheck->SetCommand(this, 
                                              "ScalarBarOrientationCallback");



  this->Script("pack %s -fill x", this->ScalarBarFrame->GetWidgetName());
  this->Script("pack %s %s -side top -expand t -fill x",
               this->ScalarBarCheckFrame->GetWidgetName(),
               this->ColorRangeFrame->GetWidgetName());
  this->Script("pack %s %s %s %s -side left",
               this->ScalarBarCheck->GetWidgetName(),
               this->ScalarBarOrientationCheck->GetWidgetName(),
               this->ColorMapMenuLabel->GetWidgetName(),
               this->ColorMapMenu->GetWidgetName());
  this->Script("pack %s -side left -expand f",
               this->ColorRangeResetButton->GetWidgetName());
  this->Script("pack %s %s -side left -expand t -fill x",
               this->ColorRangeMinEntry->GetWidgetName(),
               this->ColorRangeMaxEntry->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVColorMap::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  char tclName[100];
  
  this->vtkKWWidget::SetApplication(pvApp);
  
  sprintf(tclName, "LookupTable%d", this->InstanceCount);
  this->SetLookupTableTclName(tclName);
  pvApp->BroadcastScript("vtkLookupTable %s", this->LookupTableTclName);
  

  // Not actually parallel.  Only on process 0.
  sprintf(tclName, "ScalarBar%d", this->InstanceCount);
  this->SetScalarBarTclName(tclName);
  this->Script("vtkScalarBarActor %s", this->GetScalarBarTclName());
  this->Script("[%s GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport",
               this->GetScalarBarTclName());
  this->Script("[%s GetPositionCoordinate] SetValue 0.87 0.25",
               this->GetScalarBarTclName());
  this->Script("%s SetOrientationToVertical",
               this->GetScalarBarTclName());
  this->Script("%s SetWidth 0.13", this->GetScalarBarTclName());
  this->Script("%s SetHeight 0.5", this->GetScalarBarTclName());

  this->UpdateScalarBarTitle();
  
  this->Script("%s SetLookupTable %s",
               this->ScalarBarTclName, 
               this->LookupTableTclName);
  
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetName(const char* name)
{
  if (name != NULL)
    {
    char *str;
    str = new char [strlen(name) + 128];
    sprintf(str, "GetPVColorMap {%s}", name);
    this->SetTraceReferenceCommand(str);
    delete [] str;
    }

  this->SetParameterName(name);
  this->UpdateScalarBarTitle();

  this->ResetScalarRange();
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetColorSchemeToRedBlue()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s SetHueRange 0 0.666667",
                         this->LookupTableTclName);
  pvApp->BroadcastScript("%s SetSaturationRange 1 1",
                         this->LookupTableTclName);
  pvApp->BroadcastScript("%s SetValueRange 1 1",
                         this->LookupTableTclName);
  pvApp->BroadcastScript("%s Build",
                         this->LookupTableTclName);

  this->GetPVRenderView()->EventuallyRender();

  this->AddTraceEntry("$kw(%s) SetColorSchemeToRedBlue", this->GetTclName());
}


//----------------------------------------------------------------------------
void vtkPVColorMap::SetColorSchemeToBlueRed()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s SetHueRange 0.666667 0",
                         this->LookupTableTclName);
  pvApp->BroadcastScript("%s SetSaturationRange 1 1",
                         this->LookupTableTclName);
  pvApp->BroadcastScript("%s SetValueRange 1 1",
                         this->LookupTableTclName);
  pvApp->BroadcastScript("%s Build",
                         this->LookupTableTclName);

  this->GetPVRenderView()->EventuallyRender();

  this->AddTraceEntry("$kw(%s) SetColorSchemeToBlueRed", this->GetTclName());
}


//----------------------------------------------------------------------------
void vtkPVColorMap::SetColorSchemeToGrayscale()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  pvApp->BroadcastScript("%s SetHueRange 0 0",
                         this->LookupTableTclName);
  pvApp->BroadcastScript("%s SetSaturationRange 0 0",
                         this->LookupTableTclName);
  pvApp->BroadcastScript("%s SetValueRange 0 1",
                         this->LookupTableTclName);
  pvApp->BroadcastScript("%s Build",
                         this->LookupTableTclName);

  this->GetPVRenderView()->EventuallyRender();

  this->AddTraceEntry("$kw(%s) SetColorSchemeToGrayscale", this->GetTclName());

}

//----------------------------------------------------------------------------
void vtkPVColorMap::ScalarBarCheckCallback()
{
  this->SetScalarBarVisibility(this->ScalarBarCheck->GetState());
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVColorMap::ScalarBarOrientationCallback()
{
  int state = this->ScalarBarOrientationCheck->GetState();
  
  if (state)
    {
    this->SetScalarBarOrientationToVertical();
    //this->AddTraceEntry("$kw(%s) SetScalarBarOrientationToVertical", this->GetTclName());
    }
  else
    {
    this->SetScalarBarOrientationToHorizontal();
    //this->AddTraceEntry("$kw(%s) SetScalarBarOrientationToHorizontal", this->GetTclName());
    }
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarRange(float min, float max)
{
  this->SetScalarRangeInternal(min, max);
  this->AddTraceEntry("$kw(%s) SetScalarRange %f %f", this->GetTclName(),
                      min, max);
}


//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarRangeInternal(float min, float max)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ScalarRange[0] == min && this->ScalarRange[1] == max)
    {
    return;
    }

  this->ScalarRange[0] = min;
  this->ScalarRange[1] = max;

  this->ColorRangeMinEntry->SetValue(this->ScalarRange[0], 5);
  this->ColorRangeMaxEntry->SetValue(this->ScalarRange[1], 5);

  pvApp->BroadcastScript("%s SetTableRange %f %f", 
                         this->LookupTableTclName, min, max);
  //this->Script("%s Build", this->LookupTableTclName);
  //this->Script("%s Modified", this->LookupTableTclName);

}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetVectorComponent(int component, int numberOfComponents)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->VectorComponent == component && 
      this->NumberOfVectorComponents == numberOfComponents)
    {
    return;
    }

  this->VectorComponent = component;
  this->NumberOfVectorComponents = numberOfComponents;

  // Change the title of the scalar bar.
  this->UpdateScalarBarTitle();

  pvApp->BroadcastScript("%s SetVectorComponent %d", 
                         this->LookupTableTclName, component);

}


//----------------------------------------------------------------------------
void vtkPVColorMap::ResetScalarRange()
{
  float range[2];
  float tmp[2];
  vtkPVSourceCollection *sourceList;
  vtkPVSource *pvs;
  vtkPVData *pvd;

  if (this->Application == NULL || this->PVRenderView == NULL)
    {
    vtkErrorMacro("Trying to reset scalar range without application and view.");
    return;
    }

  range[0] = VTK_LARGE_FLOAT;
  range[1] = -VTK_LARGE_FLOAT;

  // Compute global scalar range ...
  sourceList = this->PVRenderView->GetPVWindow()->GetSourceList("Sources");
  sourceList->InitTraversal();
  while ( (pvs = sourceList->GetNextPVSource()) )
    {
    pvd = pvs->GetPVOutput();
    // For point data ...
    pvd->GetArrayComponentRange(tmp, 1, this->ParameterName, 0);
    if (tmp[0] < range[0])
      {
      range[0] = tmp[0];
      }
    if (tmp[1] > range[1])
      {
      range[1] = tmp[1];
      }
    // For cell data ...
    pvd->GetArrayComponentRange(tmp, 0, this->ParameterName, 0);
    if (tmp[0] < range[0])
      {
      range[0] = tmp[0];
      }
    if (tmp[1] > range[1])
      {
      range[1] = tmp[1];
      }
    }

  if (range[1] < range[0])
    {
    range[0] = 0.0;
    range[1] = 1.0;
    }

  this->SetScalarRange(range[0], range[1]);

  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
vtkPVApplication* vtkPVColorMap::GetPVApplication()
{
  if (this->Application == NULL)
    {
    return NULL;
    }
  
  if (this->Application->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->Application);
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}


//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarVisibility(int val)
{
  vtkRenderer *ren;
  char *tclName;

  if (this->ScalarBarVisibility == val)
    {
    return;
    }
  this->ScalarBarVisibility = val;
  
  // Make sure the UI is up to date.
  if (val)
    {
    this->ScalarBarCheck->SetState(1);
    }
  else
    {
    this->ScalarBarCheck->SetState(0);
    }


  if (!this->GetPVRenderView())
    {
    return;
    }
  
  ren = this->GetPVRenderView()->GetRenderer();
  tclName = this->GetPVRenderView()->GetRendererTclName();
  
  if (ren == NULL)
    {
    return;
    }
  
  // I am going to add and remove it from the renderer instead of using visibility.
  // Composites should really have multiple props.
  
  if (ren)
    {
    if (val)
      {
      this->Script("%s AddActor %s", tclName, this->GetScalarBarTclName());
      // This is here in case process 0 has not geometry.  
      // We have to explicitly build the color map.
      this->Script("%s Build", this->LookupTableTclName);
      this->Script("%s Modified", this->LookupTableTclName);
      }
    else
      {
      this->Script("%s RemoveActor %s", tclName, this->GetScalarBarTclName());
      }
    }

  this->AddTraceEntry("$kw(%s) SetScalarBarVisibility %d", this->GetTclName(),
                      val);
}


//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarOrientation(int vertical)
{
  if (this->ScalarBarOrientation == vertical)
    {
    return;
    }

  this->ScalarBarOrientation = vertical;
  if (vertical)
    {
    this->ScalarBarOrientationCheck->SetState(1);
    this->Script("[%s GetPositionCoordinate] SetValue 0.87 0.25",
                 this->GetScalarBarTclName());
    this->Script("%s SetOrientationToVertical", this->GetScalarBarTclName());
    this->Script("%s SetHeight 0.5", this->GetScalarBarTclName());
    this->Script("%s SetWidth 0.13", this->GetScalarBarTclName());
    }
  else
    {
    this->ScalarBarOrientationCheck->SetState(0);
    this->Script("[%s GetPositionCoordinate] SetValue 0.25 0.13",
                 this->GetScalarBarTclName());
    this->Script("%s SetOrientationToHorizontal", this->GetScalarBarTclName());
    this->Script("%s SetHeight 0.13", this->GetScalarBarTclName());
    this->Script("%s SetWidth 0.5", this->GetScalarBarTclName());
    }
  this->GetPVRenderView()->EventuallyRender();

  this->AddTraceEntry("$kw(%s) SetScalarBarOrientation %d", this->GetTclName(),
                      vertical);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarOrientationToVertical()
{
  this->SetScalarBarOrientation(1);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarOrientationToHorizontal()
{
  this->SetScalarBarOrientation(0);
}


//----------------------------------------------------------------------------
void vtkPVColorMap::SaveInTclScript(ofstream *file)
{
  float position[2];
  char* result;
  char* renTclName;

  renTclName = this->GetPVRenderView()->GetRendererTclName();

  if (this->ScalarBarVisibility)
    {
    *file << "vtkScalarBarActor " << this->ScalarBarTclName << "\n\t"
          << "[" << this->ScalarBarTclName
          << " GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport\n\t"
          << "[" << this->ScalarBarTclName
          << " GetPositionCoordinate] SetValue ";
    this->Script("set tempResult [[%s GetPositionCoordinate] GetValue]",
                 this->ScalarBarTclName);
    result = this->GetPVApplication()->GetMainInterp()->result;
    sscanf(result, "%f %f", &position[0], &position[1]);
    *file << position[0] << " " << position[1] << "\n\t"
          << this->ScalarBarTclName << " SetOrientationTo";
    this->Script("set tempResult [%s GetOrientation]",
                 this->ScalarBarTclName);
    result = this->GetPVApplication()->GetMainInterp()->result;
    if (strncmp(result, "0", 1) == 0)
      {
      *file << "Horizontal\n\t";
      }
    else
      {
      *file << "Vertical\n\t";
      }
    *file << this->ScalarBarTclName << " SetWidth ";
    this->Script("set tempResult [%s GetWidth]",
                 this->ScalarBarTclName);
    result = this->GetPVApplication()->GetMainInterp()->result;
    *file << result << "\n\t";
    *file << this->ScalarBarTclName << " SetHeight ";
    this->Script("set tempResult [%s GetHeight]",
                 this->ScalarBarTclName);
    result = this->GetPVApplication()->GetMainInterp()->result;
    *file << result << "\n\t"
          << this->ScalarBarTclName << " SetLookupTable "
          << this->LookupTableTclName << "\n\t"
          << this->ScalarBarTclName << " SetTitle {" 
          << this->ParameterName << "}\n";
    *file << renTclName << " AddProp " << this->ScalarBarTclName << "\n";
    }


}


//----------------------------------------------------------------------------
void vtkPVColorMap::UpdateScalarBarTitle()
{
  if (this->ScalarBarTclName == NULL)
    {
    return;
    }

  if (this->NumberOfVectorComponents == 3)
    {
    if (this->VectorComponent == 0)
      {
      this->Script("%s SetTitle {%s X}", this->ScalarBarTclName,
                   this->ParameterName);
      }
    if (this->VectorComponent == 1)
      {
      this->Script("%s SetTitle {%s Y}", this->ScalarBarTclName,
                   this->ParameterName);
      }
    if (this->VectorComponent == 2)
      {
      this->Script("%s SetTitle {%s Z}", this->ScalarBarTclName,
                   this->ParameterName);
      }
    return;
    }

  if (this->NumberOfVectorComponents > 1)
    {
    this->Script("%s SetTitle {%s %d}", this->ScalarBarTclName,
                 this->ParameterName, this->VectorComponent);
    return;
    }

  this->Script("%s SetTitle {%s}", this->ScalarBarTclName,
               this->ParameterName);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::ColorRangeEntryCallback()
{
  float min, max;

  min = this->ColorRangeMinEntry->GetValueAsFloat();
  max = this->ColorRangeMaxEntry->GetValueAsFloat();
  
  // Avoid the bad range error
  if (max <= min)
    {
    max = min + 0.00001;
    }

  this->SetScalarRange(min, max);
  if ( this->GetPVRenderView() )
    {
    this->GetPVRenderView()->EventuallyRender();
    }
}



//----------------------------------------------------------------------------
void vtkPVColorMap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "VectorComponent: " << this->VectorComponent << endl;
  os << indent << "LookupTableTclName: " 
     << (this->LookupTableTclName ? this->LookupTableTclName : "none" )
     << endl;
  os << indent << "ScalarBarTclName: " 
     << (this->ScalarBarTclName ? this->ScalarBarTclName : "none" )
     << endl;
  
  os << indent << "ScalarBarVisibility: " << this->ScalarBarVisibility << endl;
  if (this->ScalarBarOrientation)
    {
    os << indent << "ScalarBarOrientation: Vertical\n";
    }
  else
    {
    os << indent << "ScalarBarOrientation: Horizontal\n";
    }

  os << indent << "ScalarRange: " << this->ScalarRange[0] << ", "
     << this->ScalarRange[1] << endl;
}
