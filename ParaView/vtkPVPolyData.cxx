/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyData.cxx
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
#include "vtkPVShrinkPolyData.h"
#include "vtkPVGetRemoteGhostCells.h"
#include "vtkPVConeSource.h"
#include "vtkPVGlyph3D.h"
#include "vtkPVPolyDataNormals.h"
#include "vtkPVTubeFilter.h"
#include "vtkPVParallelDecimate.h"
#include "vtkKWView.h"
#include "vtkKWEventNotifier.h"

#include "vtkKWScale.h"
#include "vtkKWPushButton.h"
#include "vtkKWEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVAssignment.h"

#include "vtkPVApplication.h"
#include "vtkPVMenuButton.h"
#include "vtkDataSetMapper.h"
#include "vtkParallelDecimate.h"
#include "vtkPVActorComposite.h"

int vtkPVPolyDataCommand(ClientData cd, Tcl_Interp *interp,
		                     int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVPolyData::vtkPVPolyData()
{
  this->CommandFunction = vtkPVPolyDataCommand;
  
  this->DecimateButton = vtkKWCheckButton::New();
  this->LocalRepresentation = NULL;
}

//----------------------------------------------------------------------------
vtkPVPolyData::~vtkPVPolyData()
{
  this->DecimateButton->Delete();
  this->DecimateButton = NULL;
  
  this->SetLocalRepresentation(NULL);
}

//----------------------------------------------------------------------------
vtkPVPolyData* vtkPVPolyData::New()
{
  return new vtkPVPolyData();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::Shrink()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  // shrink factor defaults to 0.5, which seems reasonable
  vtkPVShrinkPolyData *shrink;
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  shrink = vtkPVShrinkPolyData::New();
  shrink->Clone(pvApp);
  
  shrink->SetInput(this);
  
  shrink->SetName("shrink");

  this->GetPVSource()->GetView()->AddComposite(shrink);

  // The window here should probably be replaced with the view.
  window->SetCurrentSource(shrink);
  window->GetSourceList()->Update();
  
  shrink->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::TubeFilter()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVTubeFilter *tube;
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  tube = vtkPVTubeFilter::New();
  tube->Clone(pvApp);
  
  tube->SetInput(this);
  
  tube->SetName("tube");
  
  this->GetPVSource()->GetView()->AddComposite(tube);
  
  window->SetCurrentSource(tube);
  window->GetSourceList()->Update();
  
  tube->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::PolyDataNormals()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPolyDataNormals *normal;
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  normal = vtkPVPolyDataNormals::New();
  normal->Clone(pvApp);
  
  normal->SetInput(this);
  
  normal->SetName("normal");

  this->GetPVSource()->GetView()->AddComposite(normal);

  window->SetCurrentSource(normal);
  window->GetSourceList()->Update();
  
  normal->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::Glyph()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVGlyph3D *glyph;
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  glyph = vtkPVGlyph3D::New();
  glyph->Clone(pvApp);
  
  glyph->SetInput(this);
  glyph->SetScaleModeToDataScalingOff();
  
  glyph->SetName("glyph");
 
  this->GetPVSource()->GetView()->AddComposite(glyph);
  
  window->SetCurrentSource(glyph);
  window->GetSourceList()->Update();

  glyph->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::GetGhostCells()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVGetRemoteGhostCells *rgc;
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  rgc = vtkPVGetRemoteGhostCells::New();
  rgc->Clone(pvApp);
  
  rgc->SetInput(this);
  
  rgc->SetName("get ghost cells");
  
  this->GetPVSource()->GetView()->AddComposite(rgc);
  
  window->SetCurrentSource(rgc);
  window->GetSourceList()->Update();

  rgc->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::ParallelDecimate()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVParallelDecimate *paraDeci;
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  paraDeci = vtkPVParallelDecimate::New();
  paraDeci->Clone(pvApp);
  
  paraDeci->SetInput(this);
  
  paraDeci->SetName("parallel decimate");
  
  this->GetPVSource()->GetView()->AddComposite(paraDeci);
  
  window->SetCurrentSource(paraDeci);
  window->GetSourceList()->Update();
  
  paraDeci->Delete();
}

//----------------------------------------------------------------------------
int vtkPVPolyData::Create(char *args)
{
  if (this->vtkPVData::Create(args) == 0)
    {
    return 0;
    }
  
  this->FiltersMenuButton->AddCommand("vtkShrinkPolyData", this,
				      "Shrink");
  this->FiltersMenuButton->AddCommand("vtkGlyph3D", this,
				      "Glyph");
  this->FiltersMenuButton->AddCommand("vtkGetRemoteGhostCells", this,
				      "GetGhostCells");
  this->FiltersMenuButton->AddCommand("vtkPolyDataNormals", this,
				      "PolyDataNormals");
  this->FiltersMenuButton->AddCommand("vtkTubeFilter", this,
				      "TubeFilter");
  this->FiltersMenuButton->AddCommand("vtkParallelDecimate", this,
				      "ParallelDecimate");
  
  this->DecimateButton->SetParent(this);
  this->DecimateButton->Create(this->Application, "-text Decimate");
  this->DecimateButton->SetCommand(this, "Decimate");
  this->Script("pack %s", this->DecimateButton->GetWidgetName());
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVPolyData::Decimate()
{
  vtkMultiProcessController *controller;
  vtkPVApplication *pvApp = this->GetPVApplication();
  int myId;
  
  controller = pvApp->GetController();
  
  if (controller == NULL)
    {
    vtkErrorMacro("You must have a controller to do a parallel decimate.");
    return;
    }
  
  vtkPVParallelDecimate *decimate;
  vtkPVPolyData *output = vtkPVPolyData::New();
  
  output->Clone(pvApp);
  output->SetAssignment(this->GetAssignment());
  
  decimate = vtkPVParallelDecimate::New();
  decimate->Clone(pvApp);
  
  decimate->SetInput(this);
  decimate->SetOutput(output);
    
  pvApp->BroadcastScript("[%s GetPolyData] DebugOn", output->GetTclName());
  cerr << "output tcl name: " << output->GetTclName() << endl;
  
  this->SetLocalRepresentation(output);

  cerr << "LocalRepresentation: " << this->LocalRepresentation << endl;

  // we want the delay of decimation to occur when the user selects the 
  // decimate button.  (This is a parallel method.)
//  decimate->Update();
  
  output->Delete();
  decimate->Delete();
  
//  vtkMultiProcessController *controller;
//  int numProcs, myId, i;
//  vtkPolyData *polyData = vtkPolyData::New();
//  vtkCleanPolyData *clean = vtkCleanPolyData::New();
//  vtkDecimatePro *decimate = vtkDecimatePro::New();
//  vtkAppendPolyData *append = vtkAppendPolyData::New();
//  vtkPVApplication *pvApp = this->GetPVApplication();
//  vtkPolyData *pd;
  
//  controller = pvApp->GetController();
//  numProcs = controller->GetNumberOfProcesses();
//  myId = controller->GetLocalProcessId();

//  if (this->GetPVApplication()->GetController() == NULL)
//    {
//    vtkErrorMacro("You must have a controller before calling Decimate.");
//    return;
//    }
  
//  if (myId == 0)
//    {
//    if (!this->DecimateButton->GetState())
//      {
//      this->GetActorComposite()->GetMapper()->SetInput(this->GetPolyData());
//      return;
//      }
//    else
//      {
//      pvApp->BroadcastScript("%s Decimate", this->GetTclName());
//      }
//    }
  
//  pd = this->GetPolyData();

//  for (i = 0; i < ceil(log(numProcs)); i++)
//    {
//    if ((myId % (int)pow(2, i)) == 0)
//      {
//      cerr << "before: " << pd->GetNumberOfCells()
//	   << " cells" << endl;
//      clean->SetInput(pd);
//      clean->Update();
//      decimate->SetInput(clean->GetOutput());
//      decimate->SetTargetReduction(0.5);
//      decimate->BoundaryVertexDeletionOff();
//      decimate->PreserveTopologyOn();
//      decimate->SplittingOff();
//      decimate->Update();
//      pd = decimate->GetOutput();
//      cerr << "after: " << decimate->GetOutput()->GetNumberOfCells()
//	   << " cells" << endl;
//      if ((myId % (int)pow(2, i+1)) != 0)
//	{
//	controller->Send(pd, myId - pow(2, i), 10);
//	}
//      else
//	{
//	if ((myId + pow(2, i)) < numProcs)
//	  {
//	  controller->Receive(polyData, myId + pow(2, i), 10);
//	  append->AddInput(pd);
//	  append->AddInput(polyData);
//	  append->Update();
//	  pd = append->GetOutput();
//	  cerr << "local rep. has "
//	       << pd->GetNumberOfCells()
//	       << " cells" << endl;
//	  }
//	}
//       }
//    polyData->Reset();
//    }
  
//  if (myId == 0)
//    {
//    clean->SetInput(pd);
//    clean->Update();
//    decimate->SetInput(clean->GetOutput());
//    decimate->SetTargetReduction(0.5);
//    decimate->BoundaryVertexDeletionOff();
//    decimate->PreserveTopologyOn();
//    decimate->SplittingOff();
//    decimate->Update();
    // Should we break the pipeline or create a special parallel decimate
    // filter out of this method?
//    decimate->GetOutput()->SetMaximumNumberOfPieces(1000);

//    this->SetLocalRepresentation(decimate->GetOutput());
//    cerr << "input to mapper has " << decimate->GetOutput()->GetNumberOfCells()
//	 << " cells" << endl;
//    }
  
//  polyData->Delete();
//  polyData = NULL;
//  clean->Delete();
//  clean = NULL;
//  decimate->Delete();
//  decimate = NULL;
//  append->Delete();
//  append = NULL;
}

//----------------------------------------------------------------------------
void vtkPVPolyData::SetData(vtkDataSet *data)
{
  vtkPolyData *polyData = vtkPolyData::SafeDownCast(data);
  
  if (data != NULL && polyData == NULL)
    {
    vtkErrorMacro("Expecting a polydata object");
    return;
    }
  
  this->vtkPVData::SetData(polyData);

  if (this->ActorComposite)
    {
    this->ActorComposite->SetApplication(this->Application);
    this->ActorComposite->SetInput(polyData);
    }
}


//----------------------------------------------------------------------------
void vtkPVPolyData::SetLocalRepresentation(vtkPVPolyData *data)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int myId;
  
  if (this->LocalRepresentation == data)
    {
    return;
    }
  
  myId = pvApp->GetController()->GetLocalProcessId();
  
  cerr << "Setting Local Representation to " << data << endl;
  
  if (myId == 0)
    {
    pvApp->BroadcastScript("%s SetLocalRepresentation %s",
			   this->GetTclName(), data->GetTclName());
    
    // Manage callbacks.
    if (this->LocalRepresentation && data == NULL)
      {
      this->Application->GetEventNotifier()->
	RemoveCallback("InteractiveRenderStart",
		       this->GetPVSource()->GetWindow(), this,
		       "InteractiveOnCallback" );
      this->Application->GetEventNotifier()->
	RemoveCallback("InteractiveRenderEnd",
		       this->GetPVSource()->GetWindow(),
		       this, "InteractiveOffCallback" );
      }
    if (this->LocalRepresentation == NULL && data != NULL)
      {
      cerr << "Adding Notifier callbacks\n";
      this->Application->GetEventNotifier()->
	AddCallback("InteractiveRenderStart", this->GetPVSource()->GetWindow(),
		    this, "InteractiveOnCallback" );
      this->Application->GetEventNotifier()->
	AddCallback("InteractiveRenderEnd", this->GetPVSource()->GetWindow(), 
		    this, "InteractiveOffCallback" );
      }
    }
  
  if (this->LocalRepresentation)
    {
    this->LocalRepresentation->UnRegister(this);
    this->LocalRepresentation = NULL;
    }
  if (data)
    {
    this->LocalRepresentation = data;
    data->Register(this);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::InteractiveOnCallback()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int myId = pvApp->GetController()->GetLocalProcessId();
  
  cerr << "OnCallback, this->LocalRepresentation: " << this->LocalRepresentation << endl;
  
  if (myId == 0)
    {
    pvApp->BroadcastScript("%s InteractiveOnCallback", this->GetTclName());
    }
  
  if (this->LocalRepresentation)
    {
    cerr << "LocalRepresentation tcl name: " << this->LocalRepresentation->GetTclName() << endl;
    this->GetActorComposite()->GetMapper()->
      SetInput(this->LocalRepresentation->GetPolyData());
    cerr << myId << " : Setting mapper input to " 
	 << this->LocalRepresentation->GetPolyData() << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVPolyData::InteractiveOffCallback()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int myId = pvApp->GetController()->GetLocalProcessId();
  
  if (myId == 0)
    {
    pvApp->BroadcastScript("%s InteractiveOffCallback", this->GetTclName());
    }
  
  this->GetActorComposite()->GetMapper()->SetInput(this->GetPolyData());
}

//----------------------------------------------------------------------------
vtkPolyData *vtkPVPolyData::GetPolyData()
{
  return (vtkPolyData*)this->Data;
}

