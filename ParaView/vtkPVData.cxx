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
#include "vtkPVContourFilter.h"
#include "vtkPVElevationFilter.h"
#include "vtkPVColorByProcess.h"
#include "vtkPVCutter.h"
#include "vtkPVAssignment.h"
#include "vtkPVApplication.h"
#include "vtkPVActorComposite.h"
#include "vtkPVMenuButton.h"

int vtkPVDataCommand(ClientData cd, Tcl_Interp *interp,
		     int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVData::vtkPVData()
{
  this->CommandFunction = vtkPVDataCommand;

  this->Data = NULL;
  this->PVSource = NULL;
  
  this->FiltersMenuButton = vtkPVMenuButton::New();
  
  this->Assignment = NULL;

  this->ActorCompositeButton = vtkKWPushButton::New();
  
  // This is initialized in "Clone();"
  this->ActorComposite = NULL;
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

  this->Update();
  
  this->FiltersMenuButton->SetParent(this);
  this->FiltersMenuButton->Create(this->Application, "");
  this->FiltersMenuButton->SetButtonText("Filters");
  this->FiltersMenuButton->AddCommand("vtkContourFilter", this,
				      "Contour");
  if (this->Data->GetPointData()->GetScalars() == NULL)
    {
    this->Script("%s entryconfigure 3 -state disabled",
		 this->FiltersMenuButton->GetMenu()->GetWidgetName());
    }
  else
    {
    this->Script("%s entryconfigure 3 -state normal",
		 this->FiltersMenuButton->GetMenu()->GetWidgetName());
    }
  
  this->FiltersMenuButton->AddCommand("vtkElevationFilter", this,
				      "Elevation");
  this->FiltersMenuButton->AddCommand("vtkColorByProcess", this,
				      "ColorByProcess");
  this->FiltersMenuButton->AddCommand("vtkCutter", this, "Cutter");
  
  this->Script("pack %s", this->FiltersMenuButton->GetWidgetName());
  
  this->ActorCompositeButton->SetParent(this);
  this->ActorCompositeButton->Create(this->Application, "");
  this->ActorCompositeButton->SetLabel("Get Actor Composite");
  this->ActorCompositeButton->SetCommand(this, "ShowActorComposite");
  this->Script("pack %s", this->ActorCompositeButton->GetWidgetName());

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVData::ShowActorComposite()
{
  this->GetActorComposite()->ShowProperties();
}

//----------------------------------------------------------------------------
void vtkPVData::Contour()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVContourFilter *contour;
  float *range;
  
  contour = vtkPVContourFilter::New();
  contour->Clone(pvApp);
  
  contour->SetInput(this);
  
  this->Update();
  // This should be replaced by a parallel version.
  range = this->Data->GetScalarRange();
  contour->SetValue(0, (range[1]+range[0])/2.0);
      
  contour->SetName("contour");

  vtkPVWindow *window = vtkPVWindow::SafeDownCast(
    this->GetPVSource()->GetView()->GetParentWindow());
  this->GetPVSource()->GetView()->AddComposite(contour);
  
  window->SetCurrentSource(contour);
  
  contour->Delete();
}

//----------------------------------------------------------------------------
void vtkPVData::Cutter()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVCutter *cutter = vtkPVCutter::New();
  
  cutter->Clone(pvApp);
  cutter->SetInput(this);
  
  cutter->SetCutPlane(0, 0, 0, 0, 0, 1);

  this->GetPVSource()->GetView()->AddComposite(cutter);
  cutter->SetName("cutter");
  
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  window->SetCurrentSource(cutter);
  
  cutter->Delete();
}
  
//----------------------------------------------------------------------------
void vtkPVData::Elevation()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVElevationFilter *elevation;
  float bounds[6];

  // This should go through the PVData who will collect the info.
  this->GetBounds(bounds);
  
  elevation = vtkPVElevationFilter::New();
  elevation->Clone(pvApp);
  
  elevation->SetInput(this);
  
  this->GetPVSource()->GetView()->AddComposite(elevation);
  elevation->SetName("elevation");
  
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  elevation->SetLowPoint(bounds[0], 0.0, 0.0);
  elevation->SetHighPoint(bounds[1], 0.0, 0.0);

  window->SetCurrentSource(elevation);
  
  elevation->Delete();
}

//----------------------------------------------------------------------------
void vtkPVData::ColorByProcess()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVColorByProcess *pvFilter;
  
  pvFilter = vtkPVColorByProcess::New();
  pvFilter->Clone(pvApp);
  
  pvFilter->SetInput(this);
  
  this->GetPVSource()->GetView()->AddComposite(pvFilter);
  pvFilter->SetName("color by process");
  
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  window->SetCurrentSource(pvFilter);
  
  pvFilter->Delete();
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
vtkPVActorComposite* vtkPVData::GetActorComposite()
{
  return this->ActorComposite;
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






