/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPGenericEnSightReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPGenericEnSightReader.h"

#include "vtkCallbackCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataArrayCollection.h"
#include "vtkDataArraySelection.h"
#include "vtkEnSight6BinaryReader.h"
#include "vtkEnSight6Reader.h"
#include "vtkEnSightGoldBinaryReader.h"
#include "vtkEnSightGoldReader.h"
#include "vtkIdListCollection.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPEnSightGoldBinaryReader.h"
#include "vtkPEnSightGoldReader.h"

#include <assert.h>
#include <ctype.h> /* isspace */
#include <map>
#include <string>

vtkStandardNewMacro(vtkPGenericEnSightReader);

//----------------------------------------------------------------------------
vtkPGenericEnSightReader::vtkPGenericEnSightReader()
{
  // -2 is the default starting value
  this->MultiProcessLocalProcessId = -2;
  this->MultiProcessNumberOfProcesses = -2;
}

//----------------------------------------------------------------------------
vtkPGenericEnSightReader::~vtkPGenericEnSightReader() = default;

//----------------------------------------------------------------------------
int vtkPGenericEnSightReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int version = this->DetermineEnSightVersion();
  int createReader = 1;
  if (this->GetMultiProcessNumberOfProcesses() < 2 ||
    (version == vtkPGenericEnSightReader::ENSIGHT_6) ||
    (version == vtkPGenericEnSightReader::ENSIGHT_6_BINARY))
  {
    // single process mode, so use parents request information
    // or we can't handle this data format in parallel
    return vtkGenericEnSightReader::RequestInformation(request, inputVector, outputVector);
  }
  else if (version == vtkPGenericEnSightReader::ENSIGHT_GOLD)
  {
    if (this->Reader)
    {
      if (strcmp(this->Reader->GetClassName(), "vtkPEnSightGoldReader") == 0)
      {
        createReader = 0;
      }
      else
      {
        this->Reader->Delete();
      }
    }
    if (createReader)
    {
      this->Reader = vtkPEnSightGoldReader::New();
    }
  }
  else if (version == vtkPGenericEnSightReader::ENSIGHT_GOLD_BINARY)
  {
    if (this->Reader)
    {
      if (strcmp(this->Reader->GetClassName(), "vtkPEnSightGoldBinaryReader") == 0)
      {
        createReader = 0;
      }
      else
      {
        this->Reader->Delete();
      }
    }
    if (createReader)
    {
      this->Reader = vtkPEnSightGoldBinaryReader::New();
    }
  }
  else
  {
    vtkErrorMacro("Error determining EnSightVersion");
    this->EnSightVersion = -1;
    return 0;
  }

  this->EnSightVersion = version;

  // Copy current array selections to internal reader.
  this->SetReaderDataArraySelectionSetsFromSelf();
  this->Reader->SetReadAllVariables(this->ReadAllVariables);
  this->Reader->SetCaseFileName(this->GetCaseFileName());
  this->Reader->SetFilePath(this->GetFilePath());

  // The following line, explicitly initializing this->ByteOrder to
  // FILE_UNKNOWN_ENDIAN,  MUST !!NOT!! be removed as it is used to
  // force vtkEnSightGoldBinaryReader::ReadPartId(...) to determine
  // the actual endian type. Otherwise the endian type, the default
  // value from combobox 'Byte Order' of the user interface -------
  // FILE_BIG_ENDIAN unless the user manually toggles the combobox,
  // would be forwarded to  this->Reader->ByteOrder through the next
  // line and therefore would prevent vtkEnSightGoldBinaryReader::
  // ReadPartId(...) from automatically checking the endian type. As
  // a consequence, little-endian files such as the one mentioned in
  // bug #0008237 would not be loadable. The following line might be
  // removed ONLY WHEN the combobox is removed through
  // ParaViews\Servers\ServerManager\Resources\readers.xml.
  // Thus it is highly suggested that the following line be retained
  // to guarantee the fix to bug #0007424 -- automatic determination
  // of the endian type.
  this->ByteOrder = FILE_UNKNOWN_ENDIAN;

  this->Reader->SetByteOrder(this->ByteOrder);
  vtkPGenericEnSightReader* reader = dynamic_cast<vtkPGenericEnSightReader*>(this->Reader);
  if (reader)
  {
    // this dynamic cast never should fail
    reader->RequestInformation(request, inputVector, outputVector);
  }
  this->Reader->SetParticleCoordinatesByIndex(this->ParticleCoordinatesByIndex);

  this->SetTimeSets(this->Reader->GetTimeSets());
  if (!this->TimeValueInitialized)
  {
    this->SetTimeValue(this->Reader->GetTimeValue());
  }
  this->MinimumTimeValue = this->Reader->GetMinimumTimeValue();
  this->MaximumTimeValue = this->Reader->GetMaximumTimeValue();

  // Copy new data array selections from internal reader.
  this->SetDataArraySelectionSetsFromReader();

  return 1;
}

//----------------------------------------------------------------------------
// MultiProcess Number Of Processes Cache. Will be read a lot of times
// -2: initialized (not set)
// 0: No MultiProcess Controller
// X: Number of processes
int vtkPGenericEnSightReader::GetMultiProcessNumberOfProcesses()
{
  if (this->MultiProcessNumberOfProcesses == -2)
  {
    // Initialize
    if (vtkMultiProcessController::GetGlobalController() == nullptr)
      this->MultiProcessNumberOfProcesses = 0;
    else
      this->MultiProcessNumberOfProcesses =
        vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses();
  }

  return this->MultiProcessNumberOfProcesses;
}

//----------------------------------------------------------------------------
// MultiProcess Local Process Id Cache. Will be read a lot of times
// -2: initialized (not set)
// -1: No MultiProcess Controller
// X: Local Process Id
int vtkPGenericEnSightReader::GetMultiProcessLocalProcessId()
{
  if (this->MultiProcessLocalProcessId == -2)
  {
    // Initialize
    if (vtkMultiProcessController::GetGlobalController() == nullptr)
      this->MultiProcessLocalProcessId = -1;
    else
      this->MultiProcessLocalProcessId =
        vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  }

  return this->MultiProcessLocalProcessId;
}

//----------------------------------------------------------------------------
void vtkPGenericEnSightReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MultiProcessLocalProcessId: " << this->MultiProcessLocalProcessId << endl;
  os << indent << "MultiProcessNumberOfProcesses: " << this->MultiProcessNumberOfProcesses << endl;
}
