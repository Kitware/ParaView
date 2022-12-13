/*=========================================================================

  Plugin:   DigitalSignalProcessing
  Module:   vtkSoundQuantitiesCalculator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSoundQuantitiesCalculator.h"

#include "vtkAccousticUtilities.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkTriangle.h"
#include "vtkTriangleFilter.h"

#include <cmath>
#include <numeric>

//-----------------------------------------------------------------------------
namespace details
{
/**
 * Compute the surfacic integral of points data over a mesh.
 * Cells are expected to be triangles.
 *
 * @warning This is not a perfectly right surfacic integral as we're squaring up the values
 * fetched from @c data when integrating over the surface.
 *
 * @todo this should be moved in a more appropriate place in VTK, when the squared up value
 * thing can be factorized out (we could imagine a fonctor acting as a modifier in the input)
 */
double SurfacicIntegral(vtkPoints* points, vtkCellArray* cells, vtkDoubleArray* data)
{
  constexpr double third = 1.0 / 3.0;
  double integral = 0.0;
  vtkNew<vtkIdList> cellPts;
  vtkNew<vtkTriangle> triangle;

  for (vtkIdType tri = 0; tri < cells->GetNumberOfCells(); ++tri)
  {
    cells->GetCellAtId(tri, cellPts);
    double sumOverTriangle = 0;
    for (vtkIdType i = 0; i < 3; ++i)
    {
      const double& val = data->GetValue(cellPts->GetId(i));
      sumOverTriangle += val * val;
    }
    triangle->Initialize(3, cellPts->begin(), points);

    integral += third * triangle->ComputeArea() * sumOverTriangle;
  }

  return integral;
}
} // namespace details

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSoundQuantitiesCalculator);

//-----------------------------------------------------------------------------
vtkSoundQuantitiesCalculator::vtkSoundQuantitiesCalculator()
{
  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
void vtkSoundQuantitiesCalculator::SetSourceData(vtkMultiBlockDataSet* input)
{
  this->SetInputData(1, input);
}

//------------------------------------------------------------------------------
void vtkSoundQuantitiesCalculator::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
vtkMultiBlockDataSet* vtkSoundQuantitiesCalculator::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return vtkMultiBlockDataSet::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//------------------------------------------------------------------------------
int vtkSoundQuantitiesCalculator::FillInputPortInformation(int port, vtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
      return 1;
    case 1:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
      return 1;
    default:
      return 0;
  }
}

