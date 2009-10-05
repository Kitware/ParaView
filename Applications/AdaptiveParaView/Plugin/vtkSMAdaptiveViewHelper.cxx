/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAdaptiveViewHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAdaptiveViewHelper.h"

#include "vtkObjectFactory.h"
#include "vtkSMAdaptiveViewProxy.h"
#include "vtkSMRenderViewProxy.h"

vtkStandardNewMacro(vtkSMAdaptiveViewHelper);
vtkCxxRevisionMacro(vtkSMAdaptiveViewHelper, "1.1");
//----------------------------------------------------------------------------
vtkSMAdaptiveViewHelper::vtkSMAdaptiveViewHelper()
{
  this->AdaptiveView = 0;
}

//----------------------------------------------------------------------------
vtkSMAdaptiveViewHelper::~vtkSMAdaptiveViewHelper()
{
  this->SetAdaptiveView(0);
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveViewHelper::EventuallyRender()
{
  if (this->AdaptiveView)
    {
    this->AdaptiveView->StillRender();
    }
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveViewHelper::Render()
{
  if (this->AdaptiveView)
    {
    this->AdaptiveView->InteractiveRender();
    }
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkSMAdaptiveViewHelper::GetRenderWindow()
{
  return (this->AdaptiveView&&this->AdaptiveView->GetRootView()?
          this->AdaptiveView->GetRootView()->GetRenderWindow() : 0);

}

//----------------------------------------------------------------------------
void vtkSMAdaptiveViewHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AdaptiveView: " << this->AdaptiveView << endl;
}


