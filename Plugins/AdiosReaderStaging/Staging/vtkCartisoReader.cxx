/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCartisoReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCartisoReader.h"

#include "vtkExtentTranslator.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <adios.h>
#include <adios_error.h>
#include <adios_read.h>

#include <string>

vtkStandardNewMacro(vtkCartisoReader);
vtkCxxSetObjectMacro(vtkCartisoReader, Controller, vtkMultiProcessController);

struct vtkCartisoReader::Internals
{
  std::string StreamName;
  ADIOS_FILE* File;
  ADIOS_READ_METHOD ReadMethod;
  MPI_Comm Comm;

  bool Cached;
  int GlobalDim[3];
  vtkNew<vtkImageData> Data;
};

vtkCartisoReader::vtkCartisoReader()
  : Controller(nullptr)
  , StreamName(nullptr)
  , TimeOut(300.0f)
  , Step(-1)
  , StreamEnded(false)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->SetStreamName("cartiso.full.bp");

  this->Internal = new Internals;
  this->Internal->File = nullptr;
  this->Internal->ReadMethod = ADIOS_READ_METHOD_FLEXPATH;
  // this->Internal->ReadMethod = ADIOS_READ_METHOD_BP;

  this->Internal->Cached = false;
  this->Internal->GlobalDim[0] = this->Internal->GlobalDim[1] = this->Internal->GlobalDim[2] = 0;
}

vtkCartisoReader::~vtkCartisoReader()
{
  this->Finalize();

  this->SetController(nullptr);
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkCartisoReader::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Controller: "
     << "\n";
  this->Controller->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Stream name: " << this->StreamName << "\n";
  os << indent << "Timeout: " << this->TimeOut << "\n";
  os << indent << "Image Data Cache: "
     << "\n";
  this->Internal->Data->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
void vtkCartisoReader::Initialize()
{
  if (!this->Internal->File)
  {
    vtkMPICommunicator* mpiComm =
      vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator());
    this->Internal->Comm = *mpiComm->GetMPIComm()->GetHandle();

    std::string sn(this->StreamName);
    std::size_t p = sn.rfind("_writer_info.txt");
    if (p < sn.length())
    {
      this->Internal->StreamName = sn.substr(0, p);
    }
    else
    {
      this->Internal->StreamName = sn;
    }

    adios_read_init_method(this->Internal->ReadMethod, this->Internal->Comm, "");
    this->Internal->File = adios_read_open_stream(this->Internal->StreamName.c_str(),
      this->Internal->ReadMethod, this->Internal->Comm, ADIOS_LOCKMODE_CURRENT, this->TimeOut);
    this->Step = this->Internal->File->current_step;
  }
}

void vtkCartisoReader::Finalize()
{
  if (this->Internal->File)
  {
    adios_read_close(this->Internal->File);
    adios_read_finalize_method(this->Internal->ReadMethod);
    adios_finalize(this->Controller->GetLocalProcessId());

    this->Internal->File = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkCartisoReader::AdvanceStep()
{
  if (this->Internal->File)
  {
    if (adios_advance_step(this->Internal->File, 0, 2.0f) == 0)
    {
      this->Step = this->Internal->File->current_step;
      this->Internal->Cached = false;
      this->Modified();
    }
    else
    {
      if (adios_errno == err_end_of_stream)
      {
        this->StreamEnded = true;
        vtkWarningMacro("The stream has ended");
      }
      else if (adios_errno == err_step_notready)
      {
        vtkWarningMacro("next step is not ready yet");
      }
    }
  }
}

//----------------------------------------------------------------------------
int vtkCartisoReader::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Initialize();

  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkCartisoReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (this->Internal->File && !this->Internal->Cached)
  {
    int ni, nj, nk;
    adios_schedule_read(this->Internal->File, nullptr, "ni", 0, 1, &ni);
    adios_schedule_read(this->Internal->File, nullptr, "nj", 0, 1, &nj);
    adios_schedule_read(this->Internal->File, nullptr, "nk", 0, 1, &nk);
    adios_perform_reads(this->Internal->File, 1);

    int wholeExtent[6] = { 0, ni - 1, 0, nj - 1, 0, nk - 1 };
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent, 6);
    outInfo->Set(CAN_PRODUCE_SUB_EXTENT(), 1);

    this->Internal->GlobalDim[0] = ni;
    this->Internal->GlobalDim[1] = nj;
    this->Internal->GlobalDim[2] = nk;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkCartisoReader::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkCartisoReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkImageData* output = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->Internal->File && !this->Internal->Cached)
  {
    int* localExtents = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

    // ADIOS expects column major arrays in C/C++. Transpose dimensions and
    // indices instead of transposing the array
    uint64_t start[3];
    start[0] = localExtents[4];
    start[1] = localExtents[2];
    start[2] = localExtents[0];
    uint64_t count[3];
    count[0] = localExtents[5] - localExtents[4] + 1;
    count[1] = localExtents[3] - localExtents[2] + 1;
    count[2] = localExtents[1] - localExtents[0] + 1;

    vtkIdType numTuples = count[0] * count[1] * count[2];
    vtkNew<vtkFloatArray> value;
    value->SetName("value");
    value->SetNumberOfComponents(1);
    value->SetNumberOfTuples(numTuples);

    vtkNew<vtkFloatArray> noise;
    noise->SetName("noise");
    noise->SetNumberOfComponents(1);
    noise->SetNumberOfTuples(numTuples);

    float deltax, deltay, deltaz;
    int tstep;
    adios_schedule_read(this->Internal->File, nullptr, "tstep", 0, 1, &tstep);
    adios_schedule_read(this->Internal->File, nullptr, "deltax", 0, 1, &deltax);
    adios_schedule_read(this->Internal->File, nullptr, "deltay", 0, 1, &deltay);
    adios_schedule_read(this->Internal->File, nullptr, "deltaz", 0, 1, &deltaz);

    ADIOS_SELECTION* sel = adios_selection_boundingbox(3, start, count);
    adios_schedule_read(this->Internal->File, sel, "value", 0, 1, value->GetPointer(0));
    adios_schedule_read(this->Internal->File, sel, "noise", 0, 1, noise->GetPointer(0));

    adios_perform_reads(this->Internal->File, 1);
    adios_selection_delete(sel);
    adios_release_step(this->Internal->File);

    this->Internal->Data->SetExtent(localExtents);
    this->Internal->Data->SetOrigin(0.0, 0.0, 0.0);
    this->Internal->Data->SetSpacing(deltax, deltay, deltaz);
    this->Internal->Data->GetPointData()->AddArray(value.GetPointer());
    this->Internal->Data->GetPointData()->AddArray(noise.GetPointer());
    this->Internal->Data->GetPointData()->SetActiveScalars("value");

    vtkNew<vtkFloatArray> tstepField;
    tstepField->SetName("tstep");
    tstepField->SetNumberOfComponents(1);
    tstepField->SetNumberOfTuples(1);
    tstepField->SetValue(0, tstep);
    this->Internal->Data->GetFieldData()->AddArray(tstepField.GetPointer());

    this->Internal->Cached = true;
  }

  output->ShallowCopy(this->Internal->Data.GetPointer());

  return 1;
}