//------------------------------------------------------------------------------
int vtkSoundQuantitiesCalculator::RequestData(vtkInformation* /*request*/,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Retrieve inputs
  vtkDataSet* inputMesh = vtkDataSet::GetData(inputVector[0]);
  vtkMultiBlockDataSet* mbInput = vtkMultiBlockDataSet::GetData(inputVector[1]);

  if (!inputMesh || !mbInput)
  {
    vtkErrorMacro("At least one of the two inputs was not found.");
    return 0;
  }

  // Check pressure array name
  if (this->PressureArrayName.empty() || this->PressureArrayName == "None")
  {
    vtkErrorMacro("Pressure array must be specified.");
    return 0;
  }

  // Copy input geometry to output (do not copy data because the output is not temporal anymore)
  vtkDataSet* output = vtkDataSet::GetData(outputVector, 0);
  output->CopyStructure(inputMesh);

  // Compute every quantity we want and add them to the output
  if (this->ComputeMeanPressure)
  {
    if (!this->ProcessData(inputMesh, mbInput, output))
    {
      vtkErrorMacro("Data processing failed.");
      return 0;
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSoundQuantitiesCalculator::ProcessData(
  vtkDataSet* inputMesh, vtkMultiBlockDataSet* inputTables, vtkDataSet* output)
{
  // Retrieve number of timesteps from first block
  vtkTable* table = vtkTable::SafeDownCast(inputTables->GetBlock(0));

  if (!table)
  {
    vtkErrorMacro("Source blocks should be of type vtkTable.");
    return 0;
  }

  // Retrieve number of blocks (equal to the number of points)
  const vtkIdType numOfBlocks = inputTables->GetNumberOfBlocks();

  // Define output arrays
  constexpr const char* pMeanName = "Mean Pressure (Pa)";
  constexpr const char* pRmsNamePa = "RMS Pressure (Pa)";
  constexpr const char* pRmsNameDb = "RMS Pressure (dB)";
  constexpr const char* powerName = "Acoustic Power (dB)";

  vtkNew<vtkDoubleArray> pMeanArray;
  pMeanArray->SetNumberOfValues(numOfBlocks);
  pMeanArray->SetName(pMeanName);

  vtkNew<vtkDoubleArray> pRmsPaArray;
  pRmsPaArray->SetNumberOfValues(numOfBlocks);
  pRmsPaArray->SetName(pRmsNamePa);

  vtkNew<vtkDoubleArray> pRmsDbArray;
  pRmsDbArray->SetNumberOfValues(numOfBlocks);
  pRmsDbArray->SetName(pRmsNameDb);

  for (vtkIdType i = 0; i < numOfBlocks; ++i)
  {
    table = vtkTable::SafeDownCast(inputTables->GetBlock(i));
    if (!table)
    {
      vtkErrorMacro("Source blocks should be of type vtkTable.");
      return 0;
    }

    vtkDataArray* pressureArray =
      vtkDataArray::SafeDownCast(table->GetColumnByName(this->PressureArrayName.c_str()));
    if (!pressureArray)
    {
      vtkErrorMacro("Could not find array named " << this->PressureArrayName << ".");
      return 0;
    }

    // Compute mean pressure
    const auto pressureRange = vtk::DataArrayValueRange(pressureArray);
    const double mean =
      std::accumulate(pressureRange.cbegin(), pressureRange.cend(), 0.0) / pressureRange.size();
    pMeanArray->SetValue(i, mean);

    // Compute additional derived quantities
    if (this->ComputeRMSPressure)
    {
      double rms = 0.0;
      for (double pressure : pressureRange)
      {
        const double delta = mean - pressure;
        rms += delta * delta;
      }
      rms = std::sqrt(rms / pressureRange.size());

      pRmsPaArray->SetValue(i, rms);
      pRmsDbArray->SetValue(i, 20.0 * std::log10(rms / vtkAccousticUtilities::REF_PRESSURE));
    }
  }

  // Add mean pressure to output
  output->GetPointData()->AddArray(pMeanArray);

  if (this->ComputeRMSPressure)
  {
    output->GetPointData()->AddArray(pRmsPaArray);
    output->GetPointData()->AddArray(pRmsDbArray);

    // Compute acoustic power. This needs a surfacic integral
    if (this->ComputePower)
    {
      vtkPolyData* poly = vtkPolyData::SafeDownCast(inputMesh);
      if (!poly)
      {
        vtkWarningMacro("Cannot compute acoustic power without a polyData");
      }
      else
      {
        vtkNew<vtkDoubleArray> acousticPower;
        acousticPower->SetNumberOfValues(1);
        acousticPower->SetName(powerName);
        vtkNew<vtkTriangleFilter> triangleFilter;
        triangleFilter->SetInputData(poly);
        triangleFilter->Update();
        vtkPolyData* processed = triangleFilter->GetOutput();

        const double mediumFactor = 1.0 / (this->MediumDensity * this->MediumSoundVelocity);
        const double power =
          details::SurfacicIntegral(processed->GetPoints(), processed->GetPolys(), pRmsPaArray) *
          mediumFactor;
        acousticPower->SetValue(0, 10.0 * std::log10(power / vtkAccousticUtilities::REF_POWER));

        output->GetFieldData()->AddArray(acousticPower);
      }
    }
  }
  else if (this->ComputePower)
  {
    vtkWarningMacro("Cannot compute acoustic power without computing the RMS");
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSoundQuantitiesCalculator::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Pressure Array Name:" << this->PressureArrayName << std::endl;
  os << indent << "Medium Density:" << this->MediumDensity << std::endl;
  os << indent << "Medium Sound Velocity:" << this->MediumSoundVelocity << std::endl;
  os << indent << "Compute Mean Pressure:" << this->ComputeMeanPressure << std::endl;
  os << indent << "Compute RMS Pressure:" << this->ComputeRMSPressure << std::endl;
  os << indent << "Compute Acoustic Power:" << this->ComputePower << std::endl;
}
