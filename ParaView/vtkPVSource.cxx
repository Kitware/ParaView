/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSource.cxx
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

#include "vtkPVSource.h"
#include "vtkPVApplication.h"
#include "vtkPVComposite.h"
#include "vtkKWView.h"
#include "vtkKWRenderView.h"

int vtkPVSourceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSource::vtkPVSource()
{
  this->CommandFunction = vtkPVSourceCommand;
  
  this->Composite = NULL;
  this->Input = NULL;
  this->Output = NULL;
}

//----------------------------------------------------------------------------
vtkPVSource::~vtkPVSource()
{
  this->SetComposite(NULL);
  if (this->Output)
    {
    this->Output->UnRegister(this);
    this->Output = NULL;
    }
  
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
}

//----------------------------------------------------------------------------
vtkPVSource* vtkPVSource::New()
{
  return new vtkPVSource();
}

//----------------------------------------------------------------------------
void vtkPVSource::Clone(vtkPVApplication *pvApp)
{
  if (this->Application)
    {
    vtkErrorMacro("Application has already been set.");
    }
  this->SetApplication(pvApp);

  // Clone this object on every other process.
  pvApp->BroadcastScript("%s %s", this->GetClassName(), this->GetTclName());
}


//----------------------------------------------------------------------------
void vtkPVSource::SetComposite(vtkPVComposite *comp)
{
  if (this->Composite == comp)
    {
    return;
    }
  this->Modified();

  if (this->Composite)
    {
    vtkPVComposite *tmp = this->Composite;
    this->Composite = NULL;
    tmp->UnRegister(this);
    }
  if (comp)
    {
    this->Composite = comp;
    comp->Register(this);
    }
}

//----------------------------------------------------------------------------
void vtkPVSource::SetPVData(vtkPVData *data)
{
  if (this->Output == data)
    {
    return;
    }
  this->Modified();

  if (this->Output)
    {
    // extra careful for circular references
    vtkPVData *tmp = this->Output;
    this->Output = NULL;
    // Manage double pointer.
    tmp->SetPVSource(NULL);
    tmp->UnRegister(this);
    }
  if (data)
    {
    this->Output = data;
    data->Register(this);
    // Manage double pointer.
    data->SetPVSource(this);
    }
}
  
//----------------------------------------------------------------------------
// Data must be set first.  This is OK, because Source will merge with PVComposite ...
void vtkPVSource::SetAssignment(vtkPVAssignment *a)
{
  if (this->Output == NULL)
    {
    vtkErrorMacro("Cannot make assignment.  Output has not been created.");
    return;
    }
  this->Output->SetAssignment(a); 
}


//----------------------------------------------------------------------------
int vtkPVSource::Create(char *args)
{
  if (this->Application == NULL)
    {
    vtkErrorMacro("This object has not been cloned yet.");
    return 0;
    }
  
  // create the top level
  this->Script("frame %s %s", this->GetWidgetName(), args);
  
  return 1;
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVSource::GetPVApplication()
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
