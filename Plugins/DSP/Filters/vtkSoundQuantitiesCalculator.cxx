// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSoundQuantitiesCalculator.h"

#include "vtkAccousticUtilities.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkDSPIterator.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
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
void vtkSoundQuantitiesCalculator::SetSourceData(vtkDataSet* source)
{
  this->SetInputData(1, source);
}

//------------------------------------------------------------------------------
void vtkSoundQuantitiesCalculator::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
int vtkSoundQuantitiesCalculator::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    return 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
int vtkSoundQuantitiesCalculator::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[1]);

  if (!input)
  {
    vtkErrorMacro("Missing input!");
    return 0;
  }

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  if (!output || !output->IsA(input->GetClassName()))
  {
    vtkSmartPointer<vtkDataSet> newOutput = vtk::TakeSmartPointer(input->NewInstance());
    info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkSoundQuantitiesCalculator::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // Output is not temporal
  auto outInfo = outputVector->GetInformationObject(0);
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  return 1;
}

//------------------------------------------------------------------------------
int vtkSoundQuantitiesCalculator::RequestData(vtkInformation* /*request*/,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputTables = vtkDataObject::GetData(inputVector[0]);
  vtkDataSet* inputMesh = vtkDataSet::GetData(inputVector[1]);
  if (!inputMesh || !inputTables)
  {
    vtkErrorMacro("Missing Input or source");
    return 0;
  }

  // Check pressure array name
  if (this->PressureArrayName.empty())
  {
    vtkErrorMacro("Pressure array must be specified.");
    return 0;
  }

  // Copy input geometry and arrays to output
  vtkDataSet* output = vtkDataSet::GetData(outputVector, 0);
  output->CopyStructure(inputMesh);
  output->CopyAttributes(inputMesh);

  // Compute every quantity we want and add them to the output
  if (this->ComputeMeanPressure)
  {
    if (!this->ProcessData(inputMesh, inputTables, output))
    {
      vtkErrorMacro("Data processing failed.");
      return 0;
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSoundQuantitiesCalculator::ProcessData(
  vtkDataSet* inputMesh, vtkDataObject* inputTables, vtkDataSet* output)
{

  auto dspIterator = vtkDSPIterator::GetInstance(inputTables);
  if (!dspIterator)
  {
    vtkErrorMacro("Unable to generate iterator!");
    return 0;
  }

  vtkIdType nPoints = inputMesh->GetNumberOfPoints();
  dspIterator->GoToFirstItem();
  if (dspIterator->IsDoneWithTraversal() || nPoints == 0)
  {
    // Silent return with both empty input, warn if not both
    if (!(dspIterator->IsDoneWithTraversal() && nPoints == 0))
    {
      vtkWarningMacro("Unexpected incoherent partially empty inputs, results may be incorrect");
    }
    return 1;
  }

  // Define output arrays
  constexpr const char* pMeanName = "Mean Pressure (Pa)";
  constexpr const char* pRmsNamePa = "RMS Pressure (Pa)";
  constexpr const char* pRmsNameDb = "RMS Pressure (dB)";
  constexpr const char* powerName = "Acoustic Power (dB)";

  vtkNew<vtkDoubleArray> pMeanArray;
  pMeanArray->SetNumberOfValues(nPoints);
  pMeanArray->SetName(pMeanName);

  vtkNew<vtkDoubleArray> pRmsPaArray;
  vtkNew<vtkDoubleArray> pRmsDbArray;

  if (this->ComputeRMSPressure)
  {
    pRmsPaArray->SetNumberOfValues(nPoints);
    pRmsPaArray->SetName(pRmsNamePa);
    pRmsDbArray->SetNumberOfValues(nPoints);
    pRmsDbArray->SetName(pRmsNameDb);
  }

  vtkIdType cnt = 0;
  for (dspIterator->GoToFirstItem(); !dspIterator->IsDoneWithTraversal();
       dspIterator->GoToNextItem())
  {
    vtkTable* table = dspIterator->GetCurrentTable();
    if (!table)
    {
      // An empty table should correspond to an *absent* point, do not increase counter
      continue;
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
    pMeanArray->SetValue(cnt, mean);

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

      pRmsPaArray->SetValue(cnt, rms);
      pRmsDbArray->SetValue(cnt, 20.0 * std::log10(rms / vtkAccousticUtilities::REF_PRESSURE));
    }
    cnt++;
  }

  if (cnt != nPoints)
  {
    vtkWarningMacro("Iteration over the two inputs has not completed because of a dimensional "
                    "issue, result may be incorrect");
  }

  // Remove temporal pressure array if present
  output->GetPointData()->RemoveArray(this->PressureArrayName.c_str());

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
