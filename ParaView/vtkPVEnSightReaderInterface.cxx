/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVEnSightReaderInterface.cxx
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

#include "vtkPVEnSightReaderInterface.h"
#include "vtkGenericEnSightReader.h"

int vtkPVEnSightReaderInterfaceCommand(ClientData cd, Tcl_Interp *interp,
				       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVEnSightReaderInterface::vtkPVEnSightReaderInterface()
{
  this->CommandFunction = vtkPVEnSightReaderInterfaceCommand;
}

//----------------------------------------------------------------------------
vtkPVEnSightReaderInterface* vtkPVEnSightReaderInterface::New()
{
  return new vtkPVEnSightReaderInterface();
}

//----------------------------------------------------------------------------
vtkPVSource *vtkPVEnSightReaderInterface::CreateCallback()
{
  char tclName[100], outputTclName[100], srcTclName[100];
  vtkDataSet *d;
  vtkPVData *pvd;
  vtkGenericEnSightReader *reader;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  int numOutputs, i;
  
  // Create the vtkSource.
  sprintf(tclName, "%s%d", this->RootName, this->InstanceCount);
  // Create the object through tcl on all processes.
  reader = (vtkGenericEnSightReader *)
    (pvApp->MakeTclObject(this->SourceClassName, tclName));
  if (reader == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return NULL;
    }
  pvApp->Script("%s SetCaseFileName [tk_getOpenFile]", tclName);

  if (strcmp(reader->GetCaseFileName(), "") == 0)
    {
    pvApp->BroadcastScript("%s Delete", tclName);
    return NULL;
    }
    
  pvApp->BroadcastScript("%s SetCaseFileName %s", tclName,
			 reader->GetCaseFileName());
  reader->Update();
  
  numOutputs = reader->GetNumberOfOutputs();
  
  for (i = 0; i < numOutputs; i++)
    {
    sprintf(outputTclName, "%sOutput%d", tclName, i);
    d = (vtkDataSet*)(pvApp->MakeTclObject(reader->GetOutput(i)->GetClassName(),
					   outputTclName));
    pvApp->BroadcastScript("%s ShallowCopy [%s GetOutput %d]",
			   outputTclName, tclName, i);
    pvd = vtkPVData::New();
    pvd->SetApplication(pvApp);
    pvd->SetVTKData(d, outputTclName);
    pvs = vtkPVSource::New();
    sprintf(srcTclName, "%s_%d", tclName, i+1);
    pvs->SetName(srcTclName);
    pvs->SetNthPVOutput(0, pvd);

    this->PVWindow->GetMainView()->AddComposite(pvs);
    this->PVWindow->SetCurrentPVSource(pvs);

    pvs->AcceptCallback();
    
    pvs->Delete();
    pvd->Delete();
    }

  pvApp->BroadcastScript("%s Delete", tclName);
  
  ++this->InstanceCount;
  return pvs;
} 
