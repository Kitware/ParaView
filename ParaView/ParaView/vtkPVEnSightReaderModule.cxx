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
#include "vtkEnSightReader.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVWindow.h"
#include "vtkPVRenderView.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVSelectTimeSet.h"
#include "vtkKWFrame.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWLabeledFrame.h"

#include "vtkDataArrayCollection.h"

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
  vtkObject* ret = 
    vtkObjectFactory::CreateInstance("vtkPVEnSightReaderModule");
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

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::InitializePrototype()
{
  this->Superclass::InitializePrototype();
}

//----------------------------------------------------------------------------
char* vtkPVEnSightReaderModule::CreateTclName(const char* fname)
{
  char *tclName;
  char *extension;
  int position;
  char *endingSlash = NULL;
  char *newTclName;
  char *tmp;

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

  return tclName;
}

//----------------------------------------------------------------------------
int vtkPVEnSightReaderModule::InitialTimeSelection(
  const char* tclName, vtkGenericEnSightReader* reader, float& time)
{
    vtkKWDialog* timeDialog = vtkKWDialog::New();
    timeDialog->Create(this->Application, "");
    timeDialog->SetMasterWindow(this->GetPVWindow());
    timeDialog->SetTitle("Select time step");
    
    vtkPVSelectTimeSet* firstSelect = vtkPVSelectTimeSet::New();
    firstSelect->SetParent(timeDialog);
    firstSelect->Create(this->Application);
    firstSelect->SetLabel("Select initial time step:");
    firstSelect->SetObjectTclName(tclName);
    firstSelect->SetTraceName("selectTimeSet");
    firstSelect->SetReader(reader);
    firstSelect->Reset();
    this->Application->Script("pack %s -side top -fill x -expand t", 
			      firstSelect->GetWidgetName());

    vtkKWFrame* frame = vtkKWFrame::New();
    frame->SetParent(timeDialog);
    frame->Create(this->Application, 0);

    vtkKWWidget* okbutton = vtkKWWidget::New();
    okbutton->SetParent(frame);
    okbutton->Create(this->Application,"button","-text OK -width 16");
    okbutton->SetCommand(timeDialog, "OK");
    this->Script("pack %s -side left -expand yes",
		 okbutton->GetWidgetName());
    okbutton->Delete();

    vtkKWWidget* cancelbutton = vtkKWWidget::New();
    cancelbutton->SetParent(frame);
    cancelbutton->Create(this->Application,"button","-text CANCEL -width 16");
    cancelbutton->SetCommand(timeDialog, "Cancel");
    this->Script("pack %s -side left -expand yes",
		 cancelbutton->GetWidgetName());
    cancelbutton->Delete();

    this->Script("pack %s -side top -expand yes", frame->GetWidgetName());
    frame->Delete();

    int status = timeDialog->Invoke();
    time = firstSelect->GetTimeValue();
    firstSelect->Delete();

    timeDialog->Delete();

    return status;
}

