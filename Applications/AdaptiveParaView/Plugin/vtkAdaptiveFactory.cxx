/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdaptiveFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAdaptiveFactory.h"
#include "vtkSMAdaptiveOutputPort.h"
#include "vtkPVSGeometryInformation.h"
#include "vtkVersion.h"

//----------------------------------------------------------------------------
// If the following macro is used, and this code is compiled as a shared
// library, and the environment variable VTK_AUTOLOAD_PATH includes the
// directory where the shared library may be found, then vtk will load the
// library and register this factory the first time a vtk object is allocated.
//
//VTK_FACTORY_INTERFACE_IMPLEMENT(vtkAdaptiveFactory)

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAdaptiveFactory);

//----------------------------------------------------------------------------
VTK_CREATE_CREATE_FUNCTION(vtkSMAdaptiveOutputPort);

//----------------------------------------------------------------------------
VTK_CREATE_CREATE_FUNCTION(vtkPVSGeometryInformation);

//----------------------------------------------------------------------------
vtkAdaptiveFactory::vtkAdaptiveFactory()
{
  this->RegisterOverride("vtkSMOutputPort",
                         "vtkSMAdaptiveOutputPort",
                         "Adaptive",
                         1,
                         vtkObjectFactoryCreatevtkSMAdaptiveOutputPort);
  this->RegisterOverride("vtkPVGeometryInformation",
                         "vtkPVSGeometryInformation",
                         "Adaptive",
                         1,
                         vtkObjectFactoryCreatevtkPVSGeometryInformation);
}

//----------------------------------------------------------------------------
vtkAdaptiveFactory::~vtkAdaptiveFactory()
{

}

//----------------------------------------------------------------------------
const char* vtkAdaptiveFactory::GetDescription()
{
  return "Adaptive Paraview Object Factory";
}

//----------------------------------------------------------------------------
const char* vtkAdaptiveFactory::GetVTKSourceVersion()
{
  return VTK_SOURCE_VERSION;
}

//----------------------------------------------------------------------------
void vtkAdaptiveFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Adaptive Paraview Object Factory" << endl;
}


