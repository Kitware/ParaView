// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVDataSizeInformation.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"

vtkStandardNewMacro(vtkPVDataSizeInformation);
//----------------------------------------------------------------------------
vtkPVDataSizeInformation::vtkPVDataSizeInformation()
{
  this->Initialize();
}

//----------------------------------------------------------------------------
vtkPVDataSizeInformation::~vtkPVDataSizeInformation() = default;

//----------------------------------------------------------------------------
void vtkPVDataSizeInformation::CopyFromObject(vtkObject* object)
{
  vtkPVDataInformation* dinfo = vtkPVDataInformation::New();

  vtkAlgorithm* alg = vtkAlgorithm::SafeDownCast(object);
  if (alg)
  {
    dinfo->CopyFromObject(alg->GetOutputDataObject(0));
  }
  else
  {
    dinfo->CopyFromObject(object);
  }
  this->MemorySize = dinfo->GetMemorySize();
  dinfo->Delete();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVDataSizeInformation::AddInformation(vtkPVInformation* info)
{
  vtkPVDataSizeInformation* dsinfo = vtkPVDataSizeInformation::SafeDownCast(info);
  if (!dsinfo)
  {
    vtkErrorMacro("Could not cast object to data size information.");
    return;
  }
  this->MemorySize += dsinfo->MemorySize;
}

//----------------------------------------------------------------------------
void vtkPVDataSizeInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->MemorySize;
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVDataSizeInformation::CopyFromStream(const vtkClientServerStream* css)
{
  if (!css->GetArgument(0, 0, &this->MemorySize))
  {
    vtkErrorMacro("Error parsing memory size.");
  }
  else
  {
    this->Modified();
  }
}
//----------------------------------------------------------------------------
void vtkPVDataSizeInformation::Initialize()
{
  this->MemorySize = 0;
}

//----------------------------------------------------------------------------
void vtkPVDataSizeInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MemorySize: " << this->MemorySize << endl;
}
