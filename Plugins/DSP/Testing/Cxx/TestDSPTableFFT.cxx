// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDSPTableFFT.h"

#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkMathUtilities.h"
#include "vtkMultiDimensionBrowser.h"
#include "vtkSpatioTemporalHarmonicsSource.h"
#include "vtkTable.h"
#include "vtkTableFFT.h"
#include "vtkTemporalMultiplexing.h"

#include <cstdlib>

namespace
{
constexpr double TOL = 1e-6;

bool TableEq(vtkTable* lhs, vtkTable* rhs)
{
  if (!lhs && !rhs)
  {
    return true;
  }

  if (!lhs || !rhs)
  {
    std::cerr << "One of the tables is nullptr but not the other one" << std::endl;
    return false;
  }

  auto lhsRD = lhs->GetRowData();
  auto rhsRD = rhs->GetRowData();
  if (lhsRD->GetNumberOfArrays() != rhsRD->GetNumberOfArrays())
  {
    std::cerr << "Tables have different number of arrays" << std::endl;
    std::cerr << lhsRD->GetNumberOfArrays() << " != " << rhsRD->GetNumberOfArrays() << std::endl;
    return false;
  }

  for (vtkIdType iArr = 0; iArr < lhsRD->GetNumberOfArrays(); ++iArr)
  {
    auto lArr = lhsRD->GetArray(iArr);
    auto rArr = rhsRD->GetArray(iArr);

    if (!lArr && !rArr)
    {
      continue;
    }

    if (!lArr || !rArr)
    {
      std::cerr << "One of the arrays at position " << iArr << " is nullptr but not the other one"
                << std::endl;
      return false;
    }

    if (std::string(lArr->GetName()) != std::string(rArr->GetName()))
    {
      std::cerr << "The arrays at position " << iArr << " do not have the same name" << std::endl;
      return false;
    }

    auto lRange = vtk::DataArrayValueRange(lArr);
    auto rRange = vtk::DataArrayValueRange(rArr);
    if (lRange.size() != rRange.size())
    {
      std::cerr << "The arrays at position " << iArr << " do not have the same size\n"
                << lRange.size() << " != " << rRange.size() << std::endl;
      return false;
    }

    for (vtkIdType iV = 0; iV < lRange.size(); ++iV)
    {
      if (!vtkMathUtilities::FuzzyCompare(
            static_cast<double>(lRange[iV]), static_cast<double>(rRange[iV]), TOL))
      {
        std::cerr << "Array " << iArr << " values disagree at position " << iV << "\n"
                  << lRange[iV] << " != " << rRange[iV] << std::endl;
        return false;
      }
    }
  }
  return true;
}

}

int TestDSPTableFFT(int, char*[])
{
  vtkNew<vtkSpatioTemporalHarmonicsSource> source;

  vtkNew<vtkTemporalMultiplexing> multiplex;
  multiplex->SetInputConnection(source->GetOutputPort(0));
  multiplex->EnableAttributeArray("SpatioTemporalHarmonics");

  // DSP pipeline
  vtkNew<vtkDSPTableFFT> dspFFT;
  dspFFT->SetInputConnection(multiplex->GetOutputPort(0));
  dspFFT->CreateFrequencyColumnOn();
  dspFFT->SetWindowingFunction(vtkTableFFT::RECTANGULAR);
  dspFFT->Update();

  vtkNew<vtkMultiDimensionBrowser> browser;
  browser->SetInputConnection(dspFFT->GetOutputPort(0));
  browser->SetIndex(0);
  browser->Update();

  // Normal Pipeline
  vtkNew<vtkMultiDimensionBrowser> configurateTable;
  configurateTable->SetInputConnection(multiplex->GetOutputPort(0));
  configurateTable->SetIndex(0);

  vtkNew<vtkTableFFT> singleShotFFT;
  singleShotFFT->SetInputConnection(configurateTable->GetOutputPort(0));
  singleShotFFT->CreateFrequencyColumnOn();
  singleShotFFT->SetWindowingFunction(vtkTableFFT::RECTANGULAR);
  singleShotFFT->Update();

  vtkIdType nPoints = vtkDataSet::SafeDownCast(source->GetOutput())->GetNumberOfPoints();
  for (vtkIdType iP = 0; iP < nPoints; iP += 10)
  {
    browser->SetIndex(iP);
    browser->Update();

    configurateTable->SetIndex(iP);
    singleShotFFT->Update();

    auto lhs = vtkTable::SafeDownCast(singleShotFFT->GetOutput());
    if (!lhs)
    {
      std::cerr << "Output of table fft was nullptr" << std::endl;
      return EXIT_FAILURE;
    }

    auto rhs = vtkTable::SafeDownCast(browser->GetOutput());
    if (!rhs)
    {
      std::cerr << "Output of dsp table fft was nullptr" << std::endl;
      return EXIT_FAILURE;
    }

    if (!::TableEq(lhs, rhs))
    {
      std::cerr << "Tables at index " << iP << " do not aggree" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
