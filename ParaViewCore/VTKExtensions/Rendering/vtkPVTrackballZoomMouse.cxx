/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTrackballZoomMouse.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTrackballZoomMouse.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkPVInteractorStyle.h"

vtkStandardNewMacro(vtkPVTrackballZoomMouse);

//-------------------------------------------------------------------------
vtkPVTrackballZoomMouse::vtkPVTrackballZoomMouse()
{
  this->ZoomPosition[0] = 0;
  this->ZoomPosition[1] = 0;
}

//-------------------------------------------------------------------------
vtkPVTrackballZoomMouse::~vtkPVTrackballZoomMouse()
{
}

//-------------------------------------------------------------------------
void vtkPVTrackballZoomMouse::OnButtonDown(int x, int y, vtkRenderer *ren,
  vtkRenderWindowInteractor * rwi)
{
  this->Superclass::OnButtonDown(x, y, ren, rwi);
  rwi->GetEventPosition(this->ZoomPosition);
}

//-------------------------------------------------------------------------
void vtkPVTrackballZoomMouse::OnMouseMove(int x, int y,
  vtkRenderer *ren,
  vtkRenderWindowInteractor *rwi)
{
  double dy = rwi->GetLastEventPosition()[1] - y;
  double k = dy * this->ZoomScale;
  vtkPVInteractorStyle::DollyToPosition((1.0 - k), this->ZoomPosition, ren);
  rwi->Render();
}

//-------------------------------------------------------------------------
void vtkPVTrackballZoomMouse::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ZoomPosition: "
    << this->ZoomPosition[0] << " " << this->ZoomPosition[1] << endl;
}
