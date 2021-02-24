/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPGenericIOMultiBlockWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPGenericIOMultiBlockWriter.h"

#include "GenericIOWriter.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeUInt64Array.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnstructuredGrid.h"

#include "vtkGenericIOUtilities.h"

//----------------------------------------------------------------------------
class vtkPGenericIOMultiBlockWriter::vtkInternals
{
public:
  vtkInternals() { this->Writer = nullptr; }

  ~vtkInternals()
  {
    if (this->Writer != nullptr)
    {
      delete this->Writer;
    }
  }

  gio::GenericIOWriter* Writer;
};

vtkStandardNewMacro(vtkPGenericIOMultiBlockWriter);
//----------------------------------------------------------------------------
vtkPGenericIOMultiBlockWriter::vtkPGenericIOMultiBlockWriter()
{
  this->Internals = new vtkInternals;
  this->Controller = vtkMultiProcessController::GetGlobalController();
  this->FileName = nullptr;
}

//----------------------------------------------------------------------------
vtkPGenericIOMultiBlockWriter::~vtkPGenericIOMultiBlockWriter()
{
  vtkGenericIOUtilities::SafeDeleteString(this->FileName);
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkPGenericIOMultiBlockWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkPGenericIOMultiBlockWriter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
static inline void addCoordinates(
  std::map<std::pair<int, std::string>, std::vector<char> >& dataArrays,
  gio::GenericIOWriter* writer, vtkUnstructuredGrid* grid, uint64_t blockId)
{
  std::string xName = "x", yName = "y", zName = "z";
  while (grid->GetPointData()->HasArray(xName.c_str()))
  {
    xName += "x";
  }
  while (grid->GetPointData()->HasArray(yName.c_str()))
  {
    yName += "y";
  }
  while (grid->GetPointData()->HasArray(zName.c_str()))
  {
    zName += "z";
  }
  std::pair<int, std::string> x(blockId, xName);
  std::pair<int, std::string> y(blockId, yName);
  std::pair<int, std::string> z(blockId, zName);
  dataArrays[x] = std::vector<char>();
  dataArrays[y] = std::vector<char>();
  dataArrays[z] = std::vector<char>();
  std::vector<char>& xData = dataArrays[x];
  std::vector<char>& yData = dataArrays[y];
  std::vector<char>& zData = dataArrays[z];
  if (!writer->HasVariable(xName.c_str()))
  {
    writer->AddVariable(
      xName.c_str(), gio::GENERIC_IO_DOUBLE_TYPE, gio::GenericIOWriter::ValueIsPhysCoordX);
  }
  if (!writer->HasVariable(yName.c_str()))
  {
    writer->AddVariable(
      yName.c_str(), gio::GENERIC_IO_DOUBLE_TYPE, gio::GenericIOWriter::ValueIsPhysCoordY);
  }
  if (!writer->HasVariable(zName.c_str()))
  {
    writer->AddVariable(
      zName.c_str(), gio::GENERIC_IO_DOUBLE_TYPE, gio::GenericIOWriter::ValueIsPhysCoordZ);
  }
  vtkIdType numPoints = grid->GetNumberOfPoints();
  xData.resize(numPoints * sizeof(double));
  yData.resize(numPoints * sizeof(double));
  zData.resize(numPoints * sizeof(double));
  double* xArray = (double*)&xData[0];
  double* yArray = (double*)&yData[0];
  double* zArray = (double*)&zData[0];
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    double pt[3];
    grid->GetPoint(i, pt);
    xArray[i] = pt[0];
    yArray[i] = pt[1];
    zArray[i] = pt[2];
  }
  writer->AddDataForVariableInBlock(x.first, x.second, xArray);
  writer->AddDataForVariableInBlock(y.first, y.second, yArray);
  writer->AddDataForVariableInBlock(z.first, z.second, zArray);
}

//----------------------------------------------------------------------------
static inline gio::GenericIOPrimitiveTypes getGIOTypeFor(vtkDataArray* array)
{
  if (vtkDoubleArray::FastDownCast(array) != nullptr)
  {
    return gio::GENERIC_IO_DOUBLE_TYPE;
  }
  else if (vtkFloatArray::FastDownCast(array) != nullptr)
  {
    return gio::GENERIC_IO_FLOAT_TYPE;
  }
  else if (vtkIntArray::FastDownCast(array) != nullptr)
  {
    return gio::GENERIC_IO_INT32_TYPE;
  }
  else if (vtkTypeInt64Array::FastDownCast(array) != nullptr)
  {
    return gio::GENERIC_IO_INT64_TYPE;
  }
  else if (vtkUnsignedIntArray::FastDownCast(array) != nullptr)
  {
    return gio::GENERIC_IO_UINT32_TYPE;
  }
  else if (vtkTypeUInt64Array::FastDownCast(array) != nullptr)
  {
    return gio::GENERIC_IO_UINT64_TYPE;
  }
  return gio::GENERIC_IO_UCHAR_TYPE;
}

