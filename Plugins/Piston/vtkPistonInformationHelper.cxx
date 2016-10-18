/*=========================================================================

  Program:   ParaView
  Module:    vtkPistonInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPistonInformationHelper.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPistonDataObject.h"

vtkStandardNewMacro(vtkPistonInformationHelper);

//----------------------------------------------------------------------------
vtkPistonInformationHelper::vtkPistonInformationHelper()
{
}

//----------------------------------------------------------------------------
vtkPistonInformationHelper::~vtkPistonInformationHelper()
{
}

//----------------------------------------------------------------------------
void vtkPistonInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const char* vtkPistonInformationHelper::GetPrettyDataTypeString()
{
  return "Piston Data Object";
}

//----------------------------------------------------------------------------
bool vtkPistonInformationHelper::ValidateType(vtkDataObject* data)
{
  vtkPistonDataObject* pdo = vtkPistonDataObject::SafeDownCast(data);
  if (!pdo)
  {
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
int vtkPistonInformationHelper::GetNumberOfDataSets()
{
  return 1;
}

//----------------------------------------------------------------------------
double* vtkPistonInformationHelper::GetBounds()
{
  vtkPistonDataObject* pdo = vtkPistonDataObject::SafeDownCast(this->Data);
  return pdo->GetBounds();
}

//----------------------------------------------------------------------------
vtkTypeInt64 vtkPistonInformationHelper::GetNumberOfCells()
{
  return 0;
}

//----------------------------------------------------------------------------
vtkTypeInt64 vtkPistonInformationHelper::GetNumberOfPoints()
{
  return 0;
}

//----------------------------------------------------------------------------
vtkTypeInt64 vtkPistonInformationHelper::GetNumberOfRows()
{
  return 0;
}
