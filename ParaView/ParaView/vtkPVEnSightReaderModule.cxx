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
#include "vtkKWOKCancelDialog.h"
#include "vtkPVEnSightArraySelection.h"
#include "vtkKWOptionMenu.h"

#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVPassThrough.h"
#include "vtkPVRenderView.h"
#include "vtkPVSelectTimeSet.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"

#ifdef VTK_USE_MPI
# include "vtkPVEnSightVerifier.h"
#endif

#include <ctype.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVEnSightReaderModule);
vtkCxxRevisionMacro(vtkPVEnSightReaderModule, "1.34");

int vtkPVEnSightReaderModuleCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVEnSightReaderModule::vtkPVEnSightReaderModule()
{
  this->CommandFunction = vtkPVEnSightReaderModuleCommand;

  this->NumberOfRequestedPointVariables = 0;
  this->NumberOfRequestedCellVariables = 0;
  this->RequestedPointVariables = 0;
  this->RequestedCellVariables = 0;
  
  this->VerifierTclName = 0;
#ifdef VTK_USE_MPI
  this->Verifier = 0;
#endif

  this->ByteOrder = FILE_BIG_ENDIAN;
  this->PackFileEntry = 0;
  this->AddFileEntry = 0;
  this->RunningScript = 0;
  this->TimeValue = 0.0;
  
  this->Reader = 0;
  this->MasterFile = 0;
}

