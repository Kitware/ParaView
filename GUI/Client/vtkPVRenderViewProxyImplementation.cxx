/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderViewProxyImplementation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRenderViewProxyImplementation.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"

vtkCxxRevisionMacro(vtkPVRenderViewProxyImplementation, "1.1");
vtkStandardNewMacro(vtkPVRenderViewProxyImplementation);


vtkPVRenderViewProxyImplementation::vtkPVRenderViewProxyImplementation()
{
  this->PVRenderView = 0;
}

vtkPVRenderViewProxyImplementation::~vtkPVRenderViewProxyImplementation()
{
}

//----------------------------------------------------------------------------
void vtkPVRenderViewProxyImplementation::SetPVRenderView(vtkPVRenderView *view)
{
  if (this->PVRenderView != view)
    {
    // to avoid circular references
    this->PVRenderView = view;
    }
} 


void vtkPVRenderViewProxyImplementation::EventuallyRender()
{
  if(this->PVRenderView)
    {
    this->PVRenderView->EventuallyRender();
    }
}

vtkRenderWindow* vtkPVRenderViewProxyImplementation::GetRenderWindow()
{
  if(this->PVRenderView)
    {
    return this->PVRenderView->GetRenderWindow();
    }
  return 0;
}

void vtkPVRenderViewProxyImplementation::Render()
{ 
  if(this->PVRenderView)
    {
    this->PVRenderView->Render();
    }
}

void vtkPVRenderViewProxyImplementation::PrintSelf(ostream& os, vtkIndent indent)
{ 
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PVRenderView: " << this->GetPVRenderView() << endl;
}
