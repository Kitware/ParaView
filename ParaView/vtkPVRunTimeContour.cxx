/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVRunTimeContour.cxx
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

#include "vtkPVRunTimeContour.h"
#include "vtkPVCommandList.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"
#include "vtkPVScalarBar.h"
#include "vtkPVPolyData.h"
#include "vtkPVImageData.h"
#include "vtkPVAssignment.h"

int vtkPVRunTimeContourCommand(ClientData cd, Tcl_Interp *interp,
			       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVRunTimeContour::vtkPVRunTimeContour()
{
  this->CommandFunction = vtkPVRunTimeContourCommand;
  this->SetNumberOfPVOutputs(4);
}

//----------------------------------------------------------------------------
vtkPVRunTimeContour* vtkPVRunTimeContour::New()
{
  return new vtkPVRunTimeContour();
}

//----------------------------------------------------------------------------
void vtkPVRunTimeContour::AcceptCallback()
{
  int i;
  vtkPVWindow *window;
  vtkPVData *input;
  vtkPVActorComposite *ac;
  vtkPVScalarBar *sb;

  window = this->GetWindow();

  if (this->GetVTKSource())
    {
    ((vtkRunTimeContour*)this->GetVTKSource())->UpdateWidgets();
  
    // Call the commands to set ivars from widget values.
    for (i = 0; i < this->AcceptCommands->GetLength(); ++i)
      {
      this->Script(this->AcceptCommands->GetCommand(i));
      }  
    
    // Initialize the outputs if necessary.
    for (i = 0; i < this->GetNumberOfPVOutputs(); i++)
      {
      if (this->GetNthPVOutput(i) == NULL && this->GetVTKSource())
	{ // This is the first time, initialize data.
	input = this->GetNthPVInput(0);
	
	// this probably needs to know which output we're initializing
	if (i == 0)
	  {
	  this->InitializePVOutput(i);
	  }
	else
	  {
	  this->InitializePVImageOutput(i);
	  }
	
	this->CreateDataPage(i);
	ac = this->GetNthPVOutput(i)->GetActorComposite();
	window->GetMainView()->AddComposite(ac);
	sb = this->GetNthPVOutput(i)->GetPVScalarBar();
	window->GetMainView()->AddComposite(sb);
	// Make the last data invisible.
	if (input)
	  {
	  input->GetActorComposite()->SetVisibility(0);
	  input->GetPVScalarBar()->SetVisibility(0);
	  }
	window->GetMainView()->ResetCamera();
	}
      }
    
    window->GetMainView()->SetSelectedComposite(this);  
    this->UpdateNavigationCanvas();
    this->GetView()->Render();
    window->GetSourceList()->Update();
    }
}

//----------------------------------------------------------------------------
// I had to copy this from vtkPVImageSource because this filter has PVOutputs
// that are vtkPVImageData, but this filter only inherits from
// vtkPVPolyDataSource.
void vtkPVRunTimeContour::InitializePVImageOutput(int idx)
{
  vtkPVImageData *output;
  vtkPVData *input;
  vtkPVAssignment *assignment;  

  output = vtkPVImageData::New();
  output->Clone(this->GetPVApplication());
  this->SetNthPVImageOutput(idx, output);

  input = this->GetPVInput();  
  if (input != NULL)
    {
    assignment = input->GetAssignment();
    }
  else
    {
    assignment = vtkPVAssignment::New();
    assignment->Clone(this->GetPVApplication());
    assignment->SetOriginalImage((vtkPVImageData*)this->PVOutputs[idx]);
    }
  
  output->SetAssignment(assignment);
}

//----------------------------------------------------------------------------
void vtkPVRunTimeContour::SetNthPVImageOutput(int idx, vtkPVImageData *pvi)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetNthPVImageOutput %d %s", this->GetTclName(), idx,
			   pvi->GetTclName());
    }

  this->vtkPVSource::SetNthPVOutput(idx, pvi);
  vtkSource *s = this->GetVTKSource();
  vtkImageData *i = (vtkImageData*)(s->GetOutputs()[idx]);
  pvi->SetData(i);  
}
