/*=========================================================================

  Program:   ParaView
  Module:    vtkImageFileSeriesReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageFileSeriesReader.h"

#include "vtkImageReader2.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"

namespace
{
  bool vtkEqual(vtkStringArray* self, vtkStringArray* other)
    {
    if (self->GetNumberOfValues() != other->GetNumberOfValues())
      {
      return false;
      }
    for (vtkIdType cc=0, max=self->GetNumberOfValues(); cc<max; cc++)
      {
      if (self->GetValue(cc) != other->GetValue(cc))
        {
        return false;
        }
      }
    return true;
    }
}

vtkStandardNewMacro(vtkImageFileSeriesReader);
//----------------------------------------------------------------------------
vtkImageFileSeriesReader::vtkImageFileSeriesReader()
{
  this->ReadAsImageStack = false;
}

//----------------------------------------------------------------------------
vtkImageFileSeriesReader::~vtkImageFileSeriesReader()
{
}

//----------------------------------------------------------------------------
int vtkImageFileSeriesReader::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    this->ResetTimeRanges();
    this->UpdateMetaData();
    this->UpdateFileNames();

    if (this->ReadAsImageStack)
      {
      vtkInformation *outInfo = outputVector->GetInformationObject(0);
      outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
      }
    }
  if (this->ReadAsImageStack && this->Reader)
    {
    this->_FileIndex = 0;
    return this->Reader->ProcessRequest(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkImageFileSeriesReader::UpdateFileNames()
{
  vtkImageReader2* imageReader = vtkImageReader2::SafeDownCast(this->Reader);
  if (!imageReader)
    {
    return;
    }

  this->BeforeFileNameMTime = this->GetMTime();
  if (this->ReadAsImageStack)
    {
    vtkNew<vtkStringArray> filenames;
    filenames->SetNumberOfTuples(this->GetNumberOfFileNames());
    for (unsigned int cc=0, max = this->GetNumberOfFileNames(); cc < max; cc++)
      {
      filenames->SetValue(cc, this->GetFileName(cc));
      }
    if (imageReader->GetFileNames() == NULL)
      {
      imageReader->SetFileNames(filenames.GetPointer());
      }
    else if (vtkEqual(imageReader->GetFileNames(), filenames.GetPointer()) == false)
            // check if filenames indeed changed.
      {
      imageReader->SetFileNames(filenames.GetPointer());
      }
    }
  else
    {
    imageReader->SetFileName(this->GetFileName(this->_FileIndex));
    int ext[6];
    imageReader->GetDataExtent(ext);
    ext[4] = ext[5] = 0;
    imageReader->SetDataExtent(ext);
    }
  this->FileNameMTime = this->Reader->GetMTime();
}

//----------------------------------------------------------------------------
void vtkImageFileSeriesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
