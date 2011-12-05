/*=========================================================================

Program:   ParaView
Module:    TestSMIntVectorProperty.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkInitializationHelper.h"
#include "vtkPVServerOptions.h"
#include "vtkSMProxyManager.h"
#include "vtkProcessModule.h"
#include "vtkSMIntVectorProperty.h"

#define VERIFY(expr) if(!(expr)){ std::cerr << "failed: " << #expr << std::endl; return -1;}

int main(int argc, char *argv[])
{
  vtkPVServerOptions* options = vtkPVServerOptions::New();
  vtkInitializationHelper::Initialize(argc, argv,
                                      vtkProcessModule::PROCESS_CLIENT,
                                      options);

  vtkSMIntVectorProperty *property = vtkSMIntVectorProperty::New();

  property->SetNumberOfElements(4);
  VERIFY(property->GetNumberOfElements() == 4);

  property->Delete();

  //---------------------------------------------------------------------------
  vtkInitializationHelper::Finalize();
  options->Delete();

  return 0;
}
