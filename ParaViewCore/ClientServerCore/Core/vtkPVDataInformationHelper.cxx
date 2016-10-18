/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataInformationHelper.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"

//----------------------------------------------------------------------------
vtkPVDataInformationHelper::vtkPVDataInformationHelper()
{
}

//----------------------------------------------------------------------------
vtkPVDataInformationHelper::~vtkPVDataInformationHelper()
{
}

//----------------------------------------------------------------------------
void vtkPVDataInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVDataInformationHelper::CopyFromDataObject(vtkPVDataInformation* pvdi, vtkDataObject* data)
{
  if (!this->ValidateType(data))
  {
    return;
  }
  this->Data = data;

  pvdi->SetDataClassName(data->GetClassName());
  pvdi->DataSetType = data->GetDataObjectType();
  pvdi->NumberOfDataSets = this->GetNumberOfDataSets();

  double* dataBounds = this->GetBounds();
  memcpy(pvdi->Bounds, dataBounds, 6 * sizeof(double));

  pvdi->MemorySize = data->GetActualMemorySize();
  pvdi->NumberOfCells = this->GetNumberOfCells();
  pvdi->NumberOfPoints = this->GetNumberOfPoints();
  pvdi->NumberOfRows = this->GetNumberOfRows();
}
