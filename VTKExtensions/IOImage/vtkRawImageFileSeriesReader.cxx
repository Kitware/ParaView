// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRawImageFileSeriesReader.h"

#include "vtkImageReader2.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkRawImageFileSeriesReader);
//----------------------------------------------------------------------------
vtkRawImageFileSeriesReader::vtkRawImageFileSeriesReader()
  : FileDimensionality(2)
{
  for (int i = 0; i < 3; i++)
  {
    this->Dimensions[i] = 0;
    this->MinimumIndex[i] = 0;
  }
}

//----------------------------------------------------------------------------
vtkRawImageFileSeriesReader::~vtkRawImageFileSeriesReader() = default;

//----------------------------------------------------------------------------
void vtkRawImageFileSeriesReader::UpdateReaderDataExtent()
{
  vtkImageReader2* imageReader = vtkImageReader2::SafeDownCast(this->Reader);
  if (!imageReader)
  {
    return;
  }
  imageReader->SetFileDimensionality(this->FileDimensionality);

  int ext[6];
  ext[0] = this->MinimumIndex[0];
  ext[1] = this->MinimumIndex[0] + std::max(this->Dimensions[0] - 1, 0);
  ext[2] = this->MinimumIndex[1];
  ext[3] = this->MinimumIndex[1] + std::max(this->Dimensions[1] - 1, 0);

  if (this->ReadAsImageStack && this->GetNumberOfFileNames() > 1 && this->FileDimensionality == 2)
  {
    ext[4] = 0;
    ext[5] = static_cast<int>(this->GetNumberOfFileNames()) - 1;
  }
  else
  {
    ext[4] = this->MinimumIndex[2];
    ext[5] = this->MinimumIndex[2] + std::max(this->Dimensions[2] - 1, 0);
  }
  imageReader->SetDataExtent(ext);
}

//----------------------------------------------------------------------------
void vtkRawImageFileSeriesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Dimensions: (" << this->Dimensions[0];
  for (int idx = 1; idx < 3; ++idx)
  {
    os << ", " << this->Dimensions[idx];
  }
  os << ")\n";
  os << indent << "MinimumIndex: (" << this->MinimumIndex[0];
  for (int idx = 1; idx < 3; ++idx)
  {
    os << ", " << this->MinimumIndex[idx];
  }
  os << ")\n";
  os << indent << "File Dimensionality: " << this->FileDimensionality << "\n";
}
