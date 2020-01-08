/*=========================================================================

  Program:   ParaView
  Module:    vtkOMETIFFChannelCalculator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOMETIFFChannelCalculator.h"

#include "vtkDataArraySelection.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkScalarsToColors.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <algorithm>

vtkStandardNewMacro(vtkOMETIFFChannelCalculator);
//----------------------------------------------------------------------------
vtkOMETIFFChannelCalculator::vtkOMETIFFChannelCalculator()
{
  this->ChannelSelection = vtkDataArraySelection::New();
}

//----------------------------------------------------------------------------
vtkOMETIFFChannelCalculator::~vtkOMETIFFChannelCalculator()
{
  this->ChannelSelection->Delete();
  this->ChannelSelection = nullptr;
}

//----------------------------------------------------------------------------
void vtkOMETIFFChannelCalculator::SetLUT(const char* channelName, vtkScalarsToColors* stc)
{
  if (this->LUTMap[channelName].LUT != stc)
  {
    this->LUTMap[channelName].LUT = stc;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkOMETIFFChannelCalculator::SetWeight(const char* channelName, double weight)
{
  if (this->LUTMap[channelName].Weight != weight)
  {
    this->LUTMap[channelName].Weight = weight;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkOMETIFFChannelCalculator::GetMTime()
{
  auto mtime = std::max(this->Superclass::GetMTime(), this->ChannelSelection->GetMTime());
  for (const auto& apair : this->LUTMap)
  {
    if (apair.second.LUT)
    {
      mtime = std::max(mtime, apair.second.LUT->GetMTime());
    }
  }
  return mtime;
}

//----------------------------------------------------------------------------
int vtkOMETIFFChannelCalculator::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkOMETIFFChannelCalculator::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkDataSet::GetData(inputVector[0], 0);
  auto output = vtkDataSet::GetData(outputVector, 0);

  output->ShallowCopy(input);

  auto outPD = output->GetPointData();

  const auto numPoints = output->GetNumberOfPoints();

  vtkNew<vtkUnsignedCharArray> finalColors;
  finalColors->SetNumberOfComponents(4);
  finalColors->SetNumberOfTuples(numPoints);
  finalColors->FillValue(static_cast<unsigned char>(0));
  finalColors->SetName("Colors");

  for (int cc = 0, max = this->ChannelSelection->GetNumberOfArrays(); cc < max; ++cc)
  {
    auto aname = this->ChannelSelection->GetArrayName(cc);
    if (!this->ChannelSelection->ArrayIsEnabled(aname))
    {
      continue;
    }

    auto iter = this->LUTMap.find(aname);
    if (iter == this->LUTMap.end())
    {
      vtkErrorMacro("No LUT specified for `" << aname << "`!");
      return 0;
    }

    auto colors = iter->second.LUT->MapScalars(
      outPD->GetArray(iter->first.c_str()), VTK_COLOR_MODE_DEFAULT, 0, VTK_RGBA);
    for (vtkIdType idx = 0; idx < numPoints; ++idx)
    {
      vtkVector4d dest;
      finalColors->GetTuple(idx, dest.GetData());
      dest = dest / vtkVector4d(255.0);

      vtkVector4d src;
      colors->GetTuple(idx, src.GetData());
      src = src / vtkVector4d(255.0);

      src[3] *= iter->second.Weight;

      dest[0] = src[3] * src[0] + dest[0];
      dest[1] = src[3] * src[1] + dest[1];
      dest[2] = src[3] * src[2] + dest[2];
      dest[3] = src[3] + (1.0 - src[3]) * dest[3];

      // clamp to [0, 1].
      for (int k = 0; k < 3; ++k)
      {
        dest[k] = std::max(0.0, dest[k]);
        dest[k] = std::min(dest[k], 1.0);
      }

      finalColors->SetTypedComponent(idx, 0, static_cast<unsigned char>(dest[0] * 255.0));
      finalColors->SetTypedComponent(idx, 1, static_cast<unsigned char>(dest[1] * 255.0));
      finalColors->SetTypedComponent(idx, 2, static_cast<unsigned char>(dest[2] * 255.0));
      finalColors->SetTypedComponent(idx, 3, static_cast<unsigned char>(dest[3] * 255.0));
    }
    colors->Delete();
  }

  outPD->AddArray(finalColors);

  return 1;
}

//----------------------------------------------------------------------------
void vtkOMETIFFChannelCalculator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
