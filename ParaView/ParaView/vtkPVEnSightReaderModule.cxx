/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnSightReaderModule.cxx
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
#include "vtkPVEnSightReaderModule.h"
#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkGenericEnSightReader.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVWindow.h"
#include "vtkPVRenderView.h"
#include "vtkPVSourceCollection.h"

#include <ctype.h>

int vtkPVEnSightReaderModuleCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVEnSightReaderModule::vtkPVEnSightReaderModule()
{
  this->CommandFunction = vtkPVEnSightReaderModuleCommand;
}

//----------------------------------------------------------------------------
vtkPVEnSightReaderModule::~vtkPVEnSightReaderModule()
{
}

//----------------------------------------------------------------------------
vtkPVEnSightReaderModule* vtkPVEnSightReaderModule::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVEnSightReaderModule");
  if(ret)
    {
    return (vtkPVEnSightReaderModule*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVEnSightReaderModule;
}


//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::CreateProperties()
{
  this->Superclass::CreateProperties();
}

void vtkPVEnSightReaderModule::InitializePrototype()
{
  this->Superclass::InitializePrototype();
}

int vtkPVEnSightReaderModule::ReadFile(const char* fname, 
				       vtkPVReaderModule*& clone)
{
  clone = 0;

  // Copied verbatim from the old vtkPVEnSightReaderInterface class.
  char *tclName, *outputTclName, *srcTclName, *tmp;
  vtkDataSet *d;
  vtkPVData *pvd = 0;
  vtkGenericEnSightReader *reader;
  vtkPVSource *pvs = 0;
  vtkPVApplication *pvApp = this->GetPVApplication();
  int numOutputs, i;
  char *fullPath;
  char *extension;
  int position;
  char *endingSlash = NULL;
  char *newTclName;
  char *result;
  
  
  extension = strrchr(fname, '.');
  position = extension - fname;
  tclName = new char[position+1];
  strncpy(tclName, fname, position);
  tclName[position] = '\0';
  
  if ((endingSlash = strrchr(tclName, '/')))
    {
    position = endingSlash - tclName + 1;
    newTclName = new char[strlen(tclName) - position + 1];
    strcpy(newTclName, tclName + position);
    delete [] tclName;
    tclName = new char[strlen(newTclName)+1];
    strcpy(tclName, newTclName);
    delete [] newTclName;
    }
  if (isdigit(tclName[0]))
    {
    // A VTK object names beginning with a digit is invalid.
    newTclName = new char[strlen(tclName) + 3];
    sprintf(newTclName, "PV%s", tclName);
    tclName = new char[strlen(newTclName)+1];
    strcpy(tclName, newTclName);
    delete [] newTclName;
    }
  
  tmp = new char[strlen(tclName)+1 + (this->PrototypeInstanceCount%10)+1];
  sprintf(tmp, "%s%d", tclName, this->PrototypeInstanceCount);
  delete [] tclName;
  tclName = new char[strlen(tmp)+1];
  strcpy(tclName, tmp);
  delete [] tmp;
  
  // Create the object through tcl on all processes.
  reader = (vtkGenericEnSightReader *)
    (pvApp->MakeTclObject(this->SourceClassName, tclName));
  if (reader == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return VTK_ERROR;
    }
  pvApp->Script("%s SetCaseFileName \"%s\"", tclName, fname);

  if (strcmp(reader->GetCaseFileName(), "") == 0)
    {
    pvApp->BroadcastScript("%s Delete", tclName);
    return VTK_ERROR;
    }

  pvApp->BroadcastScript("%s SetFilePath \"%s\"", tclName,
			 reader->GetFilePath());
  pvApp->BroadcastScript("%s SetCaseFileName \"%s\"", tclName,
			 reader->GetCaseFileName());
  
  fullPath = new char[strlen(reader->GetFilePath()) +
                     strlen(reader->GetCaseFileName()) + 1];
  sprintf(fullPath, "%s%s", reader->GetFilePath(), reader->GetCaseFileName());
  delete [] fullPath;
  
  pvApp->BroadcastScript("%s Update", tclName);
  
  numOutputs = reader->GetNumberOfOutputs();
  
  for (i = 0; i < numOutputs; i++)
    {
    outputTclName = new char[strlen(tclName)+7 + (i%10)+1];
    sprintf(outputTclName, "%sOutput%d", tclName, i);
    d = (vtkDataSet*)(pvApp->MakeTclObject(reader->GetOutput(i)->GetClassName(),
					   outputTclName));
    pvApp->BroadcastScript("%s ShallowCopy [%s GetOutput %d]",
			   outputTclName, tclName, i);
    if (d->IsA("vtkStructuredGrid"))
      {
      pvApp->BroadcastScript(
        "%s SetPointVisibility [[%s GetOutput %d] GetPointVisibility]",
        outputTclName, tclName, i);
      }
    pvd = vtkPVData::New();
    pvd->SetPVApplication(pvApp);
    pvd->SetVTKData(d, outputTclName);

    pvs = vtkPVSource::New();
    pvs->SetParametersParent(
      this->GetPVWindow()->GetMainView()->GetPropertiesParent());
    pvs->SetApplication(pvApp);

    pvs->SetView(this->GetPVWindow()->GetMainView());
    pvs->CreateProperties();

    // Now this is a bit of a hack to get tcl variables of these multiple
    // sources.  It is important that this trace entry occurs before the
    // source is set to current, because the set current source adds its
    // own trace which uses the variable.
    pvApp->AddTraceEntry("set kw(%s) [$kw(%s) GetPreviousPVSource %d]",
                         pvs->GetTclName(), this->GetPVWindow()->GetTclName(),
                         numOutputs - 1  - i);
    pvs->SetTraceInitialized(1);

    srcTclName = new char[strlen(tclName)+2 + ((i+1)%10)+1];
    sprintf(srcTclName, "%s_%d", tclName, i+1);
    pvs->SetName(srcTclName);
    pvs->SetPVOutput(pvd);

    this->GetPVWindow()->GetSourceList("Sources")->AddItem(pvs);
    pvs->Accept(0);
    this->GetPVWindow()->SetCurrentPVSource(pvs);

    pvs->Delete();
    pvd->Delete();
    delete [] outputTclName;
    delete [] srcTclName;
    }

  pvApp->BroadcastScript("%s Delete", tclName);

  delete [] tclName;
  
  this->PrototypeInstanceCount++;
  return VTK_OK;
}

