/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVEnSightVerifier.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVEnSightVerifier.h"

#include "vtkDataArrayCollection.h"
#include "vtkEnSightMasterServerReader.h"
#include "vtkEnSightReader.h"
#include "vtkGenericEnSightReader.h"
#include "vtkMPIController.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVEnSightVerifier, "1.2");
vtkStandardNewMacro(vtkPVEnSightVerifier);

vtkCxxSetObjectMacro(vtkPVEnSightVerifier, Controller, 
		     vtkMPIController);

//----------------------------------------------------------------------------
vtkPVEnSightVerifier::vtkPVEnSightVerifier()
{
  this->Controller = 0;

  this->CaseFileName = 0;

  this->EnSightVersion = -1;
  this->PieceCaseFileName = 0;

  this->LastError = -1;
}

//----------------------------------------------------------------------------
vtkPVEnSightVerifier::~vtkPVEnSightVerifier()
{
  int i;
  
  this->SetController(0);
  this->SetCaseFileName(0);
  this->SetPieceCaseFileName(0);
}

//----------------------------------------------------------------------------
int vtkPVEnSightVerifier::CheckForError(int status)
{

  if (this->Controller)
    {
    vtkMPICommunicator* comm = vtkMPICommunicator::SafeDownCast(
      this->Controller->GetCommunicator());
    if ( comm )
      {
      int numProcs = this->Controller->GetNumberOfProcesses();
      int* res = new int[numProcs];
      int myid = this->Controller->GetLocalProcessId();

      // Collect all the status info to the 0th node
      comm->Gather(&status, res, 1, 0);

      int result = VTK_OK;

      // 0th node checks if any of the status' is not VTK_OK
      if ( myid == 0 )
	{
	for (int i=0; i<numProcs; i++)
	  {
	  if ( res[i] != VTK_OK )
	    {
	    result = VTK_ERROR;
	    break;
	    }
	  }
	}
      delete[] res;

      // Broadcast the result to all processes
      comm->Broadcast(&result, 1, 0);
      return result;
      }
    }

  return VTK_ERROR;
}

//----------------------------------------------------------------------------
template <class T>
int vtkPVEnSightVerifierCompareValues(T* data, int numVals, 
				      vtkMPIController* controller)
{

  if (controller)
    {
    vtkMPICommunicator* comm = vtkMPICommunicator::SafeDownCast(
      controller->GetCommunicator());
    if ( comm )
      {
      int numProcs = controller->GetNumberOfProcesses();
      T* values = new T[numProcs*numVals];
      int myid = controller->GetLocalProcessId();

      // Collect all the values to the 0th node
      comm->Gather(data, values, numVals, 0);

      int result = VTK_OK;
      // 0th node checks if any of the values is not the same as first
      if ( myid == 0 )
	{
	for (int i=1; i<numProcs; i++)
	  {
	  for (int j=0; j<numVals; j++)
	    {
	    if ( values[i*numVals+j] != values[j] )
	      {
	      result = VTK_ERROR;
	      break;
	      }
	    }
	  }
	}

      delete[] values;
      // Broadcast the result to all processes
      comm->Broadcast(&result, 1, 0);
      return result;
      }
    }

  return VTK_ERROR;
}

//----------------------------------------------------------------------------
int vtkPVEnSightVerifier::CompareValues(int value)
{
  return vtkPVEnSightVerifierCompareValues(&value, 1, this->Controller);
}


//----------------------------------------------------------------------------
int vtkPVEnSightVerifier::CompareTimeSets(vtkEnSightReader* reader)
{
  vtkDataArrayCollection* timeSets = reader->GetTimeSets();

  // First check the number of time sets.
  int numTimeSets = timeSets->GetNumberOfItems();
  if ( this->CompareValues(numTimeSets) != VTK_OK )
    {
    return vtkPVEnSightVerifier::NUMBER_OF_SETS_MISMATCH;
    }
  if ( numTimeSets == 0 )
    {
    return vtkPVEnSightVerifier::OK;
    }
  
  // Next compare the number of steps in all time sets.
  int i,j;
  int totalNumSteps=0;
  int* numTimeSteps = new int[numTimeSets];
  for(i=0; i<numTimeSets; i++)
    {
    numTimeSteps[i] = static_cast<int>(
      timeSets->GetItem(i)->GetNumberOfTuples());
    totalNumSteps += numTimeSteps[i];
    }
  if ( vtkPVEnSightVerifierCompareValues(numTimeSteps, numTimeSets,
					 this->Controller) != VTK_OK )
    {
    delete[] numTimeSteps;
    return vtkPVEnSightVerifier::NUMBER_OF_STEPS_MISMATCH;
    }
  delete[] numTimeSteps;

  // Next compare values
  float* timeValues = new float[totalNumSteps];
  int index=0;
  for(i=0; i<numTimeSets; i++)
    {
    vtkDataArray* array = timeSets->GetItem(i);
    int numValues = array->GetNumberOfTuples();
    for(j=0; j<numValues; j++)
      {
      timeValues[index] = array->GetTuple1(j);
      index++;
      }
    }

  if ( vtkPVEnSightVerifierCompareValues(timeValues, totalNumSteps,
					 this->Controller) != VTK_OK )
    {
    delete[] timeValues;
    return vtkPVEnSightVerifier::TIME_VALUES_MISMATCH;
    }
  delete[] timeValues;

  return vtkPVEnSightVerifier::OK;
}

