/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVData.cxx
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
#include "vtkPVData.h"
#include "vtkPVPolyData.h" 
#include "vtkPVSource.h"
#include "vtkKWView.h"
#include "vtkPVWindow.h"
#include "vtkKWApplication.h"
#include "vtkPVCutter.h"
#include "vtkPVAssignment.h"
#include "vtkPVApplication.h"
#include "vtkPVActorComposite.h"
#include "vtkPVScalarBar.h"
#include "vtkKWMenuButton.h"
#include "vtkElevationFilter.h"
#include "vtkSingleContourFilter.h"
#include "vtkExtractEdges.h"
#include "vtkPVDataSetToDataSetFilter.h"

int vtkPVDataCommand(ClientData cd, Tcl_Interp *interp,
		     int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVData::vtkPVData()
{
  this->CommandFunction = vtkPVDataCommand;

  this->Data = NULL;
  this->PVSource = NULL;
  
  this->FiltersMenuButton = vtkKWMenuButton::New();
  
  this->Assignment = NULL;

  this->ActorCompositeButton = vtkKWPushButton::New();
  this->ScalarBarButton = vtkKWPushButton::New();

  // This is initialized in "Clone();"
  this->ActorComposite = NULL;
  this->ScalarBar = NULL;

  this->PVSourceCollection = vtkPVSourceCollection::New();
}

//----------------------------------------------------------------------------
vtkPVData::~vtkPVData()
{
  this->SetAssignment(NULL);
  this->SetData(NULL);
  this->SetPVSource(NULL);

  this->FiltersMenuButton->Delete();
  this->FiltersMenuButton = NULL;
  
  if (this->ActorComposite)
    {
    this->ActorComposite->UnRegister(this);
    this->ActorComposite = NULL;
    }
  
  this->ActorCompositeButton->Delete();
  this->ActorCompositeButton = NULL;

  if (this->ScalarBar)
    {
    this->ScalarBar->Delete();
    this->ScalarBar = NULL;
    }
  
  this->ScalarBarButton->Delete();
  this->ScalarBarButton = NULL;
  
  this->PVSourceCollection->Delete();
  this->PVSourceCollection = NULL;  
}

//----------------------------------------------------------------------------
vtkPVData* vtkPVData::New()
{
  return new vtkPVData();
}

//----------------------------------------------------------------------------
void vtkPVData::Clone(vtkPVApplication *pvApp)
{
  if (this->Application)
    {
    vtkErrorMacro("Application has already been set.");
    }
  this->SetApplication(pvApp);

  // Clone this object on every other process.
  pvApp->BroadcastScript("%s %s", this->GetClassName(), this->GetTclName());
  
  // The clones might as well have an application.
  pvApp->BroadcastScript("%s SetApplication %s", this->GetTclName(),
			 pvApp->GetTclName());  
  
  // Now create an actor composite with identical tcl names in every process.
  vtkPVActorComposite *c;
  c = vtkPVActorComposite::New();
  c->Clone(pvApp);
  this->SetActorComposite(c);
  c->Delete();

  vtkPVScalarBar *sb;
  sb = vtkPVScalarBar::New();
  sb->Clone(pvApp);
  this->SetScalarBar(sb);
  sb->Delete();

  // What about deleting the cloned composites?
}


//----------------------------------------------------------------------------
int vtkPVData::Create(char *args)
{
  if (this->Application == NULL)
    {
    vtkErrorMacro("Object has not been cloned yet.");
    return 0;
    }
  
  // create the top level
  this->Script("frame %s %s", this->GetWidgetName(), args);

  // The contour entry checks for scalars.
  this->Update();
  
  this->FiltersMenuButton->SetParent(this);
  this->FiltersMenuButton->Create(this->Application, "-relief raised -bd 2");
  this->FiltersMenuButton->SetButtonText("Filters");
  this->FiltersMenuButton->AddCommand("ContourFilter", this,
				      "Contour");
  if (this->Data->GetPointData()->GetScalars() == NULL)
    {
    this->Script("%s entryconfigure 3 -state disabled",
		 this->FiltersMenuButton->GetMenu()->GetWidgetName());
    // If there are not point scalars, the scalar bar should not be
    // visible either.
    this->GetScalarBar()->VisibilityOff();
    }
  else
    {
    this->Script("%s entryconfigure 3 -state normal",
		 this->FiltersMenuButton->GetMenu()->GetWidgetName());
    }
  
  this->FiltersMenuButton->AddCommand("ElevationFilter", this,
				      "Elevation");
  this->FiltersMenuButton->AddCommand("ExtractEdges", this,
				      "ExtractEdges");
  this->FiltersMenuButton->AddCommand("ColorByProcess", this,
				      "ColorByProcess");
  this->FiltersMenuButton->AddCommand("Cutter", this, "Cutter");
  
  this->Script("pack %s", this->FiltersMenuButton->GetWidgetName());
  
  this->ActorCompositeButton->SetParent(this);
  this->ActorCompositeButton->Create(this->Application, "");
  this->ActorCompositeButton->SetLabel("Get Actor Composite");
  this->ActorCompositeButton->SetCommand(this, "ShowActorComposite");
  this->ScalarBarButton->SetParent(this);
  this->ScalarBarButton->Create(this->Application, "");
  this->ScalarBarButton->SetLabel("Get Scalar Bar Parameters");
  this->ScalarBarButton->SetCommand(this, "ShowScalarBarParameters");
  this->Script("pack %s %s", this->ActorCompositeButton->GetWidgetName(),
               this->ScalarBarButton->GetWidgetName());

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVData::ShowActorComposite()
{
  this->GetActorComposite()->ShowProperties();
}

//----------------------------------------------------------------------------
void vtkPVData::ShowScalarBarParameters()
{
  this->GetScalarBar()->ShowProperties();
}


//----------------------------------------------------------------------------
void vtkPVData::Contour()
{
  static int instanceCount = 0;
  vtkPVDataSetToPolyDataFilter *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  float *range = this->GetData()->GetScalarRange();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVDataSetToPolyDataFilter::SafeDownCast(
    pvApp->MakePVSource("vtkPVDataSetToPolyDataFilter",
			"vtkSingleContourFilter",
			"Contour", ++instanceCount));
  if (f == NULL) {return;}
  f->SetPVInput(this);
  ((vtkSingleContourFilter*)f->GetVTKSource())->SetFirstValue((range[0] +
							       range[1])/2.0);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);
  
  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddPVInputList();
  f->AddLabeledEntry("Value", "SetFirstValue", "GetFirstValue");
  f->AddLabeledToggle("ComputeNormals", "SetComputeNormals",
		      "GetComputeNormals");
  f->AddLabeledToggle("ComputeGradients", "SetComputeGradients",
		     "GetComputeGradients");
  f->AddLabeledToggle("ComputeScalars", "SetComputeScalars",
		      "GetComputeScalars");
  f->UpdateParameterWidgets();
  
  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
}

//----------------------------------------------------------------------------
void vtkPVData::Cutter()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVCutter *cutter = vtkPVCutter::New();
  
  cutter->Clone(pvApp);
  cutter->SetPVInput(this);
  
  cutter->SetOrigin(0, 0, 0);
  cutter->SetNormal(0, 0, 1);

  this->GetPVSource()->GetView()->AddComposite(cutter);
  cutter->SetName("cutter");
  
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  window->SetCurrentSource(cutter);
  cutter->AddPVInputList();
  
  cutter->Delete();
}
  
