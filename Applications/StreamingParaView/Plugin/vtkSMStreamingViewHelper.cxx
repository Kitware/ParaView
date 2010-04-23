/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingViewHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStreamingViewHelper.h"

#include "vtkObjectFactory.h"
#include "vtkSMStreamingViewProxy.h"
#include "vtkSMRenderViewProxy.h"

vtkStandardNewMacro(vtkSMStreamingViewHelper);
//----------------------------------------------------------------------------
vtkSMStreamingViewHelper::vtkSMStreamingViewHelper()
{
  this->StreamingView = 0;
}

//----------------------------------------------------------------------------
vtkSMStreamingViewHelper::~vtkSMStreamingViewHelper()
{
  this->SetStreamingView(0);
}

//----------------------------------------------------------------------------
void vtkSMStreamingViewHelper::EventuallyRender()
{
  if (this->StreamingView)
    {
    this->StreamingView->StillRender();
    }
}

//----------------------------------------------------------------------------
void vtkSMStreamingViewHelper::Render()
{
  if (this->StreamingView)
    {
    this->StreamingView->InteractiveRender();
    }
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkSMStreamingViewHelper::GetRenderWindow()
{
  return (this->StreamingView&&this->StreamingView->GetRootView()?
          this->StreamingView->GetRootView()->GetRenderWindow() : 0);

}

//----------------------------------------------------------------------------
void vtkSMStreamingViewHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "StreamingView: " << this->StreamingView << endl;
}


