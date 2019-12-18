/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkUnstructuredPOPReader.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredPOPReader.h"
#include "vtkCallbackCommand.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkCleanUnstructuredGrid.h"
#include "vtkDataArraySelection.h"
#include "vtkExtentTranslator.h"
#include "vtkFloatArray.h"
#include "vtkGradientFilter.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkMultiProcessController.h"
#endif

#include "vtk_netcdf.h"
#include "vtksys/SystemTools.hxx"

#include <algorithm>
#include <iterator>
#include <set>
#include <string>
#include <vector>

namespace
{
//-----------------------------------------------------------------------------
// If the grid is wrapped, we need to merge the point values for the
// shared points that exist on the same process.
template <class DataType>
void FixArrayForGridWrapping(DataType* array, size_t* start, size_t* count, ptrdiff_t* rStride,
  int numberOfDimensions, int fileId, int varidp)
{
  // first get the new values
  DataType newValues[count[0] * count[1]];
  size_t newCount[3] = { count[0], count[1], 1 };
  size_t newStart[3] = { start[0], start[1], 0 };
  // last newStride shouldn't matter since we're only reading 1 entry in that dimension anyways
  ptrdiff_t newStride[3] = { rStride[0], rStride[1], 1 };
  int offset = numberOfDimensions == 2 ? 1 : 0;
  nc_get_vars_float(fileId, varidp, newStart + offset, newCount + offset, newStride, newValues);

  // shift the values and replace
  DataType value;
  if (numberOfDimensions == 3)
  {
    // keep a running tab of the the amount each tuple has to shift
    vtkIdType shiftAmount = static_cast<vtkIdType>(count[0] * count[1]) - 1;
    for (vtkIdType iz = static_cast<vtkIdType>(count[0]) - 1; iz >= 0; iz--)
    {
      for (vtkIdType iy = static_cast<vtkIdType>(count[1]) - 1; iy >= 0; iy--)
      {
        if (shiftAmount != 0)
        {
          for (vtkIdType ix = static_cast<vtkIdType>(count[2]) - 1; ix >= 0; ix--)
          {
            vtkIdType index = ix + iy * count[2] + iz * count[1] * count[2];
            std::copy(array + index, array + index + 1, array + index + shiftAmount);
            if (shiftAmount < 0)
            {
              vtkGenericWarningMacro("Problem shifting values");
            }
          }
        }
        // this lat/depth line is shifted so I can now add in the value
        array[count[2] + iy * (count[2] + 1) + iz * (count[2] + 1) * count[1]] =
          newValues[iy + count[1] * iz];
        shiftAmount--;
      }
    }
  }
  else if (numberOfDimensions == 2)
  {
    vtkIdType shiftAmount =
      static_cast<vtkIdType>(count[1]) - 1; // the amount each tuple has to shift
    for (vtkIdType iy = static_cast<vtkIdType>(count[1]) - 1; iy >= 0; iy--)
    {
      if (shiftAmount != 0)
      {
        for (vtkIdType ix = static_cast<vtkIdType>(count[2]) - 1; ix >= 0; ix--)
        {
          vtkIdType index = ix + iy * count[2];
          std::copy(array + index, array + index + 1, array + index + shiftAmount);
          if (shiftAmount < 0)
          {
            vtkGenericWarningMacro("Problem shifting values");
          }
        }
      }
      // this lat/depth line is shifted so I can now add in the value
      array[count[2] + iy * (count[2] + 1)] = newValues[iy];
      shiftAmount--;
    }
  }
}

//-----------------------------------------------------------------------------
// convert a group of scalar arrays into a vector (3 component) array
void ConvertScalarsToVector(vtkPointData* pointData, std::vector<std::string>& scalarArrayNames)
{
  vtkFloatArray* vectorArray = vtkFloatArray::New();
  vectorArray->SetName("velocity");
  vectorArray->SetNumberOfComponents(3);
  vtkIdType numberOfTuples = pointData->GetArray(scalarArrayNames[0].c_str())->GetNumberOfTuples();
  vectorArray->SetNumberOfTuples(numberOfTuples);
  pointData->AddArray(vectorArray);
  vectorArray->Delete();
  std::vector<vtkFloatArray*> scalarArrays;
  for (std::vector<std::string>::iterator it = scalarArrayNames.begin();
       it != scalarArrayNames.end(); it++)
  {
    scalarArrays.push_back(vtkFloatArray::SafeDownCast(pointData->GetArray(it->c_str())));
  }
  float values[3] = { 0, 0, 0 };
  for (vtkIdType i = 0; i < numberOfTuples; i++)
  {
    for (size_t j = 0; j < scalarArrays.size(); j++)
    {
      scalarArrays[j]->GetTypedTuple(i, values + j);
    }
    if (values[0] < -1e+31)
    {
      values[0] = values[1] = values[2] = 0;
    }
    vectorArray->SetTypedTuple(i, values);
  }
  // remove the old arrays
  for (std::vector<std::string>::iterator it = scalarArrayNames.begin();
       it != scalarArrayNames.end(); it++)
  {
    pointData->RemoveArray(it->c_str());
  }
}

//-----------------------------------------------------------------------------
// Given local ijk indices (in 0 to numberOfI, numberOfJ, numberOfK instead of
// in WholeExtent, return the corresponding POP index.
size_t GetPOPIndexFromGridIndices(
  int numDimensions, size_t* count, size_t* start, size_t* stride, int i, int j, int k)
{
  if (numDimensions == 3)
  {
    size_t iIndex = start[2] + i * stride[2];
    size_t jIndex = start[1] + j * stride[1];
    size_t kIndex = start[0] + k * stride[0];
    return iIndex + jIndex * count[2] + kIndex * count[2] * count[1];
  }
  size_t iIndex = start[1] + i * stride[1];
  size_t jIndex = start[0] + j * stride[0];
  return iIndex + jIndex * count[1];
}

//-----------------------------------------------------------------------------
// Given a point, compute the direction cosines assuming that the point
// is also a vector starting from (0,0,0).
void ComputeDirectionCosines(const double point[3], double directionCosines[3][3])
{
  memcpy(directionCosines[0], point, sizeof(double) * 3);
  // first direction is aligned with the radial direction, i.e. the one we want
  vtkMath::Normalize(directionCosines[0]);
  // the other 2 directions are arbitrarily set. i don't really care
  // what those directions are as long as they're perpendicular
  vtkMath::Perpendiculars(directionCosines[0], directionCosines[1], directionCosines[2], 0);
}

} // end anonymous namespace

vtkStandardNewMacro(vtkUnstructuredPOPReader);

//============================================================================
#define CALL_NETCDF(call)                                                                          \
  {                                                                                                \
    int errorcode = call;                                                                          \
    if (errorcode != NC_NOERR)                                                                     \
    {                                                                                              \
      vtkErrorMacro(<< "netCDF Error: " << nc_strerror(errorcode));                                \
      return 0;                                                                                    \
    }                                                                                              \
  }

//============================================================================
class vtkUnstructuredPOPReaderInternal
{
public:
  vtkSmartPointer<vtkDataArraySelection> VariableArraySelection;
  // a mapping from the list of all variables to the list of available
  // point-based variables
  std::vector<int> VariableMap;
  vtkUnstructuredPOPReaderInternal()
  {
    this->VariableArraySelection = vtkSmartPointer<vtkDataArraySelection>::New();
  }
};

//----------------------------------------------------------------------------
// set default values
vtkUnstructuredPOPReader::vtkUnstructuredPOPReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->FileName = NULL;
  this->OpenedFileName = NULL;
  this->Stride[0] = this->Stride[1] = this->Stride[2] = 1;
  this->NCDFFD = 0;
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkUnstructuredPOPReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->Internals = new vtkUnstructuredPOPReaderInternal;
  this->Internals->VariableArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this->SelectionObserver);
  // radius of the earth in meters assuming the earth is a perfect
  // sphere.  it isn't.  the range going from 6,353 km to 6,384 km.
  // we can revisit this later.
  this->Radius = 6371000;
  for (int i = 0; i < 3; i++)
  {
    this->VOI[i * 2] = 0;
    this->VOI[i * 2 + 1] = -1;
  }
  this->SubsettingXMin = this->SubsettingXMax = false;
  this->ReducedHeightResolution = false;
  this->VectorGrid = 0;
  this->VerticalVelocity = false;
}

