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

#include "vtkDataArrayCollection.h"
#include "vtkDataSet.h"
#include "vtkEnSightReader.h"
#include "vtkObjectFactory.h"
#include "vtkToolkits.h"

#include "vtkKWFrame.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMessageDialog.h"

#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVRenderView.h"
#include "vtkPVSelectTimeSet.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"

#ifdef VTK_USE_MPI
# include "vtkPVEnSightVerifier.h"
#endif

#include <ctype.h>

int vtkPVEnSightReaderModuleCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVEnSightReaderModule::vtkPVEnSightReaderModule()
{
  this->CommandFunction = vtkPVEnSightReaderModuleCommand;

  this->VerifierTclName = 0;
#ifdef VTK_USE_MPI
  this->Verifier = 0;
#endif
}

//----------------------------------------------------------------------------
vtkPVEnSightReaderModule::~vtkPVEnSightReaderModule()
{
  this->DeleteVerifier();
}

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::DeleteVerifier()
{
#ifdef VTK_USE_MPI
  vtkPVApplication *pvApp = this->GetPVApplication();
  if (pvApp && this->VerifierTclName)
    {
    pvApp->BroadcastScript("%s Delete", this->VerifierTclName);
    }
  this->SetVerifierTclName(0);
#endif
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
char* vtkPVEnSightReaderModule::CreateTclName(const char* fname)
{
  char *tclName;
  char *extension;
  int position;
  char *endingSlash = NULL;
  char *newTclName;
  char *tmp;

  // Remove the path and get the filename
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
  
  // Append the instance count to the name.
  tmp = new char[strlen(tclName)+1 + (this->PrototypeInstanceCount%10)+1];
  sprintf(tmp, "%s%d", tclName, this->PrototypeInstanceCount);
  delete [] tclName;
  tclName = new char[strlen(tmp)+1];
  strcpy(tclName, tmp);
  delete [] tmp;

  return tclName;
}

//----------------------------------------------------------------------------
// Prompt the user for the time step loaded first.
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
int vtkPVEnSightReaderModule::CanReadFile(const char* fname)
{
  // Find the EnSight file type.
  vtkGenericEnSightReader *info;
  info = vtkGenericEnSightReader::New();
  info->SetCaseFileName(fname);
  int fileType = info->DetermineEnSightVersion();
  info->Delete();

  int retVal = 0;
  switch (fileType)
    {
    case vtkGenericEnSightReader::ENSIGHT_6:
    case vtkGenericEnSightReader::ENSIGHT_6_BINARY:
    case vtkGenericEnSightReader::ENSIGHT_GOLD:
    case vtkGenericEnSightReader::ENSIGHT_GOLD_BINARY:
#ifdef VTK_USE_MPI
    case vtkGenericEnSightReader::ENSIGHT_MASTER_SERVER:
#endif
      retVal = 1;
      break;
    }
  return retVal;
}

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::DisplayErrorMessage(const char* message,
						   int warning)
{
  vtkPVWindow* window = this->GetPVWindow();

  if (window && window->GetUseMessageDialog())
    {
    if (warning)
      {
      vtkKWMessageDialog::PopupMessage(
	this->Application, window,
	"Warning",  message, vtkKWMessageDialog::WarningIcon);
      }
    else
      {
      vtkKWMessageDialog::PopupMessage(
	this->Application, window,
	"Error",  message, vtkKWMessageDialog::ErrorIcon);
      }
    }
  else
    {
    if (warning)
      {
      vtkWarningMacro(<< message);
      }
    else
      {
      vtkErrorMacro(<< message);
      }
    }
}

// The next three should never get called if ParaView is not compiled with MPI

//----------------------------------------------------------------------------
// Make sure that all case files in the master file are compatible
// and create the appropriate EnSight reader.
vtkEnSightReader* vtkPVEnSightReaderModule::VerifyMasterFile(
  vtkPVApplication* pvApp, const char* tclName, const char* filename)
{
#ifdef VTK_USE_MPI

  // Create the EnSight verifier.
  char* vtclName = new char[strlen(tclName)+strlen("Ver")+1];
  sprintf(vtclName, "%sVer", tclName);
  this->SetVerifierTclName(vtclName);
  delete[] vtclName;

  this->Verifier = static_cast<vtkPVEnSightVerifier*>(
    pvApp->MakeTclObject("vtkPVEnSightVerifier", this->VerifierTclName));

  pvApp->BroadcastScript("%s SetController [Application GetController]", 
			 this->VerifierTclName);
  pvApp->BroadcastScript("%s SetCaseFileName \"%s\"", this->VerifierTclName,
			 filename);
  pvApp->BroadcastScript("%s VerifyVersion", this->VerifierTclName);

  if ( this->Verifier->GetLastError() != vtkPVEnSightVerifier::OK )
    {
    this->DisplayErrorMessage("Error while reading the master file. Either "
			      "one of the case files listed in the "
			      "master file can not be read or the versions "
			      "of the case files do not match.", 0);
    return 0;
    }


  // Create the appropriate reader.
  vtkEnSightReader* reader=0;
  switch (this->Verifier->GetEnSightVersion())
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
      vtkErrorMacro("Unrecognized file type " 
		    << this->Verifier->GetEnSightVersion());
    }

  pvApp->BroadcastScript("%s SetCaseFileName [%s GetPieceCaseFileName]",
			 tclName, this->VerifierTclName);
  

  return reader;
#else
  return 0;
#endif
}

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::VerifyTimeSets(vtkPVApplication *pvApp,
					      const char* tclName)
{
#ifdef VTK_USE_MPI

  pvApp->BroadcastScript("%s ReadAndVerifyTimeSets %s", this->VerifierTclName,
			 tclName);

  if ( this->Verifier->GetLastError() != vtkPVEnSightVerifier::OK )
    {
    this->DisplayErrorMessage("The time information of the case files in "
			      "the master file do not match. This might "
			      "cause inconsistent behaviour.", 1);
    }
#endif
}