//----------------------------------------------------------------------------
void vtkPVData::Elevation()
{
  static int instanceCount = 0;
  vtkPVDataSetToDataSetFilter *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  float *bounds = this->GetData()->GetBounds();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVDataSetToDataSetFilter::SafeDownCast(
    pvApp->MakePVSource("vtkPVDataSetToDataSetFilter",
			"vtkElevationFilter",
			"Elevation", ++instanceCount));
  if (f == NULL) {return;}
  f->SetPVInput(this);
  ((vtkElevationFilter*)f->GetVTKSource())->SetLowPoint(bounds[0], 0, 0);
  ((vtkElevationFilter*)f->GetVTKSource())->SetHighPoint(bounds[1], 0, 0);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);
  
  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddPVInputList();
  f->AddVector3Entry("LowPoint", "X", "Y", "Z", "SetLowPoint", "GetLowPoint");
  f->AddVector3Entry("HighPoint", "X", "Y", "Z", "SetHighPoint",
		     "GetHighPoint");
  f->AddVector2Entry("ScalarRange", "Min", "Max", "SetScalarRange",
		     "GetScalarRange");
  f->UpdateParameterWidgets();
  
  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
}

//----------------------------------------------------------------------------
void vtkPVData::ExtractEdges()
{
  static int instanceCount = 0;
  vtkPVDataSetToPolyDataFilter *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVDataSetToPolyDataFilter::SafeDownCast(
    pvApp->MakePVSource("vtkPVDataSetToPolyDataFilter",
			"vtkExtractEdges",
			"ExtractEdges", ++instanceCount));
  if (f == NULL) {return;}
  f->SetPVInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);
  
  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddPVInputList();
  f->UpdateParameterWidgets();
  
  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByProcess()
{
  static int instanceCount = 0;
  vtkPVDataSetToDataSetFilter *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVDataSetToDataSetFilter::SafeDownCast(
    pvApp->MakePVSource("vtkPVDataSetToDataSetFilter",
			"vtkColorByProcess",
			"ColorByProcess", ++instanceCount));
  if (f == NULL) {return;}
  f->SetPVInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);
  
  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddPVInputList();
  f->UpdateParameterWidgets();
  
  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
}


