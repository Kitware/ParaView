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
  for (int i = 0; i < 6; i++)
  {
    this->DataExtent[i] = 0;
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
  // see superclass on why we also check on the number of file names here
  if (this->ReadAsImageStack && this->GetNumberOfFileNames() > 1 && this->FileDimensionality == 2)
  {
    int ext[6] = { this->DataExtent[0], this->DataExtent[1], this->DataExtent[2],
      this->DataExtent[3], 0, static_cast<int>(this->GetNumberOfFileNames()) - 1 };
    imageReader->SetDataExtent(ext);
  }
  else
  {
    imageReader->SetDataExtent(this->DataExtent);
  }
}

//----------------------------------------------------------------------------
void vtkRawImageFileSeriesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DataExtent: (" << this->DataExtent[0];
  for (int idx = 1; idx < 6; ++idx)
  {
    os << ", " << this->DataExtent[idx];
  }
  os << ")\n";
  os << indent << "File Dimensionality: " << this->FileDimensionality << "\n";
}