//----------------------------------------------------------------------------
static inline void computeDataForArray(
  vtkDataArray* array, char* data, int indexInTuple, gio::GenericIOPrimitiveTypes type)
{
  switch (type)
  {
    case gio::GENERIC_IO_DOUBLE_TYPE:
    {
      double* ptr = (double*)data;
      vtkDoubleArray* darray = vtkDoubleArray::FastDownCast(array);
      for (int j = 0; j < array->GetNumberOfTuples(); ++j)
      {
        ptr[j] = darray->GetTuple(j)[indexInTuple];
      }
    }
    break;
    case gio::GENERIC_IO_FLOAT_TYPE:
    {
      float* ptr = (float*)data;
      vtkFloatArray* darray = vtkFloatArray::FastDownCast(array);
      for (int j = 0; j < array->GetNumberOfTuples(); ++j)
      {
        ptr[j] = darray->GetTuple(j)[indexInTuple];
      }
    }
    break;
    case gio::GENERIC_IO_INT32_TYPE:
    {
      int* ptr = (int*)data;
      vtkIntArray* darray = vtkIntArray::FastDownCast(array);
      for (int j = 0; j < array->GetNumberOfTuples(); ++j)
      {
        ptr[j] = darray->GetTuple(j)[indexInTuple];
      }
    }
    break;
    case gio::GENERIC_IO_INT64_TYPE:
    {
      long long* ptr = (long long*)data;
      vtkTypeInt64Array* darray = vtkTypeInt64Array::FastDownCast(array);
      for (int j = 0; j < array->GetNumberOfTuples(); ++j)
      {
        ptr[j] = darray->GetTuple(j)[indexInTuple];
      }
    }
    break;
    case gio::GENERIC_IO_UINT32_TYPE:
    {
      unsigned int* ptr = (unsigned int*)data;
      vtkUnsignedIntArray* darray = vtkUnsignedIntArray::FastDownCast(array);
      for (int j = 0; j < array->GetNumberOfTuples(); ++j)
      {
        ptr[j] = darray->GetTuple(j)[indexInTuple];
      }
    }
    break;
    case gio::GENERIC_IO_UINT64_TYPE:
    {
      unsigned long long* ptr = (unsigned long long*)data;
      vtkTypeUInt64Array* darray = vtkTypeUInt64Array::FastDownCast(array);
      for (int j = 0; j < array->GetNumberOfTuples(); ++j)
      {
        ptr[j] = darray->GetTuple(j)[indexInTuple];
      }
    }
    break;
    default:
    case gio::GENERIC_IO_UCHAR_TYPE:
    {
      unsigned char* ptr = (unsigned char*)data;
      for (int j = 0; j < array->GetNumberOfTuples(); ++j)
      {
        ptr[j] = (unsigned char)array->GetTuple(j)[indexInTuple];
      }
    }
    break;
  }
}