//----------------------------------------------------------------------------
// This is a bit more complicated than you would expect, because I have to 
// handle the case when some processes have empty pieces.
void vtkPVData::GetBounds(float bounds[6])
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  float tmp[6];
  float id, num, emptyFlag;
  
  // Just some error checking.
  if (controller->GetLocalProcessId() != 0)
    {
    vtkErrorMacro("This method should only be called from processes 0");
    return;
    }
  
  if (this->Data == NULL)
    {
    bounds[0] = bounds[1] = bounds[2] = 0.0;
    bounds[3] = bounds[4] = bounds[5] = 0.0;
    }
  
  pvApp->BroadcastScript("%s TransmitBounds", this->GetTclName());
  
  if (this->Assignment == NULL)
    {
    vtkWarningMacro("Cannot update without Assignment.");
    }
  else
    {
    this->Data->SetUpdateExtent(this->Assignment->GetPiece(),
				this->Assignment->GetNumberOfPieces(),
				0);
    this->Data->Update();
    }
  
  if (this->Data->GetNumberOfCells() > 0)
    {
    this->Data->GetBounds(bounds);
    emptyFlag = 0;
    }
  else
    {
    emptyFlag = 1;
    }
  
  num = controller->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    if (emptyFlag)
      {      
      controller->Receive(&emptyFlag, 1, id, 993);
      controller->Receive(bounds, 6, id, 994);
      }
    else
      {
      controller->Receive(&emptyFlag, 1, id, 993);
      controller->Receive(tmp, 6, id, 994);
      if ( ! emptyFlag )
	{
	if (tmp[0] < bounds[0])
	  {
	  bounds[0] = tmp[0];
	  }
	if (tmp[1] > bounds[1])
	  {
	  bounds[1] = tmp[1];
	  }
	if (tmp[2] < bounds[2])
	  {
	  bounds[2] = tmp[2];
	  }
	if (tmp[3] > bounds[3])
	  {
	  bounds[3] = tmp[3];
	  }
	if (tmp[4] < bounds[4])
	  {
	  bounds[4] = tmp[4];
	  }
	if (tmp[5] > bounds[5])
	  {
	  bounds[5] = tmp[5];
	  }
	}
      // emptyFlag was 1 before this block. Restore it.
      emptyFlag = 1;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVData::TransmitBounds()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  float bounds[6];
  float emptyFlag;
  
  // Try to update data.
  if (this->Assignment == NULL)
    {
    vtkWarningMacro("Cannot update without Assignment.");
    }
  else
    {
    this->Data->SetUpdateExtent(this->Assignment->GetPiece(),
				this->Assignment->GetNumberOfPieces(),
				0);
    this->Data->Update();
    }  

  if (this->Data == NULL || this->Data->GetNumberOfCells() == 0)
    {
    emptyFlag = 1;
    bounds[0] = bounds[1] = bounds[2] = 0.0;
    bounds[3] = bounds[4] = bounds[5] = 0.0;
    }
  else
    {
    this->Data->GetBounds(bounds);
    emptyFlag = 0;
    }
  
  controller->Send(&emptyFlag, 1, 0, 993);
  controller->Send(bounds, 6, 0, 994);  
}

//----------------------------------------------------------------------------
int vtkPVData::GetNumberOfCells()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  float id, num;
  int numCells, numRemoteCells;
  
  // Just some error checking.
  if (controller->GetLocalProcessId() != 0)
    {
    vtkErrorMacro("This method should only be called from processes 0");
    return -1;
    }
  
  if (this->Data == NULL)
    {
    numCells = 0;
    }
  
  pvApp->BroadcastScript("%s TransmitNumberOfCells", this->GetTclName());
  
  if (this->Assignment == NULL)
    {
    vtkWarningMacro("Cannot update without Assignment.");
    }
  else
    {
    this->Data->SetUpdateExtent(this->Assignment->GetPiece(),
				this->Assignment->GetNumberOfPieces(),
				0);
    this->Data->Update();
    numCells = this->Data->GetNumberOfCells();
    }
  
  num = controller->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    controller->Receive((int*)(&numRemoteCells), 1, id, 994);
    numCells += numRemoteCells;
    }
  
  return numCells;
}