//----------------------------------------------------------------------------
int vtkPVEnSightReaderModule::ReadFile(const char* fname, 
				       vtkPVReaderModule*& clone)
{

  clone = 0;

  char *tclName, *outputTclName, *connectionTclName;
  vtkDataSet *d;
  vtkPVData *pvd = 0;
  vtkPVSource *pvs = 0;
  vtkPVApplication *pvApp = this->GetPVApplication();
  int numOutputs, i;

  vtkPVWindow* window = this->GetPVWindow();
  if (!window)
    {
    vtkErrorMacro("PVWindow is not set. Can not create interface.");
    return VTK_ERROR;
    }

  tclName = this->CreateTclName(fname);

  // Find the EnSight file type.
  vtkGenericEnSightReader *info;
  info = vtkGenericEnSightReader::New();
  info->SetCaseFileName(fname);
  int fileType = info->DetermineEnSightVersion();
  info->Delete();

  // Create the right EnSight reader object through tcl on all processes.
  vtkEnSightReader* reader = 0;
  switch (fileType)
    {
    case vtkGenericEnSightReader::ENSIGHT_6:
      reader = static_cast<vtkEnSightReader*>
	(pvApp->MakeTclObject("vtkEnSight6Reader", tclName));
      break;
    case vtkGenericEnSightReader::ENSIGHT_6_BINARY:
      reader = static_cast<vtkEnSightReader*>
	(pvApp->MakeTclObject("vtkEnSight6BinaryReader", tclName));
      break;
    case vtkGenericEnSightReader::ENSIGHT_GOLD:
      reader = static_cast<vtkEnSightReader*>
	(pvApp->MakeTclObject("vtkEnSightGoldReader", tclName));
      break;
    case vtkGenericEnSightReader::ENSIGHT_GOLD_BINARY:
      reader = static_cast<vtkEnSightReader*>
	(pvApp->MakeTclObject("vtkEnSightGoldBinaryReader", tclName));
      break;
    default:
      vtkErrorMacro("Unrecognized file type " << fileType);
      delete[] tclName;
      return VTK_ERROR;
    }

  if (!reader)
    {
    vtkErrorMacro("Could not get pointer from object.");
    delete[] tclName;
    return VTK_ERROR;
    }
  pvApp->Script("%s SetCaseFileName \"%s\"", tclName, fname);

  if (strcmp(reader->GetCaseFileName(), "") == 0)
    {
    pvApp->BroadcastScript("%s Delete", tclName);
    delete[] tclName;
    return VTK_ERROR;
    }

  pvApp->BroadcastScript("%s SetFilePath \"%s\"", tclName,
			 reader->GetFilePath());
  pvApp->BroadcastScript("%s SetCaseFileName \"%s\"", tclName,
			 reader->GetCaseFileName());

  reader->UpdateInformation();

  int numTimeSets = reader->GetTimeSets()->GetNumberOfItems();
  if ( numTimeSets > 0 )
    {
    reader->SetTimeValue(reader->GetTimeSets()->GetItem(0)->GetTuple1(0));
    float time;
    if ( this->InitialTimeSelection(tclName, reader, time) == 0 )
      {
      pvApp->BroadcastScript("%s Delete", tclName);
      delete[] tclName;
      return VTK_ERROR;
      }
    pvApp->BroadcastScript("%s SetTimeValue %12.5e", tclName, time);
    }

  pvApp->BroadcastScript("%s Update", tclName);
  
  numOutputs = reader->GetNumberOfOutputs();
  if (numOutputs < 1)
    {
    if (window->GetUseMessageDialog())
      {
      vtkKWMessageDialog::PopupMessage(
	this->Application, window,
	"Error", 
	"This file contains no parts or can not be read. "
	"Please check the file for errors.",
	vtkKWMessageDialog::ErrorIcon);
      }
    else
      {
      vtkWarningMacro(	"This file contains no parts or can not be read. "
			"Please check the file for errors." )
      }
    return VTK_ERROR;
    }

  // Create the main reader source.
  pvs = vtkPVSource::New();
  pvs->SetParametersParent(window->GetMainView()->GetPropertiesParent());
  pvs->SetApplication(pvApp);
  pvs->SetVTKSource(reader, tclName);
  pvs->SetView(window->GetMainView());
  pvs->SetName(tclName);
  pvs->SetTraceInitialized(1);

  if (numOutputs > 1)
    {
    pvs->HideDisplayPageOn();
    }

  // Since, at the end of creation, the main reader will be
  // the current source, we initialize it's variable with
  // GetCurrentPVSource
  pvApp->AddTraceEntry("set kw(%s) [$kw(%s) GetCurrentPVSource]",
		       pvs->GetTclName(), window->GetTclName());

  vtkPVSource* connection;
  vtkPVData* connectionOutput;
  for (i = 0; i < numOutputs; i++)
    {
    // ith output (PVData)
    // Create replacement outputs of proper type and replace the
    // current ones.
    outputTclName = new char[strlen(tclName)+7 + (i%10)+1];
    sprintf(outputTclName, "%sOutput%d", tclName, i);
    d = static_cast<vtkDataSet*>(pvApp->MakeTclObject(
      reader->GetOutput(i)->GetClassName(), outputTclName));
    pvApp->BroadcastScript("%s ShallowCopy [%s GetOutput %d]",
			   outputTclName, tclName, i);
    pvApp->BroadcastScript("%s ReplaceNthOutput %d %s", tclName, i, 
			   outputTclName);

    pvd = vtkPVData::New();
    pvd->SetPVApplication(pvApp);
    pvd->SetVTKData(d, outputTclName);
    pvs->SetNthPVOutput(i, pvd);
    delete [] outputTclName;

    if (numOutputs > 1)
      {
      // ith connection point and it's output
      // These are dummy filters (vtkPassThroughFilter) which
      // simply shallow copy their input to their output.
      connection = vtkPVSource::New();
      connection->SetParametersParent(
	window->GetMainView()->GetPropertiesParent());
      connection->SetApplication(pvApp);
      connectionTclName = new char[strlen(tclName)+2 + ((i+1)%10)+1];
      sprintf(connectionTclName, "%s_%d", tclName, i+1);
      vtkSource* source = static_cast<vtkSource*>(
	pvApp->MakeTclObject("vtkPassThroughFilter", connectionTclName));
      connection->SetVTKSource(reader, connectionTclName);
      connection->SetView(window->GetMainView());
      connection->SetName(connectionTclName);
      connection->HideParametersPageOn();
      connection->SetTraceInitialized(1);
      
      outputTclName = new char[strlen(connectionTclName)+7 + (i%10)+1];
      sprintf(outputTclName, "%sOutput%d", connectionTclName, i);
      d = static_cast<vtkDataSet*>(pvApp->MakeTclObject(
	reader->GetOutput(i)->GetClassName(), outputTclName));
      
      
      connectionOutput = vtkPVData::New();
      connectionOutput->SetPVApplication(pvApp);
      connectionOutput->SetVTKData(d, outputTclName);
      connection->SetPVOutput(connectionOutput);
      
      connection->CreateProperties();
      // Created is called only on the first output when Accept()
      // is invoked on the input. We need to call it for the others.
      // (if it is verified that the Display pages for these outputs
      // are not necessary, this can be removed)
      if ( i > 0 )
	{
	pvd->CreateProperties();
	}
      connection->SetPVInput(pvd);
      pvApp->BroadcastScript("%s SetOutput %s", connectionTclName, 
			     outputTclName);

      connectionOutput->Delete();
      delete [] outputTclName;
      delete [] connectionTclName;
      
      window->GetSourceList("Sources")->AddItem(connection);
      connection->UpdateParameterWidgets();
      connection->Accept(0);
      window->SetCurrentPVSource(connection);

      // In the trace, the connection points are named by looking at
      // the outputs/consumers of the reader (CurrentPVSource)
      pvApp->AddTraceEntry("set kw(%s) "
			   "[[[$kw(%s) GetCurrentPVSource] GetPVOutput %d] "
			   "GetPVConsumer 0]",
			   connection->GetTclName(), 
			   window->GetTclName(), i);
      
      connection->Delete();
      }

    pvd->Delete();
    
    }

  pvs->CreateProperties();
  pvs->SetTraceInitialized(1);

  if ( numTimeSets > 0 )
    {
    vtkPVSelectTimeSet* select = vtkPVSelectTimeSet::New();
    select->SetParent(pvs->ParameterFrame->GetFrame());
    select->SetPVSource(pvs);
    select->SetLabel("Select time value");
    select->Create(pvApp);
    select->GetFrame()->ShowHideFrameOn();
    select->SetObjectTclName(tclName);
    select->SetTraceName("selectTimeSet");
    select->SetModifiedCommand(pvs->GetTclName(), 
 			       "SetAcceptButtonColorToRed");
    select->SetReader(reader);
    pvApp->Script("pack %s -side top -fill x -expand t", 
 		  select->GetWidgetName());
    pvs->AddPVWidget(select);
    select->Delete();
    }

  window->GetSourceList("Sources")->AddItem(pvs);
  pvs->UpdateParameterWidgets();
  if (numOutputs > 1)
    {
    pvs->GetPVOutput()->SetVisibilityInternal(0);
    }
  pvs->Accept(0);
  window->SetCurrentPVSource(pvs);
  
  pvs->Delete();
  

  delete [] tclName;

  
  this->PrototypeInstanceCount++;
  return VTK_OK;
}

