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
#include "vtkLookupTable.h"
#include "vtkPVRenderView.h"
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVSourceCollection.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarWidget.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVColorMap);
vtkCxxRevisionMacro(vtkPVColorMap, "1.14");

int vtkPVColorMapCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//vtkCxxSetObjectMacro(vtkPVColorMap,PVRenderView,vtkPVRenderView);
//----------------------------------------------------------------------------
// No register count because of reference loop.
void vtkPVColorMap::SetPVRenderView(vtkPVRenderView *rv)
{
  this->PVRenderView = rv;
}

//----------------------------------------------------------------------------
vtkPVColorMap::vtkPVColorMap()
{
  this->CommandFunction = vtkPVColorMapCommand;

  // Used to be in vtkPVActorComposite
  static int instanceCount = 0;

  this->ScalarBarTitle = NULL;
  this->ArrayName = NULL;
  this->ScalarBarVisibility = 0;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
  this->Initialized = 0;

  this->PVRenderView = NULL;
  this->LookupTableTclName = NULL;
  this->LookupTable = NULL;
  this->ScalarBar = NULL;

  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;    
  
  this->NumberOfVectorComponents = 1;
  this->VectorComponent = 0;


  // User interaface.
  this->ScalarBarFrame = vtkKWLabeledFrame::New();

  this->ScalarBarTitleEntry = vtkKWLabeledEntry::New();
  this->ArrayNameLabel = vtkKWLabel::New();
  this->ScalarBarCheckFrame = vtkKWWidget::New();
  this->ScalarBarCheck = vtkKWCheckButton::New();
  
  // Stuff for setting the range of the color map.
  this->ColorRangeFrame = vtkKWWidget::New();
  this->ColorRangeResetButton = vtkKWPushButton::New();
  this->ColorRangeMinEntry = vtkKWLabeledEntry::New();
  this->ColorRangeMaxEntry = vtkKWLabeledEntry::New();

  this->ColorMapMenuLabel = vtkKWLabel::New();
  this->ColorMapMenu = vtkKWOptionMenu::New();

  this->BackButton = vtkKWPushButton::New();
}

