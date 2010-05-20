/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamingFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkStreamingFactory.h"
#include "vtkSMStreamingOutputPort.h"
#include "vtkPVSGeometryInformation.h"
#include "vtkVersion.h"

//----------------------------------------------------------------------------
// If the following macro is used, and this code is compiled as a shared
// library, and the environment variable VTK_AUTOLOAD_PATH includes the
// directory where the shared library may be found, then vtk will load the
// library and register this factory the first time a vtk object is allocated.
//
//VTK_FACTORY_INTERFACE_IMPLEMENT(vtkStreamingFactory)

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkStreamingFactory);

//----------------------------------------------------------------------------
VTK_CREATE_CREATE_FUNCTION(vtkSMStreamingOutputPort);

//----------------------------------------------------------------------------
VTK_CREATE_CREATE_FUNCTION(vtkPVSGeometryInformation);

//----------------------------------------------------------------------------
vtkStreamingFactory::vtkStreamingFactory()
{
  this->RegisterOverride("vtkSMOutputPort",
                         "vtkSMStreamingOutputPort",
                         "Streaming",
                         1,
                         vtkObjectFactoryCreatevtkSMStreamingOutputPort);
  this->RegisterOverride("vtkPVGeometryInformation",
                         "vtkPVSGeometryInformation",
                         "Streaming",
                         1,
                         vtkObjectFactoryCreatevtkPVSGeometryInformation);
}

//----------------------------------------------------------------------------
vtkStreamingFactory::~vtkStreamingFactory()
{

}

//----------------------------------------------------------------------------
const char* vtkStreamingFactory::GetDescription()
{
  return "Streaming Paraview Object Factory";
}

//----------------------------------------------------------------------------
const char* vtkStreamingFactory::GetVTKSourceVersion()
{
  return VTK_SOURCE_VERSION;
}

//----------------------------------------------------------------------------
void vtkStreamingFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Streaming Paraview Object Factory" << endl;
}


