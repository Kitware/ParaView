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

#ifdef VTK_USE_PARALLEL
#  include "vtkMultiProcessController.h"
#endif

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
#include "vtkConeSource.h"

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
    this->File = NULL;
    }
  // --------------------------------------------------------------------------
  virtual ~Internals()
    {
    if(this->File) delete this->File;
    }
  // --------------------------------------------------------------------------
  void UpdateFileName(const char* currentFileName)
    {
    if(!this->File)
      {
      this->File = new AdiosFile(currentFileName);

      // Make sure that file is open and metadata loaded
      this->File->Open();
      }
    else // Check if the filename has changed
      {
      if(strcmp( currentFileName, this->File->FileName.c_str() ) != 0)
        {
        delete this->File;
        this->File = new AdiosFile(currentFileName);

        // Make sure that file is open and metadata loaded
        this->File->Open();
        }
      }
    }
  // --------------------------------------------------------------------------
  void FillMultiBlockDataSet(vtkMultiBlockDataSet* output)
    {
    // Pixie output
    // block 0 <=> RectilinearGrid
    // block 1 <=> StructuredGrid

    vtkDataSet* rectilinearGrid = NULL;
    vtkDataSet* structuredGrid = NULL;

    int realTimeStep = (int) this->TimeStep;

    AdiosVariableMapIterator iter = this->File->Variables.begin();
    for(; iter != this->File->Variables.end(); iter++)
      {
      const AdiosVariable *var = &iter->second;

      // Skip invalid timesteps
      ostringstream stream;
      stream << "/Timestep_" << realTimeStep << "/";
      if(var->Name.find(stream.str().c_str()) == vtkstd::string::npos)
        continue;

      // Skip invalid nodes coordinates array
      ostringstream streamNodes;
      streamNodes << "/Timestep_" << realTimeStep << "/nodes/";
      if(var->Name.find(streamNodes.str().c_str()) != vtkstd::string::npos)
        continue;

      // Skip invalid cell coordinates array
      ostringstream streamCells;
      streamCells << "/Timestep_" << realTimeStep << "/cells/";
      if(var->Name.find(streamCells.str().c_str()) != vtkstd::string::npos)
        continue;


      vtkDataArray* array = this->File->ReadVariable(var->Name.c_str(), realTimeStep);
      switch(this->File->GetDataType(var->Name.c_str())) // FIXME method is not correct !!!!!!!!!!!1
        {
        case VTK_RECTILINEAR_GRID:
          if(!rectilinearGrid)  // Create if needed
            rectilinearGrid = this->File->GetPixieRectilinearGrid(realTimeStep);
          rectilinearGrid->GetCellData()->AddArray(array);
          break;
        case VTK_STRUCTURED_GRID:
          if(!structuredGrid)  // Create if needed
            structuredGrid = this->File->GetPixieStructuredGrid(var->Name.c_str(), realTimeStep);
          structuredGrid->GetPointData()->AddArray(array);
          break;
        default:
          cout << "Do not know what to put var " << var->Name.c_str() << endl;
        }
      array->Delete();
      }

    if(rectilinearGrid)
      {
      rectilinearGrid->GetInformation()->Set( vtkDataObject::DATA_TIME_STEPS(),
                                              &this->TimeStep, 1);
      output->SetBlock(0, rectilinearGrid);
      rectilinearGrid->FastDelete();
      }
    if(structuredGrid)
      {
      structuredGrid->GetInformation()->Set( vtkDataObject::DATA_TIME_STEPS(),
                                             &this->TimeStep, 1);
      output->SetBlock(1, structuredGrid);
      structuredGrid->FastDelete();
      }
    output->GetInformation()->Set( vtkDataObject::DATA_TIME_STEPS(),
                                   &this->TimeStep, 1);

    // XGC output
    // block 0 <=> UnstructuredGrid

    //return this->File->GetPixieRectilinearGrid(realTimeStep);
    }
  // --------------------------------------------------------------------------
  int GetNumberOfTimeSteps()
    {
    this->File->GetNumberOfTimeSteps();
    }
  // --------------------------------------------------------------------------
  double GetTimeStep(int idx)
    {
    return this->File->GetRealTimeStep(idx);
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
private:
  AdiosFile* File;
  double TimeStep;
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
  if (this->GetFileName() == NULL)
    {
    vtkWarningMacro( << "FileName must be set");
    return 0;
    }
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

  // Create tmp time structure
  int nbTimesteps = this->Internal->GetNumberOfTimeSteps();
  double* timestepsValues = new double[this->Internal->GetNumberOfTimeSteps()];
  double timeRange[2];
  for(int i=0; i < nbTimesteps; i++)
    {
    timestepsValues[i] = this->Internal->GetTimeStep(i);
    }
  timeRange[0] = timestepsValues[0];
  timeRange[1] = timestepsValues[nbTimesteps - 1];

  // Set information objects
  vtkInformation *info = outputVector->GetInformationObject(0);
  info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), 1);
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timestepsValues,
            nbTimesteps);
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

  // Free tmp time structure
  delete[] timestepsValues;

  return 1;
}
//----------------------------------------------------------------------------
// call 3
int vtkAdiosReader::RequestUpdateExtent(vtkInformation*,
                        vtkInformationVector**,
                        vtkInformationVector* outputVector)
{
  vtkInformation *info = outputVector->GetInformationObject(0);
  this->Internal->SetActiveTimeStep(
      info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0]);
  return 1;
}
//----------------------------------------------------------------------------
// call 4
int vtkAdiosReader::RequestData(vtkInformation *, vtkInformationVector** vtkNotUsed( inputVector ),
    vtkInformationVector* outputVector)
{
  vtkInformation *info = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet *output =
      vtkMultiBlockDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  this->Internal->UpdateFileName(this->GetFileName());
  this->Internal->FillMultiBlockDataSet(output);

  return 1;
}
//----------------------------------------------------------------------------
int vtkAdiosReader::ReadOutputType()
{
  return VTK_MULTIBLOCK_DATA_SET;
}
//----------------------------------------------------------------------------
