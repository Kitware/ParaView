/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataSetReaderModule.cxx
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
#include "vtkPVDataSetReaderModule.h"
#include "vtkObjectFactory.h"
#include "vtkPVData.h"
#include "vtkPVWindow.h"
#include "vtkPVRenderView.h"
#include "vtkPDataSetReader.h"
#include "vtkPVDataSetFileEntry.h"
#include "vtkKWFrame.h"

#include <ctype.h>

int vtkPVDataSetReaderModuleCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVDataSetReaderModule::vtkPVDataSetReaderModule()
{
  this->CommandFunction = vtkPVDataSetReaderModuleCommand;
}

//----------------------------------------------------------------------------
vtkPVDataSetReaderModule::~vtkPVDataSetReaderModule()
{
}

//----------------------------------------------------------------------------
vtkPVDataSetReaderModule* vtkPVDataSetReaderModule::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVDataSetReaderModule");
  if(ret)
    {
    return (vtkPVDataSetReaderModule*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVDataSetReaderModule;
}


//----------------------------------------------------------------------------
void vtkPVDataSetReaderModule::CreateProperties()
{
}

void vtkPVDataSetReaderModule::InitializePrototype()
{
  this->Superclass::InitializePrototype();
}

int vtkPVDataSetReaderModule::ReadFile(const char* fname, 
				       vtkPVReaderModule*& clone)
{
  clone = 0;

  char *tclName, *extentTclName, *tmp;
  char *outputTclName;
  vtkPDataSetReader *s; 
  vtkDataSet *d;
  vtkPVData *pvd;
  vtkPVReaderModule *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  char *extension;
  int extensionPosition;
  char *endingSlash = NULL;
  int slashPosition;

  // Get the root name between the last slash and last period.
  slashPosition = 0;
  extensionPosition = strlen(fname);
  if ((extension = strrchr(fname, '.')))
    {
    extensionPosition = extension - fname;
    }
  if ((endingSlash = strrchr(fname, '/')))
    {
    slashPosition = endingSlash - fname + 1;
    }
  tclName = new char[extensionPosition-slashPosition+1+10];
  strncpy(tclName, fname+slashPosition, extensionPosition-slashPosition);
  tclName[extensionPosition-slashPosition] = '\0';

  if (isdigit(tclName[0]))
    {
    // A VTK object name beginning with a digit is invalid.
    tmp = new char[strlen(tclName)+3+10];
    strcpy(tmp, tclName);
    delete [] tclName;
    tclName = new char[strlen(tmp)+1+10];
    sprintf(tclName, "PV%s", tmp);
    delete [] tmp;
    }
  // Append the unique number for the name.
  tmp = new char[strlen(tclName)+1+10];
  strcpy(tmp, tclName);
  delete [] tclName;
  tclName = new char[strlen(tmp)+1 + (this->PrototypeInstanceCount%10)+1 + 10];
  sprintf(tclName, "%s%d", tmp, this->PrototypeInstanceCount);
  delete [] tmp;
  
  // Create the vtkSource.
  // Create the object through tcl on all processes.
  s = (vtkPDataSetReader *)(pvApp->MakeTclObject(
    this->SourceClassName, tclName));
  if (s == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return VTK_ERROR;
    }
  
  pvs = vtkPVReaderModule::New();
  pvs->SetParametersParent(
    this->GetPVWindow()->GetMainView()->GetPropertiesParent());
  pvs->SetApplication(pvApp);
  pvs->SetVTKSource(s, tclName);
  pvs->SetName(tclName);  
  pvApp->BroadcastScript("%s SetFileName %s",
                         tclName, fname);
  
  // Add the new Source to the View, and make it current.
  pvs->SetView(this->GetPVWindow()->GetMainView());
  // By-pass vtkPVReaderModule's CreateProperties()
  pvs->vtkPVSource::CreateProperties();
  this->GetPVWindow()->SetCurrentPVSource(pvs);
  this->GetPVWindow()->ShowCurrentSourceProperties();

  if (pvs)
    {
    if (pvs->GetTraceInitialized() == 0)
      { 
      vtkPVApplication* pvApp=this->GetPVApplication();
      pvApp->AddTraceEntry("set kw(%s) [%s GetCurrentPVSource]", 
			   pvs->GetTclName(), 
			   pvApp->GetMainWindow()->GetTclName());
      pvs->SetTraceInitialized(1);
      }
    }

  // Create the output.
  pvd = vtkPVData::New();
  pvd->SetPVApplication(pvApp);

  outputTclName = new char[strlen(tclName)+7+10];
  sprintf(outputTclName, "%sOutput", tclName);
  s->UpdateInformation();
  switch (s->GetDataType())
    {
    case VTK_POLY_DATA:
      d = (vtkDataSet *)(pvApp->MakeTclObject("vtkPolyData", outputTclName));
      break;
    case VTK_UNSTRUCTURED_GRID:
      d = (vtkDataSet *)(pvApp->MakeTclObject("vtkUnstructuredGrid", outputTclName));
      break;
    case VTK_STRUCTURED_GRID:
      d = (vtkDataSet *)(pvApp->MakeTclObject("vtkStructuredGrid", outputTclName));
      break;
    case VTK_RECTILINEAR_GRID:
      d = (vtkDataSet *)(pvApp->MakeTclObject("vtkRectilinearGrid", outputTclName));
      break;
    case VTK_STRUCTURED_POINTS:
    case VTK_IMAGE_DATA:
      d = (vtkDataSet *)(pvApp->MakeTclObject("vtkImageData", outputTclName));
      break;
    default:
      vtkErrorMacro("Could not determine output type.");
      pvs->Delete();
      pvd->Delete();
      return VTK_ERROR;
    }
  pvd->SetVTKData(d, outputTclName);

  // Connect the source and data.
  pvs->SetPVOutput(pvd);
  // It would be nice to have the vtkPVSource set this up, but for multiple
  // outputs, how do we know the method?
  // Relay the connection to the VTK objects.  
  pvApp->BroadcastScript("%s SetOutput %s", pvs->GetVTKSourceTclName(),
			 pvd->GetVTKDataTclName());   

  extentTclName = new char[strlen(tclName)+11 + 
			  (this->PrototypeInstanceCount%10)+1 + 10];
  sprintf(extentTclName, "%s%dTranslator", tclName, 
	  this->PrototypeInstanceCount);
  pvApp->BroadcastScript("vtkPVExtentTranslator %s", extentTclName);
  pvApp->BroadcastScript("%s SetOriginalSource [%s GetOutput]",
                         extentTclName, pvs->GetVTKSourceTclName());
  pvApp->BroadcastScript("%s SetExtentTranslator %s",
                         pvd->GetVTKDataTclName(), extentTclName);
  // Hold onto name so it can be deleted.
  pvs->SetExtentTranslatorTclName(extentTclName);

  pvd->Delete();

  // Add a file entry widget.
  vtkPVFileEntry *entry;
  entry = vtkPVDataSetFileEntry::New();
  entry->SetParent(pvs->GetParameterFrame()->GetFrame());
  entry->SetObjectVariable(pvs->GetVTKSourceTclName(), "FileName");
  entry->SetModifiedCommand(pvs->GetTclName(), "SetAcceptButtonColorToRed");
  entry->SetExtension("vtk");
  entry->SetBalloonHelpString("New file must have same type of output.");
  entry->Create(this->Application);
  entry->SetLabel("File Name");
  this->Script("pack %s -fill x -expand t", entry->GetWidgetName());
  pvs->AddPVWidget(entry);
  entry->SetValue(fname); 
  entry->Delete();
  entry = NULL;

  pvd->InsertExtractPiecesIfNecessary();

  //pvs->Accept(0);

  delete [] tclName;
  delete [] extentTclName;
  delete [] outputTclName;

  clone = pvs;
  this->PrototypeInstanceCount++;
  return VTK_OK;
}

