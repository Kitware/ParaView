/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVDataSetReaderInterface.cxx
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

#include "vtkPVDataSetReaderInterface.h"
#include "vtkDataSetReader.h"
#include "vtkObjectFactory.h"
#include <ctype.h>

int vtkPVDataSetReaderInterfaceCommand(ClientData cd, Tcl_Interp *interp,
				       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVDataSetReaderInterface::vtkPVDataSetReaderInterface()
{
  this->CommandFunction = vtkPVDataSetReaderInterfaceCommand;
  this->FileName = NULL;
}

//----------------------------------------------------------------------------
vtkPVDataSetReaderInterface* vtkPVDataSetReaderInterface::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVDataSetReaderInterface");
  if(ret)
    {
    return (vtkPVDataSetReaderInterface*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVDataSetReaderInterface;
}


//----------------------------------------------------------------------------
vtkPVSource *vtkPVDataSetReaderInterface::CreateCallback()
{
  char tclName[100], outputTclName[100];
  vtkDataSet *d;
  vtkPVData *pvd;
  vtkDataSetReader *reader;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  int numOutputs, i;
  char* result;
  char *extension;
  int position;
  char *endingSlash = NULL;
  char *newTclName;  
  
  // Create the vtkReader on all processes (through tcl).
  if (!this->GetDataFileName())
    {
    pvApp->Script("set newFileName [tk_getOpenFile -filetypes {{{VTK Data Sets} {.vtk}} {{All Files} {.*}}}]");
    result = pvApp->GetMainInterp()->result;
    if (strcmp(result, "") == 0)
      {
      return NULL;
      }
    this->SetDataFileName(result);
    }
  
  extension = strrchr(this->DataFileName, '.');
  position = extension - this->DataFileName;
  strncpy(tclName, this->DataFileName, position);
  tclName[position] = '\0';
  
  if ((endingSlash = strrchr(tclName, '/')))
    {
    position = endingSlash - tclName + 1;
    newTclName = new char[strlen(tclName) - position + 1];
    strcpy(newTclName, tclName + position);
    strcpy(tclName, "");
    strcat(tclName, newTclName);
    delete [] newTclName;
    }
  if (isdigit(tclName[0]))
    {
    // A VTK object name beginning with a digit is invalid.
    newTclName = new char[strlen(tclName) + 3];
    sprintf(newTclName, "PV%s", tclName);
    strcpy(tclName, "");
    strcat(tclName, newTclName);
    delete [] newTclName;
    }

  sprintf(tclName, "%s%d", tclName, this->InstanceCount);
  
  reader = (vtkDataSetReader *)
              (pvApp->MakeTclObject(this->SourceClassName, tclName));
  if (reader == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return NULL;
    }

  pvApp->Script("%s SetFileName %s", tclName, this->GetDataFileName());
  
  // No file name?  Just abort.
  if (strcmp(reader->GetFileName(), "") == 0)
    {
    pvApp->BroadcastScript("%s Delete", tclName);
    return NULL;
    }
    
  // Broadcast file name to readers on all processes.
  pvApp->BroadcastScript("%s SetFileName %s", tclName,
			 reader->GetFileName());
  this->SetFileName(reader->GetFileName());
  // Let the reader create its outputs.
  pvApp->BroadcastScript("%s Update", tclName);
  
  // Create dummy source for each output.
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
    pvs->SetPropertiesParent(this->PVWindow->GetMainView()->GetPropertiesParent());
    pvs->SetApplication(pvApp);
    pvs->SetInterface(this);

    this->PVWindow->GetMainView()->AddComposite(pvs);
    pvs->CreateProperties();
    this->PVWindow->SetCurrentPVSource(pvs);
    
    pvs->SetName(tclName);
    pvs->SetNthPVOutput(0, pvd);

    pvs->AcceptCallback();
    
    pvs->Delete();
    pvd->Delete();
    }

  // Get rid of the original reader.
  pvApp->BroadcastScript("%s Delete", tclName);
  
  ++this->InstanceCount;
  
  // so we get prompted for a file name if another data set reader is created
  this->SetDataFileName(NULL);
  
  return pvs;
} 

void vtkPVDataSetReaderInterface::SaveInTclScript(ofstream *file, const char* sourceName)
{
  *file << "\t" << sourceName << " SetFileName " << this->FileName << "\n";
}
