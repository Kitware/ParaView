/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGetRemoteGhostCells.cxx
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

#include "vtkPVGetRemoteGhostCells.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVPolyData.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"

int vtkPVGetRemoteGhostCellsCommand(ClientData cd, Tcl_Interp *interp,
				    int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVGetRemoteGhostCells::vtkPVGetRemoteGhostCells()
{
  this->CommandFunction = vtkPVGetRemoteGhostCellsCommand;
  
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);
  this->SourceButton = vtkKWPushButton::New();
  this->SourceButton->SetParent(this->Properties);
  this->GhostLevelEntry = vtkKWLabeledEntry::New();
  this->GhostLevelEntry->SetParent(this->Properties);

  vtkGetRemoteGhostCells *rgc = vtkGetRemoteGhostCells::New();
  this->SetPolyDataSource(rgc);
  rgc->Delete();
}

//----------------------------------------------------------------------------
vtkPVGetRemoteGhostCells::~vtkPVGetRemoteGhostCells()
{ 
  this->Accept->Delete();
  this->Accept = NULL;
  this->SourceButton->Delete();
  this->SourceButton = NULL;
  this->GhostLevelEntry->Delete();
  this->GhostLevelEntry = NULL;
}

//----------------------------------------------------------------------------
vtkPVGetRemoteGhostCells* vtkPVGetRemoteGhostCells::New()
{
  return new vtkPVGetRemoteGhostCells();
}

//----------------------------------------------------------------------------
void vtkPVGetRemoteGhostCells::CreateProperties()
{  
  // must set the application
  this->vtkPVSource::CreateProperties();
  
  this->GhostLevelEntry->Create(this->Application);
  this->GhostLevelEntry->SetLabel("Ghost Level: ");
  this->GhostLevelEntry->SetValue(this->GetRemoteGhostCells()->GetOutput()->
				  GetUpdateGhostLevel());
  
  this->SourceButton->Create(this->Application, "-text GetSource");
  this->SourceButton->SetCommand(this, "SelectInputSource");
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "GhostLevelChanged");
  this->Script("pack %s %s %s",
	       this->SourceButton->GetWidgetName(),
	       this->Accept->GetWidgetName(),
	       this->GhostLevelEntry->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVGetRemoteGhostCells::GhostLevelChanged()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVWindow *window = this->GetWindow();
  
  if (this->GetPVData() == NULL)
    { // This is the first time. Create the data.
    this->InitializeData();
    }
  
  // This line needs to be after data is initialized because we need to set
  // the ghost level of the poly data mapper, which isn't created until
  // the data is initialized.
  this->SetGhostLevel(this->GhostLevelEntry->GetValueAsInt());
  
  window->GetMainView()->SetSelectedComposite(this);
  
  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVGetRemoteGhostCells::SetGhostLevel(int level)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetGhostLevel %d", this->GetTclName(), 
			   level);
    }
  
  this->GetOutput()->GetActorComposite()->GetMapper()->SetGhostLevel(level);
}

//----------------------------------------------------------------------------
void vtkPVGetRemoteGhostCells::SetApplication(vtkKWApplication *app)
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);

  this->GetRemoteGhostCells()->SetController(pvApp->GetController());
  this->vtkPVPolyDataToPolyDataFilter::SetApplication(app);
}

//----------------------------------------------------------------------------
vtkGetRemoteGhostCells *vtkPVGetRemoteGhostCells::GetRemoteGhostCells()
{
  return vtkGetRemoteGhostCells::SafeDownCast(this->PolyDataSource);
}