//----------------------------------------------------------------------------
vtkPVColorMap::~vtkPVColorMap()
{
  // Used to be in vtkPVActorComposite........
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ArrayName)
    {
    delete [] this->ArrayName;
    this->ArrayName = NULL;
    }
  if (this->ScalarBarTitle)
    {
    delete [] this->ScalarBarTitle;
    this->ScalarBarTitle = NULL;
    }

  this->SetPVRenderView(NULL);

  if (this->LookupTableTclName)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->LookupTableTclName);
      }
    this->SetLookupTableTclName(NULL);
    this->LookupTable = NULL;
    }

  if (this->ScalarBar)
    {
    this->ScalarBar->Delete();
    this->ScalarBar = NULL;
    }
    
  // User interaface.
  this->ScalarBarFrame->Delete();
  this->ScalarBarFrame = NULL;

  this->ScalarBarTitleEntry->Delete();
  this->ScalarBarTitleEntry = NULL;
  this->ArrayNameLabel->Delete();
  this->ArrayNameLabel = NULL;
  this->ScalarBarCheckFrame->Delete();
  this->ScalarBarCheckFrame = NULL;
  this->ScalarBarCheck->Delete();
  this->ScalarBarCheck = NULL;
  
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

  this->BackButton->Delete();
  this->BackButton = NULL;
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

  this->ScalarBarTitleEntry->SetParent(this->ScalarBarFrame->GetFrame());
  this->ScalarBarTitleEntry->Create(this->Application);
  this->ScalarBarTitleEntry->SetLabel("Title");
  this->Script("bind %s <KeyPress-Return> {%s NameEntryCallback}",
               this->ScalarBarTitleEntry->GetEntry()->GetWidgetName(),
               this->GetTclName());
  this->Script("bind %s <FocusOut> {%s NameEntryCallback}",
               this->ScalarBarTitleEntry->GetEntry()->GetWidgetName(),
               this->GetTclName()); 

  this->ArrayNameLabel->SetParent(this->ScalarBarFrame->GetFrame());
  this->ArrayNameLabel->Create(this->Application, "");
  this->ArrayNameLabel->SetLabel("Parameter: ");

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

  this->BackButton->SetParent(this);
  this->BackButton->Create(this->Application, "-text {Back}");
  this->BackButton->SetCommand(this, "BackButtonCallback");

  this->Script("pack %s -fill x", this->ScalarBarFrame->GetWidgetName());
  this->Script("pack %s -fill x -padx 2 -pady 2", this->BackButton->GetWidgetName());
  this->Script("pack %s %s %s %s -side top -expand t -fill x",
               this->ScalarBarTitleEntry->GetWidgetName(),
               this->ArrayNameLabel->GetWidgetName(),
               this->ScalarBarCheckFrame->GetWidgetName(),
               this->ColorRangeFrame->GetWidgetName());
  this->Script("pack %s %s %s -side left",
               this->ScalarBarCheck->GetWidgetName(),
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
  this->LookupTable = static_cast<vtkLookupTable*>(
    pvApp->MakeTclObject("vtkLookupTable", this->LookupTableTclName));
  

  this->ScalarBar = vtkScalarBarWidget::New();
  this->ScalarBar->SetInteractor(
    this->PVRenderView->GetPVWindow()->GetGenericInteractor());
  this->ScalarBar->GetScalarBarActor()->GetPositionCoordinate()
    ->SetValue(0.87, 0.25);
  this->ScalarBar->GetScalarBarActor()->SetWidth(0.13);
  this->ScalarBar->GetScalarBarActor()->SetHeight(0.5);

  this->UpdateScalarBarTitle();

  this->ScalarBar->GetScalarBarActor()->SetLookupTable(this->LookupTable);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::BackButtonCallback()
{
  if (this->PVRenderView == NULL)
    {
    return;
    }
  this->PVRenderView->GetPVWindow()->ShowCurrentSourceProperties();
}


//----------------------------------------------------------------------------
void vtkPVColorMap::SetArrayName(const char* str)
{
  if ( this->ArrayName == NULL && str == NULL) 
    { 
    return;
    }
  if ( this->ArrayName && str && (!strcmp(this->ArrayName,str))) 
    { 
    return;
    }
  if (this->ArrayName)
    {
    delete [] this->ArrayName;
    this->ArrayName = NULL;
    }
  if (str)
    {
    this->ArrayName = new char[strlen(str)+1];
    strcpy(this->ArrayName,str);
    }
  if (str)
    {
    char *tmp;
    tmp = new char[strlen(str)+128];
    sprintf(tmp, "Parameter: %s", str);
    this->ArrayNameLabel->SetLabel(tmp);
    delete [] tmp;
    }
  this->ResetScalarRange();
}

//----------------------------------------------------------------------------
int vtkPVColorMap::MatchArrayName(const char* str)
{
  if (str == NULL || this->ArrayName == NULL)
    {
    return 0;
    }
  if (strcmp(str, this->ArrayName) == 0)
    {
    return 1;
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkPVColorMap::NameEntryCallback()
{
  this->SetScalarBarTitle(this->ScalarBarTitleEntry->GetValue());
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarTitle(const char* name)
{
  this->AddTraceEntry("$kw(%s) SetName {%s}", name);
  this->SetScalarBarTitleNoTrace(name);
}

//----------------------------------------------------------------------------
void vtkPVColorMap::SetScalarBarTitleNoTrace(const char* name)
{
  if ( this->ScalarBarTitle == NULL && name == NULL) 
    { 
    return;
    }
  if ( this->ScalarBarTitle && name && (!strcmp(this->ScalarBarTitle,name))) 
    { 
    return;
    }
  if (this->ScalarBarTitle) 
    { 
    delete [] this->ScalarBarTitle; 
    this->ScalarBarTitle = NULL;
    }
  if (name)
    {
    this->ScalarBarTitle = new char[strlen(name)+1];
    strcpy(this->ScalarBarTitle,name);
    } 


  this->ScalarBarTitleEntry->SetValue(name);
  if (name != NULL)
    {
    char *str;
    str = new char [strlen(name) + 128];
    sprintf(str, "GetPVColorMap {%s}", name);
    this->SetTraceReferenceCommand(str);
    delete [] str;
    }

  this->UpdateScalarBarTitle();
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
    pvd->GetArrayComponentRange(tmp, 1, this->ArrayName, 0);
    if (tmp[0] < range[0])
      {
      range[0] = tmp[0];
      }
    if (tmp[1] > range[1])
      {
      range[1] = tmp[1];
      }
    // For cell data ...
    pvd->GetArrayComponentRange(tmp, 0, this->ArrayName, 0);
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
  
  if (ren == NULL)
    {
    return;
    }
  
  // I am going to add and remove it from the renderer instead of using
  // visibility.  Composites should really have multiple props.
  
  if (ren)
    {
    if (val)
      {
      this->ScalarBar->SetEnabled(1);
      // This is here in case process 0 has not geometry.  
      // We have to explicitly build the color map.
      this->LookupTable->Build();
      this->LookupTable->Modified();
      }
    else
      {
      this->ScalarBar->SetEnabled(0);
      }
    }

  this->AddTraceEntry("$kw(%s) SetScalarBarVisibility %d", this->GetTclName(),
                      val);
}


//----------------------------------------------------------------------------
void vtkPVColorMap::SaveInTclScript(ofstream *file)
{
  float* position;
  char* result;
  char* renTclName;

  char scalarBarTclName[128];
  sprintf(scalarBarTclName, "ScalarBar%d", this->InstanceCount);
  renTclName = this->GetPVRenderView()->GetRendererTclName();

  if (this->ScalarBarVisibility)
    {
    *file << "vtkScalarBarWidget " << scalarBarTclName << "\n\t";
//    *file << renTclName << " AddProp " << scalarBarTclName << "\n";
    }


}


//----------------------------------------------------------------------------
void vtkPVColorMap::UpdateScalarBarTitle()
{
  if (this->ScalarBar == NULL || this->ScalarBarTitle == NULL)
    {
    return;
    }

  if (this->NumberOfVectorComponents == 3)
    {
    if (this->VectorComponent == 0)
      {
      ostrstream ostr;
      ostr << this->ScalarBarTitle << " X" << ends;
      this->ScalarBar->GetScalarBarActor()->SetTitle(ostr.str());
      ostr.rdbuf()->freeze(0);
      }
    if (this->VectorComponent == 1)
      {
      ostrstream ostr;
      ostr << this->ScalarBarTitle << " Y" << ends;
      this->ScalarBar->GetScalarBarActor()->SetTitle(ostr.str());
      ostr.rdbuf()->freeze(0);
      }
    if (this->VectorComponent == 2)
      {
      ostrstream ostr;
      ostr << this->ScalarBarTitle << " Z" << ends;
      this->ScalarBar->GetScalarBarActor()->SetTitle(ostr.str());
      ostr.rdbuf()->freeze(0);
      }
    }
  else if (this->NumberOfVectorComponents > 1)
    {
    ostrstream ostr;
    ostr << this->ScalarBarTitle << " " << this->VectorComponent << ends;
    this->ScalarBar->GetScalarBarActor()->SetTitle(ostr.str());
    ostr.rdbuf()->freeze(0);
    }
  else
    {
    ostrstream ostr;
    ostr << this->ScalarBarTitle<< ends;
    this->ScalarBar->GetScalarBarActor()->SetTitle(ostr.str());
    ostr.rdbuf()->freeze(0);
    }
  if (this->PVRenderView)
    {
    this->PVRenderView->EventuallyRender();
    }
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
  os << indent << "ScalarBarTitle: " 
     << (this->ScalarBarTitle ? this->ScalarBarTitle : "none" )
     << endl;
  os << indent << "ArrayName: " 
     << (this->ArrayName ? this->ArrayName : "none" )
     << endl;
  os << indent << "VectorComponent: " << this->VectorComponent << endl;
  os << indent << "LookupTableTclName: " 
     << (this->LookupTableTclName ? this->LookupTableTclName : "none" )
     << endl;
  os << indent << "ScalarBar: " << this->ScalarBar << endl;
  
  os << indent << "ScalarBarVisibility: " << this->ScalarBarVisibility << endl;

  os << indent << "ScalarRange: " << this->ScalarRange[0] << ", "
     << this->ScalarRange[1] << endl;
}
