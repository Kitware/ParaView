/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianHelperBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLagrangianHelperBase.h"

#include "vtkLagrangianMatidaIntegrationModel.h"
#include "vtkObjectFactory.h"

vtkCxxSetObjectMacro(vtkLagrangianHelperBase, IntegrationModel, vtkLagrangianBasicIntegrationModel);

//---------------------------------------------------------------------------
vtkLagrangianHelperBase::vtkLagrangianHelperBase()
{
  this->IntegrationModel = vtkLagrangianMatidaIntegrationModel::New();
}

//---------------------------------------------------------------------------
vtkLagrangianHelperBase::~vtkLagrangianHelperBase()
{
  this->SetIntegrationModel(NULL);
}

//----------------------------------------------------------------------------
void vtkLagrangianHelperBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IntegrationModel: " << this->IntegrationModel << endl;
}

//----------------------------------------------------------------------------
bool vtkLagrangianHelperBase::ParseDoubleValues(
  const char*& arrayString, int numberOfComponents, double* array)
{
  const char* constants = arrayString;
  char* tmp;
  bool success = true;
  for (int i = 0; i < numberOfComponents; i++)
  {
    if (strncmp(constants, "None", 4) == 0)
    {
      success = false;
      constants += 5;
      continue;
    }
    else
    {
      double value = strtod(constants, &tmp);
      constants = tmp + 1;
      array[i] = value;
    }
  }
  arrayString = constants;
  return success;
}
