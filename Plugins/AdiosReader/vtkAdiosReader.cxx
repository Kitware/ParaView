/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdiosReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAdiosReader.h"
#include "vtkAdiosInternals.h"

#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtksys/ios/sstream>
#include <vtkstd/string>
#include <vtkstd/map>

#include <sys/stat.h>
#include <assert.h>

#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include <vtkRectilinearGrid.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkStructuredGrid.h>
#include <vtkDataSet.h>
#include <vtkConeSource.h>

#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkCharArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkShortArray.h>
#include <vtkUnsignedShortArray.h>
#include <vtkIntArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkLongArray.h>
#include <vtkUnsignedLongArray.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkPoints.h>

#include <vtksys/ios/sstream>
using vtksys_ios::ostringstream;

//*****************************************************************************
class vtkAdiosReader::Internals
{
public:
  Internals()
    {
    this->MeshFile = NULL;
    this->TimeStep = 0;
    this->AdiosInitialized = false;
    this->NeedAdiosInitialization = true;
    }
  // --------------------------------------------------------------------------
  virtual ~Internals()
    {
    if(this->MeshFile)
      {
      delete this->MeshFile;
      this->MeshFile = NULL;
      }
    if(this->AdiosInitialized)
      {
      AdiosGlobal::Finalize();
      }
    }
  // --------------------------------------------------------------------------
  void UpdateFileName(const char* currentFileName)
    {
    if(!currentFileName)
      return;

    if(this->NeedAdiosInitialization && !this->AdiosInitialized)
      {
      this->AdiosInitialized = true;
      AdiosGlobal::Initialize();
      }

    if(!this->MeshFile)
      {
      this->MeshFile = new AdiosFile(currentFileName);

      // Make sure that file is open and metadata loaded
      this->MeshFile->Open();
      }
    else // Check if the filename has changed
      {
      if(strcmp( currentFileName, this->MeshFile->FileName.c_str()) != 0)
        {
        delete this->MeshFile; // not NULL because we are in the else
        this->MeshFile = new AdiosFile(currentFileName);

        // Make sure that file is open and metadata loaded
        this->MeshFile->Open();
        }
      }
    }
  // --------------------------------------------------------------------------
  void FillOutput(vtkDataObject* output)
    {
    vtkMultiBlockDataSet* multiBlock = vtkMultiBlockDataSet::SafeDownCast(output);
    // Pixie output
    // block 0 <=> RectilinearGrid
    // block 1 <=> StructuredGrid
    vtkMultiBlockDataSet* pixieOutput = multiBlock;
    vtkDataSet* rectilinearGrid = NULL;
    vtkDataSet* structuredGrid = NULL;

    int realTimeStep = (int) this->TimeStep;

    // Build geometry and read associted data
    rectilinearGrid = this->MeshFile->GetPixieRectilinearGrid(realTimeStep);
    if(rectilinearGrid)
      {
//      rectilinearGrid->GetInformation()->Set( vtkDataObject::DATA_TIME_STEPS(),
//                                              &this->TimeStep, 1);
      pixieOutput->SetBlock(0, rectilinearGrid);
      rectilinearGrid->FastDelete();
      }

    structuredGrid = this->MeshFile->GetPixieStructuredGrid(realTimeStep);
    if(structuredGrid)
      {
//      structuredGrid->GetInformation()->Set( vtkDataObject::DATA_TIME_STEPS(),
//                                             &this->TimeStep, 1);
      pixieOutput->SetBlock(1, structuredGrid);
      structuredGrid->FastDelete();
      }
//    pixieOutput->GetInformation()->Set( vtkDataObject::DATA_TIME_STEPS(),
//                                        &this->TimeStep, 1);

    // Close file to release resources
    if(this->MeshFile) this->MeshFile->Close();
    }
  // --------------------------------------------------------------------------
  int GetNumberOfTimeSteps()
    {
    int nbTime = 0;
    if(this->MeshFile)
      {
      nbTime = this->MeshFile->GetNumberOfTimeSteps();
      if(nbTime == -1)
        {
        return 0;
        }
      }
    return nbTime;
    }
  // --------------------------------------------------------------------------
  double GetTimeStep(int idx)
    {
    if(this->GetNumberOfTimeSteps() == 0)
      return 0;
    return this->MeshFile->GetRealTimeStep(idx);
    }
  // --------------------------------------------------------------------------
  void SetActiveTimeStep(double value)
    {
    this->TimeStep = value;
    }
  // --------------------------------------------------------------------------
  double GetActiveTimeStep()
    {
    return this->TimeStep;
    }
  // --------------------------------------------------------------------------
  bool Open()
    {
    return this->MeshFile->Open();
    }

public:
  bool NeedAdiosInitialization;

private:
  AdiosFile* MeshFile;
  double TimeStep;
  bool AdiosInitialized;
};
//*****************************************************************************
vtkStandardNewMacro(vtkAdiosReader);
//----------------------------------------------------------------------------
vtkAdiosReader::vtkAdiosReader()
{
  this->FileName = NULL;
  this->Internal = new Internals();
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkAdiosReader::~vtkAdiosReader()
{
  this->SetFileName(NULL);
  if(this->Internal)
    {
    delete this->Internal;
    this->Internal = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkAdiosReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: "
     << (this->FileName? this->FileName:"(none)") << "\n";
}

//----------------------------------------------------------------------------
int vtkAdiosReader::CanReadFile(const char* name)
{
  // First make sure the file exists.  This prevents an empty file
  // from being created on older compilers.
  struct stat fs;
  if(stat(name, &fs) != 0)
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
// call 1
int vtkAdiosReader::RequestDataObject(vtkInformation *, vtkInformationVector**,
                                      vtkInformationVector* outputVector)
{
  vtkInformation *info = outputVector->GetInformationObject(0);

  switch(this->ReadOutputType())
    {
    case VTK_MULTIBLOCK_DATA_SET:
    if (vtkMultiBlockDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT())))
      {
      // The output is already created
      return 1;
      }
    else
      {
      vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::New();
      this->GetExecutive()->SetOutputData(0, output);
      output->Delete();
      this->GetOutputPortInformation(0)->Set(vtkDataObject::DATA_EXTENT_TYPE(),
                                             output->GetExtentType());
      }
    break;
    case VTK_POLY_DATA:
      if (vtkPolyData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT())))
        {
        // The output is already created
        return 1;
        }
      else
        {
        vtkPolyData* output = vtkPolyData::New();
        this->GetExecutive()->SetOutputData(0, output);
        output->Delete();
        this->GetOutputPortInformation(0)->Set(vtkDataObject::DATA_EXTENT_TYPE(),
                                               output->GetExtentType());
        }
      break;
    case VTK_RECTILINEAR_GRID:
      if (vtkRectilinearGrid::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT())))
        {
        // The output is already created
        return 1;
        }
      else
        {
        vtkRectilinearGrid* output = vtkRectilinearGrid::New();
        this->GetExecutive()->SetOutputData(0, output);
        output->Delete();
        this->GetOutputPortInformation(0)->Set(vtkDataObject::DATA_EXTENT_TYPE(),
                                               output->GetExtentType());
        }
      break;

    case VTK_UNSTRUCTURED_GRID:
      if (vtkUnstructuredGrid::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT())))
        {
        // The output is already created
        return 1;
        }
      else
        {
        vtkUnstructuredGrid* output = vtkUnstructuredGrid::New();
        this->GetExecutive()->SetOutputData(0, output);
        output->Delete();
        this->GetOutputPortInformation(0)->Set(vtkDataObject::DATA_EXTENT_TYPE(),
                                               output->GetExtentType());
        }
      break;
    default:
      vtkErrorMacro("Invalid output type for Adios Reader");
      return 0;
    }
  return 1;
}
//----------------------------------------------------------------------------
// call 2
int vtkAdiosReader::RequestInformation(vtkInformation *, vtkInformationVector** ,
                                       vtkInformationVector* outputVector)
{
  if (this->GetFileName() == NULL)
    {
    vtkWarningMacro( << "FileName must be set");
    return 0;
    }
  this->Internal->UpdateFileName(this->GetFileName());

  // Get information object to fill
  vtkInformation *info = outputVector->GetInformationObject(0);

  // Deal with Number of pieces
  info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  // Deal with time information if any
  int nbTimesteps = this->Internal->GetNumberOfTimeSteps();
  if (nbTimesteps > 0)
    {
    double* timestepsValues = new double[nbTimesteps];
    double timeRange[2] = {0,1};
    for(int i=0; i < nbTimesteps; i++)
      {
      timeRange[1] = timestepsValues[i] = this->Internal->GetTimeStep(i);
      if(i == 0)
        {
        timeRange[0] = timestepsValues[0];
        }
      }
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timestepsValues,
              nbTimesteps);
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    delete[] timestepsValues;
    }

  return 1;
}
//----------------------------------------------------------------------------
// call 3
int vtkAdiosReader::RequestUpdateExtent(vtkInformation*,
                        vtkInformationVector**,
                        vtkInformationVector* outputVector)
{
  vtkInformation *info = outputVector->GetInformationObject(0);
  if(info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    this->Internal->SetActiveTimeStep(
        info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0]);
    }
  return 1;
}
//----------------------------------------------------------------------------
// call 4
int vtkAdiosReader::RequestData(vtkInformation *, vtkInformationVector** vtkNotUsed( inputVector ),
    vtkInformationVector* outputVector)
{
  vtkInformation *info = outputVector->GetInformationObject(0);
  vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());
  this->Internal->UpdateFileName(this->GetFileName());
  this->Internal->FillOutput(output);
  return 1;
}
//----------------------------------------------------------------------------
int vtkAdiosReader::ReadOutputType()
{
//  if (this->GetFileName() == NULL)
//    {
//    vtkWarningMacro( << "FileName must be set");
//    return 0;
//    }
//  this->Internal->UpdateFileName(this->GetFileName());
//  if(this->Internal->IsPixieFormat())
    return VTK_MULTIBLOCK_DATA_SET;
//  return VTK_UNSTRUCTURED_GRID;
}
//----------------------------------------------------------------------------
void vtkAdiosReader::SetReadMethodToBP()
{
  AdiosGlobal::SetReadMethodToBP();
  this->Internal->NeedAdiosInitialization = false;
}

//----------------------------------------------------------------------------
void vtkAdiosReader::SetReadMethodToDART()
{
  AdiosGlobal::SetReadMethodToDART();
  this->Internal->NeedAdiosInitialization = true;
}

//----------------------------------------------------------------------------
void vtkAdiosReader::SetReadMethod(int methodEnum)
{
  AdiosGlobal::SetReadMethod(methodEnum);
  this->Internal->NeedAdiosInitialization = (methodEnum > 1);
}

//----------------------------------------------------------------------------
void vtkAdiosReader::SetAdiosApplicationId(int id)
{
  AdiosGlobal::SetApplicationId(id);
}

//----------------------------------------------------------------------------
void vtkAdiosReader::PollForNewTimeSteps()
{
  if(this->Internal->Open())
    {
    this->Modified();
    }
}