//----------------------------------------------------------------------------
vtkPVEnSightReaderModule::~vtkPVEnSightReaderModule()
{
  this->DeleteVerifier();
  
  if (this->Reader)
    {
    this->Reader->UnRegister(this);
    }
  
  int i;
  for (i = 0; i < this->NumberOfRequestedPointVariables; i++)
    {
    delete [] this->RequestedPointVariables[i];
    }
  if (this->RequestedPointVariables)
    {
    delete [] this->RequestedPointVariables;
    }
  
  for (i = 0; i < this->NumberOfRequestedCellVariables; i++)
    {
    delete [] this->RequestedCellVariables[i];
    }
  if (this->RequestedCellVariables)
    {
    delete [] this->RequestedCellVariables;
    }
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
char* vtkPVEnSightReaderModule::CreateTclName(const char* fname)
{
  char *tclName;
  const char *extension;
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
  int len;
  if ( this->PrototypeInstanceCount > 0 )
    {
    len = static_cast<int>(
      log10(static_cast<double>(this->PrototypeInstanceCount)));
    }
  else
    {
    len = 1;
    }
  tmp = new char[strlen(tclName) + 1 + len + 1];
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
  firstSelect->SetTraceNameState(vtkPVWidget::SelfInitialized);
  firstSelect->SetReader(reader);
  firstSelect->Reset();
  this->Application->Script("pack %s -side top -fill x -expand t", 
                            firstSelect->GetWidgetName());
  
  vtkKWFrame* frame = vtkKWFrame::New();
  frame->SetParent(timeDialog);
  frame->Create(this->Application, 0);
  
  vtkKWWidget* okbutton = vtkKWWidget::New();
  okbutton->SetParent(frame);
  okbutton->Create(this->Application,"button","-text OK");
  okbutton->SetCommand(timeDialog, "OK");
  
  vtkKWWidget* cancelbutton = vtkKWWidget::New();
  cancelbutton->SetParent(frame);
  cancelbutton->Create(this->Application,"button","-text Cancel");
  cancelbutton->SetCommand(timeDialog, "Cancel");
  
  this->Script("pack %s %s -side left -expand t -fill both -padx 2 -pady 2",
               okbutton->GetWidgetName(),
               cancelbutton->GetWidgetName());
  
  okbutton->Delete();
  cancelbutton->Delete();
  
  this->Script("pack %s -side top -expand yes -fill both", 
               frame->GetWidgetName());
  frame->Delete();
  
  int status = timeDialog->Invoke();
  time = firstSelect->GetTimeValue();
  firstSelect->Delete();
  
  timeDialog->Delete();
  
  return status;
}

//----------------------------------------------------------------------------
// Prompt the user for the first variables loaded
int vtkPVEnSightReaderModule::InitialVariableSelection(const char* tclName,
                                                       int askEndian)
{
  int shouldInvoke = askEndian;

  vtkPVApplication *pvApp = this->GetPVApplication();

  vtkKWOKCancelDialog *dialog = vtkKWOKCancelDialog::New();
  dialog->Create(this->Application, "");
  dialog->SetMasterWindow(this->GetPVWindow());
  if (askEndian)
    {
    dialog->SetTitle("Select variables/byte order");
    }
  else
    {
    dialog->SetTitle("Select variables");
    }
  
  vtkPVEnSightArraySelection *pointSelect = vtkPVEnSightArraySelection::New();
  pointSelect->SetParent(dialog);
  pointSelect->SetAttributeName("Point");
  pointSelect->SetVTKReaderTclName(tclName);
  pointSelect->Create(this->Application);
  pointSelect->AllOnCallback();
  
  vtkPVEnSightArraySelection *cellSelect = vtkPVEnSightArraySelection::New();
  cellSelect->SetParent(dialog);
  cellSelect->SetAttributeName("Cell");
  cellSelect->SetVTKReaderTclName(tclName);
  cellSelect->Create(this->Application);
  cellSelect->AllOnCallback();

  if (pointSelect->GetNumberOfArrays() > 0)
    {
    this->Script("pack %s -side top -fill x -expand t",
                 pointSelect->GetWidgetName());
    shouldInvoke = 1;
    }
  if (cellSelect->GetNumberOfArrays() > 0)
    {
    this->Script("pack %s -side top -fill x -expand t",
                 cellSelect->GetWidgetName());
    shouldInvoke = 1;
    }
  
  vtkKWOptionMenu* endianness;
  vtkKWLabeledFrame* lf;
  if (askEndian)
    {
    lf = vtkKWLabeledFrame::New();
    lf->SetParent(dialog);
    lf->Create(this->Application, 0);
    lf->SetLabel("Byte Order");

    endianness = vtkKWOptionMenu::New();
    endianness->SetParent(lf->GetFrame());
    endianness->Create(this->Application, "");
    endianness->AddEntry("Big Endian");
    endianness->AddEntry("Little Endian");
    endianness->SetCurrentEntry("Big Endian");

    this->Script("pack %s -side left", endianness->GetWidgetName());
    this->Script("pack %s -side top -fill x -expand t", lf->GetWidgetName());
    }

  int status=1;
  if (shouldInvoke)
    {
    status = dialog->Invoke();
    if (status)
      {
      pointSelect->Accept();
      cellSelect->Accept();
      }
    }

  if (askEndian)
    {
    const char* endiansel = endianness->GetValue();
    if ( strcmp(endiansel, "Big Endian") == 0 )
      {
      pvApp->BroadcastScript("%s SetByteOrderToBigEndian", tclName);
      }
    else
      {
      pvApp->BroadcastScript("%s SetByteOrderToLittleEndian", tclName);
      }

    endianness->Delete();
    lf->Delete();
    }

  pointSelect->Delete();
  cellSelect->Delete();
  dialog->Delete();
  
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
#ifdef VTK_USE_MPI
vtkEnSightReader* vtkPVEnSightReaderModule::VerifyMasterFile(
  vtkPVApplication* pvApp, const char* tclName, const char* filename)
{

  // Create the EnSight verifier.
  char* vtclName = new char[strlen(tclName)+strlen("Ver")+1];
  sprintf(vtclName, "%sVer", tclName);
  this->SetVerifierTclName(vtclName);
  delete[] vtclName;

  this->Verifier = static_cast<vtkPVEnSightVerifier*>(
    pvApp->MakeTclObject("vtkPVEnSightVerifier", this->VerifierTclName));

  pvApp->BroadcastScript("%s SetController [$Application GetController]", 
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
}
#else
vtkEnSightReader* vtkPVEnSightReaderModule::VerifyMasterFile(
  vtkPVApplication*, const char*, const char*)
{
  return 0;
}
#endif

//----------------------------------------------------------------------------
#ifdef VTK_USE_MPI
void vtkPVEnSightReaderModule::VerifyTimeSets(vtkPVApplication *pvApp,
                                              const char* tclName)
{

  pvApp->BroadcastScript("%s ReadAndVerifyTimeSets %s", this->VerifierTclName,
                         tclName);

  if ( this->Verifier->GetLastError() != vtkPVEnSightVerifier::OK )
    {
    this->DisplayErrorMessage("The time information of the case files in "
                              "the master file do not match. This might "
                              "cause inconsistent behaviour.", 1);
    }
}
#else
void vtkPVEnSightReaderModule::VerifyTimeSets(vtkPVApplication *, const char* )
{
}
#endif

//----------------------------------------------------------------------------
#ifdef VTK_USE_MPI
int vtkPVEnSightReaderModule::VerifyParts(vtkPVApplication *pvApp,
                                          const char* tclName)
{

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
}
#else
int vtkPVEnSightReaderModule::VerifyParts(vtkPVApplication *, const char*)
{
  return VTK_ERROR;
}
#endif


//----------------------------------------------------------------------------
int vtkPVEnSightReaderModule::Initialize(const char* fname, 
                                         vtkPVReaderModule*& clone)
{
  // Hack to get around the output type check in ClonePrototype
  this->SetOutputClassName("vtkPolyData");

  clone = 0;
  if (this->CloneAndInitialize(0, clone) != VTK_OK)
    {
    vtkErrorMacro("Error creating reader " << this->GetClassName()
                  << endl);
    clone = 0;
    return VTK_ERROR;
    }
  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkPVEnSightReaderModule::ReadFileInformation(const char* fname)
{
  char *tclName;
  int numOutputs, i;

  vtkPVApplication* pvApp = this->GetPVApplication();

  tclName = this->CreateTclName(fname);

  // Find the EnSight file type.
  vtkGenericEnSightReader *info;
  info = vtkGenericEnSightReader::New();
  info->SetCaseFileName(fname);
  int fileType = info->DetermineEnSightVersion();
  info->Delete();

  // Create the right EnSight reader object through tcl on all processes.
  this->MasterFile=0;

  if (this->Reader)
    {
    this->Reader->UnRegister(this);
    this->Reader = 0;
    }

  int askEndian=0;
  switch (fileType)
    {
    case vtkGenericEnSightReader::ENSIGHT_6:
      this->Reader = static_cast<vtkEnSightReader*>
        (pvApp->MakeTclObject("vtkEnSight6Reader", tclName));
      break;
    case vtkGenericEnSightReader::ENSIGHT_6_BINARY:
      this->Reader= static_cast<vtkEnSightReader*>
        (pvApp->MakeTclObject("vtkEnSight6BinaryReader", tclName));
      askEndian=1;
      break;
    case vtkGenericEnSightReader::ENSIGHT_GOLD:
      this->Reader = static_cast<vtkEnSightReader*>
        (pvApp->MakeTclObject("vtkEnSightGoldReader", tclName));
      break;
    case vtkGenericEnSightReader::ENSIGHT_GOLD_BINARY:
      this->Reader= static_cast<vtkEnSightReader*>
        (pvApp->MakeTclObject("vtkEnSightGoldBinaryReader", tclName));
      askEndian=1;
      break;
#ifdef VTK_USE_MPI
    case vtkGenericEnSightReader::ENSIGHT_MASTER_SERVER:
      this->Reader = this->VerifyMasterFile(pvApp, tclName, fname);
      this->MasterFile = 1;
      askEndian=1;
      break;
#endif
    default:
      vtkErrorMacro("Unrecognized file type " << fileType);
      delete[] tclName;
      this->Delete();
      return VTK_ERROR;
    }

  if (!this->Reader)
    {
    vtkErrorMacro("Could not create appropriate EnSight reader.");
    delete[] tclName;
    this->DeleteVerifier();
    this->Delete();
    return VTK_ERROR;
    }
  this->Reader->Register(this);

  if (this->MasterFile)
    {
    this->VerifyTimeSets(pvApp, tclName);
    }
  else
    {
    pvApp->Script("%s SetCaseFileName \"%s\"", tclName, fname);
    
    if (strcmp(this->Reader->GetCaseFileName(), "") == 0)
      {
      pvApp->BroadcastScript("%s Delete", tclName);
      delete[] tclName;
      this->Delete();
      return VTK_ERROR;
      }

    pvApp->BroadcastScript("%s SetFilePath \"%s\"", tclName,
                           this->Reader->GetFilePath());
    pvApp->BroadcastScript("%s SetCaseFileName \"%s\"", tclName,
                           this->Reader->GetCaseFileName());

    this->Reader->UpdateInformation();
    }

  int numTimeSets = this->Reader->GetTimeSets()->GetNumberOfItems();
  float time = this->TimeValue;

  if ( numTimeSets > 0 )
    {
    this->Reader->SetTimeValue(
      this->Reader->GetTimeSets()->GetItem(0)->GetTuple1(0));
    if (!this->RunningScript)
      {
      if ( this->InitialTimeSelection(tclName, this->Reader, time) == 0 )
        {
        pvApp->BroadcastScript("%s Delete", tclName);
        delete[] tclName;
        this->DeleteVerifier();
        this->Delete();
        return VTK_ERROR;
        }
      }
    pvApp->BroadcastScript("%s SetTimeValue %12.5e", tclName, time);
    }

  pvApp->BroadcastScript("%s ReadAllVariablesOff", tclName);
  int numVariables = this->Reader->GetNumberOfVariables() +
    this->Reader->GetNumberOfComplexVariables();
  
  if (!this->RunningScript)
    {
    if ( this->InitialVariableSelection(tclName, askEndian) == 0)
      {
      pvApp->BroadcastScript("%s Delete", tclName);
      delete [] tclName;
      this->DeleteVerifier();
      this->Delete();
      return VTK_ERROR;
      }
    }
  else
    {
    if (numVariables > 0)
      {
      for (i = 0; i < this->NumberOfRequestedPointVariables; i++)
        {
        pvApp->BroadcastScript("%s AddPointVariableName %s",
                               tclName, this->RequestedPointVariables[i]);
        }
      for (i = 0; i < this->NumberOfRequestedCellVariables; i++)
        {
        pvApp->BroadcastScript("%s AddCellVariableName %s",
                               tclName, this->RequestedCellVariables[i]);
        }
      if ( this->ByteOrder == FILE_BIG_ENDIAN )
        {
        pvApp->BroadcastScript("%s SetByteOrderToBigEndian", tclName);
        }
      else
        {
        pvApp->BroadcastScript("%s SetByteOrderToLittleEndian", tclName);
        }

      }
    }
      

#ifdef VTK_USE_MPI
  if (this->MasterFile)
    {
    if (this->VerifyParts(pvApp, tclName) != VTK_OK)
      {
      pvApp->BroadcastScript("%s Delete", tclName);
      delete[] tclName;
      this->DeleteVerifier();
      this->Delete();
      return VTK_ERROR;
      }
    }
  else
    {
    pvApp->BroadcastScript("%s Update", tclName);
    }
#else
  pvApp->BroadcastScript("%s Update", tclName);
#endif

  pvApp->AddTraceEntry("$kw(%s) RunningScriptOn", this->GetTclName());

  if (numVariables > 0)
    {
    int numNonComplexVariables = this->Reader->GetNumberOfVariables();
    int numComplexVariables = this->Reader->GetNumberOfComplexVariables();
    
    for (i = 0; i < numNonComplexVariables; i++)
      {
      switch (this->Reader->GetVariableType(i))
        {
        case 0:
        case 1:
        case 2:
        case 6:
        case 7:
          if (this->Reader->IsRequestedVariable(this->Reader->GetDescription(i), 0))
            {
            pvApp->AddTraceEntry("$kw(%s) AddPointVariable %s",
                                 this->GetTclName(), 
                                 this->Reader->GetDescription(i));
            }
          break;
        case 3:
        case 4:
        case 5:
          if (this->Reader->IsRequestedVariable(this->Reader->GetDescription(i), 1))
            {
            pvApp->AddTraceEntry("$kw(%s) AddCellVariable %s",
                                 this->GetTclName(), 
                                 this->Reader->GetDescription(i));
            }
          break;
        }
      }
    for (i = 0; i < numComplexVariables; i++)
      {
      switch (this->Reader->GetComplexVariableType(i))
        {
        case 8:
        case 9:
          if (this->Reader->IsRequestedVariable(
            this->Reader->GetComplexDescription(i), 0))
            {
            pvApp->AddTraceEntry("$kw(%s) AddPointVariable %s",
                                 this->GetTclName(), 
                                 this->Reader->GetComplexDescription(i));
            }
          break;
        case 10:
        case 11:
          if (this->Reader->IsRequestedVariable(
            this->Reader->GetComplexDescription(i), 1))
            {
            pvApp->AddTraceEntry("$kw(%s) AddCellVariable %s",
                                 this->GetTclName(), 
                                 this->Reader->GetComplexDescription(i));
            }
          break;
        }
      }
    }
  if (askEndian)
    {
    pvApp->AddTraceEntry("$kw(%s) SetByteOrder %d", 
                         this->GetTclName(), 
                         this->Reader->GetByteOrder());
    }
  
  numOutputs = this->Reader->GetNumberOfOutputs();
  if (numOutputs < 1)
    {
    this->DisplayErrorMessage("This file contains no parts or can not be "
                              "read. Please check the file for errors.", 0);
    this->Delete();
    return VTK_ERROR;
    }

  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkPVEnSightReaderModule::Finalize(const char* fname)
{
  vtkPVWindow* window = this->GetPVWindow();
  vtkPVApplication* pvApp = this->GetPVApplication();
  char* tclName = this->CreateTclName(fname);

  // Create the main reader source.
  vtkPVEnSightReaderModule *pvs = vtkPVEnSightReaderModule::New();
  pvs->AddFileEntry = 0;
  pvs->PackFileEntry = 0;
  pvs->SetParametersParent(window->GetMainView()->GetSourceParent());
  pvs->SetApplication(pvApp);
  pvs->AddVTKSource(this->Reader, tclName);
  pvs->SetView(window->GetMainView());
  pvs->SetName(tclName);
  pvs->SetTraceInitialized(1);


  // Since, at the end of creation, the main reader will be
  // the current source, we initialize it's variable with
  // GetCurrentPVSource
  pvApp->AddTraceEntry("set kw(%s) [$kw(%s) GetCurrentPVSource]",
                       pvs->GetTclName(), window->GetTclName());

  const char* desc = this->RemovePath(fname);
  if (desc)
    {
    pvs->SetLabelNoTrace(desc);
    }
  window->GetSourceList("Sources")->AddItem(pvs);


  pvs->CreateProperties();
  pvs->SetTraceInitialized(1);

  // Time selection widget.
  int numTimeSets = this->Reader->GetTimeSets()->GetNumberOfItems();
  if ( numTimeSets > 0 )
    {
    vtkPVSelectTimeSet* select = vtkPVSelectTimeSet::New();
    select->SetParent(pvs->GetParameterFrame()->GetFrame());
    select->SetPVSource(pvs);
    select->SetLabel("Select time value");
    select->Create(pvApp);
    select->GetLabeledFrame()->ShowHideFrameOn();
    select->SetObjectTclName(tclName);
    select->SetTraceName("selectTimeSet");
    select->SetTraceNameState(vtkPVWidget::SelfInitialized);
    select->SetModifiedCommand(pvs->GetTclName(), 
                               "SetAcceptButtonColorToRed");
    select->SetReader(this->Reader);
    pvApp->Script("pack %s -side top -fill x -expand t", 
                  select->GetWidgetName());
    pvs->AddPVWidget(select);
    select->Delete();
    }

  int numVariables = this->Reader->GetNumberOfVariables() +
    this->Reader->GetNumberOfComplexVariables();
  if ( numVariables > 0 )
    {
    vtkPVEnSightArraySelection *pointSelect =
      vtkPVEnSightArraySelection::New();
    pointSelect->SetParent(pvs->GetParameterFrame()->GetFrame());
    pointSelect->SetPVSource(pvs);
    pointSelect->SetVTKReaderTclName(tclName);
    pointSelect->SetAttributeName("Point");
    pointSelect->Create(pvApp);
    pointSelect->SetTraceName("selectPointVariables");
    pointSelect->SetTraceNameState(vtkPVWidget::SelfInitialized);
    pointSelect->SetModifiedCommand(pvs->GetTclName(),
                                    "SetAcceptButtonColorToRed");

    vtkPVEnSightArraySelection *cellSelect = vtkPVEnSightArraySelection::New();
    cellSelect->SetParent(pvs->GetParameterFrame()->GetFrame());
    cellSelect->SetPVSource(pvs);
    cellSelect->SetVTKReaderTclName(tclName);
    cellSelect->SetAttributeName("Cell");
    cellSelect->Create(pvApp);
    cellSelect->SetTraceName("selectCellVariables");
    cellSelect->SetTraceNameState(vtkPVWidget::SelfInitialized);
    cellSelect->SetModifiedCommand(pvs->GetTclName(),
                                   "SetAcceptButtonColorToRed");

    if (pointSelect->GetNumberOfArrays() > 0)
      {
      pvApp->Script("pack %s -side top -fill x -expand t",
                    pointSelect->GetWidgetName());
      }
    if (cellSelect->GetNumberOfArrays() > 0)
      {
      pvApp->Script("pack %s -side top -fill x -expand t",
                    cellSelect->GetWidgetName());
      }
    
    pvs->AddPVWidget(pointSelect);
    pvs->AddPVWidget(cellSelect);
    pointSelect->Delete();
    cellSelect->Delete();
    }
  
  pvs->UpdateParameterWidgets();
  
  // need AcceptCallbackFlag to be set so Reset isn't called when VTK
  // parameters change during Accept
  pvs->AcceptCallback();
  window->SetCurrentPVSource(pvs);
  
  pvs->Delete();
  
  delete [] tclName;
  this->DeleteVerifier();
  
  this->Delete();
  return VTK_OK;
}

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::AddPointVariable(const char* variableName)
{
  int size = this->NumberOfRequestedPointVariables;
  int i;
  
  char **newNameList = new char *[size]; // temporary array
  
  // copy variable names and attribute types to temporary arrays
  for (i = 0; i < size; i++)
    {
    newNameList[i] = new char[strlen(this->RequestedPointVariables[i]) + 1];
    strcpy(newNameList[i], this->RequestedPointVariables[i]);
    delete [] this->RequestedPointVariables[i];
    }
  if (this->RequestedPointVariables)
    {
    delete [] this->RequestedPointVariables;
    }
  
  // make room for new variable names
  this->RequestedPointVariables = new char *[size+1];
  
  // copy existing variables names back to the RequestedPointVariables array
  for (i = 0; i < size; i++)
    {
    this->RequestedPointVariables[i] = new char[strlen(newNameList[i]) + 1];
    strcpy(this->RequestedPointVariables[i], newNameList[i]);
    delete [] newNameList[i];
    }
  delete [] newNameList;
  
  // add new variable name at end of RequestedPointVariables array
  this->RequestedPointVariables[size] = new char[strlen(variableName) + 1];
  strcpy(this->RequestedPointVariables[size], variableName);
  
  this->NumberOfRequestedPointVariables++;
}

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::AddCellVariable(const char* variableName)
{
  int size = this->NumberOfRequestedCellVariables;
  int i;
  
  char **newNameList = new char *[size]; // temporary array
  
  // copy variable names and attribute types to temporary arrays
  for (i = 0; i < size; i++)
    {
    newNameList[i] = new char[strlen(this->RequestedCellVariables[i]) + 1];
    strcpy(newNameList[i], this->RequestedCellVariables[i]);
    delete [] this->RequestedCellVariables[i];
    }
  if (this->RequestedCellVariables)
    {
    delete [] this->RequestedCellVariables;
    }
  
  // make room for new variable names
  this->RequestedCellVariables = new char *[size+1];
  
  // copy existing variables names back to the RequestedCellVariables array
  for (i = 0; i < size; i++)
    {
    this->RequestedCellVariables[i] = new char[strlen(newNameList[i]) + 1];
    strcpy(this->RequestedCellVariables[i], newNameList[i]);
    delete [] newNameList[i];
    }
  delete [] newNameList;
  
  // add new variable name at end of RequestedPointVariables array
  this->RequestedCellVariables[size] = new char[strlen(variableName) + 1];
  strcpy(this->RequestedCellVariables[size], variableName);
  
  this->NumberOfRequestedCellVariables++;
}


//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::SetByteOrderToBigEndian()
{
  this->ByteOrder = FILE_BIG_ENDIAN;
}

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::SetByteOrderToLittleEndian()
{
  this->ByteOrder = FILE_LITTLE_ENDIAN;
}

//----------------------------------------------------------------------------
const char *vtkPVEnSightReaderModule::GetByteOrderAsString()
{
  if ( this->ByteOrder ==  FILE_LITTLE_ENDIAN)
    {
    return "LittleEndian";
    }
  else
    {
    return "BigEndian";
    }
}

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::SaveInTclScript(ofstream *file, 
                                               int vtkNotUsed(interactiveFlag),
                                               int vtkNotUsed(vtkFlag))
{
  vtkGenericEnSightReader *reader = 
    vtkGenericEnSightReader::SafeDownCast(this->GetVTKSource());

  if (reader == NULL)
    {
    vtkErrorMacro("GenericEnsightReader: Bad guess. " << this->GetVTKSource()->GetClassName());
    return;
    }

  // This should not be needed, but We can check anyway.
  if (this->VisitedFlag != 0)
    {
    return;
    }

  this->VisitedFlag = 1;

  // Save the object in the script.
  *file << "\n" << this->GetVTKSource()->GetClassName()
        << " " << this->GetVTKSourceTclName() << "\n";

  *file << "\t" << this->GetVTKSourceTclName() << " SetFilePath {"
        << reader->GetFilePath() << "}\n";

  *file << "\t" << this->GetVTKSourceTclName() << " SetCaseFileName {"
        << reader->GetCaseFileName() << "}\n";
   
  *file << "\t" << this->GetVTKSourceTclName() << " SetTimeValue "
        << reader->GetTimeValue() << "\n";
   
  *file << "\t" << this->GetVTKSourceTclName() << " Update\n";

}

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ByteOrder: " << this->ByteOrder << endl;
  os << indent << "TimeValue: " << this->TimeValue << endl;
  os << indent << "RunningScript: " << this->RunningScript << endl;
}