//----------------------------------------------------------------------------
int vtkPVEnSightReaderModule::VerifyParts(vtkPVApplication *pvApp,
					  const char* tclName)
{
#ifdef VTK_USE_MPI

  pvApp->BroadcastScript("%s ReadAndVerifyParts %s", this->VerifierTclName,
			 tclName);

  if ( this->Verifier->GetLastError() != vtkPVEnSightVerifier::OK )
    {
    this->DisplayErrorMessage("The numbers or the types of the parts "
			      "in the different case files do not match. "
			      "ParaView can only load case files with "
			      "matching parts in parallel.", 0);
    return VTK_ERROR;
    }
  return VTK_OK;
#else
  return VTK_ERROR;
#endif

}

//----------------------------------------------------------------------------
int vtkPVEnSightReaderModule::ReadFile(const char* fname, 
				       vtkPVReaderModule*& clone)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow* window = this->GetPVWindow();

  clone = 0;
  if (!pvApp->GetRunningParaViewScript())
    {
    return this->ReadFile(fname, 0.0, pvApp, window);
    }
  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkPVEnSightReaderModule::ReadFile(const char* fname, float timeValue,
				       vtkPVApplication* pvApp,
				       vtkPVWindow* window)
{
  char *tclName, *outputTclName, *connectionTclName;
  vtkDataSet *d;
  vtkPVData *pvd = 0;
  vtkPVSource *pvs = 0;
  int numOutputs, i;

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
  int masterFile=0;

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
#ifdef VTK_USE_MPI
    case vtkGenericEnSightReader::ENSIGHT_MASTER_SERVER:
      reader = this->VerifyMasterFile(pvApp, tclName, fname);
      masterFile = 1;
      break;
#endif
    default:
      vtkErrorMacro("Unrecognized file type " << fileType);
      delete[] tclName;
      return VTK_ERROR;
    }

  if (!reader)
    {
    vtkErrorMacro("Could not create appropriate EnSight reader.");
    delete[] tclName;
    this->DeleteVerifier();
    return VTK_ERROR;
    }

  if (masterFile)
    {
    this->VerifyTimeSets(pvApp, tclName);
    }
  else
    {
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
    }

  int numTimeSets = reader->GetTimeSets()->GetNumberOfItems();
  float time = timeValue;
  if ( numTimeSets > 0 )
    {
    reader->SetTimeValue(reader->GetTimeSets()->GetItem(0)->GetTuple1(0));
    if (!pvApp->GetRunningParaViewScript())
      {
      if ( this->InitialTimeSelection(tclName, reader, time) == 0 )
	{
	pvApp->BroadcastScript("%s Delete", tclName);
	delete[] tclName;
	this->DeleteVerifier();
	return VTK_ERROR;
	}
      }
    pvApp->BroadcastScript("%s SetTimeValue %12.5e", tclName, time);
    }

  ostrstream namestr;
  namestr << "_tmp_ensightreadermodule" << this->PrototypeInstanceCount++ 
	  << ends;
  char* name = namestr.str();
  pvApp->AddTraceEntry("vtkPVEnSightReaderModule %s", name);
  pvApp->AddTraceEntry("%s ReadFile %s %12.5e Application $kw(%s)", name, 
		       fname, time, window->GetTclName());
  pvApp->AddTraceEntry("%s Delete", name);
  delete[] name;

  if (masterFile)
    {
    if (this->VerifyParts(pvApp, tclName) != VTK_OK)
      {
      pvApp->BroadcastScript("%s Delete", tclName);
      delete[] tclName;
      this->DeleteVerifier();
      return VTK_ERROR;
      }
    }
  else
    {
    pvApp->BroadcastScript("%s Update", tclName);
    }
  
  numOutputs = reader->GetNumberOfOutputs();
  if (numOutputs < 1)
    {
    this->DisplayErrorMessage("This file contains no parts or can not be "
			      "read. Please check the file for errors.", 0);
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

  window->GetMainView()->DisableRenderingFlagOn();

  vtkPVSource* connection;
  vtkPVData* connectionOutput;
  char* extentTranslatorName = 0;
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

    extentTranslatorName = 0;
    if (masterFile)
      {
      // We use an extent translator to make sure that the
      // structured parts we load are not broken further into
      // pieces by the pipeline. We want to whole structured
      // data to be processed because the data is pre-split
      // and the whole extent is equal to the actual extent of
      // the current piece.
      extentTranslatorName = new char [strlen(outputTclName)+4];
      sprintf(extentTranslatorName, "%sExt", outputTclName);
      pvApp->BroadcastScript("vtkMultiPartExtentTranslator %s",
			     extentTranslatorName);
      pvApp->BroadcastScript("%s SetExtentTranslator %s",
			     outputTclName, extentTranslatorName);
      }

    pvd = vtkPVData::New();
    pvd->SetPVApplication(pvApp);
    pvd->SetVTKData(d, outputTclName);
    pvs->SetPVOutput(i, pvd);
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
      connection->SetVTKSource(source, connectionTclName);
      connection->SetView(window->GetMainView());
      connection->SetName(connectionTclName);
      connection->HideParametersPageOn();
      connection->SetTraceInitialized(1);
      
      outputTclName = new char[strlen(connectionTclName)+7 + (i%10)+1];
      sprintf(outputTclName, "%sOutput%d", connectionTclName, i);
      d = static_cast<vtkDataSet*>(pvApp->MakeTclObject(
	reader->GetOutput(i)->GetClassName(), outputTclName));
      
      if (masterFile)
	{
	// We use an extent translator to make sure that the
	// structured parts we load are not broken further into
	// pieces by the pipeline. See the comment above.
	pvApp->BroadcastScript("%s SetExtentTranslator %s",
			       outputTclName, extentTranslatorName);
	}
      
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

  if (masterFile)
    {
    pvApp->BroadcastScript("%s Delete", extentTranslatorName);
    delete[] extentTranslatorName;
    }

  pvs->CreateProperties();
  pvs->SetTraceInitialized(1);

  // Time selection widget.
  if ( numTimeSets > 0 )
    {
    vtkPVSelectTimeSet* select = vtkPVSelectTimeSet::New();
    select->SetParent(pvs->GetParameterFrame()->GetFrame());
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
    pvs->GetPVOutput()->GetPVConsumer(0)->GetPVOutput()->ResetColorRange();
    }

  window->ResetCameraCallback();
  window->GetMainView()->DisableRenderingFlagOff();
  pvs->Accept(0);
  window->SetCurrentPVSource(pvs);
  
  pvs->Delete();
  

  delete [] tclName;
  this->DeleteVerifier();
  
  this->PrototypeInstanceCount++;
  return VTK_OK;
}

