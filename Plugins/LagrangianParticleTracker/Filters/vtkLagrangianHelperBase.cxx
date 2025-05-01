// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLagrangianHelperBase.h"

#include "vtkLagrangianMatidaIntegrationModel.h"
#include "vtkObjectFactory.h"
#include "vtkStringScanner.h"

vtkCxxSetObjectMacro(vtkLagrangianHelperBase, IntegrationModel, vtkLagrangianBasicIntegrationModel);

//---------------------------------------------------------------------------
vtkLagrangianHelperBase::vtkLagrangianHelperBase()
{
  this->IntegrationModel = vtkLagrangianMatidaIntegrationModel::New();
}

//---------------------------------------------------------------------------
vtkLagrangianHelperBase::~vtkLagrangianHelperBase()
{
  this->SetIntegrationModel(nullptr);
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
  std::string_view constants(arrayString);
  bool success = true;
  for (int i = 0; i < numberOfComponents; i++)
  {
    if (constants.substr(0, 4) == "None")
    {
      success = false;
      constants = constants.substr(5);
    }
    else
    {
      auto result = vtk::from_chars(constants, array[i]);
      if (result.ec != std::errc{})
      {
        array[i] = 0.0;
      }
      constants = std::string_view(result.ptr + 1);
    }
  }
  arrayString = constants.data();
  return success;
}
