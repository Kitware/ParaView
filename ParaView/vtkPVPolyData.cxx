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
    
  this->SetLocalRepresentation(output);

  output->Delete();
  decimate->Delete();
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
  
  if (myId == 0)
    {
    pvApp->BroadcastScript("%s InteractiveOnCallback", this->GetTclName());
    }
  
  if (this->LocalRepresentation)
    {
    this->GetActorComposite()->GetMapper()->
      SetInput(this->LocalRepresentation->GetPolyData());
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