//----------------------------------------------------------------------------
static inline void addArray(vtkDataArray* array, gio::GenericIOWriter* writer, int blockId,
  std::map<std::pair<int, std::string>, std::vector<char> >& dataBuffers)
{
  if (array != nullptr)
  {
    gio::GenericIOPrimitiveTypes type = getGIOTypeFor(array);
    // if it is a scalar array, just use the existing data
    if (array->GetNumberOfComponents() == 1)
    {
      if (!writer->HasVariable(array->GetName()))
      {
        writer->AddVariable(array->GetName(), type);
      }
      writer->AddDataForVariableInBlock(blockId, array->GetName(), array->GetVoidPointer(0));
    }

    // if it is a vector, compute three component arrays
    else if (array->GetNumberOfComponents() == 3)
    {
      char const* suffix[] = { "_x", "_y", "_z" };
      for (int i = 0; i < 3; ++i)
      {
        std::string newName = std::string(array->GetName()) + suffix[i];
        if (!writer->HasVariable(newName))
        {
          writer->AddVariable(newName, type);
        }
        std::pair<int, std::string> pr(blockId, newName);
        const gio::VariableHeader& varInfo = writer->GetHeaderForVariable(newName);
        dataBuffers[pr] = std::vector<char>();
        dataBuffers[pr].resize(varInfo.Size * array->GetNumberOfTuples());
        char* data = &dataBuffers[pr][0];
        computeDataForArray(array, data, i, type);
        writer->AddDataForVariableInBlock(blockId, newName, data);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPGenericIOMultiBlockWriter::WriteData()
{
  vtkMultiBlockDataSet* input = vtkMultiBlockDataSet::SafeDownCast(GetInput());
  if (input == nullptr)
  {
    vtkErrorMacro(<< "Input to the vtkPGenericIOMultiBlockWriter must be a vtkMultiBlockDataSet.");
    return;
  }
  if (this->Internals->Writer)
  {
    delete this->Internals->Writer;
  }
  this->Internals->Writer = vtkGenericIOUtilities::GetWriter(
    vtkGenericIOUtilities::GetMPICommunicator(this->Controller), this->FileName);
  std::map<std::pair<int, std::string>, std::vector<char> > dataArrays;
  vtkFieldData* fieldData = input->GetFieldData();
  if (fieldData->HasArray("genericio_phys_origin"))
  {
    vtkDoubleArray* originArray =
      vtkDoubleArray::FastDownCast(fieldData->GetArray("genericio_phys_origin"));
    if (originArray)
    {
      double origin[3];
      originArray->GetTypedTuple(0, origin);
      this->Internals->Writer->SetPhysOrigin(origin);
    }
    else
    {
      vtkWarningMacro(<< "genericio_phys_origin field data array is not of type double, no origin "
                         "information will be encoded in the file.");
    }
  }
  else
  {
    vtkWarningMacro(<< "genericio_phys_origin field data array is missing, no origin information "
                       "will be encoded in the file.");
  }
  if (fieldData->HasArray("genericio_phys_scale"))
  {
    vtkDoubleArray* scaleArray =
      vtkDoubleArray::FastDownCast(fieldData->GetArray("genericio_phys_scale"));
    if (scaleArray)
    {
      double scale[3];
      scaleArray->GetTypedTuple(0, scale);
      this->Internals->Writer->SetPhysScale(scale);
    }
    else
    {
      vtkWarningMacro(<< "genericio_phys_scale field data array is not of type double, no scale "
                         "information will be encoded in the file.");
    }
  }
  else
  {
    vtkWarningMacro(<< "genericio_phys_scale field data array is missing, no scale information "
                       "will be encoded in the file.");
  }
  if (fieldData->HasArray("genericio_global_dimensions"))
  {
    vtkUnsignedLongLongArray* dimsArray =
      vtkUnsignedLongLongArray::FastDownCast(fieldData->GetArray("genericio_global_dimensions"));
    if (dimsArray)
    {
      uint64_t dims[3];
      assert(sizeof(uint64_t) == sizeof(unsigned long long));
      dimsArray->GetTypedTuple(0, (unsigned long long*)dims);
      this->Internals->Writer->SetGlobalDimensions(dims);
    }
    else
    {
      vtkWarningMacro(<< "genericio_global_dimension field data array is not of type unsigned long "
                         "long (uint64_t), this data will not be encoded in the output file.");
    }
  }
  else
  {
    vtkWarningMacro(<< "genericio_global_dimension field data array is missing, this data will not "
                       "be encoded in the output file.");
  }
  if (this->Controller->GetNumberOfProcesses() > 1)
  {
    std::vector<int> hasBlock, duplicates;
    hasBlock.resize(input->GetNumberOfBlocks(), 0);
    duplicates.resize(hasBlock.size());
    for (unsigned i = 0; i < input->GetNumberOfBlocks(); ++i)
    {
      vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(input->GetBlock(i));
      if (grid != nullptr)
      {
        hasBlock[i] = 1;
      }
    }
    this->Controller->AllReduce(
      &hasBlock[0], &duplicates[0], hasBlock.size(), vtkCommunicator::SUM_OP);
    for (size_t i = 0; i < duplicates.size(); ++i)
    {
      if (duplicates[i] > 1)
      {
        vtkErrorMacro(
          << "Blocks are duplicated across processes, this writer expects"
             " each block to be present on exactly one process.  Aborting write operation.");
        this->SetErrorCode(78); // I have no idea what to put here, so 78 it is!
        return;
      }
    }
  }
  for (unsigned i = 0; i < input->GetNumberOfBlocks(); ++i)
  {
    vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(input->GetBlock(i));
    if (grid != nullptr)
    {
      uint64_t coords[3] = { 0, 0, i }; // TODO get real coordinates, default to this
      vtkFieldData* blockFD = grid->GetFieldData();
      if (blockFD->HasArray("genericio_block_coords"))
      {
        vtkUnsignedLongLongArray* coordsArray =
          vtkUnsignedLongLongArray::FastDownCast(blockFD->GetArray("genericio_block_coords"));
        if (coordsArray)
        {
          assert(sizeof(uint64_t) == sizeof(unsigned long long));
          coordsArray->GetTypedTuple(0, (unsigned long long*)coords);
        }
        else
        {
          vtkWarningMacro(
            << "genericio_block_coords field data array on block " << i
            << " is not of type unsigned long long, block coords defaulting to [0, 0, " << i
            << "].");
        }
      }
      else
      {
        vtkWarningMacro(<< "genericio_block_coords field data array is missing for block " << i
                        << ", block coords defaulting to [0, 0, " << i << "].");
      }
      this->Internals->Writer->AddLocalBlock(i, grid->GetNumberOfPoints(), coords);
      addCoordinates(dataArrays, this->Internals->Writer, grid, i);
      vtkPointData* pointData = grid->GetPointData();
      for (int j = 0; j < pointData->GetNumberOfArrays(); ++j)
      {
        vtkDataArray* array = vtkDataArray::FastDownCast(pointData->GetAbstractArray(j));
        addArray(array, this->Internals->Writer, i, dataArrays);
      }
    }
  }
  this->Internals->Writer->Write();
}