//----------------------------------------------------------------------------
int vtkPVEnSightVerifier::CompareParts(vtkEnSightReader* reader)
{
  // First compare number of parts
  int numOutputs = reader->GetNumberOfOutputs();
  if ( this->CompareValues(numOutputs) != VTK_OK )
    {
    return vtkPVEnSightVerifier::NUMBER_OF_OUTPUTS_MISMATCH;
    }

  if ( numOutputs == 0 )
    {
    return vtkPVEnSightVerifier::OK;
    }

  // Next compare the part types
  int* outputTypes = new int[numOutputs];
  for (int i=0; i<numOutputs; i++)
    {
    vtkDataSet* output = reader->GetOutput(i);
    if (output)
      {
      outputTypes[i] = output->GetDataObjectType();
      }
    else
      {
      outputTypes[i] = -1;
      }
    }

  if (vtkPVEnSightVerifierCompareValues(outputTypes, numOutputs, 
					this->Controller) != VTK_OK)
    {
    delete[] outputTypes;
    return vtkPVEnSightVerifier::OUTPUT_TYPE_MISMATCH;
    }
  delete[] outputTypes;
  
  return vtkPVEnSightVerifier::OK;
}

//----------------------------------------------------------------------------
int vtkPVEnSightVerifier::VerifyVersion()
{
  this->EnSightVersion = -1;
  this->SetPieceCaseFileName(0);

  // Find out the case filename for this process
  vtkEnSightMasterServerReader* reader = vtkEnSightMasterServerReader::New();

  int currentPiece = this->Controller->GetLocalProcessId();

  reader->SetCaseFileName(this->CaseFileName);
  reader->SetCurrentPiece(currentPiece);
  int retVal = reader->DetermineFileName(currentPiece);
  if ( this->CheckForError(retVal) == VTK_ERROR )
    {
    reader->Delete();
    this->LastError = vtkPVEnSightVerifier::DETERMINE_FILENAME_FAILED;
    return this->LastError;
    }

  // If the filename starts with / or \ use it as it is, otherwise
  // append it to the file path obtained from the reader.
  const char* pieceFileName = reader->GetPieceCaseFileName();
  char* newFileName = 0;
  if (pieceFileName && pieceFileName[0] != '/' && pieceFileName[0] != '\\')
    {
    const char* root  = reader->GetFilePath();
    newFileName = new char[strlen(root)+strlen(pieceFileName)+2];
    sprintf(newFileName, "%s/%s", root, pieceFileName);
    }
  else
    {
    newFileName = new char[strlen(pieceFileName)+1];
    sprintf(newFileName, "%s", pieceFileName);
    }
    
  // Read and compare file types
  vtkGenericEnSightReader *info = vtkGenericEnSightReader::New();
  info->SetCaseFileName(newFileName);
  int fileType = info->DetermineEnSightVersion();
  info->Delete();

  if ( fileType == -1 )
    {
    retVal = VTK_ERROR;
    }
  else
    {
    retVal = VTK_OK;
    }
  if ( this->CheckForError(retVal) == VTK_ERROR )
    {
    reader->Delete();
    this->LastError = vtkPVEnSightVerifier::DETERMINE_VERSION_FAILED;
    return this->LastError;
    }

  if ( this->CompareValues(fileType) == VTK_ERROR )
    {
    reader->Delete();
    this->LastError = vtkPVEnSightVerifier::VERSION_MISMATCH;
    return this->LastError;
    }

  this->EnSightVersion = fileType;

  this->SetPieceCaseFileName(newFileName);
  delete[] newFileName;

  reader->Delete();

  this->LastError = vtkPVEnSightVerifier::OK;
  return this->LastError;
}

//----------------------------------------------------------------------------
int vtkPVEnSightVerifier::ReadAndVerifyTimeSets(vtkEnSightReader* reader)
{
  reader->UpdateInformation();

  this->LastError = this->CompareTimeSets(reader);
  return this->LastError;
}

//----------------------------------------------------------------------------
int vtkPVEnSightVerifier::ReadAndVerifyParts(vtkEnSightReader* reader)
{
  reader->Update();

  this->LastError = this->CompareParts(reader);
  return this->LastError;
}

//----------------------------------------------------------------------------
void vtkPVEnSightVerifier::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CaseFileName: "
     << (this->CaseFileName ? this->CaseFileName : "(none)") << endl;
  os << indent << "PieceCaseFileName: "
     << (this->PieceCaseFileName ? this->PieceCaseFileName : "(none)") << endl;
  os << indent << "Last error: " << this->LastError << endl;
  os << indent << "EnSight version: " << this-EnSightVersion << endl;
}

