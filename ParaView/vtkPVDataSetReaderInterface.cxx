/*=========================================================================

  Program:   
  Module:    vtkPVDataSetReaderInterface.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

    pvs->ExtractPieces();
    
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
