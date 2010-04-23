/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRenderViewHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRenderViewHelper.h"

#include "vtkObjectFactory.h"
#include "vtkSMRenderViewProxy.h"

vtkStandardNewMacro(vtkSMRenderViewHelper);
//----------------------------------------------------------------------------
vtkSMRenderViewHelper::vtkSMRenderViewHelper()
{
  this->RenderViewProxy = 0;
}

//----------------------------------------------------------------------------
vtkSMRenderViewHelper::~vtkSMRenderViewHelper()
{
  this->SetRenderViewProxy(0);
}

//----------------------------------------------------------------------------
void vtkSMRenderViewHelper::EventuallyRender()
{
  if (this->RenderViewProxy)
    {
    this->RenderViewProxy->StillRender();
    }
}

//----------------------------------------------------------------------------
void vtkSMRenderViewHelper::Render()
{
  if (this->RenderViewProxy)
    {
    this->RenderViewProxy->InteractiveRender();
    }
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkSMRenderViewHelper::GetRenderWindow()
{
  return (this->RenderViewProxy?
    this->RenderViewProxy->GetRenderWindow() : 0);

}

//----------------------------------------------------------------------------
void vtkSMRenderViewHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderViewProxy: " << this->RenderViewProxy << endl;
}