//----------------------------------------------------------------------------
// delete filename and netcdf file descriptor
vtkUnstructuredPOPReader::~vtkUnstructuredPOPReader()
{
  this->SetFileName(0);
  if (this->OpenedFileName)
  {
    nc_close(this->NCDFFD);
    this->SetOpenedFileName(NULL);
  }
  if (this->SelectionObserver)
  {
    this->SelectionObserver->Delete();
    this->SelectionObserver = NULL;
  }
  if (this->Internals)
  {
    delete this->Internals;
    this->Internals = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkUnstructuredPOPReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->FileName ? this->FileName : "(NULL)") << endl;
  os << indent << "OpenedFileName: " << (this->OpenedFileName ? this->OpenedFileName : "(NULL)")
     << endl;
  os << indent << "Stride: {" << this->Stride[0] << ", " << this->Stride[1] << ", "
     << this->Stride[2] << ", "
     << "}" << endl;
  os << indent << "NCDFFD: " << this->NCDFFD << endl;

  this->Internals->VariableArraySelection->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
bool vtkUnstructuredPOPReader::ReadMetaData(int wholeExtent[6])
{
  if (this->FileName == NULL)
  {
    vtkErrorMacro("FileName not set.");
    return false;
  }

  if (this->OpenedFileName == NULL || strcmp(this->OpenedFileName, this->FileName) != 0)
  {
    if (this->OpenedFileName)
    {
      nc_close(this->NCDFFD);
      this->SetOpenedFileName(NULL);
    }
    int retval = nc_open(this->FileName, NC_NOWRITE, &this->NCDFFD); // read file
    if (retval != NC_NOERR)                                          // checks if read file error
    {
      // we don't need to close the file if there was an error opening the file
      vtkErrorMacro(<< "Can't read file " << nc_strerror(retval));
      return false;
    }
    this->SetOpenedFileName(this->FileName);
  }
  // get number of variables from file
  int numberOfVariables;
  nc_inq_nvars(this->NCDFFD, &numberOfVariables);
  int dimidsp[NC_MAX_VAR_DIMS]; // NC_MAX_VAR_DIMS comes from the nc library
  int dataDimension;
  size_t dimensions[4]; // dimension value
  this->Internals->VariableMap.resize(numberOfVariables);
  char variableName[NC_MAX_NAME + 1];
  int actualVariableCounter = 0;
  // For every variable in the file
  for (int i = 0; i < numberOfVariables; i++)
  {
    this->Internals->VariableMap[i] = -1;
    // get number of dimensions
    CALL_NETCDF(nc_inq_varndims(this->NCDFFD, i, &dataDimension));
    // Variable Dimension ID's containing x,y,z coords for the rectilinear
    // grid spacing
    CALL_NETCDF(nc_inq_vardimid(this->NCDFFD, i, dimidsp));
    if (dataDimension == 3)
    {
      this->Internals->VariableMap[i] = actualVariableCounter++;
      // get variable name
      CALL_NETCDF(nc_inq_varname(this->NCDFFD, i, variableName));
      this->Internals->VariableArraySelection->AddArray(variableName);
      for (int m = 0; m < dataDimension; m++)
      {
        CALL_NETCDF(nc_inq_dimlen(this->NCDFFD, dimidsp[m], dimensions + m));
        // acquire variable dimensions
      }
      wholeExtent[0] = wholeExtent[2] = wholeExtent[4] = 0; // set extent
      wholeExtent[1] = static_cast<int>((dimensions[2] - 1) / this->Stride[0]);
      wholeExtent[3] = static_cast<int>((dimensions[1] - 1) / this->Stride[1]);
      wholeExtent[5] = static_cast<int>((dimensions[0] - 1) / this->Stride[2]);
    }
  }
  this->SubsettingXMin = this->SubsettingXMax = false;
  if (this->VOI[1] != -1 || this->VOI[3] != -1 || this->VOI[5] != -1)
  {
    for (int i = 0; i < 3; i++)
    {
      if (wholeExtent[2 * i] < this->VOI[2 * i] && this->VOI[2 * i] <= wholeExtent[2 * i + 1])
      {
        wholeExtent[2 * i] = this->VOI[2 * i];
        if (i == 0)
        {
          this->SubsettingXMin = true;
        }
      }
      if (wholeExtent[2 * i + 1] > this->VOI[2 * i + 1] &&
        this->VOI[2 * i + 1] >= wholeExtent[2 * i])
      {
        wholeExtent[2 * i + 1] = this->VOI[2 * i + 1];
        if (i == 0)
        {
          this->SubsettingXMax = true;
        }
      }
    }
  }
  if (wholeExtent[4] != 0 || wholeExtent[5] != static_cast<int>(dimensions[0] - 1) ||
    this->Stride[2] != 1)
  {
    this->ReducedHeightResolution = true;
  }

  return true;
}

//-----------------------------------------------------------------------------
int vtkUnstructuredPOPReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Let the superclass do the heavy lifting.
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
  {
    return 0;
  }

  // We handle unstructured "extents" (pieces).
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkUnstructuredPOPReader::RequestData(vtkInformation* request,
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  this->UpdateProgress(0);
  // the default implementation is to do what the old pipeline did find what
  // output is requesting the data, and pass that into ExecuteData
  // which output port did the request come from
  int outputPort = request->Get(vtkDemandDrivenPipeline::FROM_OUTPUT_PORT());
  // if output port is negative then that means this filter is calling the
  // update directly, in that case just assume port 0
  if (outputPort == -1)
  {
    outputPort = 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(outputPort);
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numberOfPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int numberOfGhostLevels =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  vtkNew<vtkUnstructuredGrid> tempGrid;
  int retVal = this->ProcessGrid(tempGrid.GetPointer(), piece, numberOfPieces, numberOfGhostLevels);
  vtkNew<vtkCleanUnstructuredGrid> cleanToGrid;
  cleanToGrid->SetInputData(tempGrid.GetPointer());
  cleanToGrid->Update();
  // we use the following to get the output grid instead of this->GetOutput() since the
  // vtkInformation object passed in here may be different than the vtkInformation
  // object used in GetOutput() to get the grid pointer.
  vtkUnstructuredGrid* outputGrid =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  outputGrid->ShallowCopy(cleanToGrid->GetOutput());

  return retVal;
}

//----------------------------------------------------------------------------
int vtkUnstructuredPOPReader::ProcessGrid(
  vtkUnstructuredGrid* grid, int piece, int numberOfPieces, int numberOfGhostLevels)
{
  // Determine if the point data is going to be scalar or vector fields
  std::string fileName = this->FileName;
  size_t position = fileName.find("UVEL");
  this->VectorGrid = 1;
  int netCDFFD = -1; // netcdf file descriptor for VVEL, also used at the end of this method
  if (position != std::string::npos)
  {
    fileName.replace(position, 4, "VVEL");
    int retVal = nc_open(fileName.c_str(), NC_NOWRITE, &netCDFFD);
    if (NC_NOERR != retVal)
    {
      // we don't need to close the file if there was an error opening the file
      vtkErrorMacro(<< "Can't read file " << nc_strerror(retVal));
      netCDFFD = -1; // this may not be needed
    }
    else
    {
      int varidp = -1;
      char variableName[] = "VVEL";
      if (NC_NOERR == nc_inq_varid(netCDFFD, variableName, &varidp))
      {
        this->VectorGrid = 2;
      }
      else
      {
        vtkWarningMacro("Could not find VVEL variable");
        nc_close(netCDFFD);
        netCDFFD = -1;
      }
    }
  }
  if (this->VectorGrid == 2 && this->VerticalVelocity == true && numberOfPieces > 1)
  {
    // we need to compute gradients in order to compute the vertical
    // velocity component and to make sure that the we avoid partitioning
    // effects we add a level of ghosts.
    numberOfGhostLevels += 1;
  }
  int subExtent[6], wholeExtent[6];
  this->GetExtentInformation(piece, numberOfPieces, numberOfGhostLevels, wholeExtent, subExtent);

  // setup extents for netcdf library to read the netcdf data file
  size_t start[] = { static_cast<size_t>(subExtent[4] * this->Stride[2]),
    static_cast<size_t>(subExtent[2] * this->Stride[1]),
    static_cast<size_t>(subExtent[0] * this->Stride[0]) };

  // the points do not wrap.  e.g. for the lowest latitude the longitude
  // starts at -179.9 and finishes at 180.0
  // wrapped keeps track of whether or not this proc wraps.  this really
  // keeps track of whether or not we add on the extra cell in the x-dir
  // or not without adding extending the points in the x-direction.
  int wrapped = static_cast<int>(!this->SubsettingXMin && !this->SubsettingXMax &&
    subExtent[0] == wholeExtent[0] && subExtent[1] == wholeExtent[1]);

  size_t count[] = { static_cast<size_t>(subExtent[5] - subExtent[4] + 1),
    static_cast<size_t>(subExtent[3] - subExtent[2] + 1),
    static_cast<size_t>(subExtent[1] - subExtent[0] + 1) };

  ptrdiff_t rStride[3] = { (ptrdiff_t) this->Stride[2], (ptrdiff_t) this->Stride[1],
    (ptrdiff_t) this->Stride[0] };

  // initialize memory (raw data space, x y z axis space) and rectilinear grid
  bool firstPass = true;
  for (size_t i = 0; i < this->Internals->VariableMap.size(); i++)
  {
    if (this->Internals->VariableMap[i] != -1 &&
      this->Internals->VariableArraySelection->GetArraySetting(this->Internals->VariableMap[i]) !=
        0)
    {
      // varidp is probably i in which case nc_inq_varid isn't needed
      int varidp;
      nc_inq_varid(this->NCDFFD,
        this->Internals->VariableArraySelection->GetArrayName(this->Internals->VariableMap[i]),
        &varidp);

      if (firstPass == true)
      {
        int dimidsp[3];
        nc_inq_vardimid(this->NCDFFD, varidp, dimidsp);
        firstPass = false;
        std::vector<float> z(count[0]);
        std::vector<float> y(count[1]);
        std::vector<float> x(count[2]);
        // gets data from x,y,z axis (spacing)
        nc_get_vars_float(this->NCDFFD, dimidsp[0], start, count, rStride, &(z[0]));
        nc_get_vars_float(this->NCDFFD, dimidsp[1], start + 1, count + 1, rStride + 1, &(y[0]));
        nc_get_vars_float(this->NCDFFD, dimidsp[2], start + 2, count + 2, rStride + 2, &(x[0]));

        vtkNew<vtkPoints> points;
        grid->SetPoints(points.GetPointer());
        points->SetDataTypeToFloat();
        points->SetNumberOfPoints(count[0] * count[1] * count[2]);
        float point[3];
        vtkIntArray* indexArray = vtkIntArray::New();
        indexArray->SetNumberOfComponents(2);
        indexArray->SetNumberOfTuples(points->GetNumberOfPoints());
        indexArray->SetName("indices");
        int ind[2];
        for (size_t iz = 0; iz < count[0]; iz++)
        {
          point[2] = -z[iz];
          for (size_t iy = 0; iy < count[1]; iy++)
          {
            point[1] = y[iy];
            ind[1] = static_cast<int>(iy);
            for (size_t ix = 0; ix < count[2]; ix++)
            {
              point[0] = x[ix];
              ind[0] = static_cast<int>(ix);
              vtkIdType id = ix + iy * count[2] + iz * count[2] * count[1];
              indexArray->SetTypedTuple(id, ind);
              points->SetPoint(id, point);
            }
          }
        }
        grid->GetPointData()->AddArray(indexArray);
        indexArray->Delete();

        // need to create the cells
        vtkIdType pointIds[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        // we make sure that value is at least 1 so that we can do both quads and hexes
        size_t count2Plus = std::max(count[2], static_cast<size_t>(1));
        size_t count1Plus = std::max(count[1], static_cast<size_t>(1));
        grid->Allocate(std::max(count[0] - 1, static_cast<size_t>(1)) *
          std::max(count[1] - 1, static_cast<size_t>(1)) *
          std::max(count[2] - 1 + wrapped, static_cast<size_t>(1)));
        size_t iz = 0;
        do // make sure we loop through iz once
        {
          size_t iy = 0;
          do // make sure we loop through iy once
          {
            size_t ix = 0;
            do // make sure we loop through ix once
            {
              pointIds[0] = ix + iy * count2Plus + (1 + iz) * count2Plus * count1Plus;
              pointIds[1] = 1 + ix + iy * count2Plus + (1 + iz) * count2Plus * count1Plus;
              pointIds[2] = 1 + ix + (1 + iy) * count2Plus + (1 + iz) * count2Plus * count1Plus;
              pointIds[3] = ix + (1 + iy) * count2Plus + (1 + iz) * count2Plus * count1Plus;
              pointIds[4] = ix + iy * count2Plus + iz * count2Plus * count1Plus;
              pointIds[5] = 1 + ix + iy * count2Plus + iz * count2Plus * count1Plus;
              pointIds[6] = 1 + ix + (1 + iy) * count2Plus + iz * count2Plus * count1Plus;
              pointIds[7] = ix + (1 + iy) * count2Plus + iz * count2Plus * count1Plus;

              if (wrapped && ix == count[2] - 1)
              {
                pointIds[1] = iy * count2Plus + (1 + iz) * count2Plus * count1Plus;
                pointIds[2] = (1 + iy) * count2Plus + (1 + iz) * count2Plus * count1Plus;
                pointIds[5] = iy * count2Plus + iz * count2Plus * count1Plus;
                pointIds[6] = (1 + iy) * count2Plus + iz * count2Plus * count1Plus;
              }

              if (count[0] < 2)
              { // constant depth/logical z
                grid->InsertNextCell(VTK_QUAD, 4, pointIds + 4);
              }
              else if (count[1] < 2)
              { // constant latitude/logical y
                pointIds[6] = pointIds[1];
                pointIds[7] = pointIds[0];
                grid->InsertNextCell(VTK_QUAD, 4, pointIds + 4);
              }
              else if (count[2] < 2)
              { // constant longitude/logical x
                pointIds[6] = pointIds[0];
                pointIds[7] = pointIds[1];
                grid->InsertNextCell(VTK_QUAD, 4, pointIds + 4);
              }
              else
              {
                grid->InsertNextCell(VTK_HEXAHEDRON, 8, pointIds);
              }
              ix++;
            } while (ix < count[2] - 1 + wrapped);
            iy++;
          } while (iy < count[1] - 1);
          iz++;
        } while (iz < count[0] - 1);
      } // done creating the points and cells
      // create vtkFloatArray and get the scalars into it
      this->LoadPointData(grid, this->NCDFFD, varidp, start, count, rStride,
        this->Internals->VariableArraySelection->GetArrayName(this->Internals->VariableMap[i]));
    }
    this->UpdateProgress((i + 1.0) / this->Internals->VariableMap.size());
  }

  if (netCDFFD != -1)
  {
    int varidp = -1;
    char variableName[] = "VVEL";
    if (NC_NOERR == nc_inq_varid(netCDFFD, variableName, &varidp))
    {
      this->LoadPointData(grid, netCDFFD, varidp, start, count, rStride, variableName);
      std::vector<std::string> scalarArrayNames;
      scalarArrayNames.push_back("UVEL");
      scalarArrayNames.push_back("VVEL");
      ConvertScalarsToVector(grid->GetPointData(), scalarArrayNames);
      this->VectorGrid = 2;
    }
    nc_close(netCDFFD);
  }

  // transform from logical tripolar coordinates to a sphere.  also transforms
  // any vector quantities
  this->Transform(grid, start, count, wholeExtent, subExtent, numberOfGhostLevels, wrapped, piece,
    numberOfPieces);

  return 1;
}

//----------------------------------------------------------------------------
void vtkUnstructuredPOPReader::SelectionModifiedCallback(
  vtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<vtkUnstructuredPOPReader*>(clientdata)->Modified();
}

//-----------------------------------------------------------------------------
int vtkUnstructuredPOPReader::GetNumberOfVariableArrays()
{
  return this->Internals->VariableArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkUnstructuredPOPReader::GetVariableArrayName(int index)
{
  if (index < 0 || index >= this->GetNumberOfVariableArrays())
  {
    return NULL;
  }
  return this->Internals->VariableArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkUnstructuredPOPReader::GetVariableArrayStatus(const char* name)
{
  return this->Internals->VariableArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
void vtkUnstructuredPOPReader::SetVariableArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if (this->Internals->VariableArraySelection->ArrayExists(name) == 0)
  {
    vtkErrorMacro(<< name << " is not available in the file.");
    return;
  }
  int enabled = this->Internals->VariableArraySelection->ArrayIsEnabled(name);
  if (status != 0 && enabled == 0)
  {
    this->Internals->VariableArraySelection->EnableArray(name);
    this->Modified();
  }
  else if (status == 0 && enabled != 0)
  {
    this->Internals->VariableArraySelection->DisableArray(name);
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
bool vtkUnstructuredPOPReader::Transform(vtkUnstructuredGrid* grid, size_t* start, size_t* count,
  int* wholeExtent, int* subExtent, int numberOfGhostLevels, int wrapped, int piece,
  int numberOfPieces)
{
  if (this->VectorGrid != 1 && this->VectorGrid != 2)
  {
    vtkErrorMacro("Don't know if this should be a scalar or vector field grid.");
    return 0;
  }

  int latlonFileId = 0;
  std::string gridFileName = vtksys::SystemTools::GetFilenamePath(this->FileName) + "/GRID.nc";
  int retval = nc_open(gridFileName.c_str(), NC_NOWRITE, &latlonFileId);
  if (retval != NC_NOERR) // checks if read file error
  {
    // we don't need to close the file if there was an error opening the file
    vtkErrorMacro(<< "Can't read file " << nc_strerror(retval));
    return 0;
  }

  int varidp;
  if (this->VectorGrid == 2)
  {
    nc_inq_varid(latlonFileId, "U_LON_2D", &varidp);
  }
  else
  {
    nc_inq_varid(latlonFileId, "T_LON_2D", &varidp);
  }
  int dimensionIds[3];
  nc_inq_vardimid(latlonFileId, varidp, dimensionIds);
  size_t zeros[2] = { 0, 0 };
  size_t dimensions[3];
  nc_inq_dimlen(latlonFileId, dimensionIds[0], dimensions);
  nc_inq_dimlen(latlonFileId, dimensionIds[1], dimensions + 1);
  size_t latlonCount[2] = { dimensions[0], dimensions[1] };

  std::vector<float> realLongitude(dimensions[0] * dimensions[1]);
  nc_get_vara_float(latlonFileId, varidp, zeros, latlonCount, &(realLongitude[0]));

  if (this->VectorGrid == 2)
  {
    nc_inq_varid(latlonFileId, "U_LAT_2D", &varidp);
  }
  else
  {
    nc_inq_varid(latlonFileId, "T_LAT_2D", &varidp);
  }

  std::vector<float> realLatitude(dimensions[0] * dimensions[1]);
  nc_get_vara_float(latlonFileId, varidp, zeros, latlonCount, &(realLatitude[0]));

  nc_inq_varid(latlonFileId, "depth_t", &varidp);
  nc_inq_vardimid(latlonFileId, varidp, dimensionIds + 2);
  nc_inq_dimlen(latlonFileId, dimensionIds[2], dimensions + 2);

  std::vector<float> realHeight(dimensions[2]);
  ptrdiff_t stride = static_cast<ptrdiff_t>(this->Stride[2]);
  nc_get_vars_float(latlonFileId, varidp, start, count, &stride, &(realHeight[0]));

  // the vector arrays that need to be manipulated
  std::vector<vtkFloatArray*> vectorArrays;
  for (int i = 0; i < grid->GetPointData()->GetNumberOfArrays(); i++)
  {
    if (vtkFloatArray* array = vtkFloatArray::SafeDownCast(grid->GetPointData()->GetArray(i)))
    {
      if (array->GetNumberOfComponents() == 3)
      {
        vectorArrays.push_back(array);
      }
    }
  }

  size_t rStride[2] = { (size_t) this->Stride[1], (size_t) this->Stride[0] };

  vtkPoints* points = grid->GetPoints();

  for (size_t j = 0; j < count[1]; j++) // y index
  {
    for (size_t k = 0; k < count[0]; k++) // z index
    {
      for (size_t i = 0; i < count[2]; i++) // x index
      {
        vtkIdType index = i + j * count[2] + k * count[2] * count[1];
        if (index >= points->GetNumberOfPoints())
        {
          vtkErrorMacro("doooh");
        }
        size_t latlonIndex = GetPOPIndexFromGridIndices(2, dimensions, start + 1, rStride,
          static_cast<int>(i), static_cast<int>(j), static_cast<int>(k));
        if (latlonIndex >= dimensions[0] * dimensions[1])
        {
          vtkErrorMacro("Bad lat-lon index.");
        }
        double point[3];
        points->GetPoint(index, point);
        point[0] = realLongitude[latlonIndex];
        point[1] = realLatitude[latlonIndex];

        // convert to spherical
        double radius = this->Radius - realHeight[k];
        double lonRadians = vtkMath::RadiansFromDegrees(point[0]);
        double latRadians = vtkMath::RadiansFromDegrees(point[1]);
        bool sphere = true;
        if (sphere)
        {
          point[0] = radius * cos(latRadians) * cos(lonRadians);
          point[1] = radius * cos(latRadians) * sin(lonRadians);
          point[2] = radius * sin(latRadians);
        }
        points->SetPoint(index, point);

        for (std::vector<vtkFloatArray*>::iterator vit = vectorArrays.begin();
             vit != vectorArrays.end(); vit++)
        {
          float values[3];
          (*vit)->GetTypedTuple(index, values);

          size_t startIndex = latlonIndex;
          size_t endIndex = latlonIndex + 1;
          if (start[2] + i * rStride[1] >= dimensions[1] - 2)
          {
            startIndex = latlonIndex - 1;
            endIndex = latlonIndex;
          }
          float vals[3];

          double startLon = vtkMath::RadiansFromDegrees(realLongitude[startIndex]);
          double startLat = vtkMath::RadiansFromDegrees(realLatitude[startIndex]);

          double startPos[3] = { radius * cos(startLat) * cos(startLon),
            radius * cos(startLat) * sin(startLon), radius * sin(startLat) };

          double endLon = vtkMath::RadiansFromDegrees(realLongitude[endIndex]);
          double endLat = vtkMath::RadiansFromDegrees(realLatitude[endIndex]);

          double endPos[3] = { radius * cos(endLat) * cos(endLon),
            radius * cos(endLat) * sin(endLon), radius * sin(endLat) };

          double norm = sqrt((endPos[0] - startPos[0]) * (endPos[0] - startPos[0]) +
            (endPos[1] - startPos[1]) * (endPos[1] - startPos[1]) +
            (endPos[2] - startPos[2]) * (endPos[2] - startPos[2]));

          vals[0] = values[0] * (endPos[0] - startPos[0]) / norm;
          vals[1] = values[0] * (endPos[1] - startPos[1]) / norm;
          vals[2] = values[0] * (endPos[2] - startPos[2]) / norm;

          startIndex = latlonIndex;
          endIndex = latlonIndex + dimensions[1];
          if (start[1] + j * rStride[0] >= dimensions[0] - 2)
          {
            startIndex = latlonIndex - dimensions[1];
            endIndex = latlonIndex;
          }
          startLon = vtkMath::RadiansFromDegrees(realLongitude[startIndex]);
          startLat = vtkMath::RadiansFromDegrees(realLatitude[startIndex]);

          startPos[0] = radius * cos(startLat) * cos(startLon);
          startPos[1] = radius * cos(startLat) * sin(startLon);
          startPos[2] = radius * sin(startLat);

          endLon = vtkMath::RadiansFromDegrees(realLongitude[endIndex]);
          endLat = vtkMath::RadiansFromDegrees(realLatitude[endIndex]);

          endPos[0] = radius * cos(endLat) * cos(endLon);
          endPos[1] = radius * cos(endLat) * sin(endLon);
          endPos[2] = radius * sin(endLat);

          norm = sqrt((endPos[0] - startPos[0]) * (endPos[0] - startPos[0]) +
            (endPos[1] - startPos[1]) * (endPos[1] - startPos[1]) +
            (endPos[2] - startPos[2]) * (endPos[2] - startPos[2]));

          vals[0] += values[1] * (endPos[0] - startPos[0]) / norm;
          vals[1] += values[1] * (endPos[1] - startPos[1]) / norm;
          vals[2] += values[1] * (endPos[2] - startPos[2]) / norm;

          (*vit)->SetTypedTuple(index, vals);
        }
      }
    }
  }

  bool retVal = this->BuildGhostInformation(
    grid, numberOfGhostLevels, wholeExtent, subExtent, wrapped, piece, numberOfPieces);
  if (this->VectorGrid && this->VerticalVelocity && this->ReducedHeightResolution == false)
  {
    this->ComputeVerticalVelocity(grid, wholeExtent, subExtent, numberOfGhostLevels, latlonFileId);
    if (vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses() > 1)
    {
      // the last layer of ghost cells was added in order to do the vertical velocity calculation.
      // it needs to be removed now.
      // TODO: Berk
      // This needs to be fixed
      // grid->RemoveGhostCells(numberOfGhostLevels);
    }
  }

  nc_close(latlonFileId);
  return retVal;
}

//-----------------------------------------------------------------------------
// iterate over depth columns and then over the points in a depth column.
// the iteration starts at the bottom cell and ends at the highest cell in the
// column (consequently from a high id to a lower id).  this ignores non-fluid
// cells.  To iterate over, do:
// for(vtkIdType column=iter.BeginColumn();column!=iter.EndColumn();column=iter.NextColumn())
//    for(int k=pointIterator.GetColumnBottomPointExtentIndex(true);
//        k>=pointIterator.GetColumnTopPointExtentIndex(true);k--)
//      vtkIdType id = iter.GetCurrentId(k); // get the current point id
class VTKPointIterator
{
public:
  VTKPointIterator(int* wholeExtent, int* subExtent, int* stride, int latlonFileId)
  {
    memcpy(this->WholeExtent, wholeExtent, sizeof(int) * 6);
    memcpy(this->SubExtent, subExtent, sizeof(int) * 6);

    int varidp;
    nc_inq_varid(latlonFileId, "KMT", &varidp);
    int dimensionIds[2];
    nc_inq_vardimid(latlonFileId, varidp, dimensionIds);
    size_t zeros[2] = { 0, 0 };
    nc_inq_dimlen(latlonFileId, dimensionIds[0], this->HorizontalDimensions);
    nc_inq_dimlen(latlonFileId, dimensionIds[1], this->HorizontalDimensions + 1);

    // the index of the deepest cell to have fluid in it.
    // i probably don't need this whole array
    this->DeepestCellIndex.resize(this->HorizontalDimensions[0] * this->HorizontalDimensions[1]);
    nc_get_vara_int(
      latlonFileId, varidp, zeros, this->HorizontalDimensions, &(this->DeepestCellIndex[0]));

    memcpy(this->Stride, stride, sizeof(int) * 3);
    this->CurrentColumn = 0;
  }
  // initialize the total iteration and specify the beginning iteration value
  // for the column
  vtkIdType BeginColumn()
  {
    this->CurrentColumn = 0;
    return this->CurrentColumn;
  }
  // return the number of columns
  vtkIdType EndColumn()
  {
    return (this->SubExtent[1] - this->SubExtent[0] + 1) *
      (this->SubExtent[3] - this->SubExtent[2] + 1);
  }
  // increment the current column and reset the column index
  vtkIdType NextColumn() { return ++this->CurrentColumn; }

  // Give the i and j extent indices of this column.  Returns
  // false if the iterator is in a bad state.
  bool GetCurrentColumnExtentIndices(int& iIndex, int& jIndex)
  {
    int iDimension = this->SubExtent[1] - this->SubExtent[0] + 1;
    int jDimension = this->SubExtent[3] - this->SubExtent[2] + 1;
    if (this->CurrentColumn >= iDimension * jDimension)
    {
      vtkGenericWarningMacro("Bad column state.");
      return false;
    }
    iIndex = (this->CurrentColumn % iDimension) + this->SubExtent[0];
    jIndex = (this->CurrentColumn / iDimension) + this->SubExtent[2];
    return true;
  }
  // Given a pair of horizontal indices, set the current column
  // iterator location. Returns true if successful.
  bool SetColumn(int iIndex, int jIndex)
  {
    if (iIndex < this->SubExtent[0] || iIndex > this->SubExtent[1] || jIndex < this->SubExtent[2] ||
      jIndex > this->SubExtent[3])
    {
      vtkGenericWarningMacro("Bad indices "
        << iIndex << " " << jIndex << " proc "
        << vtkMultiProcessController::GetGlobalController()->GetLocalProcessId());
      return false;
    }
    int iDimension = this->SubExtent[1] - this->SubExtent[0] + 1;
    this->CurrentColumn = iIndex - this->SubExtent[0] + (jIndex - this->SubExtent[2]) * iDimension;
    return true;
  }
  // Return whether or not the current column is a ghost point
  // used by the reader only and removed before finishing request data.
  bool IsColumnAReaderGhost()
  {
    int iIndex, jIndex;
    if (this->GetCurrentColumnExtentIndices(iIndex, jIndex) == false)
    { // this probably isn't exactly correct but we should treat this as a ghost
      return true;
    }
    if ((this->SubExtent[0] + 1 > iIndex) && (this->WholeExtent[0] != this->SubExtent[0]))
    {
      return true;
    }
    if (this->SubExtent[1] - 1 < iIndex && this->WholeExtent[1] != this->SubExtent[1])
    {
      return true;
    }
    if (this->SubExtent[2] + 1 > jIndex && this->WholeExtent[2] != this->SubExtent[2])
    {
      return true;
    }
    if (this->SubExtent[3] - 1 < jIndex && this->WholeExtent[3] != this->SubExtent[3])
    {
      return true;
    }
    return false;
  }
  // return the bottom (i.e. smallest radius) k extent index.
  // includeReaderGhost specifies whether or not to include points
  // that are ghosts that will be deleted after the depth computation
  // is finished.
  int GetColumnBottomPointExtentIndex(bool includeReaderGhost)
  {
    if (includeReaderGhost == true || this->SubExtent[5] == this->WholeExtent[5])
    {
      return this->SubExtent[5];
    }
    return this->SubExtent[5] - 1;
  }
  // return the top (i.e. largest radiues) k extent index.
  // includeReaderGhost specifies whether or not to include points
  // that are ghosts that will be deleted after the depth computation
  // is finished.
  int GetColumnTopPointExtentIndex(bool includeReaderGhost)
  {
    if (includeReaderGhost == true || this->SubExtent[4] == this->WholeExtent[4])
    {
      return this->SubExtent[4];
    }
    return this->SubExtent[4] + 1;
  }
  // get the k extent index of the point at the bottom
  // of the ocean. this point may be on a different process.
  int GetColumnOceanBottomExtentIndex()
  {
    size_t popIndex = this->GetPOPIndex();
    return this->DeepestCellIndex[popIndex] - 1;
  }
  // for the current column and passed in kIndex,
  // return the point id for this grid. a returned value
  // of -1 indicates a bad column index and/or kExtentIndex.
  vtkIdType GetPointId(int kIndex)
  {
    if (kIndex < this->SubExtent[4] || kIndex > this->SubExtent[5])
    {
      return -1;
    }
    int iIndex, jIndex;
    if (this->GetCurrentColumnExtentIndices(iIndex, jIndex) == false)
    {
      return -1;
    }
    int i = iIndex - this->SubExtent[0];
    int j = jIndex - this->SubExtent[2];
    int k = kIndex - this->SubExtent[4];
    int iDimension = this->SubExtent[1] - this->SubExtent[0] + 1;
    int jDimension = this->SubExtent[3] - this->SubExtent[2] + 1;
    return i + j * iDimension + k * (iDimension * jDimension);
  }

  // return whether or not the column piece on this process
  // includes the point at the bottom of the ocean.  this
  // may not be unique for a given column. includeGhosts
  // is only with respect to the column index.
  bool ColumnPieceHasBottomPoint(bool includeReaderGhost)
  {
    int bottomIndex = this->GetColumnOceanBottomExtentIndex();
    if (includeReaderGhost)
    {
      if (this->SubExtent[4] + 1 > bottomIndex && this->SubExtent[4] != this->WholeExtent[4])
      {
        return false;
      }
      if (this->SubExtent[5] - 1 < bottomIndex && this->SubExtent[5] != this->WholeExtent[5])
      {
        return false;
      }
      return true;
    }
    return (bottomIndex >= this->SubExtent[4] && bottomIndex <= this->SubExtent[5]);
  }

private:
  size_t GetPOPIndex()
  {
    int iDimension = this->SubExtent[1] - this->SubExtent[0] + 1;
    int iIndex = this->CurrentColumn % iDimension;
    int jIndex = this->CurrentColumn / iDimension;
    size_t popIndex = (this->SubExtent[0] + iIndex) * this->Stride[0] +
      (this->SubExtent[2] + jIndex) * this->Stride[1] * this->HorizontalDimensions[1];
    return popIndex;
  }
  int CurrentColumn;
  int SubExtent[6];
  std::vector<int> DeepestCellIndex;
  int Stride[3];
  int WholeExtent[6];
  size_t HorizontalDimensions[2];
};

//-----------------------------------------------------------------------------
void vtkUnstructuredPOPReader::LoadPointData(vtkUnstructuredGrid* grid, int netCDFFD, int varidp,
  size_t* start, size_t* count, ptrdiff_t* rStride, const char* arrayName)
{
  vtkFloatArray* scalars = vtkFloatArray::New();
  vtkIdType numberOfTuples = grid->GetNumberOfPoints();
  float* data = new float[numberOfTuples];
  nc_get_vars_float(netCDFFD, varidp, start, count, rStride, data);
  scalars->SetArray(data, numberOfTuples, 0, 1);
  // set list of variables to display data on grid
  scalars->SetName(arrayName);
  grid->GetPointData()->AddArray(scalars);
  scalars->Delete();
  size_t length = 0;
  if (nc_inq_attlen(netCDFFD, varidp, "units", &length) == NC_NOERR)
  {
    if (length > 0)
    {
      std::string attribute(length, ' ');
      nc_get_att_text(netCDFFD, varidp, "units", &attribute[0]);
      if (attribute == "centimeter/s")
      { // need to scale to meter/s
        for (vtkIdType i = 0; i < numberOfTuples; i++)
        {
          data[i] *= 0.01;
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkUnstructuredPOPReader::ComputeVerticalVelocity(vtkUnstructuredGrid* grid, int* wholeExtent,
  int* subExtent, int numberOfGhostLevels, int latlonFileId)
{
  vtkNew<vtkUnstructuredGrid> tempGrid;
  tempGrid->ShallowCopy(grid);
  vtkNew<vtkGradientFilter> gradientFilter;
  gradientFilter->SetInputData(tempGrid.GetPointer());
  gradientFilter->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
  gradientFilter->Update();
  grid->ShallowCopy(gradientFilter->GetOutput());

  std::vector<double> dwdr(grid->GetNumberOfPoints()); // change in velocity in the radial direction
  VTKPointIterator pointIterator(wholeExtent, subExtent, this->Stride, latlonFileId);
  for (vtkIdType column = pointIterator.BeginColumn(); column != pointIterator.EndColumn();
       column = pointIterator.NextColumn())
  {
    // assuming a sphere, the direction cosines for each column is the same
    // so no need to recompute it if we iterate through this way.
    vtkIdType pointId =
      pointIterator.GetPointId(pointIterator.GetColumnBottomPointExtentIndex(true));
    double point[3];
    grid->GetPoint(pointId, point);
    double directionCosines[3][3];
    ComputeDirectionCosines(point, directionCosines);
    for (int k = pointIterator.GetColumnBottomPointExtentIndex(true);
         k >= pointIterator.GetColumnTopPointExtentIndex(true); k--)
    {
      pointId = pointIterator.GetPointId(k);
      if (pointId < 0 || pointId >= grid->GetNumberOfPoints())
      {
        vtkErrorMacro("Bad point id.");
        continue;
      }
      double gradient[3][3];
      grid->GetPointData()->GetArray("Gradients")->GetTuple(pointId, &gradient[0][0]);
      double tmp1[3][3], tmp2[3][3];
      vtkMath::Multiply3x3(directionCosines, gradient, tmp1);
      vtkMath::Multiply3x3(directionCosines, tmp1, tmp2);
      // assuming incompressible flow du/dx+dv/dy+dw/dz = 0.
      // remember that I already set the first direction to the one I want
      dwdr[pointId] = -tmp2[1][1] - tmp2[2][2];
    }
  }

  // I now have the change in velocity in the direction I want.  It's time
  // to integrate locally.  If this is in parallel, I'll go back later and
  // add in the values from integrations on other procs

  // an array to keep track of the depth.  we only populate it if we need it
  // since it requires file IO
  std::vector<float> w_dep;

  double dZero = 0;
  std::vector<double> w(grid->GetNumberOfPoints(), dZero); // vertical velocity
  for (vtkIdType column = pointIterator.BeginColumn(); column != pointIterator.EndColumn();
       column = pointIterator.NextColumn())
  {
    int oceanBottomExtentIndex = pointIterator.GetColumnOceanBottomExtentIndex();
    int gridTopExtentIndex = pointIterator.GetColumnTopPointExtentIndex(true);
    if (gridTopExtentIndex > oceanBottomExtentIndex)
    {
      continue; // don't need to integrate this column
    }
    // we start from the first non-reader ghost cell since other processes
    // will integrate up to that point and we add that in during the parallel
    // communication
    int gridBottomExtentIndex = pointIterator.GetColumnBottomPointExtentIndex(false);
    int startExtentIndex = (oceanBottomExtentIndex < gridBottomExtentIndex ? oceanBottomExtentIndex
                                                                           : gridBottomExtentIndex);
    // assume no slip/zero velocity at the "bottom".  the bottom is actually
    // a half cell length below the lowest point.
    double integratedVelocity = 0;
    double lastPoint[3] = { 0, 0, 0 };
    vtkIdType pointId = pointIterator.GetPointId(startExtentIndex);
    grid->GetPoint(pointId, lastPoint);
    double lastdwdr = dwdr[pointId];
    if (pointIterator.ColumnPieceHasBottomPoint(true) == true)
    {
      if (w_dep.size() == 0)
      { // this process needs this array so we read it in now
        int varidp;
        nc_inq_varid(latlonFileId, "w_dep", &varidp);
        int dimensionId;
        nc_inq_vardimid(latlonFileId, varidp, &dimensionId);
        size_t zero = 0;
        size_t w_depDimension;
        nc_inq_dimlen(latlonFileId, dimensionId, &w_depDimension);
        w_dep.resize(w_depDimension);
        nc_get_vara_float(latlonFileId, varidp, &zero, &w_depDimension, &(w_dep[0]));
      }
      // assuming no partial cell depths, the bottom cell is only integrated
      // over half of the distance.
      float length =
        (startExtentIndex < 2 ? 0 : .5 * (w_dep[startExtentIndex] - w_dep[startExtentIndex - 1]));
      integratedVelocity = lastdwdr * length;
    }
    w[pointId] = integratedVelocity;
    for (int index = startExtentIndex - 1; index >= gridTopExtentIndex; index--)
    {
      pointId = pointIterator.GetPointId(index);
      double currentPoint[3];
      grid->GetPoint(pointId, currentPoint);
      double currentdwdr = dwdr[pointId];
      double average = (currentdwdr + lastdwdr) * .5;
      double length = sqrt(vtkMath::Distance2BetweenPoints(currentPoint, lastPoint));
      integratedVelocity += average * length;
      w[pointId] = integratedVelocity;
      memcpy(lastPoint, currentPoint, sizeof(double) * 3);
      lastdwdr = currentdwdr;
    }
  }
  if (vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses() > 1)
  {
    this->CommunicateParallelVerticalVelocity(
      wholeExtent, subExtent, numberOfGhostLevels, pointIterator, &w[0]);
  }

  // now w[] should have the proper values and we need to add them back in
  // to the point data array
  vtkDataArray* velocityArray = grid->GetPointData()->GetArray("velocity");
  for (vtkIdType column = pointIterator.BeginColumn(); column != pointIterator.EndColumn();
       column = pointIterator.NextColumn())
  {
    vtkIdType pointId =
      pointIterator.GetPointId(pointIterator.GetColumnBottomPointExtentIndex(true));
    double direction[3];
    grid->GetPoint(pointId, direction);
    vtkMath::Normalize(direction);
    for (int k = pointIterator.GetColumnBottomPointExtentIndex(true);
         k >= pointIterator.GetColumnTopPointExtentIndex(true); k--)
    {
      double velocity[3];
      pointId = pointIterator.GetPointId(k);
      velocityArray->GetTuple(pointId, velocity);
      for (int i = 0; i < 3; i++)
      {
        velocity[i] += w[pointId] * direction[i];
      }
      velocityArray->SetTuple(pointId, velocity);
    }
  }
  grid->GetPointData()->RemoveArray("Gradients");
}

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
//-----------------------------------------------------------------------------
void vtkUnstructuredPOPReader::CommunicateParallelVerticalVelocity(int* wholeExtent, int* subExtent,
  int numberOfGhostLevels, VTKPointIterator& pointIterator, double* w)
{
  if (wholeExtent[4] == subExtent[4] && wholeExtent[5] == subExtent[5])
  {
    // no communication necessary since this process has all the points
    // in each of its vertical columns.
    return;
  }
  vtkMPIController* controller =
    vtkMPIController::SafeDownCast(vtkMultiProcessController::GetGlobalController());

  if (subExtent[5] != wholeExtent[5])
  { // process needs to receive data in order to finish its computation.
    // first determine which procs to receive data from
    std::map<int, int> sendingProcesses;
    for (vtkIdType column = pointIterator.BeginColumn(); column != pointIterator.EndColumn();
         column = pointIterator.NextColumn())
    {
      if (pointIterator.IsColumnAReaderGhost() == false &&
        pointIterator.ColumnPieceHasBottomPoint(true) == false &&
        pointIterator.GetColumnTopPointExtentIndex(true) <
          pointIterator.GetColumnOceanBottomExtentIndex())
      {
        int iIndex = 0, jIndex = 0;
        pointIterator.GetCurrentColumnExtentIndices(iIndex, jIndex);
        int kIndex = pointIterator.GetColumnBottomPointExtentIndex(false);
        int sendingProc = this->GetPointOwnerPiece(iIndex, jIndex, kIndex,
          controller->GetNumberOfProcesses(), numberOfGhostLevels, wholeExtent);
        sendingProcesses[sendingProc]++;
      }
    }
    // now receive and process the data
    for (std::map<int, int>::iterator it = sendingProcesses.begin(); it != sendingProcesses.end();
         it++)
    {
      std::vector<int> iData(it->second * 3);
      std::vector<float> fData(it->second);
      vtkMPICommunicator::Request iRequest, fRequest;
      controller->NoBlockReceive(&iData[0], it->second * 3, it->first, 4837, iRequest);
      controller->NoBlockReceive(&fData[0], it->second, it->first, 4838, fRequest);
      iRequest.Wait();
      fRequest.Wait();
      for (int i = 0; i < it->second; i++)
      {
        pointIterator.SetColumn(iData[i * 3], iData[i * 3 + 1]);
        for (int k = iData[i * 3 + 2]; k >= pointIterator.GetColumnTopPointExtentIndex(false); k--)
        {
          vtkIdType pointId = pointIterator.GetPointId(k);
          w[pointId] += fData[i];
        }
      }
    }
  }

  if (subExtent[4] != wholeExtent[4])
  { // other processes are depending on information from this process
    // a map from piece number/processId to information to be sent.
    // this needs to be done after this process has fully updated it's information
    std::map<vtkIdType, std::vector<int> > sendIndexInfo;
    std::map<vtkIdType, std::vector<float> > sendValueInfo;
    vtkNew<vtkIdList> pieceIds;
    int numberOfPieces = controller->GetNumberOfProcesses();
    for (vtkIdType column = pointIterator.BeginColumn(); column != pointIterator.EndColumn();
         column = pointIterator.NextColumn())
    {
      if (pointIterator.IsColumnAReaderGhost() == false)
      {
        int iIndex = 0, jIndex = 0;
        pointIterator.GetCurrentColumnExtentIndices(iIndex, jIndex);
        int kIndex = pointIterator.GetColumnTopPointExtentIndex(true);
        kIndex += 2 * numberOfGhostLevels - 1;
        vtkIdType pointId = pointIterator.GetPointId(kIndex);
        this->GetPiecesNeedingPoint(iIndex, jIndex, kIndex, numberOfPieces, numberOfGhostLevels,
          wholeExtent, pieceIds.GetPointer());
        for (vtkIdType i = 0; i < pieceIds->GetNumberOfIds(); i++)
        { // don't need to send to myself or if this column is all land
          if (pieceIds->GetId(i) != controller->GetLocalProcessId() &&
            kIndex < pointIterator.GetColumnOceanBottomExtentIndex() &&
            this->GetPointOwnerPiece(iIndex, jIndex, kIndex, controller->GetNumberOfProcesses(),
              numberOfGhostLevels, wholeExtent) == controller->GetLocalProcessId())
          {
            int indices[3] = { iIndex, jIndex, kIndex };
            std::copy(indices, indices + 3, std::back_inserter(sendIndexInfo[pieceIds->GetId(i)]));
            sendValueInfo[pieceIds->GetId(i)].push_back(w[pointId]);
          }
        }
      }
    }
    std::vector<vtkMPICommunicator::Request> requests;
    for (std::map<vtkIdType, std::vector<int> >::iterator it = sendIndexInfo.begin();
         it != sendIndexInfo.end(); it++)
    {
      requests.push_back(vtkMPICommunicator::Request());
      controller->NoBlockSend(&(it->second[0]), static_cast<int>(it->second.size()),
        static_cast<int>(it->first), 4837, requests.back());
      controller->NoBlockSend(&(sendValueInfo[it->first][0]),
        static_cast<int>(it->second.size() / 3), static_cast<int>(it->first), 4838,
        requests.back());
    }
    for (std::vector<vtkMPICommunicator::Request>::iterator it = requests.begin();
         it != requests.end(); it++)
    {
      it->Wait();
    }
  }
}
#else
//-----------------------------------------------------------------------------
void vtkUnstructuredPOPReader::CommunicateParallelVerticalVelocity(int* vtkNotUsed(wholeExtent),
  int* vtkNotUsed(subExtent), int vtkNotUsed(numberOfGhostLevels),
  VTKPointIterator& vtkNotUsed(pointIterator), double* vtkNotUsed(w))
{
}
#endif

//-----------------------------------------------------------------------------
int vtkUnstructuredPOPReader::GetPointOwnerPiece(
  int iIndex, int jIndex, int kIndex, int numberOfPieces, int numberOfGhostLevels, int* wholeExtent)
{
  if (iIndex < wholeExtent[0] || iIndex > wholeExtent[1] || jIndex < wholeExtent[2] ||
    jIndex > wholeExtent[3] || kIndex < wholeExtent[4] || kIndex > wholeExtent[5])
  {
    vtkWarningMacro("Bad indices");
    return -1;
  }
  int subExtent[6];
  for (int piece = 0; piece < numberOfPieces; piece++)
  {
    if (this->GetExtentInformation(
          piece, numberOfPieces, numberOfGhostLevels, wholeExtent, subExtent) == true)
    {
      // shrink the extent in the 'horizontal' direction just a bit
      // because a point shouldn't be owned by pieces where the point
      // is the last/reader ghost level
      for (int i = 0; i < 2; i++)
      {
        if (subExtent[i * 2] != wholeExtent[i * 2])
        {
          subExtent[i * 2]++;
        }
        if (subExtent[i * 2 + 1] != wholeExtent[i * 2 + 1])
        {
          subExtent[i * 2 + 1]--;
        }
      }
      if (iIndex >= subExtent[0] && iIndex <= subExtent[1] && jIndex >= subExtent[2] &&
        jIndex <= subExtent[3] && kIndex == subExtent[4] + 2 * numberOfGhostLevels - 1)
      {
        return piece;
      }
    }
  }
  vtkErrorMacro(<< iIndex << " " << jIndex << " " << kIndex << " Problem finding owner piece "
                << vtkMultiProcessController::GetGlobalController()->GetLocalProcessId());
  return -1;
}

//-----------------------------------------------------------------------------
void vtkUnstructuredPOPReader::GetPiecesNeedingPoint(int iIndex, int jIndex, int kIndex,
  int numberOfPieces, int numberOfGhostLevels, int* wholeExtent, vtkIdList* pieceIds)
{
  pieceIds->Reset();
  if (iIndex < wholeExtent[0] || iIndex > wholeExtent[1] || jIndex < wholeExtent[2] ||
    jIndex > wholeExtent[3] || kIndex < wholeExtent[4] || kIndex > wholeExtent[5])
  {
    vtkWarningMacro("Unexpected indices");
    return;
  }
  // there should be a better way to do this but at least this will work
  // but may be dreadfully slow.  ACBAUER -- fix this!!!!
  int subExtent[6];
  for (int piece = 0; piece < numberOfPieces; piece++)
  {
    if (this->GetExtentInformation(
          piece, numberOfPieces, numberOfGhostLevels, wholeExtent, subExtent) == true)
    {
      // shrink the extent in the 'horizontal' direction just a bit
      // because a point shouldn't be owned by pieces where the point
      // is the last/reader ghost level
      for (int i = 0; i < 2; i++)
      {
        if (subExtent[i * 2] != wholeExtent[i * 2])
        {
          subExtent[i * 2]++;
        }
        if (subExtent[i * 2 + 1] != wholeExtent[i * 2 + 1])
        {
          subExtent[i * 2 + 1]--;
        }
      }
      if (iIndex >= subExtent[0] && iIndex <= subExtent[1] && jIndex >= subExtent[2] &&
        jIndex <= subExtent[3] &&
        kIndex == subExtent[5] - 1) // -1 is for ignoring reader ghost level
      {
        pieceIds->InsertNextId(static_cast<vtkIdType>(piece));
      }
    }
  }
  if (pieceIds->GetNumberOfIds() == 0)
  {
    vtkErrorMacro("Problem finding pieces needing "
      << iIndex << " " << jIndex << " " << kIndex << " "
      << vtkMultiProcessController::GetGlobalController()->GetLocalProcessId());
  }
  return;
}

//-----------------------------------------------------------------------------
bool vtkUnstructuredPOPReader::GetExtentInformation(
  int piece, int numberOfPieces, int numberOfGhostLevels, int* wholeExtent, int* subExtent)
{
  if (this->ReadMetaData(wholeExtent) == false)
  {
    vtkErrorMacro("Error getting meta data.");
    return false;
  }

  vtkNew<vtkExtentTranslator> extentTranslator;
  extentTranslator->SetPiece(piece);
  extentTranslator->SetNumberOfPieces(numberOfPieces);
  // we only split in the y and z topological directions to make
  // wrapping easier for topological y max.
  std::vector<int> splitPath;
  int count = 0;
  int extents[2] = { wholeExtent[3] - wholeExtent[2], wholeExtent[5] - wholeExtent[4] };
  do
  {
    if (extents[0] >= extents[1])
    {
      extents[0] /= 2;
      splitPath.push_back(1);
    }
    else
    {
      extents[1] /= 2;
      splitPath.push_back(2);
    }
    count = (count == 0 ? 1 : count * 2);
  } while (count < numberOfPieces);
  extentTranslator->SetSplitPath(static_cast<int>(splitPath.size()), &(splitPath[0]));

  extentTranslator->SetWholeExtent(wholeExtent);

  extentTranslator->SetGhostLevel(numberOfGhostLevels);
  extentTranslator->PieceToExtent();
  extentTranslator->GetExtent(subExtent);

  return true;
}

//-----------------------------------------------------------------------------
bool vtkUnstructuredPOPReader::BuildGhostInformation(vtkUnstructuredGrid* grid,
  int numberOfGhostLevels, int* wholeExtent, int* subExtent, int wrapped, int piece,
  int numberOfPieces)
{
  int noGhostsSubExtent[6];
  this->GetExtentInformation(piece, numberOfPieces, 0, wholeExtent, noGhostsSubExtent);

  if (numberOfGhostLevels == 0)
  {
    return true;
  }
  vtkUnsignedCharArray* cellGhostArray = vtkUnsignedCharArray::New();
  cellGhostArray->SetName(vtkDataSetAttributes::GhostArrayName());
  cellGhostArray->SetNumberOfTuples(grid->GetNumberOfCells());
  grid->GetCellData()->AddArray(cellGhostArray);
  cellGhostArray->Delete();

  unsigned char* ia = cellGhostArray->GetPointer(0);
  for (vtkIdType i = 0; i < grid->GetNumberOfCells(); i++)
  {
    ia[i] = (unsigned char)0;
  }
  int actualXDimension = subExtent[1] - subExtent[0] + wrapped;
  int k = 0;
  do // k loop with at least 1 pass
  {
    int kGhostLevel = 0;
    if (k < numberOfGhostLevels && noGhostsSubExtent[4] != wholeExtent[4])
    {
      kGhostLevel = numberOfGhostLevels - k;
    }
    else if (k >= subExtent[5] - subExtent[4] - numberOfGhostLevels &&
      noGhostsSubExtent[5] != wholeExtent[5])
    {
      kGhostLevel = k - subExtent[5] + subExtent[4] + numberOfGhostLevels + 1;
    }
    int j = 0;
    do // j loop with at least 1 pass
    {
      int jGhostLevel = 0;
      if (j < numberOfGhostLevels && noGhostsSubExtent[2] != wholeExtent[2])
      {
        jGhostLevel = numberOfGhostLevels - j;
      }
      else if (j >= subExtent[3] - subExtent[2] - numberOfGhostLevels &&
        noGhostsSubExtent[3] != wholeExtent[3])
      {
        jGhostLevel = j - subExtent[3] + subExtent[2] + numberOfGhostLevels + 1;
      }
      unsigned char ghostLevel = static_cast<unsigned char>(std::max(kGhostLevel, jGhostLevel));
      // we never split in the logical x-direction so we'll never
      // need ghost cells in that direction
      if (ghostLevel)
      {
        int i = 0;
        do // i loop with at least 1 pass
        {
          int index = i + j * std::max(actualXDimension, 1) +
            k * std::max(actualXDimension, 1) * std::max(subExtent[3] - subExtent[2], 1);
          if (index < 0 || index >= grid->GetNumberOfCells())
          {
            vtkErrorMacro(<< index << " CELL ghostlevel ERROR " << grid->GetNumberOfCells());
          }
          else
          {
            ia[index] = vtkDataSetAttributes::DUPLICATECELL;
          }
          i++;
        } while (i < actualXDimension);
      }
      j++;
    } while (j < subExtent[3] - subExtent[2]);
    k++;
  } while (k < subExtent[5] - subExtent[4]);

  vtkUnsignedCharArray* pointGhostArray = vtkUnsignedCharArray::New();
  pointGhostArray->SetName(vtkDataSetAttributes::GhostArrayName());
  pointGhostArray->SetNumberOfTuples(grid->GetNumberOfPoints());
  grid->GetPointData()->AddArray(pointGhostArray);
  pointGhostArray->Delete();

  ia = pointGhostArray->GetPointer(0);
  for (vtkIdType i = 0; i < grid->GetNumberOfPoints(); i++)
  {
    ia[i] = (unsigned char)0;
  }
  actualXDimension = subExtent[1] - subExtent[0] + 1;
  k = 0;
  do // k loop with at least 1 pass
  {
    int kGhostLevel = 0;
    if (k < numberOfGhostLevels && subExtent[4] != wholeExtent[4])
    {
      kGhostLevel = numberOfGhostLevels - k;
    }
    else if (k > subExtent[5] - subExtent[4] - numberOfGhostLevels &&
      noGhostsSubExtent[5] != wholeExtent[5])
    {
      kGhostLevel = k - subExtent[5] + subExtent[4] + numberOfGhostLevels;
    }
    int j = 0;
    do // j loop with at least 1 pass
    {
      int jGhostLevel = 0;
      if (j < numberOfGhostLevels && subExtent[2] != wholeExtent[2])
      {
        jGhostLevel = numberOfGhostLevels - j;
      }
      else if (j > subExtent[3] - subExtent[2] - numberOfGhostLevels &&
        noGhostsSubExtent[3] != wholeExtent[3])
      {
        jGhostLevel = j - subExtent[3] + subExtent[2] + numberOfGhostLevels;
      }
      unsigned char ghostLevel = static_cast<unsigned char>(std::max(kGhostLevel, jGhostLevel));
      // we never split in the logical x-direction so we'll never
      // need ghost cells in that direction
      if (ghostLevel)
      {
        int i = 0;
        do // i loop with at least 1 pass
        {
          int index =
            i + j * actualXDimension + k * actualXDimension * (subExtent[3] - subExtent[2] + 1);
          if (index < 0 || index >= grid->GetNumberOfPoints())
          {
            vtkErrorMacro("POINT ghostlevel ERROR");
          }
          else
          {
            ia[index] = vtkDataSetAttributes::DUPLICATEPOINT;
          }
          i++;
        } while (i < actualXDimension);
      }
      j++;
    } while (j < subExtent[3] - subExtent[2] + 1);
    k++;
  } while (k < subExtent[5] - subExtent[4] + 1);

  return true;
}
