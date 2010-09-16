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
    this->MeshFile = NULL;
    this->DataFile = NULL;
    }
  // --------------------------------------------------------------------------
  virtual ~Internals()
    {
    if(this->MeshFile) delete this->MeshFile;
    if(this->DataFile) delete this->DataFile;
    }
  // --------------------------------------------------------------------------
  void UpdateFileName(const char* currentFileName)
    {
    if(!this->MeshFile)
      {
      this->MeshFile = new AdiosFile(currentFileName);

      // Make sure that file is open and metadata loaded
      this->MeshFile->Open();
      LoadMeshFileIfNeeded();
      }
    else // Check if the filename has changed
      {
      if((strcmp( currentFileName, this->MeshFile->FileName.c_str()) != 0) || (this->DataFile && (strcmp( currentFileName, this->DataFile->FileName.c_str()) != 0 )) )
        {
        delete this->MeshFile;
        if(this->DataFile)
          {
          delete this->DataFile;
          this->DataFile = NULL;
          }

        this->MeshFile = new AdiosFile(currentFileName);

        // Make sure that file is open and metadata loaded
        this->MeshFile->Open();
        LoadMeshFileIfNeeded();
        }
      }
    }
  // --------------------------------------------------------------------------
  void LoadMeshFileIfNeeded()
    {
    if(this->MeshFile->IsPixieFileType())
      return;

    // We are supposed to have load a data file or maybe just the mesh
    // - if data was loaded, then load the mesh
    // - if mesh was loaded, then do nothing
    if(this->MeshFile->FileName.find("xgc.mesh.bp") == vtkstd::string::npos)
      {
      // we have the field file
      vtkstd::string meshFileName = "";
      vtkstd::string::size_type i0 = this->MeshFile->FileName.rfind("xgc.");
      vtkstd::string::size_type i1 = this->MeshFile->FileName.rfind(".bp");

      if (i0 != vtkstd::string::npos && i1 != vtkstd::string::npos)
        {
        meshFileName = this->MeshFile->FileName.substr(0,i0+4) + "mesh.bp";
        this->DataFile = this->MeshFile;
        this->MeshFile = new AdiosFile(meshFileName.c_str());
        this->MeshFile->Open();
        }
      }
    }
  // --------------------------------------------------------------------------
  void FillOutput(vtkDataObject* output)
    {
    vtkMultiBlockDataSet* multiBlock = vtkMultiBlockDataSet::SafeDownCast(output);
    if(this->MeshFile->IsPixieFileType())
      {
      // Pixie output
      // block 0 <=> RectilinearGrid
      // block 1 <=> StructuredGrid
      vtkMultiBlockDataSet* pixieOutput = multiBlock;
      vtkDataSet* rectilinearGrid = NULL;
      vtkDataSet* structuredGrid = NULL;

      int realTimeStep = (int) this->TimeStep;

      AdiosVariableMapIterator iter = this->MeshFile->Variables.begin();
      for(; iter != this->MeshFile->Variables.end(); iter++)
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


      vtkDataArray* array = this->MeshFile->ReadVariable(var->Name.c_str(), realTimeStep);
      switch(this->MeshFile->GetDataType(var->Name.c_str())) // FIXME method is not correct !!!!!!!!!!!1
        {
        case VTK_RECTILINEAR_GRID:
          if(!rectilinearGrid)  // Create if needed
            rectilinearGrid = this->MeshFile->GetPixieRectilinearGrid(realTimeStep);
          rectilinearGrid->GetCellData()->AddArray(array);
          break;
        case VTK_STRUCTURED_GRID:
          if(!structuredGrid)  // Create if needed
            structuredGrid = this->MeshFile->GetPixieStructuredGrid(var->Name.c_str(), realTimeStep);
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
      pixieOutput->SetBlock(0, rectilinearGrid);
      rectilinearGrid->FastDelete();
      }
      if(structuredGrid)
      {
      structuredGrid->GetInformation()->Set( vtkDataObject::DATA_TIME_STEPS(),
                                             &this->TimeStep, 1);
      pixieOutput->SetBlock(1, structuredGrid);
      structuredGrid->FastDelete();
      }
      pixieOutput->GetInformation()->Set( vtkDataObject::DATA_TIME_STEPS(),
                                     &this->TimeStep, 1);
      }
    else // We suppose that only XGC file format is the other
      {
      // XGC output
      // block 0 <=> UnstructuredGrid
      vtkMultiBlockDataSet* xgcOutput = multiBlock;
      vtkUnstructuredGrid* unstructuredGrid = this->MeshFile->GetXGCMesh();
      if(this->DataFile) this->DataFile->AddPointDataToXGCMesh(unstructuredGrid);
      xgcOutput->SetBlock(0, unstructuredGrid);
      unstructuredGrid->FastDelete();
      }
    }
  // --------------------------------------------------------------------------
  int GetNumberOfTimeSteps()
    {
    int nbTime = 0;
    if(this->DataFile)
      {
      nbTime = this->DataFile->GetNumberOfTimeSteps();
      if(nbTime == -1)
        return 0;
      }
    nbTime = this->MeshFile->GetNumberOfTimeSteps();
    if(nbTime == -1)
      return 0;
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
  bool IsPixieFormat()
    {
    return this->MeshFile->IsPixieFileType();
    }
  // --------------------------------------------------------------------------
private:
  AdiosFile* MeshFile;
  AdiosFile* DataFile;
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
  vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());
  this->Internal->UpdateFileName(this->GetFileName());
  this->Internal->FillOutput(output);
  //output->PrintSelf(cout, vtkIndent(10));
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
