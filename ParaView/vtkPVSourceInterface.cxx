/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSourceInterface.cxx
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

#include "vtkPVSourceInterface.h"

int vtkPVSourceInterfaceCommand(ClientData cd, Tcl_Interp *interp,
			        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSourceInterface::vtkPVSourceInterface()
{
  this->InstanceCount = 1;
  this->SourceClassName = NULL;
  this->RootName = NULL;
  this->InputClassName = NULL;
  this->OutputClassName = NULL;

  this->MethodInterfaces = vtkCollection::New();

  this->CommandFunction = vtkPVSourceInterfaceCommand;

  this->PVWindow = NULL;
}

//----------------------------------------------------------------------------
vtkPVSourceInterface::~vtkPVSourceInterface()
{
  this->MethodInterfaces->Delete();
  this->MethodInterfaces = NULL;
  
  this->SetSourceClassName(NULL);
  this->SetRootName(NULL);
  this->SetInputClassName(NULL);
  this->SetOutputClassName(NULL);
  
  this->PVWindow = NULL;
}

//----------------------------------------------------------------------------
vtkPVSourceInterface* vtkPVSourceInterface::New()
{
  return new vtkPVSourceInterface();
}


//----------------------------------------------------------------------------
vtkPVSource *vtkPVSourceInterface::CreateCallback()
{
  char sourceTclName[100];
  vtkSource *s;
  //vtkPVPolyDataSource *pvs;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVMethodInterface *mInt;
  
  
  // Create the vtkSource.
  sprintf(sourceTclName, "%s%d", this->RootName, this->InstanceCount);
  ++this->InstanceCount;
  // Create the object through tcl on process 0.
  s = (vtkSource *)(pvApp->MakeTclObject(this->SourceClassName, sourceTclName));
  if (s == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return NULL;
    }
  
  pvs = vtkPVSource::New();
  pvs->SetApplication(pvApp);
  pvs->SetInterface(this);
  pvs->SetVTKSource(s);
  pvs->SetVTKSourceTclName(sourceTclName);
  pvs->SetName(sourceTclName);

  // Set the input if necessary.
  if (this->InputClassName)
    {
    vtkPVData *current = this->PVWindow->GetCurrentPVData();
    pvs->SetNthPVInput(0, current);
    }
  
  // Add the new Source to the View, and make it current.
  this->PVWindow->GetMainView()->AddComposite(pvs);
  this->PVWindow->SetCurrentPVSource(pvs);

  // Loop through the methods creating widgets.
  this->MethodInterfaces->InitTraversal();
  while ( (mInt = ((vtkPVMethodInterface*)(this->MethodInterfaces->GetNextItemAsObject()))) )
    {
    if (mInt->GetNumberOfArguments() == 1)
      {
      pvs->AddLabeledEntry(mInt->GetVariableName(), 
			   mInt->GetSetCommand(),
			   mInt->GetGetCommand());
      }
    else if (mInt->GetNumberOfArguments() == 2)
      {
      pvs->AddVector2Entry(mInt->GetVariableName(), "", "", 
			   mInt->GetSetCommand(),
			   mInt->GetGetCommand());
      }
    else if (mInt->GetNumberOfArguments() == 3)
      {
      pvs->AddVector3Entry(mInt->GetVariableName(), "", "", "",
			   mInt->GetSetCommand(),
			   mInt->GetGetCommand());
      }
    else
      {
      vtkErrorMacro("I do not handle this type yet.");
      }
    }
  pvs->UpdateParameterWidgets();
  
  pvs->Delete();

  return pvs;
} 

//----------------------------------------------------------------------------
void vtkPVSourceInterface::AddMethodInterface(vtkPVMethodInterface *mInt)
{
  this->MethodInterfaces->AddItem(mInt);
}


//----------------------------------------------------------------------------
vtkPVApplication *vtkPVSourceInterface::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->GetApplication());
}

//----------------------------------------------------------------------------
int vtkPVSourceInterface::GetIsValidInput(vtkPVData *pvd)
{
  const char *pvName;
  const char *dataName;
  
  if (this->InputClassName == NULL)
    {
    return 0;
    }
  
  pvName = pvd->GetClassName();
  dataName = this->InputClassName;
  
  // Strip off the "vtkPV" and the "vtk" to compare.
  if (strcmp(pvName+5, dataName+3) == 0)
    {
    return 1;
    }
  return 0;
}


