/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderModuleProxyImplementation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRenderModuleProxyImplementation.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderModule.h"

vtkCxxRevisionMacro(vtkPVRenderModuleProxyImplementation, "1.1");
vtkStandardNewMacro(vtkPVRenderModuleProxyImplementation);
vtkCxxSetObjectMacro(vtkPVRenderModuleProxyImplementation,PVRenderModule,vtkPVRenderModule);


vtkPVRenderModuleProxyImplementation::vtkPVRenderModuleProxyImplementation()
{
  this->PVRenderModule = 0;
}

vtkPVRenderModuleProxyImplementation::~vtkPVRenderModuleProxyImplementation()
{
  this->SetPVRenderModule(0);
}

float vtkPVRenderModuleProxyImplementation::GetZBufferValue(int x, int y)
{
  return this->PVRenderModule->GetZBufferValue(x,y);
}

void vtkPVRenderModuleProxyImplementation::PrintSelf(ostream& os, vtkIndent indent)
{ 
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PVRenderModule: " << this->GetPVRenderModule() << endl;
}