//----------------------------------------------------------------------------
void vtkPVData::TransmitNumberOfCells()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  int numCells;
  
  // Try to update data.
  if (this->Assignment == NULL)
    {
    vtkWarningMacro("Cannot update without Assignment.");
    }
  else
    {
    this->Data->SetUpdateExtent(this->Assignment->GetPiece(),
				this->Assignment->GetNumberOfPieces(),
				0);
    this->Data->Update();
    }

  numCells = this->Data->GetNumberOfCells();
  
  controller->Send((int*)(&numCells), 1, 0, 994);
}

//----------------------------------------------------------------------------
void vtkPVData::SetActorComposite(vtkPVActorComposite *c)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ActorComposite == c)
    {
    return;
    }
  if (c == NULL)
    {
    vtkErrorMacro("You should not be setting a NULL actor composite.");
    return;
    }
  this->Modified();
  
  // Broadcast to all satellite processes.
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetActorComposite %s", this->GetTclName(), 
			   c->GetTclName());
    }
  
  if (this->ActorComposite)
    {
    this->ActorComposite->UnRegister(this);
    this->ActorComposite = NULL;
    }
  c->Register(this);
  this->ActorComposite = c;
  this->ActorComposite->SetPVData(this);
  
  // Try to keep all internal relationships consistent.
  if (this->Assignment)
    {
    this->ActorComposite->SetAssignment(this->Assignment);
    }
}

//----------------------------------------------------------------------------
void vtkPVData::SetScalarBar(vtkPVScalarBar *sb)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ScalarBar == sb)
    {
    return;
    }
  if (sb == NULL)
    {
    vtkErrorMacro("You should not be setting a NULL scalar bar.");
    return;
    }
  this->Modified();
  
  // Broadcast to all satellite processes.
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetScalarBar %s", this->GetTclName(), 
			                     sb->GetTclName());
    }
  
  if (this->ScalarBar)
    {
    this->ScalarBar->UnRegister(this);
    this->ScalarBar = NULL;
    }
  sb->Register(this);
  this->ScalarBar = sb;
  this->ScalarBar->SetPVData(this);
}

//----------------------------------------------------------------------------
vtkPVActorComposite* vtkPVData::GetActorComposite()
{
  return this->ActorComposite;
}

vtkPVScalarBar *vtkPVData::GetScalarBar()
{
  return this->ScalarBar;
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVData::GetPVApplication()
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
// MAYBE WE SHOULD NOT REFERENCE COUNT HERE BECAUSE NO ONE BUT THE 
// SOURCE WIDGET WILL REFERENCE THE DATA WIDGET.
void vtkPVData::SetPVSource(vtkPVSource *source)
{
  if (this->PVSource == source)
    {
    return;
    }
  this->Modified();

  if (this->PVSource)
    {
    vtkPVSource *tmp = this->PVSource;
    this->PVSource = NULL;
    tmp->UnRegister(this);
    }
  if (source)
    {
    this->PVSource = source;
    source->Register(this);
    }
}


//----------------------------------------------------------------------------
void vtkPVData::SetAssignment(vtkPVAssignment *a)
{
  if (this->Assignment == a)
    {
    return;
    }

  vtkPVApplication *pvApp = this->GetPVApplication();
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetAssignment %s", this->GetTclName(), 
			   a->GetTclName());
    }
  
  this->ActorComposite->SetAssignment(a);
  
  if (this->Assignment)
    {
    this->Assignment->UnRegister(NULL);
    this->Assignment = NULL;
    }

  if (a)
    {
    this->Assignment = a;
    a->Register(this);
    }
}

  
//----------------------------------------------------------------------------
void vtkPVData::Update()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int myId = pvApp->GetController()->GetLocalProcessId();
  
  if (myId == 0)
    {
    pvApp->BroadcastScript("%s Update", this->GetTclName());
    }
  
  if (this->Data == NULL)
    {
    vtkErrorMacro("No data object to update.");
    }

  if (this->Assignment)
    {
    this->Data->SetUpdateExtent(this->Assignment->GetPiece(),
                                this->Assignment->GetNumberOfPieces(),
				0);
    }
  
  this->Data->Update();
}


//----------------------------------------------------------------------------
void vtkPVData::AddPVSourceToUsers(vtkPVSource *s)
{
  this->PVSourceCollection->AddItem(s);
}

//----------------------------------------------------------------------------
void vtkPVData::RemovePVSourceFromUsers(vtkPVSource *s)
{
  this->PVSourceCollection->RemoveItem(s);
}




