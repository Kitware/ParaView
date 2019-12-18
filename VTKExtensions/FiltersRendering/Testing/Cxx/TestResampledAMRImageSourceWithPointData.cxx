/*=========================================================================

  Program:   ParaView
  Module:    TestResampledAMRImageSourceWithPointData.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataPipeline.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkResampledAMRImageSource.h"
#include "vtkTesting.h"
#include "vtkXMLUniformGridAMRReader.h"

#include <string>

#define TEST_SUCCESS 0
#define TEST_FAILED 1

#define vtk_assert(x)                                                                              \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << "ERROR: Condition FAILED!! : " << #x << endl;                                          \
    return TEST_FAILED;                                                                            \
  }

int TestResampledAMRImageSourceWithPointData(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, (const char**)(argv));

  vtkNew<vtkXMLUniformGridAMRReader> reader;
  std::string filename = testing->GetDataRoot();
  filename += "/Testing/Data/amr/wavelet.vthb";
  reader->SetFileName(filename.c_str());
  reader->SetMaximumLevelsToReadByDefault(1);
  reader->Update();

  vtkOverlappingAMR* data = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));

  vtkNew<vtkResampledAMRImageSource> resampler;
  resampler->SetMaxDimensions(32, 32, 32);
  vtk_assert(resampler->NeedsInitialization() == true);

  resampler->UpdateResampledVolume(data);
  vtk_assert(resampler->NeedsInitialization() == false);

  // request a few blocks explicitly.
  vtkCompositeDataPipeline* cp = vtkCompositeDataPipeline::SafeDownCast(reader->GetExecutive());
  int blocks[] = { 1, 2, 13, 17 };
  vtkInformation* info = cp->GetOutputInformation(0);
  info->Set(vtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS(), 1);
  info->Set(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), blocks, 4);
  reader->Update();

  resampler->UpdateResampledVolume(data);
  vtk_assert(resampler->NeedsInitialization() == false);

  vtkImageData* output = vtkImageData::SafeDownCast(resampler->GetOutputDataObject(0));
  vtk_assert(output != NULL);
  vtk_assert(output->GetDimensions()[0] == 8);
  vtk_assert(output->GetDimensions()[1] == 8);
  vtk_assert(output->GetDimensions()[2] == 8);

  vtkDataArray* temp = output->GetPointData()->GetArray("RTData");
  vtk_assert(temp != NULL);
  cout << "Tuple: 14544: " << temp->GetTuple1(15) << endl;

  // FIXME: Add more validation code.
  return TEST_SUCCESS;
}
