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
#include "vtkPVSource.h"
#include "vtkKWView.h"
#include "vtkPVWindow.h"
#include "vtkPVApplication.h"
#include "vtkPVActorComposite.h"
#include "vtkKWMenuButton.h"


int vtkPVDataCommand(ClientData cd, Tcl_Interp *interp,
		     int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVData::vtkPVData()
{
  this->CommandFunction = vtkPVDataCommand;

  this->VTKData = NULL;
  this->VTKDataTclName = NULL;
  this->PVSource = NULL;
  
  this->ActorCompositeButton = vtkKWPushButton::New();
  this->PVSourceCollection = vtkPVSourceCollection::New();

  // Has an initialization component in CreateParallelTclObjects.
  this->ActorComposite = vtkPVActorComposite::New();
}

//----------------------------------------------------------------------------
vtkPVData::~vtkPVData()
{
  this->SetVTKData(NULL, NULL);
  this->SetPVSource(NULL);

  if (this->ActorComposite)
    {
    this->ActorComposite->Delete();
    this->ActorComposite = NULL;
    }
  
  this->ActorCompositeButton->Delete();
  this->ActorCompositeButton = NULL;

  this->PVSourceCollection->Delete();
  this->PVSourceCollection = NULL;  
}

//----------------------------------------------------------------------------
vtkPVData* vtkPVData::New()
{
  return new vtkPVData();
}


//----------------------------------------------------------------------------
void vtkPVData::SetApplication(vtkPVApplication *pvApp)
{
  this->ActorComposite->CreateParallelTclObjects(pvApp);
  this->vtkKWWidget::SetApplication(pvApp);
}


//----------------------------------------------------------------------------
void vtkPVData::Select(vtkKWView *v)
{
  if (this->ActorComposite)
    {
    this->ActorComposite->Select(v);
    }  
}

//----------------------------------------------------------------------------
void vtkPVData::Deselect(vtkKWView *v)
{
  if (this->ActorComposite)
    {
    this->ActorComposite->Deselect(v);
    }  
}

//----------------------------------------------------------------------------
// Tcl does the reference counting, so we are not going to put an 
// additional reference of the data.
void vtkPVData::SetVTKData(vtkDataSet *data, const char *tclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp == NULL)
    {
    vtkErrorMacro("Set the application before you set the VTKDataTclName.");
    return;
    }
  
  if (this->VTKDataTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->VTKDataTclName);
    delete [] this->VTKDataTclName;
    this->VTKDataTclName = NULL;
    this->VTKData = NULL;
    }
  if (tclName)
    {
    this->VTKDataTclName = new char[strlen(tclName) + 1];
    strcpy(this->VTKDataTclName, tclName);
    this->VTKData = data;
    
    // This is why we need a special method.  The actor comoposite can't set its input until
    // the data tcl name is set.  These dependancies on the order things are set
    // leads me to think there sould be one initialize method which sets all the variables.
    
    this->ActorComposite->SetInput(this);
    }
}




//----------------------------------------------------------------------------
int vtkPVData::Create(char *args)
{
  if (this->Application == NULL)
    {
    vtkErrorMacro("Application has not been set yet.");
    return 0;
    }
  
  // create the top level
  this->Script("frame %s %s", this->GetWidgetName(), args);

  this->ActorCompositeButton->SetParent(this);
  this->ActorCompositeButton->Create(this->Application, "");
  this->ActorCompositeButton->SetLabel("Get Actor Composite");
  this->ActorCompositeButton->SetCommand(this, "ShowActorComposite");
  this->Script("pack %s", this->ActorCompositeButton->GetWidgetName());

  
  
  return 1;
}


//----------------------------------------------------------------------------
// Data is expected to be updated.
void vtkPVData::GetBounds(float bounds[6])
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  float tmp[6];
  int id, num;
  
  if (this->VTKData == NULL)
    {
    bounds[0] = bounds[1] = bounds[2] = VTK_LARGE_FLOAT;
    bounds[3] = bounds[4] = bounds[5] = -VTK_LARGE_FLOAT;
    return;
    }

  pvApp->BroadcastScript("Application SendDataBounds %s", 
			this->VTKDataTclName);
  
  this->VTKData->GetBounds(bounds);
  
  num = controller->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    controller->Receive(tmp, 6, id, 1967);
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
}

//----------------------------------------------------------------------------
// Data is expected to be updated.
int vtkPVData::GetNumberOfCells()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkMultiProcessController *controller = pvApp->GetController();
  int tmp, numCells, id, numProcs;
  
  if (this->VTKData == NULL)
    {
    return 0;
    }

  pvApp->BroadcastScript("Application SendDataNumberOfCells %s", 
			this->VTKDataTclName);
  
  numCells = this->VTKData->GetNumberOfCells();
  
  numProcs = controller->GetNumberOfProcesses();
  for (id = 1; id < numProcs; ++id)
    {
    controller->Receive(&tmp, 1, id, 1968);
    numCells += tmp;
    }
  return numCells;
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
void vtkPVData::AddPVSourceToUsers(vtkPVSource *s)
{
  this->PVSourceCollection->AddItem(s);
}

//----------------------------------------------------------------------------
void vtkPVData::RemovePVSourceFromUsers(vtkPVSource *s)
{
  this->PVSourceCollection->RemoveItem(s);
}




